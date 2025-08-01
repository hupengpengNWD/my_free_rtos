/**
  ******************************************************************************
  * @file:    
  * @author:  hupengepng
  * @date:    
  * @email:   hupengpengHPP@outlook.com
  ******************************************************************************
  * @attention
  * 
  ******************************************************************************
  */

#include "drv_gpio_input.h"

// 引脚映射表结构
typedef struct {
    GPIO_TypeDef* port;    // 端口
    uint16_t pin;          // 引脚
    const char* conflicting_module; // 可能冲突的模块
} GpioInputPinSet;

typedef struct {
    GPIO_TypeDef* port;    // 端口
    uint16_t pin;          // 引脚
    GpioInputPinSet sets[2]; // 每引脚支持 2 组配置
} GpioInputPinMap;

// 引脚映射表（基于 LQFP64 封装，示例部分引脚）
static const GpioInputPinMap pin_map[] = {
    [0] = {
        .port = GPIOE,
        .pin = GPIO_PIN_12,
        .sets = {
            [0] = {
                .port = GPIOE,
                .pin = GPIO_PIN_12,
                .conflicting_module = NULL
            },
            [1] = {
                .port = GPIOB,
                .pin = GPIO_PIN_3,
                .conflicting_module = "TIM2"
            }
        }
    },
    [1] = {
        .port = GPIOE,
        .pin = GPIO_PIN_13,
        .sets = {
            [0] = {
                .port = GPIOE,
                .pin = GPIO_PIN_13,
                .conflicting_module = NULL
            },
            [1] = {
                .port = GPIOB,
                .pin = GPIO_PIN_0,
                .conflicting_module = "TIM3"
            }
        }
    },
    [2] = {
        .port = GPIOA,
        .pin = GPIO_PIN_6,
        .sets = {
            [0] = {
                .port = GPIOA,
                .pin = GPIO_PIN_6,
                .conflicting_module = "TIM3"
            },
            [1] = {
                .port = GPIOC,
                .pin = GPIO_PIN_6,
                .conflicting_module = "TIM3/TIM8"
            }
        }
    },
    {
        .port = NULL,
        .pin = 0,
        .sets = {
            [0] = {
                .port = NULL,
                .pin = 0,
                .conflicting_module = NULL
            },
            [1] = {
                .port = NULL,
                .pin = 0,
                .conflicting_module = NULL
            }
        }
    }
};

// 全局变量
st_gpio_input_ptr gpio_input_hendle_ptr = NULL; // GPIO 输入链表头
static st_gpioErrorDetail last_error = {GPIO_INPUT_ERROR_NONE, NULL, 0, NULL}; // 最后错误
st_soft_time gpio_input_shared_timer = {0}; // 共享定时器
uint32_t gpio_input_shared_poll_period_us = GPIO_INPUT_DEFAULT_POLL_PERIOD_US; // 共享轮询周期

// 定时器回调
void lib_gpio_input_polling(void* arg) {
    st_gpio_input_ptr ptr;
    for (ptr = gpio_input_hendle_ptr; ptr; ptr = ptr->next) {
        ptr->fsm(ptr);
    }
}

/**
  * @name     drv_gpio_input_read_level
  * @brief    读取 GPIO 引脚电平
  * @param    ptr: GPIO 输入指针
  * @retval   uint8_t: 电平值（0 或 1）
  * @remark   封装 HAL_GPIO_ReadPin
  */
uint8_t drv_gpio_input_read_level(st_gpio_input_ptr ptr) {
    if (ptr == NULL || ptr->hw_ptr == NULL) {
        return 0;
    }
    return HAL_GPIO_ReadPin((GPIO_TypeDef*)ptr->hw_ptr, ptr->pin);
}

/**
  * @name     drv_gpio_input_create
  * @brief    创建 GPIO 输入实例
  * @param    ptr: GPIO 输入指针
  * @param    port: GPIO 端口（例如 GPIOA）
  * @param    pin: GPIO 引脚（例如 GPIO_PIN_5）
  * @retval   et_gpioError: 错误码
  * @remark   
  */
et_gpioError drv_gpio_input_create(st_gpio_input_ptr ptr, GPIO_TypeDef* port, uint32_t pin) {
    // uint32_t poll_period_ms = 0;
    if (!ptr || !port || !pin) {
        last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_INVALID_CONFIG, port, pin, NULL};
        return GPIO_INPUT_ERROR_INVALID_CONFIG;
    }

    ptr->hw_ptr = port;
    ptr->pin = pin;
    ptr->read_level = drv_gpio_input_read_level;
    ptr->configure = lib_gpio_input_configure;
    ptr->configure(ptr);

    // 添加到链表
    st_gpio_input_ptr curr = gpio_input_hendle_ptr;
    int count = 0;
    while (curr) {
        if ((GPIO_TypeDef*)curr->hw_ptr == port && curr->pin == pin) {
            last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_PIN_CONFLICT, port, pin, "GPIO_INPUT"};
            return GPIO_INPUT_ERROR_PIN_CONFLICT;
        }
        count++;
        curr = curr->next;
    }
    if (count >= GPIO_INPUT_MAX_INPUTS) {
        last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_INVALID_CONFIG, port, pin, NULL};
        return GPIO_INPUT_ERROR_INVALID_CONFIG;
    }

    ptr->next = gpio_input_hendle_ptr;
    gpio_input_hendle_ptr = ptr;

    last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_NONE, NULL, 0, NULL};
    return GPIO_INPUT_ERROR_NONE;
}

/**
  * @name     drv_gpio_input_hw_init
  * @brief    初始化 GPIO 硬件为输入模式
  * @param    port: GPIO 端口（例如 GPIOA）
  * @param    pin: GPIO 引脚（例如 GPIO_PIN_5）
  * @param    pull: 上拉/下拉配置（GPIO_PULLUP, GPIO_PULLDOWN, GPIO_NOPULL）
  * @param    pin_set_index: 引脚组索引（0 为默认）
  * @retval   et_gpioError: 错误码
  * @remark   配置 GPIO 为输入模式
  */
et_gpioError drv_gpio_input_hw_init(GPIO_TypeDef* port, 
                                    uint32_t pin, 
                                    uint32_t pull, 
                                    uint8_t pin_set_index) {
    if (!port || 
        !pin || 
        (pull != GPIO_PULLUP && pull != GPIO_PULLDOWN && pull != GPIO_NOPULL)) {
        last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_INVALID_CONFIG, port, pin, NULL};
        return GPIO_INPUT_ERROR_INVALID_CONFIG;
    }

    // 查找引脚映射
    const GpioInputPinMap* map = pin_map;
    while (map->port != NULL) {
        if (map->port == port && map->pin == pin) {
            break;
        }
        map++;
    }

    if (map->port == NULL || pin_set_index >= 2 || (pin_set_index == 1 && map->sets[1].port == NULL)) {
        last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_INVALID_PIN, port, pin, NULL};
        return GPIO_INPUT_ERROR_INVALID_PIN;
    }

    const GpioInputPinSet* set = &map->sets[pin_set_index];
    if (set->port != port || set->pin != pin) {
        last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_INVALID_PIN, port, pin, NULL};
        return GPIO_INPUT_ERROR_INVALID_PIN;
    }

    // 检查引脚冲突
    if (set->conflicting_module) {
        last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_PIN_CONFLICT, port, pin, set->conflicting_module};
        return GPIO_INPUT_ERROR_PIN_CONFLICT;
    }

    // 启用 GPIO 时钟
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOD_CLK_ENABLE();
    else {
        last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_INVALID_PORT, port, pin, NULL};
        return GPIO_INPUT_ERROR_INVALID_PORT;
    }

    // 配置 GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = pull;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 检查共享定时器
    // if (shared_timer.hw_ptr == NULL) {
    //     last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_TIMER_NOT_INIT, port, pin, NULL};
    //     return GPIO_INPUT_ERROR_TIMER_NOT_INIT;
    // }

    last_error = (st_gpioErrorDetail){GPIO_INPUT_ERROR_NONE, NULL, 0, NULL};
    return GPIO_INPUT_ERROR_NONE;
}

/**
  * @name     drv_gpio_input_get_last_error
  * @brief    获取最后错误详情
  * @param    None
  * @retval   st_gpioErrorDetail: 错误详情
  * @remark   
  */
st_gpioErrorDetail drv_gpio_input_get_last_error(void) {
    return last_error;
}

/*---End of File----------------------------------------------------*/

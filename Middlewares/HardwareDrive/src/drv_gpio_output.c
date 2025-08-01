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

#include "drv_gpio_output.h"

// 引脚映射表结构
typedef struct {
    GPIO_TypeDef* port;    // 端口
    uint16_t pin;          // 引脚
    const char* conflicting_module; // 可能冲突的模块
} GpioOutputPinSet;

typedef struct {
    GPIO_TypeDef* port;    // 端口
    uint16_t pin;          // 引脚
    GpioOutputPinSet sets[2]; // 每引脚支持 2 组配置
} GpioOutputPinMap;

// 引脚映射表（基于 LQFP64 封装，示例部分引脚）
static const GpioOutputPinMap pin_map[] = {
    [0] = {
        .port = GPIOE,
        .pin = GPIO_PIN_0,
        .sets = {
            [0] = {
                .port = GPIOE,
                .pin = GPIO_PIN_0,
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
        .pin = GPIO_PIN_1,
        .sets = {
            [0] = {
                .port = GPIOE,
                .pin = GPIO_PIN_1,
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
st_gpio_output_ptr gpio_output_hendle_ptr = NULL; // GPIO实例链表头
static st_gpioOutputErrorDetail last_error = {GPIO_OUTPUT_ERROR_NONE, NULL, 0, NULL}; // 最后错误

/**
  * @name     drv_gpio_output_write_high
  * @brief    设置 GPIO 为高电平
  * @param    ptr: GPIO 输出指针
  * @retval   None
  * @remark   
  */
void drv_gpio_output_write_high(st_gpio_output_ptr ptr) {

    if (!ptr || !ptr->hw_ptr || !ptr->pin) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_CONFIG, NULL, 0, NULL};
        return;
    }
    GPIO_TypeDef* port = (GPIO_TypeDef*)ptr->hw_ptr;
    ptr->state = GPIO_STATE_HIGH;
    HAL_GPIO_WritePin(port, ptr->pin, GPIO_PIN_SET);
    if (ptr->callback) {
        ptr->callback(ptr, GPIO_EVENT_STATE_CHANGE, ptr->callback_arg);
    }

}

/**
  * @name     drv_gpio_output_write_low
  * @brief    设置 GPIO 为低电平
  * @param    ptr: GPIO 输出指针
  * @retval   None
  * @remark   
  */
void drv_gpio_output_write_low(st_gpio_output_ptr ptr) {

    if (!ptr || !ptr->hw_ptr || !ptr->pin) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_CONFIG, NULL, 0, NULL};
        return;
    }
    GPIO_TypeDef* port = (GPIO_TypeDef*)ptr->hw_ptr;
    ptr->state = GPIO_STATE_LOW;
    HAL_GPIO_WritePin(port, ptr->pin, GPIO_PIN_RESET);
    if (ptr->callback) {
        ptr->callback(ptr, GPIO_EVENT_STATE_CHANGE, ptr->callback_arg);
    }

}

/**
  * @name     drv_gpio_output_toggle_pin
  * @brief    翻转 GPIO 电平
  * @param    ptr: GPIO 输出指针
  * @retval   None
  * @remark   
  */
void drv_gpio_output_toggle_pin(st_gpio_output_ptr ptr) {

    if (!ptr || !ptr->hw_ptr || !ptr->pin) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_CONFIG, NULL, 0, NULL};
        return;
    }
    GPIO_TypeDef* port = (GPIO_TypeDef*)ptr->hw_ptr;
    ptr->state = (ptr->state == GPIO_STATE_LOW) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
    HAL_GPIO_TogglePin(port, ptr->pin);
    if (ptr->callback) {
        ptr->callback(ptr, GPIO_EVENT_STATE_CHANGE, ptr->callback_arg);
    }
}

/**
  * @name     drv_gpio_output_create
  * @brief    创建 GPIO 输出实例
  * @param    ptr: GPIO 输出指针
  * @param    port: GPIO 端口（例如 GPIOA）
  * @param    pin: GPIO 引脚（例如 GPIO_PIN_5）
  * @retval   et_gpioOutputError: 错误码
  * @remark   
  */
et_gpioOutputError drv_gpio_output_create(st_gpio_output_ptr ptr, GPIO_TypeDef* port, uint32_t pin) {
    if (!ptr || !port || !pin) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_CONFIG, port, pin, NULL};
        return GPIO_OUTPUT_ERROR_INVALID_CONFIG;
    }

    ptr->hw_ptr = (void*)port;
    ptr->pin = pin;
    ptr->set_high = drv_gpio_output_write_high;
    ptr->set_low = drv_gpio_output_write_low;
    ptr->toggle = drv_gpio_output_toggle_pin;
    ptr->configure = lib_gpio_output_configure;
    ptr->configure(ptr);

    // 添加到链表
    st_gpio_output_ptr curr = gpio_output_hendle_ptr;
    int count = 0;
    while (curr) {
        if (curr->hw_ptr == (void*)port && curr->pin == pin) {
            last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_PIN_CONFLICT, port, pin, "GPIO_OUTPUT"};
            return GPIO_OUTPUT_ERROR_PIN_CONFLICT;
        }
        count++;
        curr = curr->next;
    }
    if (count >= GPIO_OUTPUT_MAX_OUTPUTS) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_CONFIG, port, pin, NULL};
        return GPIO_OUTPUT_ERROR_INVALID_CONFIG;
    }

    ptr->next = gpio_output_hendle_ptr;
    gpio_output_hendle_ptr = ptr;

    last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_NONE, NULL, 0, NULL};
    return GPIO_OUTPUT_ERROR_NONE;
}

/**
  * @name     drv_gpio_output_hw_init
  * @brief    初始化 GPIO 硬件为输出模式
  * @param    port: GPIO 端口（例如 GPIOA）
  * @param    pin: GPIO 引脚（例如 GPIO_PIN_5）
  * @param    output_type: 输出类型（GPIO_MODE_OUTPUT_PP, GPIO_MODE_OUTPUT_OD）
  * @param    pull: 上拉/下拉配置（GPIO_PULLUP, GPIO_PULLDOWN, GPIO_NOPULL）
  * @param    pin_set_index: 引脚组索引（0 为默认）
  * @retval   et_gpioOutputError: 错误码
  * @remark   
  */
et_gpioOutputError drv_gpio_output_hw_init(GPIO_TypeDef* port, 
                                           uint32_t pin, 
                                           uint32_t output_type, 
                                           uint32_t pull, 
                                           uint8_t pin_set_index) {
    // 参数检查
    if (!port || 
        !pin  || 
        (output_type != GPIO_MODE_OUTPUT_PP && output_type != GPIO_MODE_OUTPUT_OD) ||
        (pull != GPIO_PULLUP && pull != GPIO_PULLDOWN && pull != GPIO_NOPULL)) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_CONFIG, port, pin, NULL};
        return GPIO_OUTPUT_ERROR_INVALID_CONFIG;
    }

    // 查找引脚映射
    const GpioOutputPinMap* map = pin_map;
    while (map->port != NULL) {
        if (map->port == port && map->pin == pin) {
            break;
        }
        map++;
    }

    if (map->port == NULL  || 
        pin_set_index >= 2 ||
        (pin_set_index == 1 && map->sets[1].port == NULL)) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_PIN, port, pin, NULL};
        return GPIO_OUTPUT_ERROR_INVALID_PIN;
    }

    const GpioOutputPinSet* set = &map->sets[pin_set_index];
    if (set->port != port || set->pin != pin) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_PIN, port, pin, NULL};
        return GPIO_OUTPUT_ERROR_INVALID_PIN;
    }

    // 检查引脚冲突
    if (set->conflicting_module) {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_PIN_CONFLICT, port, pin, set->conflicting_module};
        return GPIO_OUTPUT_ERROR_PIN_CONFLICT;
    }

    // 启用 GPIO 时钟
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else {
        last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_INVALID_PORT, port, pin, NULL};
        return GPIO_OUTPUT_ERROR_INVALID_PORT;
    }

    // 配置 GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = output_type;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    last_error = (st_gpioOutputErrorDetail){GPIO_OUTPUT_ERROR_NONE, NULL, 0, NULL};
    return GPIO_OUTPUT_ERROR_NONE;
}

/**
  * @name     drv_gpio_output_get_last_error
  * @brief    获取最后错误详情
  * @param    None
  * @retval   st_gpioOutputErrorDetail: 错误详情
  * @remark   
  */
st_gpioOutputErrorDetail drv_gpio_output_get_last_error(void) {
    return last_error;
}

/*---End of File----------------------------------------------------*/
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

#include "drv_timer_pwm.h"
#include <string.h>

// 引脚映射表结构
typedef struct {
    GPIO_TypeDef* port;    // 端口
    uint16_t pin;          // 引脚
    uint32_t af;           // AF 值
} PinConfig;

typedef struct {
    TIM_TypeDef* tim_instance; // 定时器实例
    PinConfig ch1;             // CH1
    PinConfig ch2;             // CH2
    PinConfig ch3;             // CH3
    PinConfig ch4;             // CH4
    PinConfig ch1n;            // CH1N
    PinConfig ch2n;            // CH2N
    PinConfig ch3n;            // CH3N
    PinConfig ch4n;            // CH4N
    uint32_t irq;              // 中断号
} PwmPinSet;

typedef struct {
    TIM_TypeDef* tim_instance; // 定时器实例
    PwmPinSet sets[2];         // 每定时器支持 2 组引脚
} PwmPinMap;

// 引脚映射表（基于 LQFP64 封装）
static const PwmPinMap pin_map[] = {
    [0] = {
        .tim_instance = TIM1,
        .sets = {
            [0] = {
                .tim_instance = TIM1,
                .ch1 = {GPIOA, GPIO_PIN_8,  GPIO_AF2_TIM1},
                .ch2 = {GPIOA, GPIO_PIN_9,  GPIO_AF2_TIM1},
                .ch3 = {GPIOA, GPIO_PIN_10, GPIO_AF2_TIM1},
                .ch4 = {GPIOA, GPIO_PIN_11, GPIO_AF2_TIM1},
                .ch1n = {GPIOA, GPIO_PIN_7,  GPIO_AF2_TIM1},
                .ch2n = {GPIOB, GPIO_PIN_0,  GPIO_AF2_TIM1},
                .ch3n = {GPIOB, GPIO_PIN_1,  GPIO_AF2_TIM1},
                .ch4n = {GPIOB, GPIO_PIN_12, GPIO_AF2_TIM1},
                .irq = TIM1_UP_TIM16_IRQn
            },
            [1] = {
                .tim_instance = TIM1,
                .ch1 = {GPIOB, GPIO_PIN_13, GPIO_AF2_TIM1},
                .ch2 = {GPIOB, GPIO_PIN_14, GPIO_AF2_TIM1},
                .ch3 = {GPIOB, GPIO_PIN_15, GPIO_AF2_TIM1},
                .ch4 = {GPIOA, GPIO_PIN_11, GPIO_AF2_TIM1},
                .ch1n = {GPIOA, GPIO_PIN_7,  GPIO_AF2_TIM1},
                .ch2n = {GPIOB, GPIO_PIN_0,  GPIO_AF2_TIM1},
                .ch3n = {GPIOB, GPIO_PIN_1,  GPIO_AF2_TIM1},
                .ch4n = {GPIOB, GPIO_PIN_12, GPIO_AF2_TIM1},
                .irq = TIM1_UP_TIM16_IRQn
            }
        }
    },
    [1] = {
        .tim_instance = TIM2,
        .sets = {
            [0] = {
                .tim_instance = TIM2,
                .ch1 = {GPIOA, GPIO_PIN_0,  GPIO_AF1_TIM2},
                .ch2 = {GPIOA, GPIO_PIN_1,  GPIO_AF1_TIM2},
                .ch3 = {GPIOA, GPIO_PIN_2,  GPIO_AF1_TIM2},
                .ch4 = {GPIOA, GPIO_PIN_3,  GPIO_AF1_TIM2},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = TIM2_IRQn
            },
            [1] = {
                .tim_instance = TIM2,
                .ch1 = {GPIOA, GPIO_PIN_5,  GPIO_AF1_TIM2},
                .ch2 = {GPIOB, GPIO_PIN_3,  GPIO_AF1_TIM2},
                .ch3 = {GPIOB, GPIO_PIN_10, GPIO_AF1_TIM2},
                .ch4 = {GPIOB, GPIO_PIN_11, GPIO_AF1_TIM2},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = TIM2_IRQn
            }
        }
    },
    [2] = {
        .tim_instance = TIM3,
        .sets = {
            [0] = {
                .tim_instance = TIM3,
                .ch1 = {GPIOA, GPIO_PIN_6,  GPIO_AF2_TIM3},
                .ch2 = {GPIOA, GPIO_PIN_7,  GPIO_AF2_TIM3},
                .ch3 = {GPIOB, GPIO_PIN_0,  GPIO_AF2_TIM3},
                .ch4 = {GPIOB, GPIO_PIN_1,  GPIO_AF2_TIM3},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = TIM3_IRQn
            },
            [1] = {
                .tim_instance = TIM3,
                .ch1 = {GPIOB, GPIO_PIN_4,  GPIO_AF2_TIM3},
                .ch2 = {GPIOB, GPIO_PIN_5,  GPIO_AF2_TIM3},
                .ch3 = {GPIOC, GPIO_PIN_8,  GPIO_AF2_TIM3},
                .ch4 = {GPIOC, GPIO_PIN_9,  GPIO_AF2_TIM3},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = TIM3_IRQn
            }
        }
    },
    [3] = {
        .tim_instance = TIM4,
        .sets = {
            [0] = {
                .tim_instance = TIM4,
                .ch1 = {GPIOB, GPIO_PIN_6,  GPIO_AF2_TIM4},
                .ch2 = {GPIOB, GPIO_PIN_7,  GPIO_AF2_TIM4},
                .ch3 = {GPIOB, GPIO_PIN_8,  GPIO_AF2_TIM4},
                .ch4 = {GPIOB, GPIO_PIN_9,  GPIO_AF2_TIM4},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = TIM4_IRQn
            },
            [1] = {
                .tim_instance = TIM4,
                .ch1 = {GPIOD, GPIO_PIN_12, GPIO_AF2_TIM4},
                .ch2 = {GPIOD, GPIO_PIN_13, GPIO_AF2_TIM4},
                .ch3 = {GPIOD, GPIO_PIN_14, GPIO_AF2_TIM4},
                .ch4 = {GPIOD, GPIO_PIN_15, GPIO_AF2_TIM4},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = TIM4_IRQn
            }
        }
    },
    [4] = {
        .tim_instance = NULL,
        .sets = {
            [0] = {
                .tim_instance = NULL,
                .ch1 = {NULL, 0, 0},
                .ch2 = {NULL, 0, 0},
                .ch3 = {NULL, 0, 0},
                .ch4 = {NULL, 0, 0},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = 0
            },
            [1] = {
                .tim_instance = NULL,
                .ch1 = {NULL, 0, 0},
                .ch2 = {NULL, 0, 0},
                .ch3 = {NULL, 0, 0},
                .ch4 = {NULL, 0, 0},
                .ch1n = {NULL, 0, 0},
                .ch2n = {NULL, 0, 0},
                .ch3n = {NULL, 0, 0},
                .ch4n = {NULL, 0, 0},
                .irq = 0
            }
        }
    }
};

// 全局变量
st_timer_pwm_ptr timer_pwm_handle_ptr = NULL; // PWM 链表头
TIM_HandleTypeDef timer_pwm_handles[TIMER_PWM_MAX_INSTANCES]; // 定时器句柄数组
static uint32_t timer_freq = 170000000; // 默认定时器时钟 170 MHz
static et_pwmDmaError last_error = PWM_DMA_ERROR_NONE; // 最后错误
static st_pwmDmaErrorDetail last_error_detail = {0}; // 最后错误详情

/**
  * @brief  设置PWM占空比
  * @param  ptr: PWM结构体指针
  * @param  duty_cycle: 占空比 (0-100%)
  * @retval None
  */
static void drv_timer_pwm_set_duty_cycle(st_timer_pwm_ptr ptr, uint32_t duty_cycle) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    if (duty_cycle > 100) duty_cycle = 100;
    ptr->duty_cycle = duty_cycle;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(ptr->timer_ptr);
    uint32_t ccr = (arr * duty_cycle) / 100;
    
    switch (ptr->channel) {
        case 1: __HAL_TIM_SET_COMPARE(ptr->timer_ptr, TIM_CHANNEL_1, ccr); break;
        case 2: __HAL_TIM_SET_COMPARE(ptr->timer_ptr, TIM_CHANNEL_2, ccr); break;
        case 3: __HAL_TIM_SET_COMPARE(ptr->timer_ptr, TIM_CHANNEL_3, ccr); break;
        case 4: __HAL_TIM_SET_COMPARE(ptr->timer_ptr, TIM_CHANNEL_4, ccr); break;
    }
}

/**
  * @brief  设置PWM频率
  * @param  ptr: PWM结构体指针
  * @param  frequency: 频率 (Hz)
  * @retval None
  */
static void drv_timer_pwm_set_frequency(st_timer_pwm_ptr ptr, uint32_t frequency) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    if (frequency < 1 || frequency > 100000) return;
    
    // 计算ARR和PSC值
    uint32_t psc = (timer_freq / (frequency * 1000)) - 1;
    uint32_t arr = 999; // 固定ARR值，便于占空比计算
    
    __HAL_TIM_SET_PRESCALER(ptr->timer_ptr, psc);
    __HAL_TIM_SET_AUTORELOAD(ptr->timer_ptr, arr);
    
    // 重新计算占空比
    if (ptr->duty_cycle > 0) {
        drv_timer_pwm_set_duty_cycle(ptr, ptr->duty_cycle);
    }
}

/**
  * @brief  设置PWM输出极性
  * @param  ptr: PWM结构体指针
  * @param  polarity: 输出极性
  * @retval None
  */
static void drv_timer_pwm_set_polarity(st_timer_pwm_ptr ptr, et_pwmDmaPolarity polarity) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    ptr->polarity = polarity;
    uint32_t cc_pol = (polarity == PWM_POLARITY_HIGH) ? TIM_OCPOLARITY_HIGH : TIM_OCPOLARITY_LOW;
    uint32_t ccn_pol = (polarity == PWM_POLARITY_HIGH) ? TIM_OCNPOLARITY_HIGH : TIM_OCNPOLARITY_LOW;
    
    // 使用HAL函数设置极性
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCPolarity = cc_pol;
    sConfigOC.OCNPolarity = ccn_pol;
    
    switch (ptr->channel) {
        case 1: 
            HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_1);
            break;
        case 2: 
            HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_2);
            break;
        case 3: 
            HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_3);
            break;
        case 4: 
            HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_4);
            break;
    }
}

/**
  * @brief  设置PWM死区时间
  * @param  ptr: PWM结构体指针
  * @param  dead_time: 死区时间 (µs)
  * @retval None
  */
static void drv_timer_pwm_set_dead_time(st_timer_pwm_ptr ptr, uint32_t dead_time) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    if (dead_time > 255) dead_time = 255; // 最大死区时间限制
    ptr->dead_time = dead_time;
    
    // 使用HAL函数设置死区时间
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
    sBreakDeadTimeConfig.DeadTime = dead_time;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;
    
    HAL_TIMEx_ConfigBreakDeadTime(ptr->timer_ptr, &sBreakDeadTimeConfig);
}

/**
  * @brief  使能PWM互补输出
  * @param  ptr: PWM结构体指针
  * @param  enable: 是否使能
  * @retval None
  */
static void drv_timer_pwm_enable_complementary_output(st_timer_pwm_ptr ptr, bool enable) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    ptr->enable_complementary = enable;
    
    // 使用HAL函数使能/禁用互补输出
    switch (ptr->channel) {
        case 1: 
            if (enable) {
                HAL_TIMEx_PWMN_Start(ptr->timer_ptr, TIM_CHANNEL_1);
            } else {
                HAL_TIMEx_PWMN_Stop(ptr->timer_ptr, TIM_CHANNEL_1);
            }
            break;
        case 2: 
            if (enable) {
                HAL_TIMEx_PWMN_Start(ptr->timer_ptr, TIM_CHANNEL_2);
            } else {
                HAL_TIMEx_PWMN_Stop(ptr->timer_ptr, TIM_CHANNEL_2);
            }
            break;
        case 3: 
            if (enable) {
                HAL_TIMEx_PWMN_Start(ptr->timer_ptr, TIM_CHANNEL_3);
            } else {
                HAL_TIMEx_PWMN_Stop(ptr->timer_ptr, TIM_CHANNEL_3);
            }
            break;
        case 4: 
            if (enable) {
                HAL_TIMEx_PWMN_Start(ptr->timer_ptr, TIM_CHANNEL_4);
            } else {
                HAL_TIMEx_PWMN_Stop(ptr->timer_ptr, TIM_CHANNEL_4);
            }
            break;
    }
}

/**
  * @brief  启动PWM输出
  * @param  ptr: PWM结构体指针
  * @retval None
  */
static void drv_timer_pwm_start(st_timer_pwm_ptr ptr) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    switch (ptr->channel) {
        case 1: HAL_TIM_PWM_Start(ptr->timer_ptr, TIM_CHANNEL_1); break;
        case 2: HAL_TIM_PWM_Start(ptr->timer_ptr, TIM_CHANNEL_2); break;
        case 3: HAL_TIM_PWM_Start(ptr->timer_ptr, TIM_CHANNEL_3); break;
        case 4: HAL_TIM_PWM_Start(ptr->timer_ptr, TIM_CHANNEL_4); break;
    }
    ptr->state = PWM_STATE_RUNNING;
}

/**
  * @brief  停止PWM输出
  * @param  ptr: PWM结构体指针
  * @retval None
  */
static void drv_timer_pwm_stop(st_timer_pwm_ptr ptr) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    switch (ptr->channel) {
        case 1: HAL_TIM_PWM_Stop(ptr->timer_ptr, TIM_CHANNEL_1); break;
        case 2: HAL_TIM_PWM_Stop(ptr->timer_ptr, TIM_CHANNEL_2); break;
        case 3: HAL_TIM_PWM_Stop(ptr->timer_ptr, TIM_CHANNEL_3); break;
        case 4: HAL_TIM_PWM_Stop(ptr->timer_ptr, TIM_CHANNEL_4); break;
    }
    ptr->state = PWM_STATE_STOPPED;
}

/**
  * @brief  配置PWM参数
  * @param  ptr: PWM结构体指针
  * @retval None
  */
static void drv_timer_pwm_configure(st_timer_pwm_ptr ptr) {
    if (ptr == NULL || ptr->timer_ptr == NULL) return;
    
    // 配置定时器基本参数
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = (ptr->polarity == PWM_POLARITY_HIGH) ? TIM_OCPOLARITY_HIGH : TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    switch (ptr->channel) {
        case 1: HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_1); break;
        case 2: HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_2); break;
        case 3: HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_3); break;
        case 4: HAL_TIM_PWM_ConfigChannel(ptr->timer_ptr, &sConfigOC, TIM_CHANNEL_4); break;
    }
    
    // 配置死区时间
    if (ptr->dead_time > 0) {
        drv_timer_pwm_set_dead_time(ptr, ptr->dead_time);
    }
    
    // 配置互补输出
    if (ptr->enable_complementary) {
        drv_timer_pwm_enable_complementary_output(ptr, true);
    }
}

/**
  * @brief  注册更新回调函数
  * @param  ptr: PWM结构体指针
  * @param  cb: 回调函数
  * @param  arg: 回调参数
  * @retval None
  */
static void drv_timer_pwm_register_update_callback(st_timer_pwm_ptr ptr, PwmUpdateCallback cb, void* arg) {
    if (ptr == NULL) return;
    
    ptr->update_callback = cb;
    ptr->callback_arg = arg;
}

/**
  * @brief  注册错误回调函数
  * @param  ptr: PWM结构体指针
  * @param  cb: 回调函数
  * @param  arg: 回调参数
  * @retval None
  */
static void drv_timer_pwm_register_error_callback(st_timer_pwm_ptr ptr, PwmErrorCallback cb, void* arg) {
    if (ptr == NULL) return;
    
    ptr->error_callback = cb;
    ptr->callback_arg = arg;
}

/**
  * @brief  获取PWM ID
  * @param  ptr: PWM结构体指针
  * @retval PWM ID
  */
static uint8_t drv_timer_pwm_get_id(st_timer_pwm_ptr ptr) {
    if (ptr == NULL) return 0;
    return (uint8_t)ptr->Pwm_id;
}

/**
  * @brief  获取硬件句柄
  * @param  ptr: PWM结构体指针
  * @retval 硬件句柄指针
  */
static void* drv_timer_pwm_get_hw_ptr(st_timer_pwm_ptr ptr) {
    if (ptr == NULL) return NULL;
    return (void*)ptr->timer_ptr;
}

/**
  * @brief  获取通道号
  * @param  ptr: PWM结构体指针
  * @retval 通道号
  */
static uint8_t drv_timer_pwm_get_channel(st_timer_pwm_ptr ptr) {
    if (ptr == NULL) return 0;
    return ptr->channel;
}

/**
  * @brief  获取占空比
  * @param  ptr: PWM结构体指针
  * @retval 占空比
  */
static uint32_t drv_timer_pwm_get_duty_cycle(st_timer_pwm_ptr ptr) {
    if (ptr == NULL) return 0;
    return ptr->duty_cycle;
}

/**
  * @brief  获取频率
  * @param  ptr: PWM结构体指针
  * @retval 频率
  */
static uint32_t drv_timer_pwm_get_frequency(st_timer_pwm_ptr ptr) {
    if (ptr == NULL) return 0;
    
    uint32_t psc = ptr->timer_ptr->Instance->PSC + 1;
    uint32_t arr = ptr->timer_ptr->Instance->ARR + 1;
    return timer_freq / (psc * arr);
}

/**
  * @brief  获取状态
  * @param  ptr: PWM结构体指针
  * @retval 状态
  */
static et_pwmDmaState drv_timer_pwm_get_state(st_timer_pwm_ptr ptr) {
    if (ptr == NULL) return PWM_STATE_STOPPED;
    return ptr->state;
}

/**
  * @brief  创建PWM实例
  * @param  ptr: PWM结构体指针
  * @param  tim_instance: 定时器实例
  * @param  id: PWM ID
  * @retval 错误码
  */
et_pwmDmaError drv_timer_pwm_create(st_timer_pwm_ptr ptr, TIM_TypeDef* tim_instance, et_PwmDma_id id) {
    if (ptr == NULL || tim_instance == NULL) {
        last_error = PWM_DMA_ERROR_INVALID_TIM;
        return last_error;
    }
    
    if (id >= PwmSum) {
        last_error = PWM_DMA_ERROR_INVALID_CHANNEL;
        return last_error;
    }
    
    // 初始化结构体
    memset(ptr, 0, sizeof(st_timer_pwm));
    ptr->Pwm_id = id;
    ptr->timer_ptr = &timer_pwm_handles[id];
    ptr->channel = 1;
    ptr->duty_cycle = 0;
    ptr->polarity = PWM_POLARITY_HIGH;
    ptr->enable_complementary = false;
    ptr->dead_time = 0;
    ptr->state = PWM_STATE_STOPPED;
    
    // 设置函数指针
    ptr->set_duty_cycle = drv_timer_pwm_set_duty_cycle;
    ptr->set_frequency = drv_timer_pwm_set_frequency;
    ptr->set_polarity = drv_timer_pwm_set_polarity;
    ptr->set_dead_time = drv_timer_pwm_set_dead_time;
    ptr->enable_complementary_output = drv_timer_pwm_enable_complementary_output;
    ptr->start = drv_timer_pwm_start;
    ptr->stop = drv_timer_pwm_stop;
    ptr->configure = drv_timer_pwm_configure;
    ptr->register_update_callback = drv_timer_pwm_register_update_callback;
    ptr->register_error_callback = drv_timer_pwm_register_error_callback;
    ptr->get_id = drv_timer_pwm_get_id;
    ptr->get_hw_ptr = drv_timer_pwm_get_hw_ptr;
    ptr->get_channel = drv_timer_pwm_get_channel;
    ptr->get_duty_cycle = drv_timer_pwm_get_duty_cycle;
    ptr->get_frequency = drv_timer_pwm_get_frequency;
    ptr->get_state = drv_timer_pwm_get_state;
    
    // 添加到链表
    ptr->next = timer_pwm_handle_ptr;
    timer_pwm_handle_ptr = ptr;
    
    last_error = PWM_DMA_ERROR_NONE;
    return last_error;
}

/**
  * @brief  硬件初始化
  * @param  tim_instance: 定时器实例
  * @param  pwm_freq: PWM频率
  * @param  pin_set_index: 引脚组索引
  * @param  channel: 通道号
  * @param  id: PWM ID
  * @retval 错误码
  */
et_pwmDmaError drv_timer_pwm_hw_init(TIM_TypeDef* tim_instance, 
                                     uint32_t pwm_freq, 
                                     uint8_t pin_set_index,
                                     uint8_t channel, 
                                     et_PwmDma_id id) {
    if (tim_instance == NULL) {
        last_error = PWM_DMA_ERROR_INVALID_TIM;
        return last_error;
    }
    
    if (pwm_freq < 1 || pwm_freq > 100000) {
        last_error = PWM_DMA_ERROR_INVALID_FREQ;
        return last_error;
    }
    
    if (channel < 1 || channel > 4) {
        last_error = PWM_DMA_ERROR_INVALID_CHANNEL;
        return last_error;
    }
    
    if (id >= PwmSum) {
        last_error = PWM_DMA_ERROR_INVALID_CHANNEL;
        return last_error;
    }
    
    // 查找引脚映射
    const PwmPinMap* pin_map_entry = NULL;
    for (int i = 0; pin_map[i].tim_instance != NULL; i++) {
        if (pin_map[i].tim_instance == tim_instance) {
            pin_map_entry = &pin_map[i];
            break;
        }
    }
    
    if (pin_map_entry == NULL || pin_set_index >= 2) {
        last_error = PWM_DMA_ERROR_INVALID_PIN_SET;
        return last_error;
    }
    
    // 初始化定时器句柄
    TIM_HandleTypeDef* htim = &timer_pwm_handles[id];
    htim->Instance = tim_instance;
    htim->Init.Prescaler = (timer_freq / (pwm_freq * 1000)) - 1;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = 999;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(htim) != HAL_OK) {
        last_error = PWM_DMA_ERROR_INIT_FAILED;
        return last_error;
    }
    
    // 配置GPIO引脚
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    const PwmPinSet* pin_set = &pin_map_entry->sets[pin_set_index];
    
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    // 配置对应通道的引脚
    PinConfig* ch_pin = NULL;
    switch (channel) {
        case 1: ch_pin = (PinConfig*)&pin_set->ch1; break;
        case 2: ch_pin = (PinConfig*)&pin_set->ch2; break;
        case 3: ch_pin = (PinConfig*)&pin_set->ch3; break;
        case 4: ch_pin = (PinConfig*)&pin_set->ch4; break;
    }
    
    if (ch_pin != NULL && ch_pin->port != NULL) {
        GPIO_InitStruct.Pin = ch_pin->pin;
        GPIO_InitStruct.Alternate = ch_pin->af;
        HAL_GPIO_Init(ch_pin->port, &GPIO_InitStruct);
    }
    
    last_error = PWM_DMA_ERROR_NONE;
    return last_error;
}

/**
  * @brief  获取最后错误
  * @retval 错误详情
  */
st_pwmDmaErrorDetail drv_timer_pwm_get_last_error(void) {
    return last_error_detail;
}

// 供协议层直接赋值的硬件操作函数实现
void drv_timer_pwm_set_duty_cycle_impl(void* pwm_ptr, uint32_t duty_cycle) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->set_duty_cycle) {
        ptr->set_duty_cycle(ptr, duty_cycle);
    }
}

void drv_timer_pwm_set_frequency_impl(void* pwm_ptr, uint32_t frequency) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->set_frequency) {
        ptr->set_frequency(ptr, frequency);
    }
}

void drv_timer_pwm_set_polarity_impl(void* pwm_ptr, uint8_t polarity) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->set_polarity) {
        ptr->set_polarity(ptr, (et_pwmDmaPolarity)polarity);
    }
}

void drv_timer_pwm_set_dead_time_impl(void* pwm_ptr, uint32_t dead_time) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->set_dead_time) {
        ptr->set_dead_time(ptr, dead_time);
    }
}

void drv_timer_pwm_enable_complementary_output_impl(void* pwm_ptr, bool enable) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->enable_complementary_output) {
        ptr->enable_complementary_output(ptr, enable);
    }
}

void drv_timer_pwm_start_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->start) {
        ptr->start(ptr);
    }
}

void drv_timer_pwm_stop_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->stop) {
        ptr->stop(ptr);
    }
}

void drv_timer_pwm_configure_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->configure) {
        ptr->configure(ptr);
    }
}

void drv_timer_pwm_register_update_callback_impl(void* pwm_ptr, void (*cb)(void*, void*), void* arg) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->register_update_callback) {
        ptr->register_update_callback(ptr, (PwmUpdateCallback)cb, arg);
    }
}

void drv_timer_pwm_register_error_callback_impl(void* pwm_ptr, void (*cb)(void*, uint32_t, void*), void* arg) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->register_error_callback) {
        ptr->register_error_callback(ptr, (PwmErrorCallback)cb, arg);
    }
}

uint8_t drv_timer_pwm_get_id_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->get_id) {
        return ptr->get_id(ptr);
    }
    return 0;
}

void* drv_timer_pwm_get_hw_ptr_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->get_hw_ptr) {
        return ptr->get_hw_ptr(ptr);
    }
    return NULL;
}

uint8_t drv_timer_pwm_get_channel_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->get_channel) {
        return ptr->get_channel(ptr);
    }
    return 0;
}

uint32_t drv_timer_pwm_get_duty_cycle_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->get_duty_cycle) {
        return ptr->get_duty_cycle(ptr);
    }
    return 0;
}

uint32_t drv_timer_pwm_get_frequency_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->get_frequency) {
        return ptr->get_frequency(ptr);
    }
    return 0;
}

uint8_t drv_timer_pwm_get_state_impl(void* pwm_ptr) {
    st_timer_pwm_ptr ptr = (st_timer_pwm_ptr)pwm_ptr;
    if (ptr && ptr->get_state) {
        return (uint8_t)ptr->get_state(ptr);
    }
    return 0;
}

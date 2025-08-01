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
#if 0

#include "drv_timer_encoder.h"

// 引脚映射表结构
typedef struct {
    TIM_TypeDef* tim_instance; // 定时器实例
    GPIO_TypeDef* port_ch1;    // CH1 端口
    uint16_t pin_ch1;          // CH1 引脚
    uint32_t af_ch1;           // CH1 AF 值
    GPIO_TypeDef* port_ch2;    // CH2 端口
    uint16_t pin_ch2;          // CH2 引脚
    uint32_t af_ch2;           // CH2 AF 值
    uint32_t irq;              // 中断号
} EncoderPinMap;

// 引脚映射表（基于 LQFP64 封装）
static const EncoderPinMap pin_map[] = {
    // { TIM1, GPIOA, GPIO_PIN_8,  GPIO_AF1_TIM1, GPIOA, GPIO_PIN_9,  GPIO_AF1_TIM1, TIM1_UP_TIM16_IRQn },
    { TIM2, GPIOA, GPIO_PIN_0,  GPIO_AF1_TIM2, GPIOA, GPIO_PIN_1,  GPIO_AF1_TIM2, TIM2_IRQn },
    { TIM3, GPIOA, GPIO_PIN_6,  GPIO_AF2_TIM3, GPIOA, GPIO_PIN_7,  GPIO_AF2_TIM3, TIM3_IRQn },
    { TIM4, GPIOB, GPIO_PIN_6,  GPIO_AF2_TIM4, GPIOB, GPIO_PIN_7,  GPIO_AF2_TIM4, TIM4_IRQn },
    { TIM5, GPIOA, GPIO_PIN_0,  GPIO_AF2_TIM5, GPIOA, GPIO_PIN_1,  GPIO_AF2_TIM5, TIM5_IRQn },
    { TIM8, GPIOC, GPIO_PIN_6,  GPIO_AF4_TIM8, GPIOC, GPIO_PIN_7,  GPIO_AF4_TIM8, TIM8_UP_IRQn },
    { NULL, NULL,  0,          0,             NULL,  0,          0,             0 } // 结束标志
};

// 全局变量
st_encoder_ptr timerEncoder_hendle_ptr = NULL; // 编码器链表头
static TIM_HandleTypeDef htim_timer_encoder; // 静态句柄，用于初始化
static uint8_t max_encoders = 4; // 最大编码器数，默认 4

/**
  * @name     lib_encoder_GetCount
  * @brief    获取当前编码器计数值
  * @param    ptr: 编码器指针
  * @return   uint32_t: 当前计数值
  * @remark   从硬件定时器计数器读取
  */
static uint32_t lib_encoder_GetCount(st_encoder_ptr ptr) {
    if (ptr->hw_ptr != NULL) {
        return __HAL_TIM_GET_COUNTER(ptr->hw_ptr);
    }
    return 0;
}

/**
  * @name     lib_encoder_GetState
  * @brief    获取编码器状态
  * @param    ptr: 编码器指针
  * @return   uint8_t: 编码器状态
  * @remark   
  */
static uint8_t lib_encoder_GetState(st_encoder_ptr ptr) {
    return ptr->state;
}

/**
  * @name     lib_encoder_GetDirection
  * @brief    获取编码器旋转方向
  * @param    ptr: 编码器指针
  * @return   et_encoderDirection: 方向 (Forward/Reverse)
  * @remark   基于定时器 DIR 位
  */
static et_encoderDirection lib_encoder_GetDirection(st_encoder_ptr ptr) {
    if (ptr->hw_ptr != NULL) {
        return (__HAL_TIM_IS_TIM_COUNTING_DOWN(ptr->hw_ptr) ? Direction_Reverse : Direction_Forward);
    }
    return Direction_Forward;
}

/**
  * @name     lib_encoder_GetSpeed
  * @brief    获取编码器速度 (脉冲/秒)
  * @param    ptr: 编码器指针
  * @return   uint32_t: 速度
  * @remark   基于计数差和固定时间间隔估算
  */
static uint32_t lib_encoder_GetSpeed(st_encoder_ptr ptr) {
    return ptr->speed;
}

/**
  * @name     lib_encoder_SetCallback
  * @brief    设置编码器回调函数和参数
  * @param    ptr: 编码器指针
  * @param    cb: 回调函数
  * @param    CbArgu: 回调参数
  * @return   None
  * @remark   用于溢出或索引脉冲
  */
static void lib_encoder_SetCallback(st_encoder_ptr ptr, void (*cb)(void*), void *CbArgu) {
    ptr->cb = cb;
    ptr->CbArgu = CbArgu;
}

/**
  * @name     lib_encoder_Stop
  * @brief    停止编码器
  * @param    ptr: 编码器指针
  * @return   None
  * @remark   从链表移除并禁用定时器
  */
static void lib_encoder_Stop(st_encoder_ptr ptr) {
    st_encoder_ptr *curr;
    for (curr = &timerEncoder_hendle_ptr; *curr; curr = &(*curr)->next) {
        if (*curr == ptr) {
            *curr = ptr->next;
            break;
        }
    }

    if (ptr->hw_ptr != NULL) {
        HAL_TIM_Encoder_Stop(ptr->hw_ptr, TIM_CHANNEL_ALL);
        __HAL_TIM_DISABLE_IT(ptr->hw_ptr, TIM_IT_UPDATE); // 禁用溢出中断
    }

    ptr->initialize(ptr);
}

/**
  * @name     lib_encoder_Start
  * @brief    启动编码器
  * @param    ptr: 编码器指针
  * @return   int: 0 成功, -1 失败 (已存在)
  * @remark   添加到链表并启用定时器
  */
static int lib_encoder_Start(st_encoder_ptr ptr) {
    st_encoder_ptr target = timerEncoder_hendle_ptr;
    while (target) {
        if (target == ptr) {
            return -1; // 已存在
        }
        target = target->next;
    }

    ptr->next = timerEncoder_hendle_ptr;
    timerEncoder_hendle_ptr = ptr;

    if (ptr->hw_ptr != NULL) {
        HAL_TIM_Encoder_Start(ptr->hw_ptr, TIM_CHANNEL_ALL);
        __HAL_TIM_ENABLE_IT(ptr->hw_ptr, TIM_IT_UPDATE); // 启用溢出中断
    }

    ptr->state = running_status;
    return 0;
}

/**
  * @name     lib_encoder_ResetCount
  * @brief    重置编码器计数值
  * @param    ptr: 编码器指针
  * @return   None
  * @remark   清零 CNT 寄存器
  */
static void lib_encoder_ResetCount(st_encoder_ptr ptr) {
    if (ptr->hw_ptr != NULL) {
        __HAL_TIM_SET_COUNTER(ptr->hw_ptr, 0);
        ptr->last_count = 0;
        ptr->speed = 0;
    }
}

/**
  * @name     lib_encoder_Initialize
  * @brief    初始化编码器结构体
  * @param    ptr: 编码器指针
  * @return   None
  * @remark   
  */
void lib_encoder_Initialize(st_encoder_ptr ptr) {
    ptr->state = stopped_status;
    ptr->last_count = 0;
    ptr->speed = 0;
    ptr->cb = NULL;
    ptr->CbArgu = NULL;
    ptr->next = NULL;
    ptr->hw_ptr = NULL;
}

/**
  * @name     lib_encoder_Configure
  * @brief    配置编码器函数指针
  * @param    ptr: 编码器指针
  * @return   None
  * @remark   
  */
void lib_encoder_Configure(st_encoder_ptr ptr) {
    ptr->stop = lib_encoder_Stop;
    ptr->start = lib_encoder_Start;
    ptr->set_callback = lib_encoder_SetCallback;
    ptr->get_state = lib_encoder_GetState;
    ptr->get_count = lib_encoder_GetCount;
    ptr->get_direction = lib_encoder_GetDirection;
    ptr->get_speed = lib_encoder_GetSpeed;
    ptr->reset_count = lib_encoder_ResetCount;
}

/**
  * @name     drv_timer_encoder_create
  * @brief    创建编码器实例
  * @param    ptr: 编码器指针
  * @return   uint8_t: 0 成功, 1 失败 (ID 无效)
  * @remark   
  */
uint8_t drv_timer_encoder_create(st_encoder_ptr ptr) {
    if (ptr->Encoder_id >= max_encoders) {
        return 1;
    }

    ptr->configure = lib_encoder_Configure;
    ptr->initialize = lib_encoder_Initialize;
    ptr->configure(ptr);
    ptr->initialize(ptr);
    ptr->hw_ptr = &htim_timer_encoder; // 关联硬件定时器句柄

    return 0;
}

/**
  * @name     drv_timer_encoder_hw_init
  * @brief    初始化硬件定时器为编码器模式并配置引脚
  * @param    tim_instance: 定时器实例 (如 TIM2, TIM3)
  * @param    prescaler: 预分频器值
  * @return   uint8_t: 0 成功, 1 失败 (无效定时器或引脚)
  * @remark   自动配置 CH1/CH2 引脚为 AF 模式
  */
uint8_t drv_timer_encoder_hw_init(TIM_TypeDef* tim_instance, uint32_t prescaler) {
    const EncoderPinMap* map = pin_map;
    while (map->tim_instance != NULL) {
        if (map->tim_instance == tim_instance) {
            break;
        }
        map++;
    }

    if (map->tim_instance == NULL) {
        return 1; // 无效定时器
    }

    // 启用定时器时钟
    if (tim_instance == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
    else if (tim_instance == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
    else if (tim_instance == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
    else if (tim_instance == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
    else if (tim_instance == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
    else if (tim_instance == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();

    // 启用 GPIO 时钟
    if (map->port_ch1 == GPIOA || map->port_ch2 == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    if (map->port_ch1 == GPIOB || map->port_ch2 == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    if (map->port_ch1 == GPIOC || map->port_ch2 == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();

    // 配置 CH1 引脚
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = map->pin_ch1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = map->af_ch1;
    HAL_GPIO_Init(map->port_ch1, &GPIO_InitStruct);

    // 配置 CH2 引脚
    GPIO_InitStruct.Pin = map->pin_ch2;
    GPIO_InitStruct.Alternate = map->af_ch2;
    HAL_GPIO_Init(map->port_ch2, &GPIO_InitStruct);

    // 初始化定时器
    htim_timer_encoder.Instance = tim_instance;
    htim_timer_encoder.Init.Prescaler = prescaler;
    htim_timer_encoder.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_timer_encoder.Init.Period = TIMER_ENCODER_MAX_COUNT;
    htim_timer_encoder.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    
    // 配置编码器模式
    TIM_Encoder_InitTypeDef sConfig = {0};
    sConfig.EncoderMode = TIM_ENCODERMODE_TI12; // A/B 相输入
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0;
    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0;
    
    if (HAL_TIM_Encoder_Init(&htim_timer_encoder, &sConfig) != HAL_OK) {
        return 1; // 初始化失败
    }

    // 启用中断
    HAL_NVIC_SetPriority(map->irq, 0, 0);
    HAL_NVIC_EnableIRQ(map->irq);

    return 0; // 成功
}

/**
  * @name     TIMx_IRQHandler
  * @brief    定时器中断服务函数
  * @param    None
  * @return   None
  * @remark   处理计数器溢出中断 (需用户注册)
  */
void TIM2_IRQHandler(void) { // 示例，使用 TIM2
    st_encoder_ptr ptr;
    for (ptr = timerEncoder_hendle_ptr; ptr; ptr = ptr->next) {
        if (__HAL_TIM_GET_FLAG(ptr->hw_ptr, TIM_FLAG_UPDATE) == SET) {
            __HAL_TIM_CLEAR_FLAG(ptr->hw_ptr, TIM_FLAG_UPDATE);
            if (ptr->cb != NULL) {
                ptr->cb(ptr->CbArgu); // 溢出回调
            }
        }
    }
}
#endif
/*---End of File----------------------------------------------------*/

#if 0

#include "drv_timer_encoder.h"

void overflow_callback(void* arg) { /* 计数器溢出处理 */ }

int main(void) {
    HAL_Init();
    if (drv_timer_encoder_hw_init(TIM2, TIMER_ENCODER_DEFAULT_PRESCALER) != 0) {
        // 初始化失败
        while (1);
    }

    st_encoder encoder;
    encoder.Encoder_id = Encoder0;
    if (drv_timer_encoder_create(&encoder) != 0) {
        // 创建失败
        while (1);
    }

    encoder.set_callback(&encoder, overflow_callback, NULL);
    encoder.start(&encoder);

    while (1) {
        uint32_t count = encoder.get_count(&encoder);
        et_encoderDirection dir = encoder.get_direction(&encoder);
        // 处理 count 和 dir
    }
}

// 注册中断（示例，使用 TIM2）
void TIM2_IRQHandler(void) {
    st_encoder_ptr ptr;
    for (ptr = timerEncoder_hendle_ptr; ptr; ptr = ptr->next) {
        if (__HAL_TIM_GET_FLAG(ptr->hw_ptr, TIM_FLAG_UPDATE) == SET) {
            __HAL_TIM_CLEAR_FLAG(ptr->hw_ptr, TIM_FLAG_UPDATE);
            if (ptr->cb != NULL) {
                ptr->cb(ptr->CbArgu);
            }
        }
    }
}

#endif


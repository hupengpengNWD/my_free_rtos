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

#include "drv_timer_countdown.h"

// 静态句柄，用于初始化
TIM_HandleTypeDef htim_timer_countdown;

/**
  * @name     drv_timer_countdown_enable
  * @brief    启用/禁用定时器中断
  * @param    ptr: 定时器指针
  * @param    opa: 1 启用, 0 禁用
  * @return   None
  * @remark   
  */
void drv_timer_countdown_enable(st_soft_time_ptr ptr, uint8_t opa) {
    TIM_HandleTypeDef* tim = (TIM_HandleTypeDef*)ptr->hw_ptr;
    if (opa == 1) {
        if (tim->Instance == TIM4) {
            HAL_NVIC_EnableIRQ(TIM4_IRQn);
        } else if (tim->Instance == TIM5) {
            HAL_NVIC_EnableIRQ(TIM5_IRQn);
        }
    } else {
        if (tim->Instance == TIM4) {
            HAL_NVIC_DisableIRQ(TIM4_IRQn);
        } else if (tim->Instance == TIM5) {
            HAL_NVIC_DisableIRQ(TIM5_IRQn);
        }
    }
}

/**
  * @name     drv_timer_countdown_create
  * @brief    创建软件定时器并关联硬件定时器
  * @param    ptr: 定时器指针
  * @return   uint8_t: 0 成功
  * @remark   支持动态分配无限定时器
  */
uint8_t drv_timer_countdown_create(st_soft_time_ptr ptr) {
    ptr->configure = lib_softtimer_configure;
    ptr->initialize = lib_softtimer_initialize;
    ptr->enable = drv_timer_countdown_enable;
    ptr->configure(ptr);
    ptr->initialize(ptr);
    ptr->hw_ptr = (void*)&htim_timer_countdown; // 关联硬件定时器句柄
    return 0;
}

/**
  * @name     drv_timer_countdown_hw_init
  * @brief    初始化硬件定时器
  * @param    tim_instance: 定时器实例 (TIM4/TIM5)
  * @param    prescaler: 分频系数
  * @return   None
  * @remark   配置为 1ms 周期性中断
  */
void drv_timer_countdown_hw_init(TIM_TypeDef* tim_instance, uint32_t prescaler) {
    // 启用定时器时钟
    if (tim_instance == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
    else if (tim_instance == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();

    // 初始化定时器
    htim_timer_countdown.Instance = tim_instance;
    htim_timer_countdown.Init.Prescaler = prescaler;
    htim_timer_countdown.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_timer_countdown.Init.Period = TIMER_COUNTDOWN_PERIOD;
    htim_timer_countdown.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_timer_countdown.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim_timer_countdown) != HAL_OK) {
        while(1);
    }

    // 配置时钟源
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim_timer_countdown, &sClockSourceConfig) != HAL_OK) {
        while(1);
    }

    // 配置中断
    if (tim_instance == TIM4) {
        HAL_NVIC_SetPriority(TIM4_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    } else if (tim_instance == TIM5) {
        HAL_NVIC_SetPriority(TIM5_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);
    }

    // 启动定时器和中断
    HAL_TIM_Base_Start_IT(&htim_timer_countdown);
}

/**
  * @name     drv_timer_countdown_irq_callback
  * @brief    定时器中断回调函数
  * @param    None
  * @return   None
  * @remark   每 1ms 调用，更新全局计数值并处理所有定时器
  */
void drv_timer_countdown_irq_callback(void) {
    lib_softtimer_update_tick();
    st_soft_time_ptr ptr = timerCountdown_hendle_ptr;
    while (ptr) { // 链表越长占用中断时间越长, 由定时器个数决定
        ptr->fsm(ptr); // 并且fsm接口越复杂占用中断时间越长，由具体业务逻辑决定  
        ptr = ptr->next;
    }
}

/*---End of File----------------------------------------------------*/
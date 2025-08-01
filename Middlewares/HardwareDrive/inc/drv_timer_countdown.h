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

#ifndef DRV_TIMER_COUNTDOWN_H
#define DRV_TIMER_COUNTDOWN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"
#include "lib_soft_timer.h"

// 硬件定时器配置
#define TIMER_COUNTDOWN_TICK_MS                1           // 硬件定时器周期 1ms
#define TIMER_COUNTDOWN_DEFAULT_PRESCALER      169         // 默认 170 MHz / (169+1) = 1 MHz (1 µs/tick)
#define TIMER_COUNTDOWN_PERIOD                 999         // 1 MHz / 1000 = 1 kHz (1 ms/tick)

extern TIM_HandleTypeDef htim_timer_countdown;

uint8_t drv_timer_countdown_create(st_soft_time_ptr ptr);
void drv_timer_countdown_hw_init(TIM_TypeDef* tim_instance, uint32_t prescaler);
void drv_timer_countdown_irq_callback(void);
void drv_timer_countdown_enable(st_soft_time_ptr ptr, uint8_t opa);

#ifdef __cplusplus
}
#endif

#endif /* DRV_TIMER_COUNTDOWN_H */
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

#ifndef DRV_TIMER_PWM_H
#define DRV_TIMER_PWM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// PWM 硬件配置
#define TIMER_PWM_MAX_INSTANCES 4
#define TIMER_PWM_MAX_CHANNELS 4
#define TIMER_PWM_DEFAULT_FREQ 1000
#define TIMER_PWM_MAX_DEAD_TIME 255

/* PWM 实例 ID */
typedef enum {
    Pwm0 = 0,
    Pwm1,
    Pwm2,
    Pwm3,
    PwmSum = 4
} et_PwmDma_id;

/* PWM 状态 */
typedef enum {
    PWM_STATE_STOPPED = 0,
    PWM_STATE_RUNNING,
} et_pwmDmaState;

/* PWM 极性 */
typedef enum {
    PWM_POLARITY_HIGH = 0,
    PWM_POLARITY_LOW,
} et_pwmDmaPolarity;

/* 错误码 */
typedef enum {
    PWM_DMA_ERROR_NONE = 0,              /*!< 无错误 */
    PWM_DMA_ERROR_INVALID_TIM,           /*!< 无效定时器 */
    PWM_DMA_ERROR_INVALID_FREQ,          /*!< 无效频率 */
    PWM_DMA_ERROR_INVALID_CHANNEL,       /*!< 无效通道 */
    PWM_DMA_ERROR_INVALID_PIN_SET,       /*!< 无效引脚组 */
    PWM_DMA_ERROR_INIT_FAILED,           /*!< 初始化失败 */
    PWM_DMA_ERROR_PIN_CONFLICT,          /*!< 引脚冲突 */
    PWM_DMA_ERROR_CHANNEL_CONFLICT,      /*!< 通道冲突 */
} et_pwmDmaError;

/* 错误详情 */
typedef struct {
    et_pwmDmaError code;
    TIM_HandleTypeDef* timer;
    uint8_t channel;
    uint32_t frequency;
    const char* conflicting_module;
} st_pwmDmaErrorDetail;

// 前置声明
struct timer_pwm;

// 回调类型统一为void*参数
typedef void (*PwmUpdateCallback)(void* pwm_ptr, void* arg);
typedef void (*PwmErrorCallback)(void* pwm_ptr, uint32_t err, void* arg);

typedef struct timer_pwm *st_timer_pwm_ptr;

typedef struct timer_pwm {
    et_PwmDma_id Pwm_id;                     /*!< PWM实例ID，用于标识不同的PWM外设 */
    TIM_HandleTypeDef* timer_ptr;            /*!< 指向STM32 HAL库Timer句柄的指针 */
    uint8_t channel;                         /*!< PWM通道号 (1-4) */
    uint32_t duty_cycle;                     /*!< 占空比 (0-100%) */
    et_pwmDmaPolarity polarity;              /*!< 输出极性 */
    bool enable_complementary;               /*!< 是否启用互补输出 */
    uint32_t dead_time;                      /*!< 死区时间 (µs) */
    et_pwmDmaState state;                    /*!< 当前状态 */
    struct timer_pwm* next;                  /*!< 链表指针 */

    // 回调函数指针
    PwmUpdateCallback update_callback;       /*!< 更新事件回调函数指针 */
    PwmErrorCallback error_callback;         /*!< 错误回调函数指针 */
    void* callback_arg;                      /*!< 回调函数的用户参数指针 */

    // 硬件操作函数指针
    void (*set_duty_cycle)(st_timer_pwm_ptr, uint32_t);           /*!< 设置占空比函数指针 */
    void (*set_frequency)(st_timer_pwm_ptr, uint32_t);            /*!< 设置频率函数指针 */
    void (*set_polarity)(st_timer_pwm_ptr, et_pwmDmaPolarity);    /*!< 设置极性函数指针 */
    void (*set_dead_time)(st_timer_pwm_ptr, uint32_t);            /*!< 设置死区时间函数指针 */
    void (*enable_complementary_output)(st_timer_pwm_ptr, bool);  /*!< 使能互补输出函数指针 */
    void (*start)(st_timer_pwm_ptr);                              /*!< 启动PWM函数指针 */
    void (*stop)(st_timer_pwm_ptr);                               /*!< 停止PWM函数指针 */
    void (*configure)(st_timer_pwm_ptr);                          /*!< 配置函数指针 */

    // 回调函数注册接口
    void (*register_update_callback)(st_timer_pwm_ptr, PwmUpdateCallback, void*);  /*!< 注册更新回调函数 */
    void (*register_error_callback)(st_timer_pwm_ptr, PwmErrorCallback, void*);    /*!< 注册错误回调函数 */

    // 获取器函数指针
    uint8_t (*get_id)(st_timer_pwm_ptr);                          /*!< 获取PWM ID函数指针 */
    void* (*get_hw_ptr)(st_timer_pwm_ptr);                        /*!< 获取硬件句柄函数指针 */
    uint8_t (*get_channel)(st_timer_pwm_ptr);                     /*!< 获取通道号函数指针 */
    uint32_t (*get_duty_cycle)(st_timer_pwm_ptr);                 /*!< 获取占空比函数指针 */
    uint32_t (*get_frequency)(st_timer_pwm_ptr);                  /*!< 获取频率函数指针 */
    et_pwmDmaState (*get_state)(st_timer_pwm_ptr);                /*!< 获取状态函数指针 */

} st_timer_pwm;

extern TIM_HandleTypeDef timer_pwm_handles[TIMER_PWM_MAX_INSTANCES];

et_pwmDmaError drv_timer_pwm_create(st_timer_pwm_ptr ptr, TIM_TypeDef* tim_instance, et_PwmDma_id id);
et_pwmDmaError drv_timer_pwm_hw_init(TIM_TypeDef* tim_instance, uint32_t pwm_freq, uint8_t pin_set_index,
                                     uint8_t channel, et_PwmDma_id id);
st_pwmDmaErrorDetail drv_timer_pwm_get_last_error(void);

// 供协议层直接赋值的硬件操作函数
void drv_timer_pwm_set_duty_cycle_impl(void* pwm_ptr, uint32_t duty_cycle);
void drv_timer_pwm_set_frequency_impl(void* pwm_ptr, uint32_t frequency);
void drv_timer_pwm_set_polarity_impl(void* pwm_ptr, uint8_t polarity);
void drv_timer_pwm_set_dead_time_impl(void* pwm_ptr, uint32_t dead_time);
void drv_timer_pwm_enable_complementary_output_impl(void* pwm_ptr, bool enable);
void drv_timer_pwm_start_impl(void* pwm_ptr);
void drv_timer_pwm_stop_impl(void* pwm_ptr);
void drv_timer_pwm_configure_impl(void* pwm_ptr);
void drv_timer_pwm_register_update_callback_impl(void* pwm_ptr, void (*cb)(void*, void*), void* arg);
void drv_timer_pwm_register_error_callback_impl(void* pwm_ptr, void (*cb)(void*, uint32_t, void*), void* arg);
uint8_t drv_timer_pwm_get_id_impl(void* pwm_ptr);
void* drv_timer_pwm_get_hw_ptr_impl(void* pwm_ptr);
uint8_t drv_timer_pwm_get_channel_impl(void* pwm_ptr);
uint32_t drv_timer_pwm_get_duty_cycle_impl(void* pwm_ptr);
uint32_t drv_timer_pwm_get_frequency_impl(void* pwm_ptr);
uint8_t drv_timer_pwm_get_state_impl(void* pwm_ptr);

#ifdef __cplusplus
}
#endif

#endif /* DRV_TIMER_PWM_H */

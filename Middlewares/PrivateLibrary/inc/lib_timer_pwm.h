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

#ifndef LIB_TIMER_PWM_H
#define LIB_TIMER_PWM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// PWM 协议相关常量
#define PWM_MAX_DUTY_CYCLE 100
#define PWM_MIN_DUTY_CYCLE 0
#define PWM_DEFAULT_FREQUENCY 1000
#define PWM_MAX_FREQUENCY 100000
#define PWM_MIN_FREQUENCY 1

// PWM 状态
typedef enum {
    PWM_PROTOCOL_STATE_STOPPED = 0,
    PWM_PROTOCOL_STATE_RUNNING,
    PWM_PROTOCOL_STATE_ERROR
} PwmProtocolState;

// PWM 极性
typedef enum {
    PWM_PROTOCOL_POLARITY_HIGH = 0,
    PWM_PROTOCOL_POLARITY_LOW
} PwmProtocolPolarity;

// PWM 错误码
typedef enum {
    PWM_PROTOCOL_ERROR_NONE = 0,
    PWM_PROTOCOL_ERROR_INVALID_DUTY_CYCLE,
    PWM_PROTOCOL_ERROR_INVALID_FREQUENCY,
    PWM_PROTOCOL_ERROR_INVALID_CHANNEL,
    PWM_PROTOCOL_ERROR_HARDWARE_ERROR,
    PWM_PROTOCOL_ERROR_NOT_INITIALIZED
} PwmProtocolError;

// PWM 回调类型
typedef void (*PwmProtocolCallback)(void* protocol, int error, uint32_t duty_cycle, uint32_t frequency, void* arg);

// 硬件操作函数指针类型
typedef void (*PwmSetDutyCycleFunc)(void* pwm_ptr, uint32_t duty_cycle);
typedef void (*PwmSetFrequencyFunc)(void* pwm_ptr, uint32_t frequency);
typedef void (*PwmSetPolarityFunc)(void* pwm_ptr, uint8_t polarity);
typedef void (*PwmSetDeadTimeFunc)(void* pwm_ptr, uint32_t dead_time);
typedef void (*PwmEnableComplementaryOutputFunc)(void* pwm_ptr, bool enable);
typedef void (*PwmStartFunc)(void* pwm_ptr);
typedef void (*PwmStopFunc)(void* pwm_ptr);
typedef void (*PwmConfigureFunc)(void* pwm_ptr);

// 回调注册函数指针类型
typedef void (*PwmRegisterUpdateCallbackFunc)(void* pwm_ptr, void (*cb)(void*, void*), void* arg);
typedef void (*PwmRegisterErrorCallbackFunc)(void* pwm_ptr, void (*cb)(void*, uint32_t, void*), void* arg);

// 获取器函数指针类型
typedef uint8_t (*PwmGetIdFunc)(void* pwm_ptr);
typedef void* (*PwmGetHwPtrFunc)(void* pwm_ptr);
typedef uint8_t (*PwmGetChannelFunc)(void* pwm_ptr);
typedef uint32_t (*PwmGetDutyCycleFunc)(void* pwm_ptr);
typedef uint32_t (*PwmGetFrequencyFunc)(void* pwm_ptr);
typedef uint8_t (*PwmGetStateFunc)(void* pwm_ptr);

// PWM 协议结构体
typedef struct timer_pwm_protocol {
    // 硬件相关
    void* pwm_dma_ptr;                                    /*!< 指向底层PWM硬件驱动的指针 */
    PwmSetDutyCycleFunc set_duty_cycle;                   /*!< 设置占空比函数指针 */
    PwmSetFrequencyFunc set_frequency;                    /*!< 设置频率函数指针 */
    PwmSetPolarityFunc set_polarity;                      /*!< 设置极性函数指针 */
    PwmSetDeadTimeFunc set_dead_time;                     /*!< 设置死区时间函数指针 */
    PwmEnableComplementaryOutputFunc enable_complementary_output;  /*!< 使能互补输出函数指针 */
    PwmStartFunc start;                                   /*!< 启动PWM函数指针 */
    PwmStopFunc stop;                                     /*!< 停止PWM函数指针 */
    PwmConfigureFunc configure;                           /*!< 配置函数指针 */
    PwmRegisterUpdateCallbackFunc register_update_callback;  /*!< 注册更新回调函数指针 */
    PwmRegisterErrorCallbackFunc register_error_callback;    /*!< 注册错误回调函数指针 */
    PwmGetIdFunc get_id;                                  /*!< 获取PWM ID函数指针 */
    PwmGetHwPtrFunc get_hw_ptr;                           /*!< 获取硬件句柄函数指针 */
    PwmGetChannelFunc get_channel;                        /*!< 获取通道号函数指针 */
    PwmGetDutyCycleFunc get_duty_cycle;                   /*!< 获取占空比函数指针 */
    PwmGetFrequencyFunc get_frequency;                    /*!< 获取频率函数指针 */
    PwmGetStateFunc get_state;                            /*!< 获取状态函数指针 */

    // 协议状态
    PwmProtocolState state;                               /*!< 当前协议状态 */
    PwmProtocolError last_error;                          /*!< 最后一次错误码 */
    uint8_t channel;                                      /*!< PWM通道号 */
    uint32_t duty_cycle;                                  /*!< 当前占空比 (0-100%) */
    uint32_t frequency;                                   /*!< 当前频率 (Hz) */
    PwmProtocolPolarity polarity;                         /*!< 输出极性 */
    bool enable_complementary;                            /*!< 是否启用互补输出 */
    uint32_t dead_time;                                   /*!< 死区时间 (µs) */
    uint32_t target_duty_cycle;                           /*!< 目标占空比 */
    uint32_t target_frequency;                            /*!< 目标频率 */
    bool auto_restart;                                    /*!< 自动重启标志 */
    uint32_t restart_delay_ms;                            /*!< 重启延迟时间 */

    // 回调
    PwmProtocolCallback protocol_callback;                /*!< 协议回调函数指针 */
    void* callback_arg;                                   /*!< 回调函数参数指针 */

} st_timer_pwm_protocol, *st_timer_pwm_protocol_ptr;

// 协议层接口函数声明
void lib_timer_pwm_configure(st_timer_pwm_protocol_ptr ptr);
void lib_timer_pwm_initialize(st_timer_pwm_protocol_ptr ptr, uint8_t channel, uint32_t frequency,
                              PwmProtocolPolarity polarity, PwmProtocolCallback protocol_callback, void* arg);
void lib_timer_pwm_set_duty_cycle(st_timer_pwm_protocol_ptr ptr, uint32_t duty_cycle);
void lib_timer_pwm_set_frequency(st_timer_pwm_protocol_ptr ptr, uint32_t frequency);
void lib_timer_pwm_set_polarity(st_timer_pwm_protocol_ptr ptr, PwmProtocolPolarity polarity);
void lib_timer_pwm_set_dead_time(st_timer_pwm_protocol_ptr ptr, uint32_t dead_time);
void lib_timer_pwm_enable_complementary_output(st_timer_pwm_protocol_ptr ptr, bool enable);
void lib_timer_pwm_start(st_timer_pwm_protocol_ptr ptr);
void lib_timer_pwm_stop(st_timer_pwm_protocol_ptr ptr);
void lib_timer_pwm_process(st_timer_pwm_protocol_ptr ptr);
void lib_timer_pwm_update(st_timer_pwm_protocol_ptr ptr);
void lib_timer_pwm_error_handler(st_timer_pwm_protocol_ptr ptr, uint32_t error);

// 工具函数
uint32_t lib_timer_pwm_validate_duty_cycle(uint32_t duty_cycle);
uint32_t lib_timer_pwm_validate_frequency(uint32_t frequency);
uint8_t lib_timer_pwm_validate_channel(uint8_t channel);
const char* lib_timer_pwm_get_error_string(PwmProtocolError error);

// 全局数据变量声明
extern uint32_t lib_timer_pwm_global_duty_cycle;
extern uint32_t lib_timer_pwm_global_frequency;

#ifdef __cplusplus
}
#endif

#endif // LIB_TIMER_PWM_H 

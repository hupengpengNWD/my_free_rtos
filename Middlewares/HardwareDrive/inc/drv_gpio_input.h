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

#ifndef DRV_GPIO_INPUT_H
#define DRV_GPIO_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "lib_gpio_input.h"

/* GPIO 输入配置 */
#define GPIO_INPUT_DEFAULT_DEBOUNCE_MS 6    // 默认去抖时间 6ms
#define GPIO_INPUT_DEFAULT_LONG_PRESS_MS 100 // 默认长按时间 100ms
#define GPIO_INPUT_MAX_INPUTS 16             // 最大支持 16 个输入实例
#define GPIO_INPUT_DEFAULT_POLL_PERIOD_US 1000 // 默认轮询周期 1ms
#define GPIO_INPUT_MIN_POLL_PERIOD_US 500   // 最小轮询周期 500us
#define GPIO_INPUT_MAX_POLL_PERIOD_US 5000  // 最大轮询周期 5ms

/* 错误码 */
typedef enum {
    GPIO_INPUT_ERROR_NONE = 0,              // 无错误
    GPIO_INPUT_ERROR_INVALID_PORT,          // 无效端口
    GPIO_INPUT_ERROR_INVALID_PIN,           // 无效引脚
    GPIO_INPUT_ERROR_INVALID_CONFIG,        // 无效配置
    GPIO_INPUT_ERROR_INIT_FAILED,           // 初始化失败
    GPIO_INPUT_ERROR_PIN_CONFLICT,          // 引脚冲突
    GPIO_INPUT_ERROR_TIMER_NOT_INIT,       // 定时器未初始化
    GPIO_INPUT_ERROR_INVALID_POLL_PERIOD    // 无效轮询周期
} et_gpioError;

/* 错误详情 */
typedef struct {
    et_gpioError code;           // 错误码
    GPIO_TypeDef* port;          // 冲突端口
    uint32_t pin;                // 冲突引脚
    const char* conflicting_module; // 冲突模块（如 "TIM2"）
} st_gpioErrorDetail;

extern st_gpio_input_ptr gpio_input_hendle_ptr;
extern st_soft_time gpio_input_shared_timer;
extern uint32_t gpio_input_shared_poll_period_us;
extern void lib_gpio_input_polling(void* arg);
et_gpioError drv_gpio_input_create(st_gpio_input_ptr ptr, GPIO_TypeDef* port, uint32_t pin);
et_gpioError drv_gpio_input_hw_init(GPIO_TypeDef* port, uint32_t pin, uint32_t pull, uint8_t pin_set_index);
st_gpioErrorDetail drv_gpio_input_get_last_error(void);
uint8_t drv_gpio_input_read_level(st_gpio_input_ptr ptr);

#ifdef __cplusplus
}
#endif

#endif /* DRV_GPIO_INPUT_H */

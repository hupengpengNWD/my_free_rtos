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

#ifndef DRV_GPIO_OUTPUT_H
#define DRV_GPIO_OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "lib_gpio_output.h"

/* GPIO 输出配置 */
#define GPIO_OUTPUT_MAX_OUTPUTS 16 // 最大支持 16 个输出实例

/* 错误码 */
typedef enum {
    GPIO_OUTPUT_ERROR_NONE = 0,              // 无错误
    GPIO_OUTPUT_ERROR_INVALID_PORT,          // 无效端口
    GPIO_OUTPUT_ERROR_INVALID_PIN,           // 无效引脚
    GPIO_OUTPUT_ERROR_INVALID_CONFIG,        // 无效配置
    GPIO_OUTPUT_ERROR_INIT_FAILED,           // 初始化失败
    GPIO_OUTPUT_ERROR_PIN_CONFLICT           // 引脚冲突
} et_gpioOutputError;

/* 错误详情 */
typedef struct {
    et_gpioOutputError code;        // 错误码
    GPIO_TypeDef* port;             // 冲突端口
    uint32_t pin;                   // 冲突引脚
    const char* conflicting_module; // 冲突模块
} st_gpioOutputErrorDetail;

extern st_gpio_output_ptr gpio_output_hendle_ptr;

et_gpioOutputError drv_gpio_output_create(st_gpio_output_ptr ptr, GPIO_TypeDef* port, uint32_t pin);
et_gpioOutputError drv_gpio_output_hw_init(GPIO_TypeDef* port, uint32_t pin, uint32_t output_type, uint32_t pull, uint8_t pin_set_index);
st_gpioOutputErrorDetail drv_gpio_output_get_last_error(void);
void drv_gpio_output_write_high(st_gpio_output_ptr ptr);
void drv_gpio_output_write_low(st_gpio_output_ptr ptr);
void drv_gpio_output_toggle_pin(st_gpio_output_ptr ptr);
void lib_gpio_output_polling(void* arg);

#ifdef __cplusplus
}
#endif

#endif /* DRV_GPIO_OUTPUT_H */

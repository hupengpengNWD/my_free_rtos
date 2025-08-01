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
  
#ifndef PROTECTOR_H
#define PROTECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>  
#include <math.h>   
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "stm32g4xx_hal.h"
#include "drv_gpio_output.h"
#include "drv_timer_countdown.h"
#include "maintain.h"
#include "lib_uart_log.h"
#include "com.h"

void protector_task_init(void);
void protector_log_level_write_callback(uint8_t param_id, const void* value, uint8_t length, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* PROTECTOR_H */

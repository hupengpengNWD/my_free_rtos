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

#ifndef MAINTAIN_H
#define MAINTAIN_H

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

#include "drv_uart_dma.h"
#include "lib_uart_log.h"
#include "lib_uart_debug.h"
#include "com.h"

// 维护命令结构
typedef struct {
    char cmd_line[128];    // 命令字符串
    bool is_valid;         // 命令是否有效
} MaintainCommand;

// 维护任务属性
extern osThreadId_t maintain_taskHandle;
extern const osThreadAttr_t maintain_task_attributes;

// 维护功能初始化函数
void maintain_task_init(void);

// 维护任务主函数
void maintain_task_func(void *argument);

// 维护协议回调函数
void maintain_protocol_callback(void* protocol, int error, uint8_t* data, uint32_t len, void* arg);

// 处理维护命令
void maintain_process_command(const char* cmd_line);

// 获取maintain模块的UART实例，供其他模块共享使用
extern st_uart_dma* get_maintain_uart_instance(void);
extern bool maintain_module_initialized(void);

// 日志等级写回调函数
void maintain_log_level_write_callback(uint8_t param_id, const void* value, uint8_t length, void* user_data);

// 参数帮助功能函数
void maintain_show_parameter_overview(void);
void maintain_show_parameter_page(int page_num);

#ifdef __cplusplus
}
#endif

#endif /* MAINTAIN_H */ 

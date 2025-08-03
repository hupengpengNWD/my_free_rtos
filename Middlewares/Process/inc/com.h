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
  
#ifndef COMCOM_H
#define COMCOM_H

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
#include "lib_uart_protocol.h"
#include "lib_soft_timer.h"


// 参数权限枚举
typedef enum {
    PARAM_PERMISSION_READ_ONLY = 0x01,    // 只读
    PARAM_PERMISSION_WRITE_ONLY = 0x02,   // 只写
    PARAM_PERMISSION_READ_WRITE = 0x03,   // 可读可写
    PARAM_PERMISSION_NONE = 0x00          // 无权限
} ParamPermission;

// 参数类型枚举
typedef enum {
    PARAM_TYPE_INT = 0x01,        // 整型
    PARAM_TYPE_FLOAT = 0x02,      // 浮点型
    PARAM_TYPE_STRING = 0x03,     // 字符串
    PARAM_TYPE_BOOL = 0x04,       // 布尔型
    PARAM_TYPE_UINT8 = 0x05,      // 8位无符号整型
    PARAM_TYPE_UINT16 = 0x06,     // 16位无符号整型
    PARAM_TYPE_UINT32 = 0x07,     // 32位无符号整型
    PARAM_TYPE_LOG_LEVEL = 0x08   // 日志等级类型
} ParamType;

// 参数写回调函数类型定义
typedef void (*ParamWriteCallback)(uint8_t param_id, const void* value, uint8_t length, void* user_data);

// 参数结构体
typedef struct {
    uint8_t param_id;             // 参数ID
    ParamType param_type;         // 参数类型
    ParamPermission permission;   // 参数权限
    uint8_t max_length;           // 最大长度（用于字符串类型）
    void* param_ptr;              // 参数地址指针
    const char* param_name;       // 参数名称（用于调试）
    const char* param_desc;       // 参数描述
    ParamWriteCallback write_callback;  // 写回调函数
    void* callback_user_data;     // 回调函数用户数据
} ParamInfo;

// 参数管理结构体
typedef struct {
    ParamInfo* param_list;        // 参数列表
    uint8_t param_count;          // 参数数量
    uint8_t max_params;           // 最大参数数量
} ParamManager;

  /*
	协议格式：
	| 帧头(1byte) | 从机编号(1byte) | 操作码(1byte) | 数据长度(1byte) | 参数编号(1byte) | 参数类型(1byte) | 参数长度(1byte) | 参数值(Nbyte) | CRC16(2byte) | 帧尾(1byte) |
 */ 

extern int lib_uart_protocol_data1;
extern float lib_uart_protocol_data2;

// 参数管理函数声明
void com_param_manager_init(void);
ParamInfo* com_param_manager_get_param(uint8_t param_id);
ParamInfo* com_param_manager_get_param_by_name(const char* param_name);
bool com_param_manager_read_param(uint8_t param_id, void* value, uint8_t* length);
bool com_param_manager_write_param(uint8_t param_id, const void* value, uint8_t length);
bool com_param_manager_set_write_callback(uint8_t param_id, ParamWriteCallback callback, void* user_data);
uint8_t com_param_manager_get_param_count(void);
ParamInfo* com_param_manager_get_param_list(void);
void com_task_init(void);

/*
 * 参数管理方法：
 * 
 * 1. 添加新参数：
 *    - 在 com_param_manager_init() 函数中注册新参数
 *    - 定义参数变量和参数信息结构
 * 
 * 2. 支持的参数类型：
 *    - PARAM_TYPE_INT: 整型
 *    - PARAM_TYPE_FLOAT: 浮点型  
 *    - PARAM_TYPE_STRING: 字符串
 *    - PARAM_TYPE_BOOL: 布尔型
 *    - PARAM_TYPE_UINT8: 8位无符号整型
 *    - PARAM_TYPE_UINT16: 16位无符号整型
 *    - PARAM_TYPE_UINT32: 32位无符号整型
 * 
 * 3. 参数权限：
 *    - PARAM_PERMISSION_READ_ONLY: 只读
 *    - PARAM_PERMISSION_WRITE_ONLY: 只写
 *    - PARAM_PERMISSION_READ_WRITE: 可读可写
 *    - PARAM_PERMISSION_NONE: 无权限
 * 
 * 4. 日志等级参数说明：
 *    - PARAM_TYPE_LOG_LEVEL: 日志等级类型 (0-4)
 *    - 0: LOG_LEVEL_NONE - 不打印任何信息
 *    - 1: LOG_LEVEL_ERROR - 只打印错误信息
 *    - 2: LOG_LEVEL_WARN - 打印警告和错误信息
 *    - 3: LOG_LEVEL_INFO - 打印执行信息、警告和错误信息
 *    - 4: LOG_LEVEL_DEBUG - 打印所有信息
 * 
 * 4. 协议格式：
 *    | 帧头(1byte) | 从机编号(1byte) | 操作码(1byte) | 数据长度(1byte) | 
 *    | 参数编号(1byte) | 参数类型(1byte) | 参数长度(1byte) | 参数值(Nbyte) | 
 *    | CRC16(2byte) | 帧尾(1byte) |
 * 
 * 5. 操作码：
 *    - OP_READ (0x01): 读取参数
 *    - OP_WRITE (0x02): 写入参数
 */

#ifdef __cplusplus
}
#endif

#endif



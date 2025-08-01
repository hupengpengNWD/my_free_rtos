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

#ifndef LIB_UART_LOG_H
#define LIB_UART_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_os2.h"
#include "stm32g4xx_hal.h"

/* 日志级别枚举 */
typedef enum {
    LOG_LEVEL_NONE = 0,    // 不打印任何信息
    LOG_LEVEL_ERROR,       // 只打印错误信息
    LOG_LEVEL_WARN,        // 打印警告和错误信息
    LOG_LEVEL_INFO,        // 打印执行信息、警告和错误信息
    LOG_LEVEL_DEBUG        // 打印所有信息
} log_level_t;

/* 日志级别宏定义 - 可以通过修改这个宏来控制日志级别 */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

/* 日志前缀宏定义 */
#define LOG_PREFIX_ERROR   "[ERROR]"
#define LOG_PREFIX_WARN    "[WARN] "
#define LOG_PREFIX_INFO    "[INFO] "
#define LOG_PREFIX_DEBUG   "[DEBUG]"

/* 多实例日志打印宏定义 */
#define MODULE_LOG_ERROR(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_ERROR) { \
            lib_uart_log_print(ptr, LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define MODULE_LOG_WARN(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_WARN) { \
            lib_uart_log_print(ptr, LOG_LEVEL_WARN, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define MODULE_LOG_INFO(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_INFO) { \
            lib_uart_log_print(ptr, LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define MODULE_LOG_DEBUG(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_DEBUG) { \
            lib_uart_log_print(ptr, LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/* 中断安全的日志打印宏定义 - 使用队列方案 */
#define MODULE_LOG_ERROR_FROM_ISR(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_ERROR) { \
            lib_uart_log_print_from_isr(ptr, LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define MODULE_LOG_WARN_FROM_ISR(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_WARN) { \
            lib_uart_log_print_from_isr(ptr, LOG_LEVEL_WARN, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define MODULE_LOG_INFO_FROM_ISR(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_INFO) { \
            lib_uart_log_print_from_isr(ptr, LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define MODULE_LOG_DEBUG_FROM_ISR(ptr, fmt, ...) \
    do { \
        if ((ptr)->log_level >= LOG_LEVEL_DEBUG) { \
            lib_uart_log_print_from_isr(ptr, LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/* 日志消息结构体 */
typedef struct {
    char message[512];                     /*!< 日志消息内容 */
    log_level_t level;                     /*!< 日志级别 */
    char module_name[16];                  /*!< 模块名称 */
    uint32_t timestamp;                    /*!< 时间戳 */
} log_message_t;

/* 硬件抽象层接口 - 由外部实现 */
typedef void (*uart_log_send_func)(void* uart_ptr, uint8_t* data, uint32_t len);

typedef void (*uart_log_register_tx_complete_callback_func)(void* uart_ptr, void (*cb)(void*, void*), void* arg);

/* 日志模块结构体 */
typedef struct uart_log {
    void* uart_dma_ptr;                    /*!< 指向底层UART硬件驱动的指针 */
    uart_log_send_func send;               /*!< 发送数据函数指针 */
    log_level_t log_level;                 /*!< 模块特定的日志等级 */
    char module_name[16];                  /*!< 模块名称，用于日志前缀 */
    SemaphoreHandle_t tx_complete_sem;     /*!< 发送完成信号量 */
    uart_log_register_tx_complete_callback_func register_tx_complete_callback; /*!< 注册发送完成回调函数 */
} st_uart_log, *st_uart_log_ptr;

/* 函数声明 */
void lib_uart_log_init(st_uart_log_ptr ptr, const char* module_name, log_level_t level);
void lib_uart_log_print(st_uart_log_ptr ptr, log_level_t level, const char* file, const char* func, int line, const char* fmt, ...);
void lib_uart_log_print_from_isr(st_uart_log_ptr ptr, log_level_t level, const char* file, const char* func, int line, const char* fmt, ...);
int lib_uart_log_process_queue(st_uart_log_ptr ptr);
void lib_uart_log_set_level(st_uart_log_ptr ptr, log_level_t level);
log_level_t lib_uart_log_get_level(st_uart_log_ptr ptr);
bool lib_uart_log_is_initialized(void);

/* 兼容性宏定义 - 保持向后兼容 */
#define LOG_ERROR(fmt, ...) \
    do { \
        if (LOG_LEVEL >= LOG_LEVEL_ERROR) { \
            lib_uart_log_print(NULL, LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_WARN(fmt, ...) \
    do { \
        if (LOG_LEVEL >= LOG_LEVEL_WARN) { \
            lib_uart_log_print(NULL, LOG_LEVEL_WARN, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_INFO(fmt, ...) \
    do { \
        if (LOG_LEVEL >= LOG_LEVEL_INFO) { \
            lib_uart_log_print(NULL, LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_DEBUG(fmt, ...) \
    do { \
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) { \
            lib_uart_log_print(NULL, LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* LIB_UART_LOG_H */ 

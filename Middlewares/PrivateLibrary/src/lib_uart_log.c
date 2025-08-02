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

#include "lib_uart_log.h"
#include <string.h>
#include <stdio.h>

/* 全局变量 */
static QueueHandle_t log_queue = NULL;           /*!< 日志消息队列 */
static bool uart_log_initialized = false;        /*!< 日志模块初始化标志 */
static st_uart_log_ptr global_uart_log_ptr = NULL; /*!< 全局UART日志实例指针 */

/* 静态函数声明 */
static const char* get_log_prefix(log_level_t level);
static void extract_filename(const char* full_path, char* filename, size_t max_len);


/**
 * @brief UART发送完成回调函数
 * @param uart_ptr UART实例指针
 * @param arg 指向st_uart_log_ptr实例的指针
 */
static void uart_log_tx_complete_callback(void* uart_ptr, void* arg) {
    st_uart_log_ptr log_ptr = (st_uart_log_ptr)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (log_ptr->tx_complete_sem) {
        // 释放发送完成信号量
        xSemaphoreGiveFromISR(log_ptr->tx_complete_sem, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/**
 * @brief 初始化UART日志模块
 * @param ptr 日志模块实例指针
 * @param module_name 模块名称
 * @param level 日志级别
 */
void lib_uart_log_init(st_uart_log_ptr ptr, const char* module_name, log_level_t level) {
    if (!ptr || !module_name) {
        return;
    }

    // 设置模块名称
    strncpy(ptr->module_name, module_name, sizeof(ptr->module_name) - 1);
    ptr->module_name[sizeof(ptr->module_name) - 1] = '\0';

    // 设置日志等级
    ptr->log_level = level;

    // 创建发送完成信号量
    ptr->tx_complete_sem = xSemaphoreCreateBinary();
    if (ptr->tx_complete_sem == NULL) {
        return;
    }

    // 注册发送完成回调
    if (ptr->register_tx_complete_callback) {
        ptr->register_tx_complete_callback(ptr->uart_dma_ptr, uart_log_tx_complete_callback, ptr);
    }

    // 创建日志队列（只在第一次初始化时创建）
    if (log_queue == NULL) {
        log_queue = xQueueCreate(20, sizeof(log_message_t));
        uart_log_initialized = true;
        global_uart_log_ptr = ptr;
    }
}

/**
 * @brief 普通日志打印函数（任务上下文）
 * @param ptr 日志模块实例指针
 * @param level 日志级别
 * @param file 文件名
 * @param func 函数名
 * @param line 行号
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void lib_uart_log_print(st_uart_log_ptr ptr, 
                        log_level_t level, 
                        const char* file, 
                        const char* func, 
                        int line, 
                        const char* fmt, ...) {
    if (!ptr || level > ptr->log_level) {
        return;
    }

    // 创建日志消息
    log_message_t log_msg;
    char filename[32];
    char temp_buffer[256];
    
    // 提取文件名（去掉路径）
    extract_filename(file, filename, sizeof(filename));
    
    // 格式化消息内容
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp_buffer, sizeof(temp_buffer), fmt, args);
    va_end(args);
    
    // 格式化完整日志信息
    snprintf(log_msg.message, sizeof(log_msg.message), "%s [%s] %s:%s:%d %s\r\n",
             get_log_prefix(level), ptr->module_name, filename, func, line, temp_buffer);
    
    log_msg.level = level;
    strncpy(log_msg.module_name, ptr->module_name, sizeof(log_msg.module_name) - 1);
    log_msg.module_name[sizeof(log_msg.module_name) - 1] = '\0';
    log_msg.timestamp = xTaskGetTickCount();

    // 将日志消息发送到队列
    if (log_queue != NULL) {
        // 使用非阻塞方式发送，避免死锁
        xQueueSend(log_queue, &log_msg, 0);
    }


}

/**
 * @brief 中断安全的日志打印函数（中断上下文）
 * @param ptr 日志模块实例指针
 * @param level 日志级别
 * @param file 文件名
 * @param func 函数名
 * @param line 行号
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void lib_uart_log_print_from_isr(st_uart_log_ptr ptr, 
                                 log_level_t level, 
                                 const char* file, 
                                 const char* func, 
                                 int line, 
                                 const char* fmt, ...) {
    if (!ptr || level > ptr->log_level) {
        return;
    }

    // 创建日志消息
    log_message_t log_msg;
    char filename[32];
    char temp_buffer[256];
    
    // 提取文件名（去掉路径）
    extract_filename(file, filename, sizeof(filename));
    
    // 格式化消息内容
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp_buffer, sizeof(temp_buffer), fmt, args);
    va_end(args);
    
    // 格式化完整日志信息
    snprintf(log_msg.message, sizeof(log_msg.message), "%s [%s] %s:%s:%d %s\r\n",
             get_log_prefix(level), ptr->module_name, filename, func, line, temp_buffer);
    
    log_msg.level = level;
    strncpy(log_msg.module_name, ptr->module_name, sizeof(log_msg.module_name) - 1);
    log_msg.module_name[sizeof(log_msg.module_name) - 1] = '\0';
    log_msg.timestamp = xTaskGetTickCountFromISR();

    // 将日志消息发送到队列（中断安全版本）
    if (log_queue != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        // 使用非阻塞方式发送，避免死锁
        if (xQueueSendFromISR(log_queue, &log_msg, &xHigherPriorityTaskWoken) != pdTRUE) {
            // 队列满时，丢弃消息而不是阻塞
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief 处理日志队列中的消息
 * @param ptr 日志模块实例指针
 * @return 处理的日志消息数量
 */
int lib_uart_log_process_queue(st_uart_log_ptr ptr) {
    if (!ptr || !log_queue) {
        return 0;
    }

    int processed_count = 0;
    log_message_t log_msg = {0};

    // 先获取队列中的消息数量
    UBaseType_t message_count = uxQueueMessagesWaiting(log_queue);
    
    // 根据消息数量，一次取出一条发送
    for (UBaseType_t i = 0; i < message_count; i++) {
        if (xQueueReceive(log_queue, &log_msg, 0) == pdTRUE) {
            // 通过UART发送日志信息
            if (ptr->send) {
                ptr->send(ptr->uart_dma_ptr, (uint8_t*)log_msg.message, strlen(log_msg.message));
                // 等待发送完成信号量
                if (ptr->tx_complete_sem) {
                    xSemaphoreTake(ptr->tx_complete_sem, portMAX_DELAY);
                }
            }
            processed_count++;
            
            // 添加小延时，确保UART发送完成
            // osDelay(10);//使用等待发送完成信号量的方法，所以不需要再延时
        }
    }

    return processed_count;
}



/**
 * @brief 设置模块日志级别
 * @param ptr 日志模块实例指针
 * @param level 新的日志级别
 */
void lib_uart_log_set_level(st_uart_log_ptr ptr, log_level_t level) {
    if (ptr) {
        ptr->log_level = level;
    }
}

/**
 * @brief 获取模块当前日志级别
 * @param ptr 日志模块实例指针
 * @return 当前日志级别
 */
log_level_t lib_uart_log_get_level(st_uart_log_ptr ptr) {
    return ptr ? ptr->log_level : LOG_LEVEL_NONE;
}

/**
 * @brief 检查UART日志模块是否已初始化
 * @return true表示已初始化，false表示未初始化
 */
bool lib_uart_log_is_initialized(void) {
    return uart_log_initialized;
}

/**
 * @brief 获取日志级别对应的前缀字符串
 * @param level 日志级别
 * @return 前缀字符串
 */
static const char* get_log_prefix(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_ERROR:
            return LOG_PREFIX_ERROR;
        case LOG_LEVEL_WARN:
            return LOG_PREFIX_WARN;
        case LOG_LEVEL_INFO:
            return LOG_PREFIX_INFO;
        case LOG_LEVEL_DEBUG:
            return LOG_PREFIX_DEBUG;
        default:
            return "[UNKNOWN]";
    }
}

/**
 * @brief 从完整路径中提取文件名
 * @param full_path 完整路径
 * @param filename 输出文件名缓冲区
 * @param max_len 缓冲区最大长度
 */
static void extract_filename(const char* full_path, char* filename, size_t max_len) {
    if (full_path == NULL || filename == NULL) {
        strncpy(filename, "unknown", max_len - 1);
        filename[max_len - 1] = '\0';
        return;
    }
    
    const char* last_slash = strrchr(full_path, '/');
    const char* last_backslash = strrchr(full_path, '\\');
    
    const char* last_separator = (last_slash > last_backslash) ? last_slash : last_backslash;
    
    if (last_separator != NULL) {
        strncpy(filename, last_separator + 1, max_len - 1);
    } else {
        strncpy(filename, full_path, max_len - 1);
    }
    
    filename[max_len - 1] = '\0';
}

/*---End of File----------------------------------------------------*/

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

#include "lib_uart_debug.h"
#include "stm32g4xx_hal.h"
#include <string.h>
#include <stdio.h>

// 私有函数声明
static void debug_protocol_idle_callback(void* uart_ptr, uint32_t size, uint32_t timeout, void* arg);
static void debug_protocol_error_callback(void* uart_ptr, uint32_t error, void* arg);
static void debug_protocol_process_received_data(st_debug_protocol_ptr ptr, uint8_t* data, uint32_t len);

/**
 * @brief 配置调试协议模块
 * @param ptr 协议实例指针
 */
void lib_debug_protocol_configure(st_debug_protocol_ptr ptr) {
    if (!ptr) return;
    
    // 初始化状态
    ptr->state = DEBUG_STATE_IDLE;
    ptr->last_error = DEBUG_PROTOCOL_ERROR_NONE;
    ptr->cmd_index = 0;
    ptr->cmd_ready = false;
    ptr->last_receive_time = 0;
    
    // 清空命令缓冲区
    memset(ptr->cmd_buffer, 0, DEBUG_MAX_CMD_LEN);
}

/**
 * @brief 初始化调试协议模块
 * @param ptr 协议实例指针
 * @param baudrate 波特率
 * @param protocol_callback 协议回调函数
 * @param arg 回调函数参数
 */
void lib_debug_protocol_initialize(st_debug_protocol_ptr ptr, uint32_t baudrate, DebugProtocolCallback protocol_callback, void* arg) {
    if (!ptr) return;
    
    // 配置协议
    lib_debug_protocol_configure(ptr);
    
    // 设置回调函数
    ptr->protocol_callback = protocol_callback;
    ptr->callback_arg = arg;
    
    // 配置硬件
    if (ptr->set_baudrate) {
        ptr->set_baudrate(ptr->uart_dma_ptr, baudrate);
    }
    
    if (ptr->enable_interrupts) {
        ptr->enable_interrupts(ptr->uart_dma_ptr);
    }
    
    // 注册回调函数
    if (ptr->register_idle_callback) {
        ptr->register_idle_callback(ptr->uart_dma_ptr, debug_protocol_idle_callback, ptr);
    }
    
    if (ptr->register_error_callback) {
        ptr->register_error_callback(ptr->uart_dma_ptr, debug_protocol_error_callback, ptr);
    }
    
    // 启动接收
    if (ptr->receive) {
        ptr->receive(ptr->uart_dma_ptr);
    }
}

/**
 * @brief 处理调试协议
 * @param ptr 协议实例指针
 */
void lib_debug_protocol_process(st_debug_protocol_ptr ptr) {
    if (!ptr) return;
    
    // 检查超时
    if (ptr->state == DEBUG_STATE_RECEIVING) {
        uint32_t current_time = HAL_GetTick();
        if (current_time - ptr->last_receive_time > DEBUG_TIMEOUT_MS) {
            // 超时，处理已接收的数据
            if (ptr->cmd_index > 0) {
                ptr->state = DEBUG_STATE_CMD_READY;
                ptr->cmd_ready = true;
                
                if (ptr->protocol_callback) {
                    ptr->protocol_callback(ptr, DEBUG_PROTOCOL_ERROR_NONE, ptr->cmd_buffer, ptr->cmd_index, ptr->callback_arg);
                }
            }
            
            ptr->state = DEBUG_STATE_IDLE;
            ptr->cmd_index = 0;
        }
    }
}

/**
 * @brief 发送数据
 * @param ptr 协议实例指针
 * @param data 数据指针
 * @param len 数据长度
 */
void lib_debug_protocol_send_data(st_debug_protocol_ptr ptr, uint8_t* data, uint32_t len) {
    if (!ptr || !data || len == 0) return;
    
    if (ptr->send) {
        ptr->send(ptr->uart_dma_ptr, data, len);
    }
}

/**
 * @brief 检查命令是否就绪
 * @param ptr 协议实例指针
 * @return true: 命令就绪, false: 命令未就绪
 */
bool lib_debug_protocol_is_cmd_ready(st_debug_protocol_ptr ptr) {
    if (!ptr) return false;
    return ptr->cmd_ready;
}

/**
 * @brief 获取命令数据
 * @param ptr 协议实例指针
 * @param data 输出数据缓冲区
 * @param len 输出数据长度
 */
void lib_debug_protocol_get_cmd(st_debug_protocol_ptr ptr, uint8_t* data, uint32_t* len) {
    if (!ptr || !data || !len) return;
    
    if (ptr->cmd_ready && ptr->cmd_index > 0) {
        memcpy(data, ptr->cmd_buffer, ptr->cmd_index);
        *len = ptr->cmd_index;
    } else {
        *len = 0;
    }
}

/**
 * @brief 清除命令
 * @param ptr 协议实例指针
 */
void lib_debug_protocol_clear_cmd(st_debug_protocol_ptr ptr) {
    if (!ptr) return;
    
    ptr->cmd_ready = false;
    ptr->cmd_index = 0;
    memset(ptr->cmd_buffer, 0, DEBUG_MAX_CMD_LEN);
    ptr->state = DEBUG_STATE_IDLE;
}

/**
 * @brief 空闲中断回调函数
 * @param uart_ptr UART实例指针
 * @param size 接收数据大小
 * @param timeout 超时时间
 * @param arg 用户参数
 */
static void debug_protocol_idle_callback(void* uart_ptr, uint32_t size, uint32_t data_start, void* arg) {
    st_debug_protocol_ptr ptr = (st_debug_protocol_ptr)arg;
    if (!ptr) return;
    
    // 获取接收缓冲区
    uint8_t* rx_buffer = NULL;
    if (ptr->get_rx_buffer) {
        rx_buffer = ptr->get_rx_buffer(ptr->uart_dma_ptr);
    }
    
    if (rx_buffer && size > 0) {
        // 从环形缓冲区中正确提取数据
        uint8_t temp_buffer[256];  // 临时缓冲区
        uint32_t actual_start = data_start % 256;  // UART_DMA_RX_BUFFER_SIZE = 256
        
        // 从DMA缓冲区的正确位置复制数据
        for (uint32_t i = 0; i < size; i++) {
            uint32_t dma_index = (actual_start + i) % 256;
            temp_buffer[i] = rx_buffer[dma_index];
        }
        
        // 处理接收到的数据
        debug_protocol_process_received_data(ptr, temp_buffer, size);
        ptr->last_receive_time = HAL_GetTick();
    }
    
    // 重新启动接收
    if (ptr->receive) {
        ptr->receive(ptr->uart_dma_ptr);
    }
}



/**
 * @brief 错误回调函数
 * @param uart_ptr UART实例指针
 * @param error 错误码
 * @param arg 用户参数
 */
static void debug_protocol_error_callback(void* uart_ptr, uint32_t error, void* arg) {
    st_debug_protocol_ptr ptr = (st_debug_protocol_ptr)arg;
    if (!ptr) return;
    
    ptr->last_error = DEBUG_PROTOCOL_ERROR_UART_ERROR;
    ptr->state = DEBUG_STATE_ERROR;
    
    if (ptr->protocol_callback) {
        ptr->protocol_callback(ptr, DEBUG_PROTOCOL_ERROR_UART_ERROR, NULL, 0, ptr->callback_arg);
    }
    
    // 重新启动接收
    if (ptr->receive) {
        ptr->receive(ptr->uart_dma_ptr);
    }
}

/**
 * @brief 处理接收到的数据
 * @param ptr 协议实例指针
 * @param data 数据指针
 * @param len 数据长度
 */
static void debug_protocol_process_received_data(st_debug_protocol_ptr ptr, uint8_t* data, uint32_t len) {
    if (!ptr || !data || len == 0) return;
    
    ptr->state = DEBUG_STATE_RECEIVING;
    
    for (uint32_t i = 0; i < len; i++) {
        uint8_t ch = data[i];
        
        // 处理特殊字符
        if (ch == '\r' || ch == '\n') {
            // 回车或换行，命令结束
            if (ptr->cmd_index > 0) {
                ptr->state = DEBUG_STATE_CMD_READY;
                ptr->cmd_ready = true;
                
                if (ptr->protocol_callback) {
                    ptr->protocol_callback(ptr, DEBUG_PROTOCOL_ERROR_NONE, ptr->cmd_buffer, ptr->cmd_index, ptr->callback_arg);
                }
            }
            
            // 重置状态
            ptr->cmd_index = 0;
            memset(ptr->cmd_buffer, 0, DEBUG_MAX_CMD_LEN);
            ptr->state = DEBUG_STATE_IDLE;
            continue;
        }
        
        if (ch == '\b') {
            // 退格键，删除前一个字符
            if (ptr->cmd_index > 0) {
                ptr->cmd_index--;
                ptr->cmd_buffer[ptr->cmd_index] = '\0';
            }
            continue;
        }
        
        // 普通字符，添加到命令缓冲区
        if (ptr->cmd_index < DEBUG_MAX_CMD_LEN - 1) {
            ptr->cmd_buffer[ptr->cmd_index] = ch;
            ptr->cmd_index++;
            ptr->cmd_buffer[ptr->cmd_index] = '\0';
        } else {
            // 缓冲区溢出
            ptr->last_error = DEBUG_PROTOCOL_ERROR_BUFFER_OVERFLOW;
            ptr->state = DEBUG_STATE_ERROR;
            
            if (ptr->protocol_callback) {
                ptr->protocol_callback(ptr, DEBUG_PROTOCOL_ERROR_BUFFER_OVERFLOW, NULL, 0, ptr->callback_arg);
            }
            
            // 重置状态
            ptr->cmd_index = 0;
            memset(ptr->cmd_buffer, 0, DEBUG_MAX_CMD_LEN);
            ptr->state = DEBUG_STATE_IDLE;
            break;
        }
    }
} 

/*---End of File----------------------------------------------------*/

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

#ifndef LIB_UART_DEBUG_H
#define LIB_UART_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 调试协议相关常量
#define DEBUG_MAX_CMD_LEN 128
#define DEBUG_TIMEOUT_MS 50

// 调试协议状态
typedef enum {
    DEBUG_STATE_IDLE = 0,
    DEBUG_STATE_RECEIVING,
    DEBUG_STATE_CMD_READY,
    DEBUG_STATE_ERROR
} DebugProtocolState;

// 调试协议错误码
typedef enum {
    DEBUG_PROTOCOL_ERROR_NONE = 0,
    DEBUG_PROTOCOL_ERROR_BUFFER_OVERFLOW,
    DEBUG_PROTOCOL_ERROR_TIMEOUT,
    DEBUG_PROTOCOL_ERROR_UART_ERROR
} DebugProtocolError;

// 调试协议回调类型
typedef void (*DebugProtocolCallback)(void* protocol, int error, uint8_t* data, uint32_t len, void* arg);

// 硬件操作函数指针类型
typedef void (*UartSendFunc)(void* uart_ptr, uint8_t* data, uint32_t len);
typedef void (*UartReceiveFunc)(void* uart_ptr);
typedef void (*UartSetBaudrateFunc)(void* uart_ptr, uint32_t baudrate);
typedef void (*UartEnableInterruptsFunc)(void* uart_ptr);
typedef uint8_t* (*UartGetRxBufferFunc)(void* uart_ptr);

// 回调注册函数指针类型
typedef void (*UartRegisterIdleCallbackFunc)(void* uart_ptr, void (*cb)(void*, uint32_t, uint32_t, void*), void* arg);
typedef void (*UartRegisterErrorCallbackFunc)(void* uart_ptr, void (*cb)(void*, uint32_t, void*), void* arg);
typedef void (*UartRegisterTxCompleteCallbackFunc)(void* uart_ptr, void (*cb)(void*, void*), void* arg);

// 调试协议结构体
typedef struct debug_protocol {
    // 硬件相关
    void* uart_dma_ptr;                                /*!< 指向底层UART硬件驱动的指针 */
    UartSendFunc send;                                 /*!< 发送数据函数指针 */
    UartReceiveFunc receive;                           /*!< 启动接收函数指针 */
    UartSetBaudrateFunc set_baudrate;                  /*!< 设置波特率函数指针 */
    UartEnableInterruptsFunc enable_interrupts;        /*!< 使能中断函数指针 */
    UartGetRxBufferFunc get_rx_buffer;                 /*!< 获取接收缓冲区函数指针 */
    UartRegisterIdleCallbackFunc register_idle_callback;    /*!< 注册空闲中断回调函数指针 */
    UartRegisterErrorCallbackFunc register_error_callback;  /*!< 注册错误回调函数指针 */
    UartRegisterTxCompleteCallbackFunc register_tx_complete_callback;  /*!< 注册发送完成回调函数指针 */

    // 协议状态
    DebugProtocolState state;                          /*!< 当前协议状态 */
    DebugProtocolError last_error;                     /*!< 最后一次错误码 */
    uint8_t cmd_buffer[DEBUG_MAX_CMD_LEN];             /*!< 命令缓冲区 */
    uint32_t cmd_index;                                /*!< 命令缓冲区索引 */
    bool cmd_ready;                                    /*!< 命令就绪标志 */
    uint32_t last_receive_time;                        /*!< 最后接收时间 */
    


    // 回调
    DebugProtocolCallback protocol_callback;           /*!< 协议回调函数指针 */
    void* callback_arg;                                /*!< 回调函数参数指针 */

    // 协议层接口函数指针
    void (*configure)(struct debug_protocol* ptr);     /*!< 配置函数指针 */
    void (*initialize)(struct debug_protocol* ptr, uint32_t baudrate, DebugProtocolCallback cb, void* arg);  /*!< 初始化函数指针 */
    void (*process)(struct debug_protocol* ptr);       /*!< 处理函数指针 */
    void (*send_data)(struct debug_protocol* ptr, uint8_t* data, uint32_t len);  /*!< 发送数据函数指针 */
    bool (*is_cmd_ready)(struct debug_protocol* ptr);  /*!< 检查命令是否就绪 */
    void (*get_cmd)(struct debug_protocol* ptr, uint8_t* data, uint32_t* len);   /*!< 获取命令数据 */
    void (*clear_cmd)(struct debug_protocol* ptr);     /*!< 清除命令 */
} st_debug_protocol, *st_debug_protocol_ptr;

// 协议层接口函数声明
void lib_debug_protocol_configure(st_debug_protocol_ptr ptr);
void lib_debug_protocol_initialize(st_debug_protocol_ptr ptr, uint32_t baudrate, DebugProtocolCallback protocol_callback, void* arg);
void lib_debug_protocol_process(st_debug_protocol_ptr ptr);
void lib_debug_protocol_send_data(st_debug_protocol_ptr ptr, uint8_t* data, uint32_t len);
bool lib_debug_protocol_is_cmd_ready(st_debug_protocol_ptr ptr);
void lib_debug_protocol_get_cmd(st_debug_protocol_ptr ptr, uint8_t* data, uint32_t* len);
void lib_debug_protocol_clear_cmd(st_debug_protocol_ptr ptr);

#ifdef __cplusplus
}
#endif

#endif // LIB_UART_DEBUG_H 

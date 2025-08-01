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

#ifndef LIB_UART_PROTOCOL_H
#define LIB_UART_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 协议相关常量
#define MAX_FRAME_LEN 64
#define MAX_PARAM_VALUE_LEN 32
#define TIMEOUT_MS 20
#define SLAVE_ID 0x01
#define FRAME_HEAD 0xAA
#define FRAME_TAIL 0x55

// 协议状态
typedef enum {
    STATE_IDLE = 0,
    STATE_RECEIVING,
    STATE_FORMAT_ERROR,
    STATE_ERROR
} UartProtocolState;

// 协议错误码
typedef enum {
    UART_PROTOCOL_ERROR_NONE = 0,
    UART_PROTOCOL_ERROR_FORMAT,
    UART_PROTOCOL_ERROR_CRC,
    UART_PROTOCOL_ERROR_SLAVE_ID
} UartProtocolError;

// 校验位类型
typedef enum {
    UART_PROTOCOL_PARITY_NONE = 0,
    UART_PROTOCOL_PARITY_ODD,
    UART_PROTOCOL_PARITY_EVEN
} UartParity;

// 协议回调类型
typedef void (*UartProtocolCallback)(void* protocol, int error, uint8_t* data, uint32_t len, void* arg);

// 硬件操作函数指针类型
typedef void (*UartSendFunc)(void* uart_ptr, uint8_t* data, uint32_t len);
typedef void (*UartReceiveFunc)(void* uart_ptr);
typedef void (*UartSetBaudrateFunc)(void* uart_ptr, uint32_t baudrate);
typedef void (*UartSetParityFunc)(void* uart_ptr, uint8_t parity);
typedef void (*UartEnableInterruptsFunc)(void* uart_ptr);
typedef uint8_t (*UartGetIdFunc)(void* uart_ptr);
typedef void* (*UartGetHwPtrFunc)(void* uart_ptr);
typedef uint8_t* (*UartGetRxBufferFunc)(void* uart_ptr);

// 回调注册函数指针类型
typedef void (*UartRegisterTxCompleteCallbackFunc)(void* uart_ptr, void (*cb)(void*, void*), void* arg);
typedef void (*UartRegisterRxCompleteCallbackFunc)(void* uart_ptr, void (*cb)(void*, void*), void* arg);
typedef void (*UartRegisterIdleCallbackFunc)(void* uart_ptr, void (*cb)(void*, uint32_t, uint32_t, void*), void* arg);
typedef void (*UartRegisterErrorCallbackFunc)(void* uart_ptr, void (*cb)(void*, uint32_t, void*), void* arg);

// 定时器函数指针类型
typedef void (*TimerSetArgumentFunc)(void*, uint8_t, uint64_t);
typedef int  (*TimerStartFunc)(void*);
typedef void (*TimerStopFunc)(void*);
typedef void (*TimerSetCallbackFunc)(void*, void (*)(void*), void*);

// 协议结构体
typedef struct uart_protocol {
    // 硬件相关
    void* uart_dma_ptr;                                /*!< 指向底层UART硬件驱动的指针，用于访问UART实例 */
    UartSendFunc send;                                 /*!< 发送数据函数指针，调用底层UART发送接口 */
    UartReceiveFunc receive;                           /*!< 启动接收函数指针，调用底层UART接收接口 */
    UartSetBaudrateFunc set_baudrate;                  /*!< 设置波特率函数指针，动态修改UART波特率 */
    UartSetParityFunc set_parity;                      /*!< 设置校验位函数指针，配置UART校验位 */
    UartEnableInterruptsFunc enable_interrupts;        /*!< 使能中断函数指针，使能UART相关中断 */
    UartGetIdFunc get_id;                              /*!< 获取UART ID函数指针，返回UART实例标识 */
    UartGetHwPtrFunc get_hw_ptr;                       /*!< 获取硬件句柄函数指针，返回UART硬件句柄 */
    UartGetRxBufferFunc get_rx_buffer;                 /*!< 获取接收缓冲区函数指针，返回底层接收缓冲区地址 */
    UartRegisterTxCompleteCallbackFunc register_tx_complete_callback;   /*!< 注册发送完成回调函数指针 */
    UartRegisterRxCompleteCallbackFunc register_rx_complete_callback;   /*!< 注册接收完成回调函数指针 */
    UartRegisterIdleCallbackFunc register_idle_callback;                /*!< 注册空闲中断回调函数指针 */
    UartRegisterErrorCallbackFunc register_error_callback;              /*!< 注册错误回调函数指针 */

    // 定时器相关
    void* timeout_timer;                               /*!< 超时定时器指针，用于帧接收超时检测 */
    TimerSetArgumentFunc timer_set_argument;           /*!< 设置定时器参数函数指针，配置定时器超时时间 */
    TimerStartFunc timer_start;                        /*!< 启动定时器函数指针，开始超时计时 */
    TimerStopFunc timer_stop;                          /*!< 停止定时器函数指针，停止超时计时 */
    TimerSetCallbackFunc timer_set_callback;           /*!< 设置定时器回调函数指针，配置超时处理函数 */

    // 协议状态
    UartProtocolState state;                           /*!< 当前协议状态，表示协议处理机的状态（空闲/接收中/错误等） */
    UartProtocolError last_error;                      /*!< 最后一次错误码，记录最近发生的协议错误 */
    uint8_t rx_buffer[MAX_FRAME_LEN];                  /*!< 协议层接收缓冲区，用于存储接收到的完整帧数据 */
    uint8_t tx_buffer[MAX_FRAME_LEN];                  /*!< 协议层发送缓冲区，用于构造和发送协议帧数据 */
    uint8_t rx_index;                                  /*!< 接收缓冲区索引，指示当前接收到的字节位置 */
    uint8_t tx_len;                                    /*!< 发送缓冲区长度，指示当前发送帧的字节数 */
    uint8_t param_buffer[MAX_PARAM_VALUE_LEN+1];       /*!< 参数缓冲区，用于临时存储解析出的参数数据 */
    void* rx_queue;                                    /*!< 接收队列指针，用于多线程环境下的数据传递 */
    void* rx_sem;                                      /*!< 接收信号量指针，用于线程同步 */

    // 回调
    UartProtocolCallback protocol_callback;            /*!< 协议回调函数指针，当接收到完整帧或发生错误时调用 */
    void* callback_arg;                                /*!< 回调函数参数指针，传递给协议回调函数的用户数据 */

    // 协议层接口函数指针
    void (*configure)(struct uart_protocol* ptr);      /*!< 配置函数指针，初始化协议层的默认配置 */
    void (*initialize)(struct uart_protocol* ptr, uint32_t baudrate, UartParity parity, UartProtocolCallback cb, void* arg);  /*!< 初始化函数指针，设置协议参数并启动接收 */
    void (*process)(struct uart_protocol* ptr);        /*!< 处理函数指针，处理接收到的数据帧 */
    void (*send_frame)(struct uart_protocol* ptr, uint8_t opcode, uint8_t param_id, uint8_t param_type, uint8_t* data, uint8_t len);  /*!< 发送帧函数指针，构造并发送协议帧 */
    void (*receive_frame)(struct uart_protocol* ptr);  /*!< 接收帧函数指针，启动帧接收过程 */
} st_uart_protocol, *st_uart_protocol_ptr;

// 协议层接口函数声明
void lib_uart_protocol_configure(st_uart_protocol_ptr ptr);
void lib_uart_protocol_initialize(st_uart_protocol_ptr ptr, uint32_t baudrate, UartParity parity, UartProtocolCallback protocol_callback, void* arg);
void lib_uart_protocol_process(st_uart_protocol_ptr ptr);
void lib_uart_protocol_send_frame(st_uart_protocol_ptr ptr, uint8_t opcode, uint8_t param_id, uint8_t param_type, uint8_t* data, uint8_t len);
void lib_uart_protocol_receive_frame(st_uart_protocol_ptr ptr);

// CRC16计算
uint16_t lib_uart_protocol_calc_crc16(const uint8_t* data, uint16_t len);

// 全局数据变量声明
extern int lib_uart_protocol_data1;
extern float lib_uart_protocol_data2;

#ifdef __cplusplus
}
#endif

#endif // LIB_UART_PROTOCOL_H

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

#ifndef DRV_UART_DMA_H
#define DRV_UART_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32g4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif

// UART DMA 配置
#define UART_DMA_DEFAULT_BAUDRATE 115200
#define UART_DMA_MAX_INSTANCES 4
#define UART_DMA_RX_BUFFER_SIZE 256
// #define UART_DMA_HW_FLOW_CONTROL // 硬件流控制（可选）

/* UART 实例 ID */
typedef enum {
    Uart0 = 0,
    Uart1,
    Uart2,
    Uart3,
    UartSum = 4
} et_UartDma_id;

/* 错误码 */
typedef enum {
    UART_DMA_ERROR_NONE = 0,            /*!< 无错误 */
    UART_DMA_ERROR_INVALID_UART,        /*!< 无效的 UART 外设 */
    UART_DMA_ERROR_INVALID_PIN,         /*!< 无效的引脚配置 */
    UART_DMA_ERROR_INVALID_CONFIG,      /*!< 无效的配置参数 */
    UART_DMA_ERROR_INIT_FAILED,         /*!< 初始化失败 */
    UART_DMA_ERROR_PIN_CONFLICT,        /*!< 引脚冲突 */
    UART_DMA_ERROR_DMA_CONFLICT,        /*!< DMA 通道冲突 */
    UART_DMA_ERROR_UART_OVERRUN,        /*!< UART 接收缓冲区溢出 */
    UART_DMA_ERROR_UART_FRAMING,        /*!< UART 帧错误 */
    UART_DMA_ERROR_UART_NOISE           /*!< UART 噪声错误 */
} et_uartDmaError;

/* 错误详情 */
typedef struct {
    et_uartDmaError code;
    UART_HandleTypeDef* uart;
    GPIO_TypeDef* port;
    uint32_t pin;
    const char* conflicting_module;
} st_uartDmaErrorDetail;

/* 校验位配置 */
typedef enum {
    PARITY_NONE = UART_PARITY_NONE,
    PARITY_ODD = UART_PARITY_ODD,
    PARITY_EVEN = UART_PARITY_EVEN
} et_uartDmaParity;

// 前置声明
struct uart_dma;

// 回调类型全部统一为void*参数
// 发送完成
typedef void (*UartTxCompleteCallback)(void* uart_ptr, void* arg);
// 接收完成
typedef void (*UartRxCompleteCallback)(void* uart_ptr, void* arg);
// 空闲中断
typedef void (*UartIdleCallback)(void* uart_ptr, uint32_t rx_len, uint32_t data_start, void* arg);
// 错误
typedef void (*UartErrorCallback)(void* uart_ptr, uint32_t err, void* arg);

typedef struct uart_dma *st_uart_dma_ptr;

typedef struct uart_dma {
    et_UartDma_id Uart_id;                    /*!< UART实例ID，用于标识不同的UART外设（Uart0-Uart3） */
    UART_HandleTypeDef* uart_ptr;             /*!< 指向STM32 HAL库UART句柄的指针，包含UART硬件配置信息 */
    DMA_HandleTypeDef* dma_tx_ptr;            /*!< 指向DMA发送通道句柄的指针，用于UART发送数据的DMA传输 */
    DMA_HandleTypeDef* dma_rx_ptr;            /*!< 指向DMA接收通道句柄的指针，用于UART接收数据的DMA传输 */
    uint8_t rx_buffer[UART_DMA_RX_BUFFER_SIZE]; /*!< 接收数据缓冲区，用于存储从UART接收到的数据 */
    uint32_t rx_len;                          /*!< 当前接收到的数据长度，在空闲中断中更新 */
    uint32_t last_processed_pos;              /*!< 上次处理的位置，用于计算本次新接收的数据 */
    bool single_byte_rx_ready;                /*!< 单字节接收就绪标志，表示是否有单字节数据可读 */
    struct uart_dma* next;                    /*!< 链表指针，用于将多个UART实例链接成链表结构 */

    // 回调函数指针
    UartTxCompleteCallback tx_complete_callback;   /*!< 发送完成回调函数指针，DMA发送完成时调用 */
    UartRxCompleteCallback rx_complete_callback;   /*!< 接收完成回调函数指针，DMA接收完成时调用 */
    UartIdleCallback idle_callback;               /*!< 空闲中断回调函数指针，UART空闲时调用，用于处理不定长数据 */
    UartErrorCallback error_callback;             /*!< 错误回调函数指针，UART或DMA错误时调用 */
    
    // 独立的回调参数
    void* tx_complete_arg;                        /*!< 发送完成回调函数的用户参数指针 */
    void* rx_complete_arg;                        /*!< 接收完成回调函数的用户参数指针 */
    void* idle_arg;                               /*!< 空闲中断回调函数的用户参数指针 */
    void* error_arg;                              /*!< 错误回调函数的用户参数指针 */
    void* callback_arg;                           /*!< 回调函数的用户参数指针，传递给所有回调函数（保持向后兼容） */

    // 硬件操作函数指针
    void (*send)(st_uart_dma_ptr, uint8_t*, uint32_t);      /*!< 发送数据函数指针，通过DMA发送指定长度的数据 */
    void (*receive)(st_uart_dma_ptr);                       /*!< 启动接收函数指针，开始DMA接收数据 */
    void (*send_byte)(st_uart_dma_ptr, uint8_t);            /*!< 发送单字节函数指针，直接发送一个字节 */
    void (*receive_byte)(st_uart_dma_ptr);                  /*!< 接收单字节函数指针，接收一个字节数据 */
    void (*enable_interrupts)(st_uart_dma_ptr);             /*!< 使能中断函数指针，使能UART和DMA中断 */
    void (*disable_interrupts)(st_uart_dma_ptr);            /*!< 禁用中断函数指针，禁用UART和DMA中断 */
    void (*set_baudrate)(st_uart_dma_ptr, uint32_t);        /*!< 设置波特率函数指针，动态修改UART波特率 */
    void (*set_parity)(st_uart_dma_ptr, et_uartDmaParity);  /*!< 设置校验位函数指针，配置UART校验位 */
    void (*configure)(st_uart_dma_ptr);                     /*!< 配置函数指针，重新配置UART参数 */

    // 回调函数注册接口
    void (*register_tx_complete_callback)(st_uart_dma_ptr, UartTxCompleteCallback, void*);   /*!< 注册发送完成回调函数 */
    void (*register_rx_complete_callback)(st_uart_dma_ptr, UartRxCompleteCallback, void*);   /*!< 注册接收完成回调函数 */
    void (*register_idle_callback)(st_uart_dma_ptr, UartIdleCallback, void*);                /*!< 注册空闲中断回调函数 */
    void (*register_error_callback)(st_uart_dma_ptr, UartErrorCallback, void*);              /*!< 注册错误回调函数 */

    // 获取器函数指针
    uint8_t (*get_id)(st_uart_dma_ptr);                     /*!< 获取UART ID函数指针，返回UART实例ID */
    void* (*get_hw_ptr)(st_uart_dma_ptr);                   /*!< 获取硬件句柄函数指针，返回UART硬件句柄 */
    uint8_t* (*get_rx_buffer)(st_uart_dma_ptr);             /*!< 获取接收缓冲区函数指针，返回接收缓冲区地址 */

} st_uart_dma;

extern UART_HandleTypeDef uart_dma_handles[UART_DMA_MAX_INSTANCES];
extern DMA_HandleTypeDef uart_dma_tx_handles[UART_DMA_MAX_INSTANCES];
extern DMA_HandleTypeDef uart_dma_rx_handles[UART_DMA_MAX_INSTANCES];

et_uartDmaError drv_uart_dma_create(st_uart_dma_ptr ptr, USART_TypeDef* uart, et_UartDma_id id);
et_uartDmaError drv_uart_dma_hw_init(USART_TypeDef* uart, uint32_t baudrate, uint8_t pin_set_index,
                                     DMA_Channel_TypeDef* tx_dma_channel, uint32_t tx_dma_request,
                                     DMA_Channel_TypeDef* rx_dma_channel, uint32_t rx_dma_request,
                                     et_UartDma_id id);
st_uartDmaErrorDetail drv_uart_dma_get_last_error(void);

// 供协议层直接赋值的硬件操作函数
void drv_uart_send_impl(void* uart_dma_ptr, uint8_t* data, uint32_t len);
void drv_uart_receive_impl(void* uart_dma_ptr);
void drv_uart_set_baudrate_impl(void* uart_dma_ptr, uint32_t baudrate);
void drv_uart_set_parity_impl(void* uart_dma_ptr, uint8_t parity);
void drv_uart_enable_interrupts_impl(void* uart_dma_ptr);
void drv_uart_disable_interrupts_impl(void* uart_dma_ptr);
uint8_t drv_uart_get_id_impl(void* uart_dma_ptr);
void* drv_uart_get_hw_ptr_impl(void* uart_dma_ptr);
uint8_t* drv_uart_get_rx_buffer_impl(void* uart_dma_ptr);
void drv_uart_register_tx_complete_callback_impl(void* uart_dma_ptr, void (*cb)(void*, void*), void* arg);
void drv_uart_register_rx_complete_callback_impl(void* uart_dma_ptr, void (*cb)(void*, void*), void* arg);
void drv_uart_register_idle_callback_impl(void* uart_dma_ptr, void (*cb)(void*, uint32_t, uint32_t, void*), void* arg);
void drv_uart_register_error_callback_impl(void* uart_dma_ptr, void (*cb)(void*, uint32_t, void*), void* arg);

#ifdef __cplusplus
}
#endif

#endif /* DRV_UART_DMA_H */

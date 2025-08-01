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

#include "lib_uart_protocol.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

#define UART_DMA_RX_BUFFER_SIZE 256

// 抽象回调函数声明（完全解耦硬件）
static void lib_uart_protocol_tx_complete_callback(void* uart_dma_ptr, void* arg);
static void lib_uart_protocol_rx_complete_callback(void* uart_dma_ptr, void* arg);
static void lib_uart_protocol_idle_callback(void* uart_dma_ptr, uint32_t rx_len, uint32_t data_start, void* arg);
static void lib_uart_protocol_error_callback(void* uart_dma_ptr, uint32_t error_code, void* arg);

// 模拟数据
int lib_uart_protocol_data1 = 42;
float lib_uart_protocol_data2 = 3.14f;

/**
  * @name     lib_uart_protocol_calc_crc16
  * @brief    计算数据块的 CRC16 校验值
  * @param    data: 数据缓冲区指针
  * @param    len: 数据长度
  * @return   uint16_t: 计算得到的 CRC16 值
  * @remark   使用 CRC16-IBM 算法，初始值 0xFFFF，多项式 0xA001
  */
uint16_t lib_uart_protocol_calc_crc16(const uint8_t* data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/**
  * @name     lib_uart_protocol_timeout_callback
  * @brief    定时器超时回调函数，重置协议状态
  * @param    arg: 回调参数，指向 st_uart_protocol 结构体
  * @return   None
  * @remark   在接收超时（TIMEOUT_MS）后调用，设置状态为错误并重置接收索引
  */
static void lib_uart_protocol_timeout_callback(void* arg) {
    st_uart_protocol_ptr ptr = (st_uart_protocol_ptr)arg;
    ptr->state = STATE_ERROR;
    ptr->last_error = UART_PROTOCOL_ERROR_FORMAT;
    if (ptr->protocol_callback) {
        ptr->protocol_callback(ptr, UART_PROTOCOL_ERROR_FORMAT, NULL, 0, ptr->callback_arg);
    }
    ptr->state = STATE_IDLE;
    ptr->rx_index = 0;
    if (ptr->timer_stop) {
        ptr->timer_stop(ptr->timeout_timer);
    }
}

/**
  * @name     lib_uart_protocol_send_frame
  * @brief    发送 UART 协议帧
  * @param    ptr: 指向 UART 协议实例的指针
  * @param    opcode: 操作码
  * @param    param_id: 参数ID
  * @param    param_type: 参数类型
  * @param    data: 数据指针
  * @param    len: 数据长度
  * @return   None
  * @remark   构造帧数据，计算 CRC 并通过函数指针发送
  */
void lib_uart_protocol_send_frame(st_uart_protocol_ptr ptr, 
                                  uint8_t opcode, 
                                  uint8_t param_id,
                                  uint8_t param_type,
                                  uint8_t* data, 
                                  uint8_t len) {
    if (!ptr || !ptr->send) return;

    ptr->tx_len = 0;

    // 构造帧头
    // 构造帧头
    ptr->tx_buffer[ptr->tx_len++] = FRAME_HEAD; //  帧头
    ptr->tx_buffer[ptr->tx_len++] = SLAVE_ID; // 从机编号
    ptr->tx_buffer[ptr->tx_len++] = opcode; // 操作码
    ptr->tx_buffer[ptr->tx_len++] = len + 5; // 数据长度 + 5个固定字段
    ptr->tx_buffer[ptr->tx_len++] = param_id; // 参数ID
    ptr->tx_buffer[ptr->tx_len++] = param_type; // 参数类型
    ptr->tx_buffer[ptr->tx_len++] = len;  // 参数长度

    // 复制数据
    if (data && len > 0) {
        memcpy(&ptr->tx_buffer[ptr->tx_len], data, len);
        ptr->tx_len += len;
    }

    // 计算CRC
    uint16_t crc = lib_uart_protocol_calc_crc16(ptr->tx_buffer, ptr->tx_len);
    memcpy(&ptr->tx_buffer[ptr->tx_len], &crc, 2);
    ptr->tx_len += 2;

    // 添加帧尾
    ptr->tx_buffer[ptr->tx_len++] = FRAME_TAIL;

    // 发送数据
    ptr->send(ptr->uart_dma_ptr, ptr->tx_buffer, ptr->tx_len);
}

/**
  * @name     lib_uart_protocol_receive_frame
  * @brief    启动 UART 帧接收
  * @param    ptr: 指向 UART 协议实例的指针
  * @return   None
  * @remark   调用函数指针启动接收
  */
void lib_uart_protocol_receive_frame(st_uart_protocol_ptr ptr) {
    if (ptr && ptr->receive) {
        ptr->receive(ptr->uart_dma_ptr);
    }
}

/**
  * @name     lib_uart_protocol_process
  * @brief    处理接收队列中的 UART 帧
  * @param    ptr: 指向 UART 协议实例的指针
  * @return   None
  * @remark   从接收队列获取数据并处理进程
  */
void lib_uart_protocol_process(st_uart_protocol_ptr ptr) {
    if (!ptr || !ptr->rx_queue) return;

    // 处理接收到的数据
    uint8_t dummy_data[4];
    if (xQueueReceive(ptr->rx_queue, dummy_data, 0) == pdTRUE) {
        // 调用回调函数处理数据
        if (ptr->protocol_callback) {
            ptr->protocol_callback(
            ptr, 
            UART_PROTOCOL_ERROR_NONE, 
            ptr->rx_buffer, 
            ptr->rx_index, 
            ptr->callback_arg);
        }
    }

}

/**
  * @name     lib_uart_protocol_initialize
  * @brief    初始化 UART 协议实例
  * @param    ptr: 指向 UART 协议实例的指针
  * @param    baudrate: 波特率
  * @param    parity: 校验位配置
  * @param    protocol_callback: 协议回调函数
  * @param    arg: 回调参数
  * @return   None
  * @remark   配置队列、信号量、定时器和 UART 参数，启动接收
  */
void lib_uart_protocol_initialize(st_uart_protocol_ptr ptr, 
                                  uint32_t baudrate, 
                                  UartParity parity, 
                                  UartProtocolCallback protocol_callback, 
                                  void* arg) {
    if (!ptr) return;

    // 初始化状态
    ptr->state = STATE_IDLE;
    ptr->rx_index = 0;
    ptr->tx_len = 0;
    ptr->last_error = UART_PROTOCOL_ERROR_NONE;
    ptr->protocol_callback = protocol_callback;
    ptr->callback_arg = arg;

    // 创建队列和信号量
    ptr->rx_queue = xQueueCreate(10, sizeof(uint8_t) * 4); // 简单的数据队列
    ptr->rx_sem = xSemaphoreCreateBinary();

    // 清空缓冲区
    memset(ptr->rx_buffer, 0, MAX_FRAME_LEN);
    memset(ptr->tx_buffer, 0, MAX_FRAME_LEN);
    memset(ptr->param_buffer, 0, MAX_PARAM_VALUE_LEN + 1);

    // 配置定时器回调
    if (ptr->timer_set_callback) {
        ptr->timer_set_callback(ptr->timeout_timer, 
        lib_uart_protocol_timeout_callback, ptr);
    }

    // 配置UART参数
    if (ptr->set_baudrate) {
        ptr->set_baudrate(ptr->uart_dma_ptr, baudrate > 0 ? baudrate : 115200);
    }
    if (ptr->set_parity) {
        ptr->set_parity(ptr->uart_dma_ptr, parity);
    }
    if (ptr->enable_interrupts) {
        ptr->enable_interrupts(ptr->uart_dma_ptr);
    }

    // 注册中断回调函数
    if (ptr->register_tx_complete_callback) {
        ptr->register_tx_complete_callback(
        ptr->uart_dma_ptr, 
        lib_uart_protocol_tx_complete_callback, 
        ptr);
    }
    if (ptr->register_rx_complete_callback) {
        ptr->register_rx_complete_callback(
        ptr->uart_dma_ptr, 
        lib_uart_protocol_rx_complete_callback, 
        ptr);
    }
    if (ptr->register_idle_callback) {
        ptr->register_idle_callback(
        ptr->uart_dma_ptr, 
        lib_uart_protocol_idle_callback, 
        ptr);
    }
    if (ptr->register_error_callback) {
        ptr->register_error_callback(
        ptr->uart_dma_ptr, 
        lib_uart_protocol_error_callback, 
        ptr);
    }

    // 启动接收
    ptr->receive_frame(ptr);
}

/**
  * @name     lib_uart_protocol_configure
  * @brief    配置 UART 协议函数指针
  * @param    ptr: 指向 UART 协议实例的指针
  * @return   None
  * @remark   初始化函数指针，供外部调用
  */
void lib_uart_protocol_configure(st_uart_protocol_ptr ptr) {
    // 这个函数现在由应用层直接设置函数指针，这里可以为空
    // 或者用于其他初始化工作
}

/**
  * @name     lib_uart_protocol_tx_complete_callback
  * @brief    协议层发送完成回调函数
  * @param    uart_dma_ptr: 指向 UART 实例的通用指针
  * @param    arg: 回调参数
  * @return   None
  * @remark   被硬件层调用
  */
static void lib_uart_protocol_tx_complete_callback(void* uart_dma_ptr, void* arg) {
    // 发送完成处理
    // 可以根据需要添加具体逻辑
}

/**
  * @name     lib_uart_protocol_rx_complete_callback
  * @brief    协议层接收完成回调函数
  * @param    uart_dma_ptr: 指向 UART 实例的通用指针
  * @param    arg: 回调参数
  * @return   None
  * @remark   被硬件层调用，重启接收
  */
static void lib_uart_protocol_rx_complete_callback(void* uart_dma_ptr, void* arg) {
    st_uart_protocol_ptr uart_protocol_ptr = (st_uart_protocol_ptr)arg;
    if (uart_protocol_ptr) {
        uart_protocol_ptr->receive_frame(uart_protocol_ptr);
    }
}

/**
  * @name     lib_uart_protocol_idle_callback
  * @brief    UART 空闲中断回调，处理不定长帧
  * @param    uart_dma_ptr: 指向 UART 实例的通用指针
  * @param    rx_len: 接收到的数据长度
  * @param    data_start: 新数据的起始位置
  * @param    arg: 回调参数
  * @return   None
  * @remark   直接处理空闲中断，无需适配器层
  */
static void lib_uart_protocol_idle_callback(void* uart_dma_ptr, 
                                            uint32_t rx_len, 
                                            uint32_t data_start,
                                            void* arg) {
    st_uart_protocol_ptr uart_protocol_ptr = (st_uart_protocol_ptr)arg;
    if (!uart_protocol_ptr || !uart_dma_ptr) return;

    // 获取接收缓冲区
    uint8_t* rx_buffer = NULL;
    if (uart_protocol_ptr->get_rx_buffer) {
        rx_buffer = uart_protocol_ptr->get_rx_buffer(uart_dma_ptr);
    }
    
    if (!rx_buffer) return;

    // 检查数据长度是否有效
    if (rx_len == 0 || rx_len >= 256) {
        uart_protocol_ptr->receive_frame(uart_protocol_ptr);
        return;
    }

    // 处理本次新接收的数据
    if (rx_len <= MAX_FRAME_LEN) {
        // 计算新数据的实际起始位置
        uint32_t actual_start = data_start % 256;  // UART_DMA_RX_BUFFER_SIZE = 256
        
        // 将新接收的数据追加到协议层缓冲区
        uint32_t available_space = MAX_FRAME_LEN - uart_protocol_ptr->rx_index;
        uint32_t copy_len = (rx_len < available_space) ? rx_len : available_space;
        
        if (copy_len > 0) {
            // 从DMA缓冲区的正确位置复制数据
            for (uint32_t i = 0; i < copy_len; i++) {
                uint32_t dma_index = (actual_start + i) % UART_DMA_RX_BUFFER_SIZE;
                uart_protocol_ptr->rx_buffer[uart_protocol_ptr->rx_index + i] = rx_buffer[dma_index];
            }
            uart_protocol_ptr->rx_index += copy_len;
        }

        // 简单的帧格式验证
        if (uart_protocol_ptr->rx_buffer[0] != FRAME_HEAD || 
            uart_protocol_ptr->rx_buffer[uart_protocol_ptr->rx_index - 1] != FRAME_TAIL) {
            uart_protocol_ptr->state = STATE_FORMAT_ERROR;
            uart_protocol_ptr->last_error = UART_PROTOCOL_ERROR_FORMAT;
            // 错误处理
            if (uart_protocol_ptr->protocol_callback) {
                uart_protocol_ptr->protocol_callback(
                uart_protocol_ptr, 
                UART_PROTOCOL_ERROR_FORMAT, 
                uart_protocol_ptr->rx_buffer, 
                uart_protocol_ptr->rx_index, 
                uart_protocol_ptr->callback_arg);
            }
            uart_protocol_ptr->state = STATE_IDLE;
            uart_protocol_ptr->rx_index = 0;
            if (uart_protocol_ptr->timer_stop) {
                uart_protocol_ptr->timer_stop(uart_protocol_ptr->timeout_timer);
            }
        } else {
            // 数据长度检查 = 参数长度 + 5个固定字段
            uint8_t expected_data_len = uart_protocol_ptr->rx_buffer[6] + 5;
            // 数据长度 + 5个字段(帧头、从机ID、操作码、数据长度、CRC、帧尾)
            uint8_t expected_total_len = expected_data_len + 5; 
            if (uart_protocol_ptr->rx_buffer[3] != expected_data_len || 
                uart_protocol_ptr->rx_index != expected_total_len) {
                // 错误处理
                uart_protocol_ptr->state = STATE_FORMAT_ERROR;
                uart_protocol_ptr->last_error = UART_PROTOCOL_ERROR_FORMAT;
                if (uart_protocol_ptr->protocol_callback) {
                    uart_protocol_ptr->protocol_callback(
                    uart_protocol_ptr, 
                    UART_PROTOCOL_ERROR_FORMAT, 
                    uart_protocol_ptr->rx_buffer, 
                    uart_protocol_ptr->rx_index, 
                    uart_protocol_ptr->callback_arg);
                }
                uart_protocol_ptr->state = STATE_IDLE;
                uart_protocol_ptr->rx_index = 0;
                if (uart_protocol_ptr->timer_stop) {
                    uart_protocol_ptr->timer_stop(uart_protocol_ptr->timeout_timer);
                }
            } else {
                // CRC校验
                // uint16_t crc = lib_uart_protocol_calc_crc16(uart_protocol_ptr->rx_buffer, uart_protocol_ptr->rx_buffer[3] + 2);                                                          
                // uint16_t received_crc = *(uint16_t*)&uart_protocol_ptr->rx_buffer[uart_protocol_ptr->rx_buffer[3] + 2];

                uint8_t crc_len = uart_protocol_ptr->rx_buffer[3] + 2; // 数据长度字段 + 2
                uint16_t crc = lib_uart_protocol_calc_crc16(uart_protocol_ptr->rx_buffer, crc_len);
                uint16_t received_crc = *(uint16_t*)&uart_protocol_ptr->rx_buffer[crc_len];
        
                if (crc != received_crc) {
                    // 错误处理
                    uart_protocol_ptr->state = STATE_FORMAT_ERROR;
                    uart_protocol_ptr->last_error = UART_PROTOCOL_ERROR_CRC;
                    if (uart_protocol_ptr->protocol_callback) {
                        uart_protocol_ptr->protocol_callback(
                        uart_protocol_ptr, 
                        UART_PROTOCOL_ERROR_CRC, 
                        uart_protocol_ptr->rx_buffer, 
                        uart_protocol_ptr->rx_index, 
                        uart_protocol_ptr->callback_arg);
                    }
                    uart_protocol_ptr->state = STATE_IDLE;
                    uart_protocol_ptr->rx_index = 0;
                    if (uart_protocol_ptr->timer_stop) {
                        uart_protocol_ptr->timer_stop(uart_protocol_ptr->timeout_timer);
                    }
                } else {
                
                    // 检查从机ID
                    if (uart_protocol_ptr->rx_buffer[1] != SLAVE_ID) {
                        // 错误处理
                        uart_protocol_ptr->state = STATE_ERROR;
                        uart_protocol_ptr->last_error = UART_PROTOCOL_ERROR_SLAVE_ID;
                        if (uart_protocol_ptr->protocol_callback) {
                            uart_protocol_ptr->protocol_callback(
                            uart_protocol_ptr, 
                            UART_PROTOCOL_ERROR_SLAVE_ID, 
                            uart_protocol_ptr->rx_buffer, 
                            uart_protocol_ptr->rx_index, 
                            uart_protocol_ptr->callback_arg);
                        }
                        uart_protocol_ptr->state = STATE_IDLE;
                        uart_protocol_ptr->rx_index = 0;
                        if (uart_protocol_ptr->timer_stop) {
                            uart_protocol_ptr->timer_stop(uart_protocol_ptr->timeout_timer);
                        }
                    } else {
                        // 解析参数数据
                        if (uart_protocol_ptr->rx_buffer[6] <= MAX_PARAM_VALUE_LEN) {
                            // 当指令是写请求时, buffer[6]的内容是参数长度（否则是0），rx_buffer[7]参数内容
                            memcpy(uart_protocol_ptr->param_buffer, &uart_protocol_ptr->rx_buffer[7], uart_protocol_ptr->rx_buffer[6]);
                            uart_protocol_ptr->param_buffer[uart_protocol_ptr->rx_buffer[6]] = '\0';            
                        }

                        // 推入接收队列
                        uint8_t dummy_data[4] = {
                            uart_protocol_ptr->rx_buffer[2], 
                            uart_protocol_ptr->rx_buffer[4], 
                            uart_protocol_ptr->rx_buffer[5], 
                            uart_protocol_ptr->rx_buffer[6]
                        };

                        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                        if (xQueueSendFromISR(uart_protocol_ptr->rx_queue, dummy_data, &xHigherPriorityTaskWoken) == pdTRUE) {                                              
                            // 队列发送成功，可以添加调试信息
                        }
                        xSemaphoreGiveFromISR(uart_protocol_ptr->rx_sem, &xHigherPriorityTaskWoken);
                        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

                        // 帧处理成功，停止超时定时器
                        if (uart_protocol_ptr->timer_stop) {
                            uart_protocol_ptr->timer_stop(uart_protocol_ptr->timeout_timer);
                        }
                    }
                }
            }
        }
        uart_protocol_ptr->rx_index = 0;
        uart_protocol_ptr->state = STATE_IDLE;
    } else {
        // 帧长度超限，报格式错误
        uart_protocol_ptr->state = STATE_FORMAT_ERROR;
        uart_protocol_ptr->last_error = UART_PROTOCOL_ERROR_FORMAT;
        if (uart_protocol_ptr->protocol_callback) {
            uart_protocol_ptr->protocol_callback(
            uart_protocol_ptr, 
            UART_PROTOCOL_ERROR_FORMAT, 
            rx_buffer, 
            rx_len, 
            uart_protocol_ptr->callback_arg);
        }
        uart_protocol_ptr->state = STATE_IDLE;
        uart_protocol_ptr->rx_index = 0;
    }

    // 重启接收
    uart_protocol_ptr->receive_frame(uart_protocol_ptr);
}


/**
  * @name     lib_uart_protocol_error_callback
  * @brief    UART 错误中断回调
  * @param    uart_dma_ptr: 指向 UART 实例的通用指针
  * @param    error_code: 错误代码
  * @param    arg: 回调参数
  * @return   None
  * @remark   处理 UART 错误，重启接收
  */
static void lib_uart_protocol_error_callback(void* uart_dma_ptr, 
                                             uint32_t error_code, 
                                             void* arg) {
    st_uart_protocol_ptr uart_protocol_ptr = (st_uart_protocol_ptr)arg;
    if (!uart_protocol_ptr || !uart_dma_ptr) return;

    uart_protocol_ptr->state = STATE_ERROR;
    uart_protocol_ptr->last_error = UART_PROTOCOL_ERROR_FORMAT;
    
    // 重启接收
    uart_protocol_ptr->receive_frame(uart_protocol_ptr);
}

/*---End of File----------------------------------------------------*/

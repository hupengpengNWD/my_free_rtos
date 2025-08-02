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

#include <string.h>
#include "drv_uart_dma.h"
#include "cmsis_os2.h"


// 前置声明
static void drv_uart_dma_register_tx_complete_callback(st_uart_dma_ptr ptr, UartTxCompleteCallback tx_complete_callback, void* arg);
static void drv_uart_dma_register_rx_complete_callback(st_uart_dma_ptr ptr, UartRxCompleteCallback rx_complete_callback, void* arg);
static void drv_uart_dma_register_idle_callback(st_uart_dma_ptr ptr, UartIdleCallback idle_callback, void* arg);
static void drv_uart_dma_register_error_callback(st_uart_dma_ptr ptr, UartErrorCallback callback, void* arg);
static uint8_t drv_uart_get_id(st_uart_dma_ptr uart_ptr);
static void* drv_uart_get_hw_ptr(st_uart_dma_ptr uart_ptr);
static uint8_t* drv_uart_get_rx_buffer(st_uart_dma_ptr uart_ptr);
// 引脚映射表结构
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    uint32_t af;
    const char* conflicting_module;
} UartDmaPinConfig;

typedef struct {
    USART_TypeDef* uart_instance;
    UartDmaPinConfig tx;
    UartDmaPinConfig rx;
#ifdef UART_DMA_HW_FLOW_CONTROL
    UartDmaPinConfig rts;
    UartDmaPinConfig cts;
#endif
    uint32_t uart_irq;
} UartDmaPinSet;

typedef struct {
    USART_TypeDef* uart_instance;
    UartDmaPinSet sets[2];
} UartDmaPinMap;

// 引脚映射表（基于 LQFP64 封装）
static const UartDmaPinMap pin_map[] = {
    [0] = {
        .uart_instance = USART1,
        .sets = {
            [0] = {
                .tx = {GPIOB, GPIO_PIN_6, GPIO_AF7_USART1, NULL},
                .rx = {GPIOB, GPIO_PIN_7, GPIO_AF7_USART1, NULL},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {GPIOA, GPIO_PIN_12, GPIO_AF7_USART1, NULL},
                .cts = {GPIOA, GPIO_PIN_11, GPIO_AF7_USART1, "TIM1"},
#endif
                .uart_irq = USART1_IRQn
            },
            [1] = {
                .tx = {GPIOB, GPIO_PIN_6, GPIO_AF7_USART1, "TIM4"},
                .rx = {GPIOB, GPIO_PIN_7, GPIO_AF7_USART1, "TIM4"},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {GPIOB, GPIO_PIN_5, GPIO_AF7_USART1, "TIM3"},
                .cts = {GPIOB, GPIO_PIN_4, GPIO_AF7_USART1, "TIM3"},
#endif
                .uart_irq = USART1_IRQn
            }
        }
    },
    [1] = {
        .uart_instance = USART2,
        .sets = {
            [0] = {
                .tx = {GPIOA, GPIO_PIN_2, GPIO_AF7_USART2, NULL},
                .rx = {GPIOA, GPIO_PIN_3, GPIO_AF7_USART2, NULL},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {GPIOA, GPIO_PIN_0, GPIO_AF7_USART2, "TIM2/TIM5"},
                .cts = {GPIOA, GPIO_PIN_1, GPIO_AF7_USART2, "TIM2/TIM5"},
#endif
                .uart_irq = USART2_IRQn
            },
            [1] = {
                .tx = {GPIOD, GPIO_PIN_5, GPIO_AF7_USART2, NULL},
                .rx = {GPIOD, GPIO_PIN_6, GPIO_AF7_USART2, NULL},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {GPIOD, GPIO_PIN_3, GPIO_AF7_USART2, NULL},
                .cts = {GPIOD, GPIO_PIN_4, GPIO_AF7_USART2, NULL},
#endif
                .uart_irq = USART2_IRQn
            }
        }
    },
    [2] = {
        .uart_instance = USART3,
        .sets = {
            [0] = {
                .tx = {GPIOB, GPIO_PIN_10, GPIO_AF7_USART3, "TIM2"},
                .rx = {GPIOB, GPIO_PIN_11, GPIO_AF7_USART3, "TIM2"},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {GPIOB, GPIO_PIN_13, GPIO_AF7_USART3, NULL},
                .cts = {GPIOB, GPIO_PIN_14, GPIO_AF7_USART3, NULL},
#endif
                .uart_irq = USART3_IRQn
            },
            [1] = {
                .tx = {GPIOD, GPIO_PIN_8, GPIO_AF7_USART3, NULL},
                .rx = {GPIOD, GPIO_PIN_9, GPIO_AF7_USART3, NULL},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {GPIOC, GPIO_PIN_10, GPIO_AF7_USART3, NULL},
                .cts = {GPIOC, GPIO_PIN_11, GPIO_AF7_USART3, NULL},
#endif
                .uart_irq = USART3_IRQn
            }
        }
    },
    [3] = {
        .uart_instance = NULL,
        .sets = {
            [0] = {
                .tx = {NULL, 0, 0, NULL},
                .rx = {NULL, 0, 0, NULL},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {NULL, 0, 0, NULL},
                .cts = {NULL, 0, 0, NULL},
#endif
                .uart_irq = 0
            },
            [1] = {
                .tx = {NULL, 0, 0, NULL},
                .rx = {NULL, 0, 0, NULL},
#ifdef UART_DMA_HW_FLOW_CONTROL
                .rts = {NULL, 0, 0, NULL},
                .cts = {NULL, 0, 0, NULL},
#endif
                .uart_irq = 0
            }
        }
    }
};

// 全局变量
// 静态分配 UART 和 DMA 句柄
UART_HandleTypeDef uart_dma_handles[UART_DMA_MAX_INSTANCES] = {0};
DMA_HandleTypeDef uart_dma_tx_handles[UART_DMA_MAX_INSTANCES] = {0};
DMA_HandleTypeDef uart_dma_rx_handles[UART_DMA_MAX_INSTANCES] = {0};
static st_uart_dma_ptr uart_dma_hendle_ptr = NULL;
static st_uartDmaErrorDetail last_error = {UART_DMA_ERROR_NONE, NULL, NULL, 0, NULL};


/**
  * @name     drv_uart_dma_send
  * @brief    通过 DMA 发送 UART 数据
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    data: 待发送数据缓冲区
  * @param    len: 数据长度
  * @return   None
  * @remark   调用 HAL_UART_Transmit_DMA 执行发送，清空接收长度
  */
static void drv_uart_dma_send(st_uart_dma_ptr ptr, uint8_t* data, uint32_t len) {
    if (ptr->uart_ptr == NULL) {
        return;
    }
    ptr->rx_len = 0;

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(ptr->uart_ptr, data, len);
    if (status != HAL_OK) {
        // 如果UART处于BUSY状态，说明上一次传输还没完成
        // 这种情况下，我们需要等待或者处理错误
        if (status == HAL_BUSY) {
            // 可以选择等待或者直接返回，让上层处理
            return;
        }
    }
}

/**
  * @name     drv_uart_dma_receive
  * @brief    启动 UART DMA 接收（循环模式）
  * @param    ptr: 指向 UART DMA 实例的指针
  * @return   None
  * @remark   配置 DMA 接收固定大小缓冲区（UART_DMA_RX_BUFFER_SIZE）
  */
static void drv_uart_dma_receive(st_uart_dma_ptr ptr) {
    if (ptr->uart_ptr == NULL) {
        return;
    }
    HAL_UART_Receive_DMA(ptr->uart_ptr, ptr->rx_buffer, UART_DMA_RX_BUFFER_SIZE);
}

/**
  * @name     drv_uart_dma_send_byte
  * @brief    通过 DMA 发送单个字节
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    data: 待发送的字节
  * @return   None
  * @remark   使用 DMA 发送单字节
  */
static void drv_uart_dma_send_byte(st_uart_dma_ptr ptr, uint8_t data) {
    if (ptr->uart_ptr == NULL) return;
    HAL_UART_Transmit_DMA(ptr->uart_ptr, &data, 1);
}

/**
  * @name     drv_uart_dma_receive_byte
  * @brief    通过 DMA 接收单个字节
  * @param    ptr: 指向 UART DMA 实例的指针
  * @return   None
  * @remark   接收单字节到缓冲区
  */
static void drv_uart_dma_receive_byte(st_uart_dma_ptr ptr) {
    if (ptr->uart_ptr == NULL) return;
    ptr->single_byte_rx_ready = false;
    HAL_UART_Receive_DMA(ptr->uart_ptr, ptr->rx_buffer, 1);
}

/**
  * @name     drv_uart_dma_enable_interrupts
  * @brief    启用 UART 和 DMA 中断
  * @param    ptr: 指向 UART DMA 实例的指针
  * @return   None
  * @remark   启用 UART 空闲中断和 DMA 通道中断
  */
static void drv_uart_dma_enable_interrupts(st_uart_dma_ptr ptr) {
    if (ptr->uart_ptr == NULL) return;
    __HAL_UART_ENABLE_IT(ptr->uart_ptr, UART_IT_IDLE);
    __HAL_UART_ENABLE_IT(ptr->uart_ptr, UART_IT_TC);  // 启用发送完成中断
    if (ptr->dma_tx_ptr && ptr->dma_tx_ptr->Instance) {
        IRQn_Type tx_irq = (ptr->dma_tx_ptr->Instance == DMA1_Channel1) ? DMA1_Channel1_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel2) ? DMA1_Channel2_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel3) ? DMA1_Channel3_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel4) ? DMA1_Channel4_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel5) ? DMA1_Channel5_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel6) ? DMA1_Channel6_IRQn :
                           DMA1_Channel7_IRQn;
        HAL_NVIC_EnableIRQ(tx_irq);
    }
    if (ptr->dma_rx_ptr && ptr->dma_rx_ptr->Instance) {
        IRQn_Type rx_irq = (ptr->dma_rx_ptr->Instance == DMA1_Channel1) ? DMA1_Channel1_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel2) ? DMA1_Channel2_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel3) ? DMA1_Channel3_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel4) ? DMA1_Channel4_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel5) ? DMA1_Channel5_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel6) ? DMA1_Channel6_IRQn :
                           DMA1_Channel7_IRQn;
        HAL_NVIC_EnableIRQ(rx_irq);
    }
}

/**
  * @name     drv_uart_dma_disable_interrupts
  * @brief    禁用 UART 和 DMA 中断
  * @param    ptr: 指向 UART DMA 实例的指针
  * @return   None
  * @remark   禁用 UART 空闲中断和 DMA 通道中断
  */
static void drv_uart_dma_disable_interrupts(st_uart_dma_ptr ptr) {
    if (ptr->uart_ptr == NULL) return;
    __HAL_UART_DISABLE_IT(ptr->uart_ptr, UART_IT_IDLE);
    __HAL_UART_DISABLE_IT(ptr->uart_ptr, UART_IT_TC);  // 禁用发送完成中断
    if (ptr->dma_tx_ptr && ptr->dma_tx_ptr->Instance) {
        IRQn_Type tx_irq = (ptr->dma_tx_ptr->Instance == DMA1_Channel1) ? DMA1_Channel1_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel2) ? DMA1_Channel2_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel3) ? DMA1_Channel3_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel4) ? DMA1_Channel4_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel5) ? DMA1_Channel5_IRQn :
                           (ptr->dma_tx_ptr->Instance == DMA1_Channel6) ? DMA1_Channel6_IRQn :
                           DMA1_Channel7_IRQn;
        HAL_NVIC_DisableIRQ(tx_irq);
    }
    if (ptr->dma_rx_ptr && ptr->dma_rx_ptr->Instance) {
        IRQn_Type rx_irq = (ptr->dma_rx_ptr->Instance == DMA1_Channel1) ? DMA1_Channel1_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel2) ? DMA1_Channel2_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel3) ? DMA1_Channel3_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel4) ? DMA1_Channel4_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel5) ? DMA1_Channel5_IRQn :
                           (ptr->dma_rx_ptr->Instance == DMA1_Channel6) ? DMA1_Channel6_IRQn :
                           DMA1_Channel7_IRQn;
        HAL_NVIC_DisableIRQ(rx_irq);
    }
}

/**
  * @name     drv_uart_dma_set_baudrate
  * @brief    设置 UART 波特率
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    baudrate: 波特率值
  * @return   None
  * @remark   重新初始化 UART 并重启接收
  */
static void drv_uart_dma_set_baudrate(st_uart_dma_ptr ptr, uint32_t baudrate) {
    if (ptr->uart_ptr == NULL || baudrate == 0) return;
    ptr->uart_ptr->Init.BaudRate = baudrate;
    HAL_UART_Init(ptr->uart_ptr);
    ptr->receive(ptr);
}

/**
  * @name     drv_uart_dma_set_parity
  * @brief    设置 UART 校验位
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    parity: 校验位配置（PARITY_NONE/PARITY_ODD/PARITY_EVEN）
  * @return   None
  * @remark   重新初始化 UART 并重启接收
  */
static void drv_uart_dma_set_parity(st_uart_dma_ptr ptr, et_uartDmaParity parity) {
    if (ptr->uart_ptr == NULL) return;
    ptr->uart_ptr->Init.Parity = parity;
    HAL_UART_Init(ptr->uart_ptr);
    ptr->receive(ptr);
}

/**
  * @name     drv_uart_dma_configure
  * @brief    配置 UART DMA 函数指针
  * @param    ptr: 指向 UART DMA 实例的指针
  * @return   None
  * @remark   初始化所有函数指针，供实例调用
  */
static void drv_uart_dma_configure(st_uart_dma_ptr ptr) {
    // 直接硬件操作
    ptr->send = drv_uart_dma_send;
    ptr->receive = drv_uart_dma_receive;
    ptr->send_byte = drv_uart_dma_send_byte;
    ptr->receive_byte = drv_uart_dma_receive_byte;
    ptr->enable_interrupts = drv_uart_dma_enable_interrupts;
    ptr->disable_interrupts = drv_uart_dma_disable_interrupts;
    ptr->set_baudrate = drv_uart_dma_set_baudrate;
    ptr->set_parity = drv_uart_dma_set_parity;

    // 初始化获取接口函数指针
    ptr->get_id = drv_uart_get_id;
    ptr->get_hw_ptr = drv_uart_get_hw_ptr;
    ptr->get_rx_buffer = drv_uart_get_rx_buffer;

    // 初始化中断回调函数注册接口
    ptr->register_tx_complete_callback = drv_uart_dma_register_tx_complete_callback;
    ptr->register_rx_complete_callback = drv_uart_dma_register_rx_complete_callback;
    ptr->register_idle_callback = drv_uart_dma_register_idle_callback;
    ptr->register_error_callback = drv_uart_dma_register_error_callback;
    
    // 初始化中断回调函数指针为NULL
    ptr->tx_complete_callback = NULL;
    ptr->rx_complete_callback = NULL;
    ptr->idle_callback = NULL;
    ptr->error_callback = NULL;
    
    // 初始化独立的回调参数为NULL
    ptr->tx_complete_arg = NULL;
    ptr->rx_complete_arg = NULL;
    ptr->idle_arg = NULL;
    ptr->error_arg = NULL;
    ptr->callback_arg = NULL;  // 保持向后兼容
}

/**
  * @name     drv_uart_dma_register_tx_complete_callback
  * @brief    注册发送完成回调函数
  * @param    tx_complete_callback: 指向 UART DMA 实例的指针
  * @param    callback: 回调函数指针
  * @param    arg: 回调函数参数
  * @return   None
  * @remark   注册发送完成中断回调函数
  */
static void drv_uart_dma_register_tx_complete_callback(st_uart_dma_ptr ptr, 
                                                       UartTxCompleteCallback tx_complete_callback, 
                                                       void* arg) {
    ptr->tx_complete_callback = tx_complete_callback;
    ptr->tx_complete_arg = arg;  // 使用独立的发送完成回调参数
}

/**
  * @name     drv_uart_dma_register_rx_complete_callback
  * @brief    注册接收完成回调函数
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    rx_complete_callback: 回调函数指针
  * @param    arg: 回调函数参数
  * @return   None
  * @remark   注册接收完成中断回调函数
  */
static void drv_uart_dma_register_rx_complete_callback(st_uart_dma_ptr ptr, 
                                                       UartRxCompleteCallback rx_complete_callback, 
                                                       void* arg) {
    ptr->rx_complete_callback = rx_complete_callback;
    ptr->rx_complete_arg = arg;  // 使用独立的接收完成回调参数
}

/**
  * @name     drv_uart_dma_register_idle_callback
  * @brief    注册空闲中断回调函数
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    idle_callback: 回调函数指针
  * @param    arg: 回调函数参数
  * @return   None
  * @remark   注册空闲中断回调函数，用于处理不定长帧
  */
static void drv_uart_dma_register_idle_callback(st_uart_dma_ptr ptr, 
                                                UartIdleCallback idle_callback, 
                                                void* arg) {
    ptr->idle_callback = idle_callback;
    ptr->idle_arg = arg;  // 使用独立的空闲中断回调参数
}

/**
  * @name     drv_uart_dma_register_error_callback
  * @brief    注册错误中断回调函数
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    callback: 回调函数指针
  * @param    arg: 回调函数参数
  * @return   None
  * @remark   注册错误中断回调函数
  */
static void drv_uart_dma_register_error_callback(st_uart_dma_ptr ptr, 
                                                 UartErrorCallback callback, 
                                                 void* arg) {
    ptr->error_callback = callback;
    ptr->error_arg = arg;  // 使用独立的错误回调参数
}

/**
  * @name     drv_uart_dma_create
  * @brief    创建 UART DMA 实例
  * @param    ptr: 指向 UART DMA 实例的指针
  * @param    uart: UART 外设实例（如 USART1）
  * @param    id: UART 实例 ID
  * @return   et_uartDmaError: 错误码（UART_DMA_ERROR_NONE 表示成功）
  * @remark   初始化实例并加入全局链表，限制最大实例数
  */
et_uartDmaError drv_uart_dma_create(st_uart_dma_ptr ptr, 
                                    USART_TypeDef* uart, 
                                    et_UartDma_id id) {
    // 参数检查
    if (!ptr || !uart || id >= UartSum) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_CONFIG, NULL, NULL, 0, "Invalid parameters"};
        return UART_DMA_ERROR_INVALID_CONFIG;
    }

    // 检查硬件是否已初始化
    UART_HandleTypeDef* huart = &uart_dma_handles[id];
    if (huart->Instance != uart) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_CONFIG, NULL, NULL, 0, "UART not initialized"};
        return UART_DMA_ERROR_INVALID_CONFIG;
    }

    // 初始化实例
    ptr->Uart_id = id;
    ptr->uart_ptr = huart;
    ptr->dma_tx_ptr = &uart_dma_tx_handles[id];
    ptr->dma_rx_ptr = &uart_dma_rx_handles[id];
    ptr->rx_len = 0;
    ptr->last_processed_pos = 0;
    ptr->single_byte_rx_ready = false;
    memset(ptr->rx_buffer, 0, UART_DMA_RX_BUFFER_SIZE);
    ptr->configure = drv_uart_dma_configure;
    ptr->configure(ptr);

    // 加入全局链表
    int count = 0;
    st_uart_dma_ptr curr = uart_dma_hendle_ptr;
    while (curr) {
        if (curr->Uart_id == id) {
            last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_CONFIG, NULL, NULL, 0, "UART ID already used"};
            return UART_DMA_ERROR_INVALID_CONFIG;
        }
        count++;
        curr = curr->next;
    }
    if (count >= UART_DMA_MAX_INSTANCES) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_CONFIG, NULL, NULL, 0, "Max instances reached"};
        return UART_DMA_ERROR_INVALID_CONFIG;
    }

    ptr->next = uart_dma_hendle_ptr;
    uart_dma_hendle_ptr = ptr;

    // 启动 DMA 接收
    ptr->receive(ptr);

    last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_NONE, NULL, NULL, 0, NULL};
    return UART_DMA_ERROR_NONE;
}


/**
  * @name     drv_uart_dma_hw_init
  * @brief    初始化 UART 硬件和 DMA 配置
  * @param    uart: UART 外设实例（如 USART1）
  * @param    baudrate: 波特率值
  * @param    pin_set_index: 引脚配置索引（0 或 1）
  * @param    tx_dma_channel: 发送 DMA 通道
  * @param    tx_dma_request: 发送 DMA 请求
  * @param    rx_dma_channel: 接收 DMA 通道
  * @param    rx_dma_request: 接收 DMA 请求
  * @param    id: UART 实例 ID
  * @return   et_uartDmaError: 错误码（UART_DMA_ERROR_NONE 表示成功）
  * @remark   配置 GPIO、UART 和 DMA，启用中断并启动接收
  */
  et_uartDmaError drv_uart_dma_hw_init(USART_TypeDef* uart, 
                                       uint32_t baudrate, 
                                       uint8_t pin_set_index,
                                       DMA_Channel_TypeDef* tx_dma_channel, 
                                       uint32_t tx_dma_request,
                                       DMA_Channel_TypeDef* rx_dma_channel, 
                                       uint32_t rx_dma_request,
                                       et_UartDma_id id) {
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (!uart || !tx_dma_channel || !rx_dma_channel || id >= UartSum) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_CONFIG, NULL, NULL, 0, "Invalid parameters"};
        return UART_DMA_ERROR_INVALID_CONFIG;
    }

    // 检查 DMA 通道
    if (tx_dma_channel != DMA1_Channel1 && tx_dma_channel != DMA1_Channel2 &&
        tx_dma_channel != DMA1_Channel3 && tx_dma_channel != DMA1_Channel4 &&
        tx_dma_channel != DMA1_Channel5 && tx_dma_channel != DMA1_Channel6 &&
        tx_dma_channel != DMA1_Channel7) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_DMA_CONFLICT, NULL, NULL, 0, "Invalid TX DMA Channel"};
        return UART_DMA_ERROR_DMA_CONFLICT;
    }
    if (rx_dma_channel != DMA1_Channel1 && rx_dma_channel != DMA1_Channel2 &&
        rx_dma_channel != DMA1_Channel3 && rx_dma_channel != DMA1_Channel4 &&
        rx_dma_channel != DMA1_Channel5 && rx_dma_channel != DMA1_Channel6 &&
        rx_dma_channel != DMA1_Channel7) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_DMA_CONFLICT, NULL, NULL, 0, "Invalid RX DMA Channel"};
        return UART_DMA_ERROR_DMA_CONFLICT;
    }

    // 查找引脚映射
    const UartDmaPinMap* map = pin_map;
    while (map->uart_instance != NULL && map->uart_instance != uart) {
        map++;
    }
    if (map->uart_instance == NULL || pin_set_index >= 2) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_PIN, NULL, NULL, 0, "Invalid UART or pin set"};
        return UART_DMA_ERROR_INVALID_PIN;
    }

    const UartDmaPinSet* set = &map->sets[pin_set_index];
    if (set->tx.conflicting_module || set->rx.conflicting_module) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_PIN_CONFLICT, NULL, set->tx.port, set->tx.pin, set->tx.conflicting_module};
        return UART_DMA_ERROR_PIN_CONFLICT;
    }

    // 启用时钟
    if (uart == USART1) {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        __HAL_RCC_USART1_CLK_ENABLE();
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
            last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, NULL, NULL, 0, "HAL_UART_Init failed"};
            return UART_DMA_ERROR_INIT_FAILED;
        }
    }
    else if (uart == USART2){
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
        PeriphClkInit.Usart2ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        __HAL_RCC_USART2_CLK_ENABLE();
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
            last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, NULL, NULL, 0, "HAL_UART_Init failed"};
            return UART_DMA_ERROR_INIT_FAILED;
        }        
    }
    else if (uart == USART3){
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
        PeriphClkInit.Usart3ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        __HAL_RCC_USART3_CLK_ENABLE();
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
            last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, NULL, NULL, 0, "HAL_UART_Init failed"};
            return UART_DMA_ERROR_INIT_FAILED;
        }  
    }
    else {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_UART, NULL, NULL, 0, "Unsupported UART"};
        return UART_DMA_ERROR_INVALID_UART;
    }

    GPIO_TypeDef* ports[4] = {set->tx.port, set->rx.port};
    int port_count = 2;
#ifdef UART_DMA_HW_FLOW_CONTROL
    ports[2] = set->rts.port;
    ports[3] = set->cts.port;
    port_count = 4;
#endif
    for (int i = 0; i < port_count; i++) {
        if (ports[i] == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        else if (ports[i] == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        else if (ports[i] == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
        else if (ports[i] == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
        else {
            last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INVALID_PIN, NULL, ports[i], 0, "Invalid GPIO port"};
            return UART_DMA_ERROR_INVALID_PIN;
        }
    }

    // 配置引脚
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    GPIO_InitStruct.Pin = set->tx.pin;
    GPIO_InitStruct.Alternate = set->tx.af;
    HAL_GPIO_Init(set->tx.port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = set->rx.pin;
    GPIO_InitStruct.Alternate = set->rx.af;
    HAL_GPIO_Init(set->rx.port, &GPIO_InitStruct);

#ifdef UART_DMA_HW_FLOW_CONTROL
    GPIO_InitStruct.Pin = set->rts.pin;
    GPIO_InitStruct.Alternate = set->rts.af;
    HAL_GPIO_Init(set->rts.port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = set->cts.pin;
    GPIO_InitStruct.Alternate = set->cts.af;
    HAL_GPIO_Init(set->cts.port, &GPIO_InitStruct);
#endif

    // 串口配置
    UART_HandleTypeDef* huart = &uart_dma_handles[id];
    DMA_HandleTypeDef* hdma_tx = &uart_dma_tx_handles[id];
    DMA_HandleTypeDef* hdma_rx = &uart_dma_rx_handles[id];

    memset(huart, 0, sizeof(UART_HandleTypeDef));
    huart->Instance = uart; 
    huart->Init.BaudRate = baudrate > 0 ? baudrate : UART_DMA_DEFAULT_BAUDRATE;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
#ifdef UART_DMA_HW_FLOW_CONTROL
    huart->Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
#else
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
#endif
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(huart) != HAL_OK) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, huart, NULL, 0, "HAL_UART_Init failed"};
        return UART_DMA_ERROR_INIT_FAILED;
    }
    if (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK){
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, huart, NULL, 0, "HAL_UART_Init failed"};
        return UART_DMA_ERROR_INIT_FAILED;
    }
    if (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK){
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, huart, NULL, 0, "HAL_UART_Init failed"};
        return UART_DMA_ERROR_INIT_FAILED;
    }
    if (HAL_UARTEx_DisableFifoMode(huart) != HAL_OK){
    
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, huart, NULL, 0, "HAL_UART_Init failed"};
        return UART_DMA_ERROR_INIT_FAILED;
    }
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_IDLEF);//add

    // 启用 DMA 时钟
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    // 配置 DMA 发送
    hdma_tx->Instance = tx_dma_channel; // 指向DMA实例
    hdma_tx->Init.Request = tx_dma_request;
    hdma_tx->Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tx->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tx->Init.MemInc = DMA_MINC_ENABLE;
    hdma_tx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_tx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_tx->Init.Mode = DMA_NORMAL;
    hdma_tx->Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(hdma_tx) != HAL_OK) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, NULL, NULL, 0, "HAL_DMA_Init TX failed"};
        return UART_DMA_ERROR_INIT_FAILED;
    }
    __HAL_LINKDMA(huart, hdmatx, *hdma_tx);

    // 配置 DMA 接收
    hdma_rx->Instance = rx_dma_channel; // 指向DMA实例
    hdma_rx->Init.Request = rx_dma_request;
    hdma_rx->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_rx->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_rx->Init.MemInc = DMA_MINC_ENABLE;
    hdma_rx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_rx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_rx->Init.Mode = DMA_CIRCULAR;  // 保持循环模式
    hdma_rx->Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(hdma_rx) != HAL_OK) {
        last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_INIT_FAILED, NULL, NULL, 0, "HAL_DMA_Init RX failed"};
        return UART_DMA_ERROR_INIT_FAILED;
    }
    __HAL_LINKDMA(huart, hdmarx, *hdma_rx);

    // 启用中断
    IRQn_Type tx_irq = (tx_dma_channel == DMA1_Channel1) ? DMA1_Channel1_IRQn :
                       (tx_dma_channel == DMA1_Channel2) ? DMA1_Channel2_IRQn :
                       (tx_dma_channel == DMA1_Channel3) ? DMA1_Channel3_IRQn :
                       (tx_dma_channel == DMA1_Channel4) ? DMA1_Channel4_IRQn :
                       (tx_dma_channel == DMA1_Channel5) ? DMA1_Channel5_IRQn :
                       (tx_dma_channel == DMA1_Channel6) ? DMA1_Channel6_IRQn :
                       DMA1_Channel7_IRQn;
    HAL_NVIC_SetPriority(tx_irq, 5, 0);
    HAL_NVIC_EnableIRQ(tx_irq);

    IRQn_Type rx_irq = (rx_dma_channel == DMA1_Channel1) ? DMA1_Channel1_IRQn :
                       (rx_dma_channel == DMA1_Channel2) ? DMA1_Channel2_IRQn :
                       (rx_dma_channel == DMA1_Channel3) ? DMA1_Channel3_IRQn :
                       (rx_dma_channel == DMA1_Channel4) ? DMA1_Channel4_IRQn :
                       (rx_dma_channel == DMA1_Channel5) ? DMA1_Channel5_IRQn :
                       (rx_dma_channel == DMA1_Channel6) ? DMA1_Channel6_IRQn :
                       DMA1_Channel7_IRQn;
    HAL_NVIC_SetPriority(rx_irq, 5, 0);
    HAL_NVIC_EnableIRQ(rx_irq);

    HAL_NVIC_SetPriority(set->uart_irq, 5, 0);
    HAL_NVIC_EnableIRQ(set->uart_irq);

    last_error = (st_uartDmaErrorDetail){UART_DMA_ERROR_NONE, NULL, NULL, 0, NULL};
    return UART_DMA_ERROR_NONE;
}

/**
  * @name     drv_uart_dma_get_last_error
  * @brief    获取最后一次 UART DMA 操作的错误详情
  * @param    None
  * @return   st_uartDmaErrorDetail: 错误详情结构体
  * @remark   返回全局错误状态，供调试或错误处理
  */
st_uartDmaErrorDetail drv_uart_dma_get_last_error(void) {
    return last_error;
}


/**
  * @name     HAL_UART_TxCpltCallback
  * @brief    UART 发送完成中断回调
  * @param    huart: 指向 UART 硬件句柄的指针
  * @return   None
  * @remark   调用注册的发送完成回调函数
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    st_uart_dma_ptr ptr;
    for (ptr = uart_dma_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->uart_ptr == huart) {
            if (ptr->tx_complete_callback) {
                ptr->tx_complete_callback(ptr, ptr->tx_complete_arg);
            }
            break;
        }
    }
}

/**
  * @name     HAL_UART_RxCpltCallback
  * @brief    UART 接收完成中断回调
  * @param    huart: 指向 UART 硬件句柄的指针
  * @return   None
  * @remark   调用注册的接收完成回调函数（DMA循环模式时不会执行）
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    st_uart_dma_ptr ptr;
    for (ptr = uart_dma_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->uart_ptr == huart) {
            if (ptr->rx_complete_callback) {
                ptr->rx_complete_callback(ptr, ptr->rx_complete_arg);
            }
            break;
        }
    }
}

/**
  * @name     HAL_UART_IDLECallback
  * @brief    UART 空闲中断回调，处理不定长帧
  * @param    huart: 指向 UART 硬件句柄的指针
  * @return   None
  * @remark   调用注册的空闲中断回调函数，并重启DMA接收
  */
void HAL_UART_IDLECallback(UART_HandleTypeDef* huart) {
    st_uart_dma_ptr ptr;
    for (ptr = uart_dma_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->uart_ptr == huart) {
            if (ptr->idle_callback) {
                // 暂停 DMA 以确保数据一致性
                HAL_UART_DMAPause(huart);
                
                // 清除空闲中断标志
                __HAL_UART_CLEAR_IDLEFLAG(huart);

                // 计算当前DMA接收位置
                uint32_t current_pos = UART_DMA_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);
                
                // 计算本次新接收的数据长度和起始位置
                uint32_t new_data_len = 0;
                uint32_t new_data_start = 0;
                
                if (current_pos >= ptr->last_processed_pos) {
                    // 没有跨越缓冲区边界
                    new_data_len = current_pos - ptr->last_processed_pos;
                    new_data_start = ptr->last_processed_pos;
                } else {
                    // 跨越了缓冲区边界
                    new_data_len = UART_DMA_RX_BUFFER_SIZE - ptr->last_processed_pos + current_pos;
                    new_data_start = ptr->last_processed_pos;
                }
                
                // 更新上次处理的位置
                ptr->last_processed_pos = current_pos;
                ptr->rx_len = new_data_len;
                
                // 调用空闲回调函数，传递本次新接收的数据长度和起始位置
                ptr->idle_callback(ptr, new_data_len, new_data_start, ptr->idle_arg);
            }
            break;
        }
    }
}


/**
  * @name     HAL_UART_ErrorCallback
  * @brief    UART 错误中断回调
  * @param    huart: 指向 UART 硬件句柄的指针
  * @return   None
  * @remark   调用注册的错误回调函数
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    st_uart_dma_ptr ptr;
    for (ptr = uart_dma_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->uart_ptr == huart) {
            if (ptr->error_callback) {
                ptr->error_callback(ptr, huart->ErrorCode, ptr->error_arg);
            }
            break;
        }
    }
}

/**
  * @brief  USART1 中断处理函数
  * @param  None
  * @return None
  * @remark 调用 HAL_UART_IRQHandler 处理 UART 中断
  */
void USART1_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&uart_dma_handles[Uart0], UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&uart_dma_handles[Uart0]);
        HAL_UART_IDLECallback(&uart_dma_handles[Uart0]);
    }
    HAL_UART_IRQHandler(&uart_dma_handles[Uart0]);
}

/**
  * @brief  USART2 中断处理函数
  * @param  None
  * @return None
  * @remark 调用 HAL_UART_IRQHandler 处理 UART 中断
  */
void USART2_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&uart_dma_handles[Uart1], UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&uart_dma_handles[Uart1]);
        HAL_UART_IDLECallback(&uart_dma_handles[Uart1]);
    }
    HAL_UART_IRQHandler(&uart_dma_handles[Uart1]);
}

/**
  * @brief  DMA1 Channel 4 中断处理函数（发送）
  * @param  None
  * @return None
  * @remark 调用 HAL_DMA_IRQHandler 处理 DMA 发送中断
  */
void DMA1_Channel4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&uart_dma_tx_handles[Uart0]);
}

/**
  * @brief  DMA1 Channel 5 中断处理函数（接收）
  * @param  None
  * @return None
  * @remark 调用 HAL_DMA_IRQHandler 处理 DMA 接收中断
  */
void DMA1_Channel5_IRQHandler(void) {
    HAL_DMA_IRQHandler(&uart_dma_rx_handles[Uart0]);
}

/**
  * @brief  DMA1 Channel 6 中断处理函数（USART2接收）
  * @param  None
  * @return None
  * @remark 调用 HAL_DMA_IRQHandler 处理 DMA 接收中断
  */
void DMA1_Channel6_IRQHandler(void) {
    HAL_DMA_IRQHandler(&uart_dma_rx_handles[Uart1]);
}

/**
  * @brief  DMA1 Channel 7 中断处理函数（USART2发送）
  * @param  None
  * @return None
  * @remark 调用 HAL_DMA_IRQHandler 处理 DMA 发送中断
  */
void DMA1_Channel7_IRQHandler(void) {
    HAL_DMA_IRQHandler(&uart_dma_tx_handles[Uart1]);
}

/**
  * @name     drv_uart_get_id
  * @brief    获取UART实例ID
  * @param    uart_ptr: 指向UART DMA实例的指针
  * @return   uint8_t: UART实例ID
  * @remark   供协议层调用，获取UART实例ID
  */
static uint8_t drv_uart_get_id(st_uart_dma_ptr uart_ptr) {
    if (uart_ptr) {
        return uart_ptr->Uart_id;
    }
    return 0xFF;
}

/**
  * @name     drv_uart_get_hw_ptr
  * @brief    获取UART硬件句柄指针
  * @param    uart_ptr: 指向UART DMA实例的指针
  * @return   void*: UART硬件句柄指针
  * @remark   供协议层调用，获取UART硬件句柄
  */
static void* drv_uart_get_hw_ptr(st_uart_dma_ptr uart_ptr) {
    if (uart_ptr) {
        return uart_ptr->uart_ptr;
    }
    return NULL;
}

/**
  * @name     drv_uart_get_rx_buffer
  * @brief    获取UART接收缓冲区指针
  * @param    uart_ptr: 指向UART DMA实例的指针
  * @return   uint8_t*: 接收缓冲区指针
  * @remark   供协议层调用，获取接收缓冲区
  */
static uint8_t* drv_uart_get_rx_buffer(st_uart_dma_ptr uart_ptr) {
    if (uart_ptr) {
        return uart_ptr->rx_buffer;
    }
    return NULL;
}

// 供协议层直接赋值的硬件操作函数实现
void drv_uart_send_impl(void* uart_dma_ptr, uint8_t* data, uint32_t len) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->send) {
        ptr->send(ptr, data, len);
    }
}
void drv_uart_receive_impl(void* uart_dma_ptr) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->receive) {
        ptr->receive(ptr);
    }
}
void drv_uart_set_baudrate_impl(void* uart_dma_ptr, uint32_t baudrate) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->set_baudrate) {
        ptr->set_baudrate(ptr, baudrate);
    }
}
void drv_uart_set_parity_impl(void* uart_dma_ptr, uint8_t parity) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->set_parity) {
        ptr->set_parity(ptr, (et_uartDmaParity)parity);
    }
}
void drv_uart_enable_interrupts_impl(void* uart_dma_ptr) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->enable_interrupts) {
        ptr->enable_interrupts(ptr);
    }
}
void drv_uart_disable_interrupts_impl(void* uart_dma_ptr) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->disable_interrupts) {
        ptr->disable_interrupts(ptr);
    }
}
uint8_t drv_uart_get_id_impl(void* uart_dma_ptr) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->get_id) {
        return ptr->get_id(ptr);
    }
    return 0xFF;
}
void* drv_uart_get_hw_ptr_impl(void* uart_dma_ptr) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->get_hw_ptr) {
        return ptr->get_hw_ptr(ptr);
    }
    return NULL;
}
uint8_t* drv_uart_get_rx_buffer_impl(void* uart_dma_ptr) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->get_rx_buffer) {
        return ptr->get_rx_buffer(ptr);
    }
    return NULL;
}
void drv_uart_register_tx_complete_callback_impl(void* uart_dma_ptr, 
                                             void (*cb)(void*, void*), 
                                             void* arg) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->register_tx_complete_callback) {
        ptr->register_tx_complete_callback(ptr, cb, arg);
    }
}
void drv_uart_register_rx_complete_callback_impl(void* uart_dma_ptr, 
                                             void (*cb)(void*, void*), 
                                             void* arg) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->register_rx_complete_callback) {
        ptr->register_rx_complete_callback(ptr, cb, arg);
    }
}
void drv_uart_register_idle_callback_impl(void* uart_dma_ptr, 
                                      void (*cb)(void*, uint32_t, uint32_t, void*), 
                                      void* arg) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->register_idle_callback) {
        ptr->register_idle_callback(ptr, cb, arg);
    }
}
void drv_uart_register_error_callback_impl(void* uart_dma_ptr, 
                                       void (*cb)(void*, uint32_t, void*), 
                                       void* arg) {
    st_uart_dma_ptr ptr = (st_uart_dma_ptr)uart_dma_ptr;
    if (ptr && ptr->register_error_callback) {
        ptr->register_error_callback(ptr, cb, arg);
    }
}
/*---End of File----------------------------------------------------*/

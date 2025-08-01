/**
  ******************************************************************************
  * @file:    drv_can_int.c
  * @author: 
  * @date: 
  * @email:  
  ******************************************************************************
  * @attention
  ******************************************************************************
  * @verbatim
  ==============================================================================
               
  ==============================================================================
  * @endverbatim
  ******************************************************************************
  */
#if 0

#include "drv_can_int.h"

// 引脚映射表结构
typedef struct {
    GPIO_TypeDef* port;    // 端口
    uint16_t pin;          // 引脚
    uint32_t af;           // AF 值
    const char* conflicting_module; // 可能冲突的模块
} CanIntPinConfig;

typedef struct {
    FDCAN_GlobalTypeDef* can_instance; // CAN 实例
    CanIntPinConfig tx;                // TX 引脚
    CanIntPinConfig rx;                // RX 引脚
    uint32_t can_irq;                  // CAN 中断号
} CanIntPinSet;

typedef struct {
    FDCAN_GlobalTypeDef* can_instance; // CAN 实例
    CanIntPinSet sets[2];              // 每 CAN 支持 2 组引脚
} CanIntPinMap;

// 引脚映射表（基于 LQFP64 封装）
static const CanIntPinMap pin_map[] = {
    { // FDCAN1
        FDCAN1,
        {
            {{GPIOA, GPIO_PIN_11, GPIO_AF9_FDCAN1, "TIM1"}, {GPIOA, GPIO_PIN_12, GPIO_AF9_FDCAN1, "TIM1"}, FDCAN1_IT0_IRQn},
            {{GPIOB, GPIO_PIN_8,  GPIO_AF9_FDCAN1, "TIM4"}, {GPIOB, GPIO_PIN_9,  GPIO_AF9_FDCAN1, "TIM4"}, FDCAN1_IT0_IRQn}
        }
    },
    { // FDCAN2
        FDCAN2,
        {
            {{GPIOB, GPIO_PIN_12, GPIO_AF9_FDCAN2, NULL}, {GPIOB, GPIO_PIN_13, GPIO_AF9_FDCAN2, NULL}, FDCAN2_IT0_IRQn},
            {{GPIOB, GPIO_PIN_5,  GPIO_AF3_FDCAN2, "TIM3"}, {GPIOB, GPIO_PIN_6,  GPIO_AF3_FDCAN2, "TIM4"}, FDCAN2_IT0_IRQn}
        }
    },
    { NULL, {{{NULL, 0, 0, NULL}}}, 0 } // 结束标志
};

// 全局变量
st_can_int_ptr timerCanInt_hendle_ptr = NULL; // CAN 链表头
static st_canIntErrorDetail last_error = {ERROR_NONE, NULL, NULL, 0, NULL}; // 最后错误

// 超时定时器回调
static void timeout_callback(void* arg) {
    st_can_int_ptr ptr = (st_can_int_ptr)arg;
    if (ptr->callback) {
        ptr->callback(ptr, TIMEOUT_EVENT, NULL, NULL, ptr->callback_arg);
    }
}

/**
  * @name     lib_can_int_get_state
  * @brief    获取 CAN 状态
  * @param    ptr: CAN 指针
  * @return   et_canIntState: 当前状态
  * @remark   
  */
static et_canIntState lib_can_int_get_state(st_can_int_ptr ptr) {
    return ptr->state;
}

/**
  * @name     lib_can_int_send
  * @brief    发送 CAN 帧
  * @param    ptr: CAN 指针
  * @param    id: CAN ID
  * @param    id_type: ID 类型（标准/扩展）
  * @param    data: 数据缓冲区
  * @param    len: 数据长度
  * @retval   None
  * @remark   
  */
static void lib_can_int_send(st_can_int_ptr ptr, uint32_t id, et_canIntIdType id_type, uint8_t* data, uint32_t len) {
    if (ptr->hw_ptr == NULL || ptr->state == CAN_STATE_TX_BUSY) return;

    ptr->state = CAN_STATE_TX_BUSY;

    FDCAN_TxHeaderTypeDef tx_header = {0};
    tx_header.Identifier = id;
    tx_header.IdType = id_type == ID_TYPE_STANDARD ? FDCAN_STANDARD_ID : FDCAN_EXTENDED_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = len <= 8 ? (len << 16) : FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;

    if (HAL_FDCAN_AddMessageToTxFifoQ(ptr->hw_ptr, &tx_header, data) == HAL_OK) {
        // 启动超时定时器（示例：100ms）
        ptr->timeout_timer.setr_argument(&ptr->timeout_timer, OnceMode, 100000); // 100ms
        ptr->timeout_timer.start(&ptr->timeout_timer);
    } else {
        ptr->state = CAN_STATE_ERROR;
        last_error = (st_canIntErrorDetail){ERROR_INIT_FAILED, ptr->hw_ptr, NULL, 0, NULL};
    }
}

/**
  * @name     lib_can_int_receive
  * @brief    启动 CAN 接收
  * @param    ptr: CAN 指针
  * @retval   None
  * @remark   
  */
static void lib_can_int_receive(st_can_int_ptr ptr) {
    if (ptr->hw_ptr == NULL || ptr->state == CAN_STATE_RX_BUSY) return;

    ptr->state = CAN_STATE_RX_BUSY;
    HAL_FDCAN_ActivateNotification(ptr->hw_ptr, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
}

/**
  * @name     lib_can_int_enable_interrupts
  * @brief    启用 CAN 中断
  * @param    ptr: CAN 指针
  * @retval   None
  * @remark   
  */
static void lib_can_int_enable_interrupts(st_can_int_ptr ptr) {
    if (ptr->hw_ptr == NULL) return;

    HAL_FDCAN_ActivateNotification(ptr->hw_ptr, FDCAN_IT_TX_FIFO_EMPTY | FDCAN_IT_RX_FIFO0_NEW_MESSAGE | 
                                       FDCAN_IT_RX_FIFO1_NEW_MESSAGE | FDCAN_IT_BUS_OFF | FDCAN_IT_PROTOCOL_ERROR, 0);
}

/**
  * @name     lib_can_int_disable_interrupts
  * @brief    禁用 CAN 中断
  * @param    ptr: CAN 指针
  * @retval   None
  * @remark   
  */
static void lib_can_int_disable_interrupts(st_can_int_ptr ptr) {
    if (ptr->hw_ptr == NULL) return;

    HAL_FDCAN_DeactivateNotification(ptr->hw_ptr, FDCAN_IT_TX_FIFO_EMPTY | FDCAN_IT_RX_FIFO0_NEW_MESSAGE | 
                                          FDCAN_IT_RX_FIFO1_NEW_MESSAGE | FDCAN_IT_BUS_OFF | FDCAN_IT_PROTOCOL_ERROR);
}

/**
  * @name     lib_can_int_set_baudrate
  * @brief    动态设置标称波特率
  * @param    ptr: CAN 指针
  * @param    baudrate: 新波特率（bps）
  * @retval   None
  * @remark   
  */
static void lib_can_int_set_baudrate(st_can_int_ptr ptr, uint32_t baudrate) {
    if (ptr->hw_ptr == NULL || baudrate == 0) return;

    ptr->hw_ptr->Init.NominalPrescaler = (HAL_RCC_GetPCLK1Freq() / (baudrate * (ptr->hw_ptr->Init.NominalTimeSeg1 + 
                                         ptr->hw_ptr->Init.NominalTimeSeg2 + 1)));
    HAL_FDCAN_Init(ptr->hw_ptr);
    ptr->receive(ptr);
}

/**
  * @name     lib_can_int_set_filter
  * @brief    配置 CAN 过滤器
  * @param    ptr: CAN 指针
  * @param    id: 过滤器 ID
  * @param    mask: 过滤器掩码（掩码模式）
  * @param    id_type: ID 类型（标准/扩展）
  * @param    filter_type: 过滤器类型（掩码/列表）
  * @retval   None
  * @remark   
  */
static void lib_can_int_set_filter(st_can_int_ptr ptr, uint32_t id, uint32_t mask, et_canIntIdType id_type, et_canIntFilterType filter_type) {
    if (ptr->hw_ptr == NULL) return;

    FDCAN_FilterTypeDef filter = {0};
    filter.IdType = id_type == ID_TYPE_STANDARD ? FDCAN_STANDARD_ID : FDCAN_EXTENDED_ID;
    filter.FilterIndex = 0;
    filter.FilterType = filter_type == FILTER_TYPE_MASK ? FDCAN_FILTER_MASK : FDCAN_FILTER_RANGE;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = id;
    filter.FilterID2 = filter_type == FILTER_TYPE_MASK ? mask : id;

    HAL_FDCAN_ConfigFilter(ptr->hw_ptr, &filter);
}

/**
  * @name     lib_can_int_initialize
  * @brief    初始化 CAN 结构体
  * @param    ptr: CAN 指针
  * @param    baudrate: 标称波特率
  * @param    frame_type: 帧类型（经典/FD）
  * @param    callback: 数据回调函数
  * @param    arg: 回调参数
  * @retval   None
  * @remark   
  */
static void lib_can_int_initialize(st_can_int_ptr ptr, 
                                   uint32_t baudrate, 
                                   et_canIntFrameType frame_type,
                                   void (*callback)(struct can_int*, et_canIntEvent, FDCAN_RxHeaderTypeDef*, uint8_t*, void*), 
                                   void* arg) {
    ptr->state = CAN_STATE_IDLE;
    ptr->rx_len = 0;
    ptr->callback = callback;
    ptr->callback_arg = arg;
    memset(ptr->rx_buffer, 0, CAN_INT_RX_FIFO_SIZE * 64);

    // 初始化超时定时器
    ptr->timeout_timer.SoftTime_id = (et_SoftTime_id)(ptr->Can_id);
    drv_timer_countdown_create(&ptr->timeout_timer);
    ptr->timeout_timer.setr_call_back(&ptr->timeout_timer, timeout_callback, ptr);

    if (ptr->hw_ptr) {
        ptr->hw_ptr->Init.NominalPrescaler = (HAL_RCC_GetPCLK1Freq() / (baudrate * (ptr->hw_ptr->Init.NominalTimeSeg1 + 
                                             ptr->hw_ptr->Init.NominalTimeSeg2 + 1)));
        ptr->hw_ptr->Init.FrameFormat = frame_type == FRAME_TYPE_CLASSIC ? FDCAN_FRAME_CLASSIC : FDCAN_FRAME_FD_NO_BRS;
        HAL_FDCAN_Init(ptr->hw_ptr);
        ptr->receive(ptr);
    }
}

/**
  * @name     lib_can_int_configure
  * @brief    配置 CAN 函数指针
  * @param    ptr: CAN 指针
  * @retval   None
  * @remark   
  */
static void lib_can_int_configure(st_can_int_ptr ptr) {
    ptr->initialize = lib_can_int_initialize;
    ptr->get_state = lib_can_int_get_state;
    ptr->send = lib_can_int_send;
    ptr->receive = lib_can_int_receive;
    ptr->enable_interrupts = lib_can_int_enable_interrupts;
    ptr->disable_interrupts = lib_can_int_disable_interrupts;
    ptr->set_baudrate = lib_can_int_set_baudrate;
    ptr->set_filter = lib_can_int_set_filter;
}

/**
  * @name     drv_can_int_create
  * @brief    创建 CAN 实例
  * @param    ptr: CAN 指针
  * @param    can: CAN 实例（例如 FDCAN1）
  * @retval   et_canIntError: 错误码
  * @remark   
  */
et_canIntError drv_can_int_create(st_can_int_ptr ptr, FDCAN_GlobalTypeDef* can) {
    if (!ptr || !can) {
        last_error = (st_canIntErrorDetail){ERROR_INVALID_CONFIG, NULL, NULL, 0, NULL};
        return ERROR_INVALID_CONFIG;
    }

    ptr->Can_id = CanSum; // 将在 hw_init 中分配
    ptr->hw_ptr = NULL; // 将在 hw_init 中分配
    ptr->configure = lib_can_int_configure;
    ptr->configure(ptr);

    // 添加到链表
    st_can_int_ptr curr = timerCanInt_hendle_ptr;
    int count = 0;
    while (curr) {
        count++;
        curr = curr->next;
    }
    if (count >= CAN_INT_MAX_INSTANCES) {
        last_error = (st_canIntErrorDetail){ERROR_INVALID_CONFIG, NULL, NULL, 0, NULL};
        return ERROR_INVALID_CONFIG;
    }

    ptr->next = timerCanInt_hendle_ptr;
    timerCanInt_hendle_ptr = ptr;

    last_error = (st_canIntErrorDetail){ERROR_NONE, NULL, NULL, 0, NULL};
    return ERROR_NONE;
}

/**
  * @name     drv_can_int_hw_init
  * @brief    初始化 CAN 硬件
  * @param    can: CAN 实例（例如 FDCAN1）
  * @param    baudrate: 标称波特率（例如 500000）
  * @param    pin_set_index: 引脚组索引（0 为默认）
  * @retval   et_canIntError: 错误码
  * @remark   
  */
et_canIntError drv_can_int_hw_init(FDCAN_GlobalTypeDef* can, uint32_t baudrate, uint8_t pin_set_index) {
    if (!can) {
        last_error = (st_canIntErrorDetail){ERROR_INVALID_CAN, NULL, NULL, 0, NULL};
        return ERROR_INVALID_CAN;
    }

    // 查找引脚映射
    const CanIntPinMap* map = pin_map;
    while (map->can_instance != NULL) {
        if (map->can_instance == can) {
            break;
        }
        map++;
    }

    if (map->can_instance == NULL || pin_set_index >= 2) {
        last_error = (st_canIntErrorDetail){ERROR_INVALID_PIN, NULL, NULL, 0, NULL};
        return ERROR_INVALID_PIN;
    }

    const CanIntPinSet* set = &map->sets[pin_set_index];

    // 检查引脚冲突
    if (set->tx.conflicting_module || set->rx.conflicting_module) {
        last_error = (st_canIntErrorDetail){ERROR_PIN_CONFLICT, NULL, set->tx.port, set->tx.pin, set->tx.conflicting_module};
        return ERROR_PIN_CONFLICT;
    }

    // 启用 CAN 和 GPIO 时钟
    if (can == FDCAN1) __HAL_RCC_FDCAN_CLK_ENABLE();
    else if (can == FDCAN2) __HAL_RCC_FDCAN_CLK_ENABLE();
    if (set->tx.port == GPIOA || set->rx.port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    if (set->tx.port == GPIOB || set->rx.port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();

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

    // 配置 FDCAN
    FDCAN_HandleTypeDef* hfdcan = malloc(sizeof(FDCAN_HandleTypeDef));
    if (!hfdcan) {
        last_error = (st_canIntErrorDetail){ERROR_INIT_FAILED, NULL, NULL, 0, NULL};
        return ERROR_INIT_FAILED;
    }

    hfdcan->Instance = can;
    hfdcan->Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan->Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan->Init.AutoRetransmission = ENABLE;
    hfdcan->Init.TransmitPause = DISABLE;
    hfdcan->Init.ProtocolException = ENABLE;
    hfdcan->Init.NominalPrescaler = (HAL_RCC_GetPCLK1Freq() / (baudrate * 20)); // 假设 Tseg1=13, Tseg2=2
    hfdcan->Init.NominalTimeSeg1 = 13;
    hfdcan->Init.NominalTimeSeg2 = 2;
    hfdcan->Init.NominalSyncJumpWidth = 1;
    hfdcan->Init.DataPrescaler = 1;
    hfdcan->Init.DataTimeSeg1 = 1;
    hfdcan->Init.DataTimeSeg2 = 1;
    hfdcan->Init.DataSyncJumpWidth = 1;
    hfdcan->Init.StdFiltersNbr = 1;
    hfdcan->Init.ExtFiltersNbr = 1;
    hfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(hfdcan) != HAL_OK) {
        free(hfdcan);
        last_error = (st_canIntErrorDetail){ERROR_INIT_FAILED, NULL, NULL, 0, NULL};
        return ERROR_INIT_FAILED;
    }

    // 配置默认过滤器（接受所有 ID）
    FDCAN_FilterTypeDef filter = {0};
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterIndex = 0;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = 0;
    filter.FilterID2 = 0;
    HAL_FDCAN_ConfigFilter(hfdcan, &filter);

    filter.IdType = FDCAN_EXTENDED_ID;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
    HAL_FDCAN_ConfigFilter(hfdcan, &filter);

    // 启用中断
    HAL_NVIC_SetPriority(set->can_irq, 0, 0);
    HAL_NVIC_EnableIRQ(set->can_irq);

    // 更新实例
    st_can_int_ptr ptr = timerCanInt_hendle_ptr;
    et_CanInt_id id = Can0;
    while (ptr) {
        if (ptr->Can_id == CanSum) {
            ptr->hw_ptr = hfdcan;
            ptr->Can_id = id;
            ptr->initialize(ptr, baudrate, FRAME_TYPE_CLASSIC, NULL, NULL);
            break;
        }
        id++;
        ptr = ptr->next;
    }

    last_error = (st_canIntErrorDetail){ERROR_NONE, NULL, NULL, 0, NULL};
    return ERROR_NONE;
}

/**
  * @name     drv_can_int_get_last_error
  * @brief    获取最后错误详情
  * @param    None
  * @retval   st_canIntErrorDetail: 错误详情
  * @remark   
  */
st_canIntErrorDetail drv_can_int_get_last_error(void) {
    return last_error;
}

/**
  * @name     HAL_FDCAN_TxFifoEmptyCallback
  * @brief    CAN 发送完成回调
  * @param    hfdcan: FDCAN 句柄
  * @retval   None
  * @remark   
  */
void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef* hfdcan) {
    st_can_int_ptr ptr;
    for (ptr = timerCanInt_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->hw_ptr == hfdcan) {
            ptr->state = CAN_STATE_IDLE;
            ptr->timeout_timer.stop(&ptr->timeout_timer);
            if (ptr->callback) {
                ptr->callback(ptr, DATA_RECEIVED, NULL, NULL, ptr->callback_arg);
            }
            break;
        }
    }
}

/**
  * @name     HAL_FDCAN_RxFifo0Callback
  * @brief    CAN 接收 FIFO0 回调（标准 ID）
  * @param    hfdcan: FDCAN 句柄
  * @param    RxFifo0ITs: FIFO0 中断标志
  * @retval   None
  * @remark   
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    st_can_int_ptr ptr;
    for (ptr = timerCanInt_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->hw_ptr == hfdcan) {
            FDCAN_RxHeaderTypeDef rx_header;
            uint8_t* data = ptr->rx_buffer + ptr->rx_len * 64;
            if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, data) == HAL_OK) {
                ptr->rx_len = (ptr->rx_len + 1) % CAN_INT_RX_FIFO_SIZE;
                ptr->timeout_timer.stop(&ptr->timeout_timer);
                if (ptr->callback) {
                    ptr->callback(ptr, DATA_RECEIVED, &rx_header, data, ptr->callback_arg);
                }
            }
            break;
        }
    }
}

/**
  * @name     HAL_FDCAN_RxFifo1Callback
  * @brief    CAN 接收 FIFO1 回调（扩展 ID）
  * @param    hfdcan: FDCAN 句柄
  * @param    RxFifo1ITs: FIFO1 中断标志
  * @retval   None
  * @remark   
  */
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo1ITs) {
    st_can_int_ptr ptr;
    for (ptr = timerCanInt_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->hw_ptr == hfdcan) {
            FDCAN_RxHeaderTypeDef rx_header;
            uint8_t* data = ptr->rx_buffer + ptr->rx_len * 64;
            if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &rx_header, data) == HAL_OK) {
                ptr->rx_len = (ptr->rx_len + 1) % CAN_INT_RX_FIFO_SIZE;
                ptr->timeout_timer.stop(&ptr->timeout_timer);
                if (ptr->callback) {
                    ptr->callback(ptr, DATA_RECEIVED, &rx_header, data, ptr->callback_arg);
                }
            }
            break;
        }
    }
}

/**
  * @name     HAL_FDCAN_ErrorCallback
  * @brief    CAN 错误回调
  * @param    hfdcan: FDCAN 句柄
  * @retval   None
  * @remark   
  */
void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef* hfdcan) {
    st_can_int_ptr ptr;
    for (ptr = timerCanInt_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->hw_ptr == hfdcan) {
            ptr->state = CAN_STATE_ERROR;
            if (HAL_FDCAN_GetError(hfdcan) & HAL_FDCAN_ERROR_PROTOCOL) {
                last_error = (st_canIntErrorDetail){ERROR_PROTOCOL_ERROR, hfdcan, NULL, 0, NULL};
            } else if (HAL_FDCAN_GetError(hfdcan) & HAL_FDCAN_ERROR_BusOff) {
                last_error = (st_canIntErrorDetail){ERROR_BUS_OFF, hfdcan, NULL, 0, NULL};
            }
            ptr->receive(ptr); // 尝试恢复
            break;
        }
    }
}

/**
  * @name     FDCANx_IT0_IRQHandler
  * @brief    CAN 中断服务函数
  * @param    None
  * @retval   None
  * @remark   
  */
void FDCAN1_IT0_IRQHandler(void) {
    st_can_int_ptr ptr;
    for (ptr = timerCanInt_hendle_ptr; ptr; ptr = ptr->next) {
        if (ptr->hw_ptr->Instance == FDCAN1) {
            HAL_FDCAN_IRQHandler(ptr->hw_ptr);
            break;
        }
    }
}
#endif
/*---End of File----------------------------------------------------*/

/**
  ******************************************************************************
  * @file:    drv_can_int.h
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

#ifndef DRV_CAN_INT_H
#define DRV_CAN_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32g4xx_hal.h"

// CAN 配置
#define CAN_INT_DEFAULT_BAUDRATE 500000 // 默认标称波特率 500 kbps
#define CAN_INT_MAX_INSTANCES 2         // 最大支持 2 个 CAN 实例
#define CAN_INT_RX_FIFO_SIZE 64         // 接收 FIFO 大小（消息数）

/* CAN 实例 ID */
typedef enum {
    Can0 = 0,
    Can1,
    CanSum = 2
} et_CanInt_id;

/* CAN 状态 */
typedef enum {
    CAN_STATE_IDLE = 0,
    CAN_STATE_TX_BUSY,
    CAN_STATE_RX_BUSY,
    CAN_STATE_ERROR
} et_canIntState;

/* CAN 事件 */
typedef enum {
    DATA_RECEIVED = 0,
    TIMEOUT_EVENT
} et_canIntEvent;

/* 错误码 */
typedef enum {
    ERROR_NONE = 0,
    ERROR_INVALID_CAN,
    ERROR_INVALID_PIN,
    ERROR_INVALID_CONFIG,
    ERROR_INIT_FAILED,
    ERROR_PIN_CONFLICT,
    ERROR_PROTOCOL_ERROR,
    ERROR_BUS_OFF,
    ERROR_FIFO_OVERFLOW
} et_canIntError;

/* 错误详情 */
typedef struct {
    et_canIntError code;           // 错误码
    FDCAN_HandleTypeDef* can;      // 冲突 CAN
    GPIO_TypeDef* port;            // 冲突端口
    uint32_t pin;                  // 冲突引脚
    const char* conflicting_module; // 冲突模块
} st_canIntErrorDetail;

/* CAN 帧类型 */
typedef enum {
    FRAME_TYPE_CLASSIC = 0,        // 经典 CAN
    FRAME_TYPE_FD                  // CAN FD
} et_canIntFrameType;

/* CAN ID 类型 */
typedef enum {
    ID_TYPE_STANDARD = 0,          // 标准 ID (11 位)
    ID_TYPE_EXTENDED               // 扩展 ID (29 位)
} et_canIntIdType;

/* CAN 过滤器类型 */
typedef enum {
    FILTER_TYPE_MASK = 0,          // 掩码过滤
    FILTER_TYPE_LIST               // 列表过滤
} et_canIntFilterType;

typedef void (*CanIntCallback)(struct can_int*, et_canIntEvent event, FDCAN_RxHeaderTypeDef* rx_header, uint8_t* data, void* arg);

#pragma pack(1)
typedef struct can_int {
    et_CanInt_id Can_id;                // CAN 实例 ID
    et_canIntState state;               // 当前状态
    FDCAN_HandleTypeDef* hw_ptr;        // FDCAN 句柄
    uint8_t rx_buffer[CAN_INT_RX_FIFO_SIZE * 64]; // 接收缓冲区（64 字节/消息）
    uint32_t rx_len;                    // 接收数据长度
    CanIntCallback callback;            // 回调函数
    void* callback_arg;                 // 回调参数
    struct can_int* next;               // 链表指针
    st_soft_time timeout_timer;         // 超时定时器

    void (*initialize)(struct can_int*, uint32_t baudrate, et_canIntFrameType frame_type, void (*)(struct can_int*, et_canIntEvent, FDCAN_RxHeaderTypeDef*, uint8_t*, void*), void*);
    void (*configure)(struct can_int*);
    et_canIntState (*get_state)(struct can_int*);
    void (*send)(struct can_int*, uint32_t id, et_canIntIdType id_type, uint8_t* data, uint32_t len);
    void (*receive)(struct can_int*);
    void (*enable_interrupts)(struct can_int*);
    void (*disable_interrupts)(struct can_int*);
    void (*set_baudrate)(struct can_int*, uint32_t baudrate);
    void (*set_filter)(struct can_int*, uint32_t id, uint32_t mask, et_canIntIdType id_type, et_canIntFilterType filter_type);
} st_can_int, *st_can_int_ptr;
#pragma pack()

/**
  * @brief  Create a CAN interrupt instance
  * @param  ptr: Pointer to the CAN structure
  * @param  can: CAN instance (e.g., FDCAN1)
  * @retval et_canIntError: Error code
  */
et_canIntError drv_can_int_create(st_can_int_ptr ptr, FDCAN_GlobalTypeDef* can);

/**
  * @brief  Initialize CAN hardware
  * @param  can: CAN instance (e.g., FDCAN1)
  * @param  baudrate: Nominal baud rate (e.g., 500000)
  * @param  pin_set_index: Pin set index (0 for default)
  * @retval et_canIntError: Error code
  */
et_canIntError drv_can_int_hw_init(FDCAN_GlobalTypeDef* can, uint32_t baudrate, uint8_t pin_set_index);

/**
  * @brief  Get the last error details
  * @param  None
  * @retval st_canIntErrorDetail: Last error details
  */
st_canIntErrorDetail drv_can_int_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_CAN_INT_H */
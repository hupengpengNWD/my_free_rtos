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

#ifndef DRV_TIMER_ENCODER_H
#define DRV_TIMER_ENCODER_H

#ifdef __cplusplus
 extern "C" {
#endif
	
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32g4xx_hal.h"

// 硬件定时器配置
#define TIMER_ENCODER_MAX_COUNT    0xFFFF  // 默认 16 位定时器最大值
#define TIMER_ENCODER_DEFAULT_PRESCALER 0  // 默认无预分频

typedef enum {
    Encoder0 = 0,  
    Encoder1,      
    Encoder2,
    Encoder3,
    EncoderSum = 4 // 默认支持 4 个编码器实例
} et_Encoder_id;

/* 编码器状态 */
typedef enum {
    stopped_status = 0,  
    running_status,      
} et_encoderState;

/* 旋转方向 */
typedef enum {
    Direction_Forward = 0,       
    Direction_Reverse,        
} et_encoderDirection;

typedef void (*EncoderCallback)(void*);

#pragma pack (1)

typedef struct encoder {
    et_Encoder_id Encoder_id;                    
    uint8_t state;        
    uint32_t last_count;  // 上次计数（用于速度计算）
    uint32_t speed;       // 速度（脉冲/秒）
    EncoderCallback cb;   // 回调函数（溢出或索引）
    struct encoder* next;  // 链表指针
    void* CbArgu;         // 回调参数
    TIM_HandleTypeDef* hw_ptr; // 硬件定时器句柄
    
    void (*initialize)(struct encoder*);
    void (*configure)(struct encoder*);
    uint32_t (*get_count)(struct encoder*);
    uint8_t (*get_state)(struct encoder*);
    et_encoderDirection (*get_direction)(struct encoder*);
    uint32_t (*get_speed)(struct encoder*);
    void (*set_callback)(struct encoder*, void (*)(void*), void*);
    void (*stop)(struct encoder*);
    int (*start)(struct encoder*);
    void (*reset_count)(struct encoder*);
} st_encoder, *st_encoder_ptr; 

uint8_t drv_timer_encoder_create(st_encoder_ptr ptr);
uint8_t drv_timer_encoder_hw_init(TIM_TypeDef* tim_instance, uint32_t prescaler);

#pragma pack ()

#ifdef __cplusplus
}
#endif

#endif 

/*---End of File--------------------------------------------------*/
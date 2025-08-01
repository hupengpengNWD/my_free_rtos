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

#ifndef LIB_SOFT_TIMER_H
#define LIB_SOFT_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

#define SFTE_ALL_ENA 1U
#define SFTE_ALL_DIS 0U

typedef enum {
    SoftTime0 = 0,  
    SoftTime1,      
    SoftTime2,
    SoftTime3,
    SoftTimeAny,    // 动态分配的定时器 ID
} et_SoftTime_id;

typedef enum {
    stoped_status = 0,  
    running_status,      
    timeout_status,   
} et_timeState;

typedef enum {
    OnceMode = 0,       
    PeriodicMode,        
} et_timeMode;

typedef void (*SoftTimerCallback)(void*);

#pragma pack(1)
typedef struct softTime {
    et_SoftTime_id SoftTime_id;                    
    uint8_t state;        
    uint8_t mode;            
    uint64_t deadline;      // 绝对超时时间 (ms)
    uint64_t countdown;     // 定时周期 (ms)
    SoftTimerCallback cb;   // 回调函数
    struct softTime* next;  // 链表指针
    void* CbArgu;           // 回调参数
    void* hw_ptr;           // 硬件定时器
    
    void (*initialize)(struct softTime*);
    void (*configure)(struct softTime*);
    uint32_t (*get_tick_count)(void);
    uint8_t (*get_state)(struct softTime*);
    void (*setr_argument)(struct softTime*, uint8_t, uint64_t);
    void (*setr_call_back)(struct softTime*, void (*)(void*), void*);
    void (*fsm)(struct softTime*);
    void (*stop)(struct softTime*);
    int (*start)(struct softTime*);
    void (*enable)(struct softTime*, uint8_t);
} st_soft_time, *st_soft_time_ptr;
#pragma pack()

extern st_soft_time_ptr timerCountdown_hendle_ptr;

void lib_softtimer_initialize(st_soft_time_ptr ptr);
void lib_softtimer_configure(st_soft_time_ptr ptr);
uint32_t lib_softtimer_get_tick_count(void);
uint8_t lib_softtimer_get_state(st_soft_time_ptr ptr);
void lib_softtimer_set_argument(st_soft_time_ptr ptr, uint8_t mode, uint64_t countdown);
void lib_softtimer_set_callback(st_soft_time_ptr ptr, void (*cb)(void*), void *CbArgu);
void lib_softtimer_fsm(st_soft_time_ptr ptr);
void lib_softtimer_stop(st_soft_time_ptr ptr);
int lib_softtimer_start(st_soft_time_ptr ptr);
void lib_softtimer_enable(st_soft_time_ptr ptr, uint8_t opa);
void lib_softtimer_update_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* LIB_SOFT_TIMER_H */

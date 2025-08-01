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

#ifndef LIB_GPIO_OUTPUT_H
#define LIB_GPIO_OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "lib_soft_timer.h"

/* GPIO 输出状态 */
typedef enum {
    GPIO_STATE_LOW = 0,   // 低电平
    GPIO_STATE_HIGH,      // 高电平
    GPIO_STATE_COUNT      // 状态总数
} et_gpio_output_state;

typedef enum {
    GPIO_MODE_MANUAL,     // 手动控制（高/低）
    GPIO_MODE_SEQUENCE,   // 时序序列模式
} et_gpio_mode;

typedef enum {
    GPIO_EVENT_STATE_CHANGE, // 状态切换
    GPIO_EVENT_SEQUENCE_DONE // 序列完成
} et_gpio_output_event;

struct gpio_output;

typedef void (*GpioOutputCallback)(struct gpio_output*, et_gpio_output_event, void*);

#pragma pack(1)
typedef struct gpio_output {
    et_gpio_mode mode;          // 工作模式
    et_gpio_output_state state;  // 当前状态
    uint64_t tick;              // 当前计时
    const uint16_t* seq_array;  // 时序数组（毫秒）
    uint32_t seq_length;        // 数组长度
    uint32_t seq_index;         // 当前索引
    uint32_t seq_count;         // 剩余重复次数
    GpioOutputCallback callback; // 回调函数
    void* callback_arg;         // 回调参数
    void* hw_ptr;               // GPIO 端口（使用通用指针隔离硬件）
    uint32_t pin;               // GPIO 引脚
    struct gpio_output* next;   // 链表指针
    st_soft_time timeout_timer; // 定时器

    void (*initialize)(struct gpio_output*, const uint16_t*, uint32_t, uint32_t, void (*)(struct gpio_output*, et_gpio_output_event, void*), void*);
    void (*configure)(struct gpio_output*);
    void (*set_high)(struct gpio_output*);
    void (*set_low)(struct gpio_output*);
    void (*toggle)(struct gpio_output*);
    void (*run_sequence)(struct gpio_output*, et_gpio_output_state);
    void (*fsm)(struct gpio_output*);
    void (*set_mode)(struct gpio_output*, et_gpio_mode);
    void (*set_sequence)(struct gpio_output*, const uint16_t*, uint32_t, uint32_t);
} st_gpio_output, *st_gpio_output_ptr;
#pragma pack()

void lib_gpio_output_initialize(st_gpio_output_ptr ptr, const uint16_t* seq_array, uint32_t seq_length, uint32_t seq_count, void (*callback)(struct gpio_output*, et_gpio_output_event, void*), void* arg);
void lib_gpio_output_configure(st_gpio_output_ptr ptr);
void lib_gpio_output_run_sequence(st_gpio_output_ptr ptr, et_gpio_output_state state);
void lib_gpio_output_fsm(st_gpio_output_ptr ptr);
void lib_gpio_output_set_mode(st_gpio_output_ptr ptr, et_gpio_mode mode);
void lib_gpio_output_set_sequence(st_gpio_output_ptr ptr, const uint16_t* seq_array, uint32_t seq_length, uint32_t seq_count);

#ifdef __cplusplus
}
#endif

#endif /* LIB_GPIO_OUTPUT_H */
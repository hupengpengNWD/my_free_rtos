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

#ifndef LIB_GPIO_INPUT_H
#define LIB_GPIO_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "lib_soft_timer.h"

/* GPIO 输入触发电平 */
typedef enum {
    GPIO_TRIGGER_LOW = 0,   // 低电平触发
    GPIO_TRIGGER_HIGH,      // 高电平触发
    GPIO_TRIGGER_COUNT      // 触发电平总数
} et_gpio_trigger;

/* GPIO 输入状态 */
typedef enum {
    GPIO_STATE_IDLE = 0,    // 未触发
    GPIO_STATE_SHORT,       // 短按
    GPIO_STATE_LONG,        // 长按
    GPIO_STATE_AMOUNT       // 状态总数
} et_gpio_input_state;
  
/* GPIO 输入事件 */
typedef enum {
    GPIO_EVENT_SHORT_PRESS, // 短按触发
    GPIO_EVENT_LONG_PRESS,  // 长按触发
    GPIO_EVENT_RELEASE,     // 释放触发
    GPIO_EVENT_LEVEL_CHANGE // 电平变化
} et_gpio_input_event;

/* GPIO 事件掩码 */
typedef enum {
    GPIO_EVENT_MASK_NONE        = 0x00,
    GPIO_EVENT_MASK_SHORT_PRESS = 0x01,
    GPIO_EVENT_MASK_LONG_PRESS  = 0x02,
    GPIO_EVENT_MASK_RELEASE     = 0x04,
    GPIO_EVENT_MASK_LEVEL_CHANGE = 0x08,
    GPIO_EVENT_MASK_ALL         = 0x0F
} et_gpio_event_mask;

struct gpio_input;

typedef void (*GpioInputCallback)(struct gpio_input*, et_gpio_input_event, void*);
typedef void (*SetPollPeriodCallback)(struct gpio_input*, uint32_t);
typedef uint8_t (*GpioReadLevelCallback)(struct gpio_input*); //读取电平的函数指针

#pragma pack(1)
typedef struct gpio_input {
    et_gpio_input_state state;  // 当前状态
    uint8_t current_level;      // 当前电平
    uint32_t debounce_ms;       // 去抖时间（毫秒）
    uint32_t long_press_ms;     // 长按阈值（毫秒）
    uint32_t debounce_count;    // 去抖计数
    uint32_t press_time;        // 按下时间（毫秒）
    GpioInputCallback callback; // 回调函数
    void* callback_arg;         // 回调参数
    void* hw_ptr;               // 硬件指针
    uint32_t pin;               // GPIO引脚
    et_gpio_trigger trigger;    // 触发电平
    uint32_t poll_period_us;    // 轮询周期（微秒）
    uint32_t event_mask;        // 事件掩码
    struct gpio_input* next;    // 链表指针
    SetPollPeriodCallback set_poll_period_callback; // 轮询周期回调
    GpioReadLevelCallback read_level; //读取电平的回调函数

    void (*initialize)(struct gpio_input*, uint32_t, uint32_t, et_gpio_trigger, uint32_t, void (*)(struct gpio_input*, et_gpio_input_event, void*), void*);
    void (*configure)(struct gpio_input*);
    void (*fsm)(struct gpio_input*);
    void (*set_trigger)(struct gpio_input*, et_gpio_trigger);
    void (*set_debounce_ms)(struct gpio_input*, uint32_t);
    void (*set_poll_period)(struct gpio_input*, uint32_t);
    void (*set_event_mask)(struct gpio_input*, uint32_t);
    uint8_t (*get_level)(struct gpio_input*);
} st_gpio_input, *st_gpio_input_ptr;
#pragma pack()

void lib_gpio_input_fsm(st_gpio_input_ptr ptr);
void lib_gpio_input_set_trigger(st_gpio_input_ptr ptr, et_gpio_trigger trigger);
void lib_gpio_input_set_debounce_ms(st_gpio_input_ptr ptr, uint32_t debounce_ms);
void lib_gpio_input_set_poll_period(st_gpio_input_ptr ptr, uint32_t poll_period_us);
void lib_gpio_input_set_event_mask(st_gpio_input_ptr ptr, uint32_t event_mask);
uint8_t lib_gpio_input_get_level(st_gpio_input_ptr ptr);
void lib_gpio_input_initialize(st_gpio_input_ptr ptr, 
                              uint32_t debounce_ms, 
                              uint32_t long_press_ms, 
                              et_gpio_trigger trigger,
                              uint32_t event_mask,
                              void (*callback)(struct gpio_input*, et_gpio_input_event, void*), 
                              void* arg);
void lib_gpio_input_configure(st_gpio_input_ptr ptr);

#ifdef __cplusplus
}
#endif

#endif /* LIB_GPIO_INPUT_H */

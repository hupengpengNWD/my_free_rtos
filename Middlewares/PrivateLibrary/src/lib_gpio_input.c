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

#include "lib_gpio_input.h"

/**
  * @name     lib_gpio_input_fsm
  * @brief    GPIO 输入状态机处理
  * @param    ptr: GPIO 输入指针
  * @retval   None
  * @remark   处理去抖、短按、长按、释放事件，支持事件掩码
  */
void lib_gpio_input_fsm(st_gpio_input_ptr ptr) {
    if (ptr->hw_ptr == NULL){
         return;
    }
    // uint8_t read_level = HAL_GPIO_ReadPin(ptr->hw_ptr, ptr->pin);
    uint8_t read_level = ptr->read_level(ptr); 
    // 去抖处理：检查当前读取的电平是否与上次记录的电平不同
    if (read_level != ptr->current_level) {
        ptr->debounce_count++;// 如果电平发生变化，增加去抖计数
        /* 检查去抖计数是否达到去抖时间所需的轮询次数
           公式：(去抖时间(ms) * 1000 / 轮询周期(µs))，计算需要多少次轮询
           例如：debounce_ms = 6ms，poll_period_us = 1000µs，则需要 6 次轮询 */
        if (ptr->debounce_count >= (ptr->debounce_ms * 1000 / ptr->poll_period_us)) {
            // 去抖时间到达，确认电平变化有效，更新当前电平
            ptr->current_level = read_level;
            // 重置去抖计数器，准备下一次去抖
            ptr->debounce_count = 0;
            if (ptr->callback && (ptr->event_mask & GPIO_EVENT_MASK_LEVEL_CHANGE)) {
                // 调用回调函数，传递电平变化事件和用户参数
                ptr->callback(ptr, GPIO_EVENT_LEVEL_CHANGE, ptr->callback_arg);
            }
        }
    } else {
        // 如果电平未变化，重置去抖计数
        ptr->debounce_count = 0;
    }

    /* 判断当前电平是否为触发状态
       如果触发模式为低电平（GPIO_TRIGGER_LOW），则 current_level == 0 时触发
       如果触发模式为高电平（GPIO_TRIGGER_HIGH），则 current_level == 1 时触发 */
    bool is_triggered = (ptr->current_level == (ptr->trigger == GPIO_TRIGGER_LOW ? 0 : 1));
    switch (ptr->state) {
        case GPIO_STATE_IDLE:
            // 如果检测到触发状态，进入短按状态
            if (is_triggered) {
                ptr->press_time = 0;// 重置按下时间计数
                ptr->state = GPIO_STATE_SHORT;
            }
            break;

        case GPIO_STATE_SHORT:
            /* 累积按下时间，将轮询周期（微秒）转换为毫秒并加到 press_time
              例如：poll_period_us = 1000µs，则每次增加 1ms */
            ptr->press_time += (ptr->poll_period_us / 1000); // 按毫秒累积
            // 如果引脚不再触发（释放）
            if (!is_triggered) {
                ptr->state = GPIO_STATE_IDLE;
                if (ptr->callback) {
                    
                    if (ptr->event_mask & GPIO_EVENT_MASK_SHORT_PRESS) {
                        // 触发短按事件回调（按下时间 < 长按阈值）
                        ptr->callback(ptr, GPIO_EVENT_SHORT_PRESS, ptr->callback_arg);
                    }
                    if (ptr->event_mask & GPIO_EVENT_MASK_RELEASE) {
                        // 触发释放事件回调
                        ptr->callback(ptr, GPIO_EVENT_RELEASE, ptr->callback_arg);
                    }
                }
            } else if (ptr->press_time >= ptr->long_press_ms) {
                /* 如果按下时间达到或超过长按阈值（例如 500ms）
                   切换到长按状态 */
                ptr->state = GPIO_STATE_LONG;
                if (ptr->callback && (ptr->event_mask & GPIO_EVENT_MASK_LONG_PRESS)) {
                    // 触发长按事件回调
                    ptr->callback(ptr, GPIO_EVENT_LONG_PRESS, ptr->callback_arg);
                }
            }
            break;

        case GPIO_STATE_LONG:// 长按状态（触发时间已超过长按阈值）
            // 如果引脚不再触发（释放）
            if (!is_triggered) {
                ptr->state = GPIO_STATE_IDLE;
                if (ptr->callback && (ptr->event_mask & GPIO_EVENT_MASK_RELEASE)) {
                    // 触发释放事件回调
                    ptr->callback(ptr, GPIO_EVENT_RELEASE, ptr->callback_arg);
                }
            }
            break;

        default:
            // 未知状态，强制切换到空闲状态，确保状态机健壮性
            ptr->state = GPIO_STATE_IDLE;
            break;
    }
}

/**
  * @name     lib_gpio_input_set_trigger
  * @brief    动态设置触发电平
  * @param    ptr: GPIO 输入指针
  * @param    trigger: 触发电平（低/高）
  * @retval   None
  * @remark   更新触发电平并重置电平
  */
void lib_gpio_input_set_trigger(st_gpio_input_ptr ptr, et_gpio_trigger trigger) {
    ptr->trigger = trigger;
    if (ptr->hw_ptr != NULL) {
        // ptr->current_level = HAL_GPIO_ReadPin(ptr->hw_ptr, ptr->pin);
        ptr->current_level = ptr->read_level(ptr);
    }
}

/**
  * @name     lib_gpio_input_set_debounce_ms
  * @brief    动态设置去抖时间
  * @param    ptr: GPIO 输入指针
  * @param    debounce_ms: 去抖时间（毫秒）
  * @retval   None
  * @remark   重置去抖计数
  */
void lib_gpio_input_set_debounce_ms(st_gpio_input_ptr ptr, uint32_t debounce_ms) {
    ptr->debounce_ms = debounce_ms > 0 ? debounce_ms : 6;
    ptr->debounce_count = 0;
}

/**
  * @name     lib_gpio_input_set_poll_period
  * @brief    动态设置轮询周期
  * @param    ptr: GPIO 输入指针
  * @param    poll_period_us: 轮询周期（微秒）
  * @retval   None
  * @remark   更新全局共享定时器周期
  */
void lib_gpio_input_set_poll_period(st_gpio_input_ptr ptr, uint32_t poll_period_us) {
    if (ptr->set_poll_period_callback) {
        ptr->set_poll_period_callback(ptr, poll_period_us);
    }
}

/**
  * @name     lib_gpio_input_set_event_mask
  * @brief    动态设置事件掩码
  * @param    ptr: GPIO 输入指针
  * @param    event_mask: 事件掩码（GPIO_EVENT_MASK_*）
  * @retval   None
  * @remark   
  */
void lib_gpio_input_set_event_mask(st_gpio_input_ptr ptr, uint32_t event_mask) {
    ptr->event_mask = event_mask;
}

/**
  * @name     lib_gpio_input_get_level
  * @brief    获取当前 GPIO 电平
  * @param    ptr: GPIO 输入指针
  * @retval   uint8_t: 当前电平（0 或 1）
  * @remark   
  */
uint8_t lib_gpio_input_get_level(st_gpio_input_ptr ptr) {
    if (ptr->hw_ptr == NULL){
        return 0;
    } 
    // return HAL_GPIO_ReadPin(ptr->hw_ptr, ptr->pin);
    return ptr->read_level(ptr); // 修改：使用函数指针读取电平
}

/**
  * @name     lib_gpio_input_initialize
  * @brief    初始化 GPIO 输入结构体
  * @param    ptr: GPIO 输入指针
  * @param    debounce_ms: 去抖时间（毫秒）
  * @param    long_press_ms: 长按阈值（毫秒）
  * @param    trigger: 触发电平（低/高）
  * @param    event_mask: 事件掩码（GPIO_EVENT_MASK_*）
  * @param    callback: 事件回调函数
  * @param    arg: 回调参数
  * @retval   None
  * @remark   
  */
void lib_gpio_input_initialize(st_gpio_input_ptr ptr, 
                              uint32_t debounce_ms, 
                              uint32_t long_press_ms, 
                              et_gpio_trigger trigger,
                              uint32_t event_mask,
                              void (*callback)(struct gpio_input*, et_gpio_input_event, void*), 
                              void* arg) {
    ptr->debounce_ms = debounce_ms > 0 ? debounce_ms : 6;
    ptr->long_press_ms = long_press_ms > 0 ? long_press_ms : 200;
    ptr->trigger = trigger;
    ptr->event_mask = event_mask > 0 ? event_mask : GPIO_EVENT_MASK_ALL;
    ptr->state = GPIO_STATE_IDLE;
    // ptr->current_level = ptr->hw_ptr ? HAL_GPIO_ReadPin(ptr->hw_ptr, ptr->pin) : 0;
    ptr->current_level = (ptr->hw_ptr && ptr->read_level) ? ptr->read_level(ptr) : 0;
    ptr->debounce_count = 0;
    ptr->press_time = 0;
    ptr->callback = callback;
    ptr->callback_arg = arg;
    ptr->poll_period_us = 1000;
}

/**
  * @name     lib_gpio_input_configure
  * @brief    配置 GPIO 输入函数指针
  * @param    ptr: GPIO 输入指针
  * @retval   None
  * @remark   
  */
void lib_gpio_input_configure(st_gpio_input_ptr ptr) {
    ptr->initialize = lib_gpio_input_initialize;
    ptr->fsm = lib_gpio_input_fsm;
    ptr->set_trigger = lib_gpio_input_set_trigger;
    ptr->set_debounce_ms = lib_gpio_input_set_debounce_ms;
    ptr->set_poll_period = lib_gpio_input_set_poll_period;
    ptr->set_event_mask = lib_gpio_input_set_event_mask;
    ptr->get_level = lib_gpio_input_get_level;
}

/*---End of File----------------------------------------------------*/

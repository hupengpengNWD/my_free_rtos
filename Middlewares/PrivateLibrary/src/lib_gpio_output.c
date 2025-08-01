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

#include "lib_gpio_output.h"


// 定时器回调
void lib_gpio_output_polling(void* arg) {
    st_gpio_output_ptr ptr = (st_gpio_output_ptr)arg;
    ptr->fsm(ptr);
}

/**
  * @name     lib_gpio_output_run_sequence
  * @brief    执行序列中的指定状态
  * @param    ptr: GPIO 输出指针
  * @param    state: 目标状态（高/低）
  * @retval   None
  * @remark   
  */
void lib_gpio_output_run_sequence(st_gpio_output_ptr ptr, et_gpio_output_state state) {
    if (ptr->state == state) {
        return;
    }

    switch (state) {
        case GPIO_STATE_LOW:
            ptr->set_low(ptr);
            break;
        case GPIO_STATE_HIGH:
            ptr->set_high(ptr);
            break;
        default:
            break;
    }
}

/**
  * @name     lib_gpio_output_fsm
  * @brief    时序序列状态机
  * @param    ptr: GPIO 输出指针
  * @retval   None
  * @remark   
  */
void lib_gpio_output_fsm(st_gpio_output_ptr ptr) {
    if (ptr->mode != GPIO_MODE_SEQUENCE || 
        ptr->seq_count == 0 || 
        ptr->seq_length == 0 || 
        ptr->seq_array == NULL) {
        ptr->timeout_timer.stop(&ptr->timeout_timer);
        return;
    }

    if (ptr->tick >= ptr->seq_array[ptr->seq_index]) {
        ptr->tick = 0;
        ptr->run_sequence(ptr, (ptr->seq_index % 2) ? GPIO_STATE_LOW : GPIO_STATE_HIGH);
        ptr->seq_index++;
        if (ptr->seq_index >= ptr->seq_length) {
            ptr->seq_index = 0;
            ptr->seq_count--;
            if (ptr->seq_count == 0) {
                ptr->timeout_timer.stop(&ptr->timeout_timer);
                if (ptr->callback) {
                    ptr->callback(ptr, GPIO_EVENT_SEQUENCE_DONE, ptr->callback_arg);
                }
            }
        }
    } else {
        ptr->tick++;
    }
}

/**
  * @name     lib_gpio_output_set_mode
  * @brief    动态设置工作模式
  * @param    ptr: GPIO 输出指针
  * @param    mode: 工作模式（手动/序列）
  * @retval   None
  * @remark   
  */
void lib_gpio_output_set_mode(st_gpio_output_ptr ptr, et_gpio_mode mode) {
    ptr->mode = mode;
    if (mode == GPIO_MODE_MANUAL) {
        ptr->timeout_timer.stop(&ptr->timeout_timer);
        ptr->seq_index = 0;
        ptr->seq_count = 0;
        ptr->tick = 0;
    } else if (mode == GPIO_MODE_SEQUENCE && 
               ptr->seq_array && 
               ptr->seq_length > 0 && 
               ptr->seq_count > 0) {
        ptr->timeout_timer.start(&ptr->timeout_timer);
    }
}

/**
  * @name     lib_gpio_output_set_sequence
  * @brief    动态设置时序序列
  * @param    ptr: GPIO 输出指针
  * @param    seq_array: 时序数组（毫秒）
  * @param    seq_length: 数组长度
  * @param    seq_count: 重复次数
  * @retval   None
  * @remark   
  */
void lib_gpio_output_set_sequence(st_gpio_output_ptr ptr, 
                                  const uint16_t* seq_array, 
                                  uint32_t seq_length, 
                                  uint32_t seq_count) {
    ptr->seq_index = 0;
    ptr->tick = 0;
    ptr->mode = GPIO_MODE_SEQUENCE;
    ptr->seq_array = seq_array;
    ptr->seq_length = seq_length;
    ptr->seq_count = seq_count;
    ptr->timeout_timer.setr_argument(&ptr->timeout_timer, PeriodicMode, 10); // 10ms
    ptr->timeout_timer.start(&ptr->timeout_timer);
}

/**
  * @name     lib_gpio_output_initialize
  * @brief    初始化 GPIO 输出结构体
  * @param    ptr: GPIO 输出指针
  * @param    seq_array: 时序数组（毫秒）
  * @param    seq_length: 数组长度
  * @param    seq_count: 重复次数
  * @param    callback: 事件回调函数
  * @param    arg: 回调参数
  * @retval   None
  * @remark   
  */
void lib_gpio_output_initialize(st_gpio_output_ptr ptr, 
                                const uint16_t* seq_array, 
                                uint32_t seq_length, 
                                uint32_t seq_count,
                                void (*callback)(struct gpio_output*, et_gpio_output_event, void*), 
                                void* arg) {
    ptr->seq_index = 0;
    ptr->tick = 0;
    ptr->mode = seq_array && seq_length > 0 && seq_count > 0 ? GPIO_MODE_SEQUENCE : GPIO_MODE_MANUAL;
    ptr->seq_array = seq_array;
    ptr->seq_length = seq_length;
    ptr->seq_count = seq_count;
    ptr->state = GPIO_STATE_LOW;
    ptr->callback = callback;
    ptr->callback_arg = arg;
    ptr->next = NULL;
    
}

/**
  * @name     lib_gpio_output_configure
  * @brief    配置 GPIO 输出函数指针
  * @param    ptr: GPIO 输出指针
  * @retval   None
  * @remark   
  */
void lib_gpio_output_configure(st_gpio_output_ptr ptr) {
    ptr->initialize = lib_gpio_output_initialize;
    ptr->run_sequence = lib_gpio_output_run_sequence;
    ptr->fsm = lib_gpio_output_fsm;
    ptr->set_mode = lib_gpio_output_set_mode;
    ptr->set_sequence = lib_gpio_output_set_sequence;
}

/*---End of File----------------------------------------------------*/
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

#include "lib_timer_pwm.h"
#include <string.h>

// 包装函数前向声明
void lib_timer_pwm_configure_wrapper(void* pwm_ptr);
void lib_timer_pwm_set_duty_cycle_wrapper(void* pwm_ptr, uint32_t duty_cycle);
void lib_timer_pwm_set_frequency_wrapper(void* pwm_ptr, uint32_t frequency);
void lib_timer_pwm_set_polarity_wrapper(void* pwm_ptr, uint8_t polarity);
void lib_timer_pwm_set_dead_time_wrapper(void* pwm_ptr, uint32_t dead_time);
void lib_timer_pwm_enable_complementary_output_wrapper(void* pwm_ptr, bool enable);
void lib_timer_pwm_start_wrapper(void* pwm_ptr);
void lib_timer_pwm_stop_wrapper(void* pwm_ptr);

// 全局数据变量定义
uint32_t lib_timer_pwm_global_duty_cycle = 0;
uint32_t lib_timer_pwm_global_frequency = PWM_DEFAULT_FREQUENCY;

// 错误信息字符串
static const char* error_strings[] = {
    "No Error",                    // 无错误
    "Invalid Duty Cycle",          // 无效占空比
    "Invalid Frequency",           // 无效频率
    "Invalid Channel",             // 无效通道
    "Hardware Error",              // 硬件错误
    "Not Initialized"              // 未初始化
};

/**
  * @brief  配置PWM协议层默认参数
  * @param  ptr: PWM协议结构体指针
  * @retval None
  */
void lib_timer_pwm_configure(st_timer_pwm_protocol_ptr ptr) {
    if (ptr == NULL) return;
    
    // 设置默认配置
    ptr->state = PWM_PROTOCOL_STATE_STOPPED;
    ptr->last_error = PWM_PROTOCOL_ERROR_NONE;
    ptr->channel = 1;
    ptr->duty_cycle = 0;
    ptr->frequency = PWM_DEFAULT_FREQUENCY;
    ptr->polarity = PWM_PROTOCOL_POLARITY_HIGH;
    ptr->enable_complementary = false;
    ptr->dead_time = 0;
    ptr->target_duty_cycle = 0;
    ptr->target_frequency = PWM_DEFAULT_FREQUENCY;
    ptr->auto_restart = false;
    ptr->restart_delay_ms = 100;
    
    // 设置函数指针 - 使用包装函数来匹配类型
    ptr->configure = (PwmConfigureFunc)lib_timer_pwm_configure_wrapper;
    ptr->set_duty_cycle = (PwmSetDutyCycleFunc)lib_timer_pwm_set_duty_cycle_wrapper;
    ptr->set_frequency = (PwmSetFrequencyFunc)lib_timer_pwm_set_frequency_wrapper;
    ptr->set_polarity = (PwmSetPolarityFunc)lib_timer_pwm_set_polarity_wrapper;
    ptr->set_dead_time = (PwmSetDeadTimeFunc)lib_timer_pwm_set_dead_time_wrapper;
    ptr->enable_complementary_output = (PwmEnableComplementaryOutputFunc)lib_timer_pwm_enable_complementary_output_wrapper;
    ptr->start = (PwmStartFunc)lib_timer_pwm_start_wrapper;
    ptr->stop = (PwmStopFunc)lib_timer_pwm_stop_wrapper;
}

/**
  * @brief  初始化PWM协议层
  * @param  ptr: PWM协议结构体指针
  * @param  channel: PWM通道号
  * @param  frequency: PWM频率
  * @param  polarity: 输出极性
  * @param  cb: 协议回调函数
  * @param  arg: 回调参数
  * @retval None
  */
void lib_timer_pwm_initialize(st_timer_pwm_protocol_ptr ptr, uint8_t channel, uint32_t frequency,
                              PwmProtocolPolarity polarity, PwmProtocolCallback cb, void* arg) {
    if (ptr == NULL) return;
    
    // 验证参数
    channel = lib_timer_pwm_validate_channel(channel);
    frequency = lib_timer_pwm_validate_frequency(frequency);
    
    // 设置参数
    ptr->channel = channel;
    ptr->frequency = frequency;
    ptr->target_frequency = frequency;
    ptr->polarity = polarity;
    ptr->protocol_callback = cb;
    ptr->callback_arg = arg;
    ptr->state = PWM_PROTOCOL_STATE_STOPPED;
    ptr->last_error = PWM_PROTOCOL_ERROR_NONE;
    
    // 注册硬件回调
    if (ptr->register_update_callback != NULL) {
        ptr->register_update_callback(ptr->pwm_dma_ptr, NULL, ptr);
    }
    if (ptr->register_error_callback != NULL) {
        ptr->register_error_callback(ptr->pwm_dma_ptr, NULL, ptr);
    }
    
    // 配置硬件
    if (ptr->configure != NULL) {
        ptr->configure(ptr);
    }
}

/**
  * @brief  设置PWM占空比
  * @param  ptr: PWM协议结构体指针
  * @param  duty_cycle: 占空比 (0-100%)
  * @retval None
  */
void lib_timer_pwm_set_duty_cycle(st_timer_pwm_protocol_ptr ptr, uint32_t duty_cycle) {
    if (ptr == NULL) {
        lib_timer_pwm_error_handler(ptr, PWM_PROTOCOL_ERROR_NOT_INITIALIZED);
        return;
    }
    
    // 验证占空比
    duty_cycle = lib_timer_pwm_validate_duty_cycle(duty_cycle);
    if (duty_cycle == PWM_MAX_DUTY_CYCLE + 1) {
        lib_timer_pwm_error_handler(ptr, PWM_PROTOCOL_ERROR_INVALID_DUTY_CYCLE);
        return;
    }
    
    ptr->target_duty_cycle = duty_cycle;
    
    // 如果PWM正在运行，立即更新
    if (ptr->state == PWM_PROTOCOL_STATE_RUNNING && ptr->set_duty_cycle != NULL) {
        ptr->set_duty_cycle(ptr->pwm_dma_ptr, duty_cycle);
        ptr->duty_cycle = duty_cycle;
        
        // 更新全局变量
        lib_timer_pwm_global_duty_cycle = duty_cycle;
        
        // 调用回调
        if (ptr->protocol_callback != NULL) {
            ptr->protocol_callback(ptr, PWM_PROTOCOL_ERROR_NONE, duty_cycle, ptr->frequency, ptr->callback_arg);
        }
    }
}

/**
  * @brief  设置PWM频率
  * @param  ptr: PWM协议结构体指针
  * @param  frequency: 频率 (Hz)
  * @retval None
  */
void lib_timer_pwm_set_frequency(st_timer_pwm_protocol_ptr ptr, uint32_t frequency) {
    if (ptr == NULL) {
        lib_timer_pwm_error_handler(ptr, PWM_PROTOCOL_ERROR_NOT_INITIALIZED);
        return;
    }
    
    // 验证频率
    frequency = lib_timer_pwm_validate_frequency(frequency);
    if (frequency == PWM_MAX_FREQUENCY + 1) {
        lib_timer_pwm_error_handler(ptr, PWM_PROTOCOL_ERROR_INVALID_FREQUENCY);
        return;
    }
    
    ptr->target_frequency = frequency;
    
    // 如果PWM正在运行，立即更新
    if (ptr->state == PWM_PROTOCOL_STATE_RUNNING && ptr->set_frequency != NULL) {
        ptr->set_frequency(ptr->pwm_dma_ptr, frequency);
        ptr->frequency = frequency;
        
        // 更新全局变量
        lib_timer_pwm_global_frequency = frequency;
        
        // 调用回调
        if (ptr->protocol_callback != NULL) {
            ptr->protocol_callback(ptr, PWM_PROTOCOL_ERROR_NONE, ptr->duty_cycle, frequency, ptr->callback_arg);
        }
    }
}

/**
  * @brief  设置PWM输出极性
  * @param  ptr: PWM协议结构体指针
  * @param  polarity: 输出极性
  * @retval None
  */
void lib_timer_pwm_set_polarity(st_timer_pwm_protocol_ptr ptr, PwmProtocolPolarity polarity) {
    if (ptr == NULL) return;
    
    ptr->polarity = polarity;
    
    if (ptr->set_polarity != NULL) {
        ptr->set_polarity(ptr->pwm_dma_ptr, (uint8_t)polarity);
    }
}

/**
  * @brief  设置PWM死区时间
  * @param  ptr: PWM协议结构体指针
  * @param  dead_time: 死区时间 (µs)
  * @retval None
  */
void lib_timer_pwm_set_dead_time(st_timer_pwm_protocol_ptr ptr, uint32_t dead_time) {
    if (ptr == NULL) return;
    
    ptr->dead_time = dead_time;
    
    if (ptr->set_dead_time != NULL) {
        ptr->set_dead_time(ptr->pwm_dma_ptr, dead_time);
    }
}

/**
  * @brief  使能PWM互补输出
  * @param  ptr: PWM协议结构体指针
  * @param  enable: 是否使能
  * @retval None
  */
void lib_timer_pwm_enable_complementary_output(st_timer_pwm_protocol_ptr ptr, bool enable) {
    if (ptr == NULL) return;
    
    ptr->enable_complementary = enable;
    
    if (ptr->enable_complementary_output != NULL) {
        ptr->enable_complementary_output(ptr->pwm_dma_ptr, enable);
    }
}

/**
  * @brief  启动PWM输出
  * @param  ptr: PWM协议结构体指针
  * @retval None
  */
void lib_timer_pwm_start(st_timer_pwm_protocol_ptr ptr) {
    if (ptr == NULL) {
        lib_timer_pwm_error_handler(ptr, PWM_PROTOCOL_ERROR_NOT_INITIALIZED);
        return;
    }
    
    if (ptr->start != NULL) {
        ptr->start(ptr->pwm_dma_ptr);
        ptr->state = PWM_PROTOCOL_STATE_RUNNING;
        
        // 应用目标参数
        if (ptr->target_duty_cycle != ptr->duty_cycle) {
            lib_timer_pwm_set_duty_cycle(ptr, ptr->target_duty_cycle);
        }
        if (ptr->target_frequency != ptr->frequency) {
            lib_timer_pwm_set_frequency(ptr, ptr->target_frequency);
        }
    }
}

/**
  * @brief  停止PWM输出
  * @param  ptr: PWM协议结构体指针
  * @retval None
  */
void lib_timer_pwm_stop(st_timer_pwm_protocol_ptr ptr) {
    if (ptr == NULL) return;
    
    if (ptr->stop != NULL) {
        ptr->stop(ptr->pwm_dma_ptr);
        ptr->state = PWM_PROTOCOL_STATE_STOPPED;
    }
}

/**
  * @brief  处理PWM协议层逻辑
  * @param  ptr: PWM协议结构体指针
  * @retval None
  */
void lib_timer_pwm_process(st_timer_pwm_protocol_ptr ptr) {
    if (ptr == NULL) return;
    
    // 检查是否需要自动重启
    if (ptr->auto_restart && ptr->state == PWM_PROTOCOL_STATE_ERROR) {
        // 这里可以实现自动重启逻辑
        // 例如：延时后重新启动PWM
    }
    
    // 更新状态
    if (ptr->get_state != NULL) {
        uint8_t hw_state = ptr->get_state(ptr->pwm_dma_ptr);
        if (hw_state == 0) { // 假设0表示停止状态
            ptr->state = PWM_PROTOCOL_STATE_STOPPED;
        } else {
            ptr->state = PWM_PROTOCOL_STATE_RUNNING;
        }
    }
}

/**
  * @brief  更新PWM参数
  * @param  ptr: PWM协议结构体指针
  * @retval None
  */
void lib_timer_pwm_update(st_timer_pwm_protocol_ptr ptr) {
    if (ptr == NULL) return;
    
    // 同步硬件状态
    if (ptr->get_duty_cycle != NULL) {
        ptr->duty_cycle = ptr->get_duty_cycle(ptr->pwm_dma_ptr);
    }
    if (ptr->get_frequency != NULL) {
        ptr->frequency = ptr->get_frequency(ptr->pwm_dma_ptr);
    }
    
    // 更新全局变量
    lib_timer_pwm_global_duty_cycle = ptr->duty_cycle;
    lib_timer_pwm_global_frequency = ptr->frequency;
}

/**
  * @brief  错误处理函数
  * @param  ptr: PWM协议结构体指针
  * @param  error: 错误码
  * @retval None
  */
void lib_timer_pwm_error_handler(st_timer_pwm_protocol_ptr ptr, uint32_t error) {
    if (ptr == NULL) return;
    
    ptr->last_error = (PwmProtocolError)error;
    ptr->state = PWM_PROTOCOL_STATE_ERROR;
    
    // 调用错误回调
    if (ptr->protocol_callback != NULL) {
        ptr->protocol_callback(ptr, error, ptr->duty_cycle, ptr->frequency, ptr->callback_arg);
    }
}

/**
  * @brief  验证占空比
  * @param  duty_cycle: 占空比
  * @retval 验证后的占空比，如果无效返回PWM_MAX_DUTY_CYCLE + 1
  */
uint32_t lib_timer_pwm_validate_duty_cycle(uint32_t duty_cycle) {
    if (duty_cycle <= PWM_MAX_DUTY_CYCLE) {
        return duty_cycle;
    }
    return PWM_MAX_DUTY_CYCLE + 1;
}

/**
  * @brief  验证频率
  * @param  frequency: 频率
  * @retval 验证后的频率，如果无效返回PWM_MAX_FREQUENCY + 1
  */
uint32_t lib_timer_pwm_validate_frequency(uint32_t frequency) {
    if (frequency >= PWM_MIN_FREQUENCY && frequency <= PWM_MAX_FREQUENCY) {
        return frequency;
    }
    return PWM_MAX_FREQUENCY + 1;
}

/**
  * @brief  验证通道号
  * @param  channel: 通道号
  * @retval 验证后的通道号
  */
uint8_t lib_timer_pwm_validate_channel(uint8_t channel) {
    if (channel >= 1 && channel <= 4) {
        return channel;
    }
    return 1; // 默认返回通道1
}

/**
  * @brief  获取错误信息字符串
  * @param  error: 错误码
  * @retval 错误信息字符串
  */
const char* lib_timer_pwm_get_error_string(PwmProtocolError error) {
    if (error < sizeof(error_strings) / sizeof(error_strings[0])) {
        return error_strings[error];
    }
    return "Unknown Error";  // 未知错误
}

// 包装函数 - 用于匹配函数指针类型
void lib_timer_pwm_configure_wrapper(void* pwm_ptr) {
    lib_timer_pwm_configure((st_timer_pwm_protocol_ptr)pwm_ptr);
}

void lib_timer_pwm_set_duty_cycle_wrapper(void* pwm_ptr, uint32_t duty_cycle) {
    lib_timer_pwm_set_duty_cycle((st_timer_pwm_protocol_ptr)pwm_ptr, duty_cycle);
}

void lib_timer_pwm_set_frequency_wrapper(void* pwm_ptr, uint32_t frequency) {
    lib_timer_pwm_set_frequency((st_timer_pwm_protocol_ptr)pwm_ptr, frequency);
}

void lib_timer_pwm_set_polarity_wrapper(void* pwm_ptr, uint8_t polarity) {
    lib_timer_pwm_set_polarity((st_timer_pwm_protocol_ptr)pwm_ptr, (PwmProtocolPolarity)polarity);
}

void lib_timer_pwm_set_dead_time_wrapper(void* pwm_ptr, uint32_t dead_time) {
    lib_timer_pwm_set_dead_time((st_timer_pwm_protocol_ptr)pwm_ptr, dead_time);
}

void lib_timer_pwm_enable_complementary_output_wrapper(void* pwm_ptr, bool enable) {
    lib_timer_pwm_enable_complementary_output((st_timer_pwm_protocol_ptr)pwm_ptr, enable);
}

void lib_timer_pwm_start_wrapper(void* pwm_ptr) {
    lib_timer_pwm_start((st_timer_pwm_protocol_ptr)pwm_ptr);
}

void lib_timer_pwm_stop_wrapper(void* pwm_ptr) {
    lib_timer_pwm_stop((st_timer_pwm_protocol_ptr)pwm_ptr);
} 
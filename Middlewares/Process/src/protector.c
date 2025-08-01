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
  
#include "protector.h"

// 保护模块任务句柄
osThreadId_t protector_taskHandle;

// 保护模块任务属性配置
const osThreadAttr_t protector_task_attributes = {
  .name = "protector_task",                    // 任务名称
  .priority = (osPriority_t) osPriorityNormal5, // 任务优先级
  .stack_size = 256 * 4                        // 任务栈大小（1024字节）
};

// 保护模块日志实例
static st_uart_log protector_log_obj;

// 日志等级变量（供com模块访问）
uint8_t protector_log_level = LOG_LEVEL_NONE;

// 日志等级变化标志
static bool protector_log_level_changing = false;

/**
 * @brief Protector模块日志等级写回调函数
 * @param param_id 参数ID
 * @param value 新值指针
 * @param length 值长度
 * @param user_data 用户数据（未使用）
 */
void protector_log_level_write_callback(uint8_t param_id, 
                                        const void* value, 
                                        uint8_t length, 
                                        void* user_data) {
                                            
    if (value && length == sizeof(uint8_t)) {
        uint8_t new_level = *((uint8_t*)value);
        // 安全地更新日志等级
        protector_log_level_changing = true;
        osDelay(5);  // 短暂延迟确保稳定
        lib_uart_log_set_level(&protector_log_obj, (log_level_t)new_level);
        osDelay(5);  // 额外延迟
        protector_log_level_changing = false;
    }
}



/**
  * @name     protector_task_func_poll
  * @brief    保护模块任务函数
  * @param    argument: 任务参数（未使用）
  * @retval   None
  * @remark   当前为占位函数，用于处理消息队列和系统监控
  */
void protector_task_func_poll(void *argument) {
    /* 无限循环 */
    for(;;) {
        // 处理消息队列，占位
        osDelay(10); // 10ms延迟，降低CPU使用率
    }
}



// GPIO输出实例
st_gpio_output red_led_obj;    // 红色LED GPIO输出实例
st_gpio_output green_led_obj;  // 绿色LED GPIO输出实例

// LED时序配置数组（单位：10ms周期）
static uint16_t red_led_sequence[] = {100, 100}; // 红色LED：1000ms高电平，1000ms低电平
static uint16_t green_led_sequence[] = {50, 50};  // 绿色LED：500ms高电平，500ms低电平 

/**
  * @name     led_gpio_callback
  * @brief    LED GPIO输出事件回调函数
  * @param    ptr: GPIO输出实例指针
  * @param    event: GPIO输出事件类型
  * @param    arg: 回调函数参数（未使用）
  * @retval   None
  * @remark   处理LED状态变化和序列完成事件，通过指针地址区分不同的LED实例
  */
void led_gpio_callback(st_gpio_output_ptr ptr, et_gpio_output_event event, void* arg) {
    // 检查当前日志等级，避免在日志等级变化时频繁输出
    if (protector_log_level <= LOG_LEVEL_NONE || protector_log_level_changing) {
        return;
    }
    
    switch (event) {
        case GPIO_EVENT_STATE_CHANGE:
            // 通过指针地址来区分不同的LED实例
            if (ptr == &red_led_obj) {
                MODULE_LOG_INFO_FROM_ISR(&protector_log_obj, "Red LED State Changed: %d", ptr->state);
            } else if (ptr == &green_led_obj) {
                MODULE_LOG_INFO_FROM_ISR(&protector_log_obj, "Green LED State Changed: %d", ptr->state);
            } else {
                MODULE_LOG_INFO_FROM_ISR(&protector_log_obj, "Unknown LED State Changed: %d", ptr->state);
            }
            break;
        case GPIO_EVENT_SEQUENCE_DONE:
            if (ptr == &red_led_obj) {
                MODULE_LOG_INFO_FROM_ISR(&protector_log_obj, "Red LED Sequence Done");
            } else if (ptr == &green_led_obj) {
                MODULE_LOG_INFO_FROM_ISR(&protector_log_obj, "Green LED Sequence Done");
            } else {
                MODULE_LOG_INFO_FROM_ISR(&protector_log_obj, "Unknown LED Sequence Done");
            }
            break;
    }
}

/**
  * @name     protector_task_init
  * @brief    保护模块初始化函数
  * @param    None
  * @retval   None
  * @remark   初始化保护模块的日志系统、LED控制、定时器和任务
  */
void protector_task_init(void) {
    // 检查maintain模块是否已初始化
    if (!maintain_module_initialized()) {
        // 如果maintain模块未初始化，直接返回
        return;
    }
    
    // 初始化日志模块
    protector_log_obj.uart_dma_ptr = get_maintain_uart_instance();
    protector_log_obj.send = drv_uart_send_impl;
    lib_uart_log_init(&protector_log_obj, "PROTECTOR", protector_log_level);
    
    // 注册日志等级参数的回调函数
    com_param_manager_set_write_callback(0x08, protector_log_level_write_callback, NULL);
    
    MODULE_LOG_INFO(&protector_log_obj, "Protector module initializing...");
    
    // 创建红色LED GPIO硬件输出实例
    drv_gpio_output_create(&red_led_obj, GPIOE, GPIO_PIN_0);
    red_led_obj.initialize(&red_led_obj, red_led_sequence, 2, 1000, led_gpio_callback, NULL);

    // 配置红色LED定时器
    red_led_obj.timeout_timer.SoftTime_id = SoftTime0;
    drv_timer_countdown_create(&red_led_obj.timeout_timer);
    red_led_obj.timeout_timer.setr_argument(&red_led_obj.timeout_timer, PeriodicMode, 10); // 10ms周期
    red_led_obj.timeout_timer.setr_call_back(&red_led_obj.timeout_timer, lib_gpio_output_polling, &red_led_obj);
    red_led_obj.timeout_timer.start(&red_led_obj.timeout_timer);

    // 创建绿色LED GPIO硬件输出实例
    drv_gpio_output_create(&green_led_obj, GPIOE, GPIO_PIN_1);  
    green_led_obj.initialize(&green_led_obj, green_led_sequence, 2, 1000, led_gpio_callback, NULL);

    // 配置绿色LED定时器
    green_led_obj.timeout_timer.SoftTime_id = SoftTime1;
    drv_timer_countdown_create(&green_led_obj.timeout_timer);
    green_led_obj.timeout_timer.enable = drv_timer_countdown_enable;
    green_led_obj.timeout_timer.setr_argument(&green_led_obj.timeout_timer, PeriodicMode, 10); // 10ms周期
    green_led_obj.timeout_timer.setr_call_back(&green_led_obj.timeout_timer, lib_gpio_output_polling, &green_led_obj);
    green_led_obj.timeout_timer.start(&green_led_obj.timeout_timer);

    // 创建保护模块任务
    protector_taskHandle = osThreadNew(protector_task_func_poll, NULL, &protector_task_attributes);
    
    MODULE_LOG_INFO(&protector_log_obj, "Protector module initialized successfully");
}



/*---End of File----------------------------------------------------*/

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
  
#include "manual.h"
#include <stdint.h>

// manual 任务属性
osThreadId_t manual_taskHandle;
const osThreadAttr_t manual_task_attributes = {
  .name = "manual_task",
  .priority = (osPriority_t) osPriorityNormal5,
  .stack_size = 256 * 4
};

// 日志模块实例
static st_uart_log manual_log_obj;

// 日志等级变量（供com模块访问）
uint8_t manual_log_level = LOG_LEVEL_NONE;

// 日志等级变化标志
static bool manual_log_level_changing = false;

/**
 * @brief Manual模块日志等级写回调函数
 * @param param_id 参数ID
 * @param value 新值指针
 * @param length 值长度
 * @param user_data 用户数据（未使用）
 */
void manual_log_level_write_callback(uint8_t param_id, 
                                     const void* value, 
                                     uint8_t length, 
                                     void* user_data) {
                                        
    if (value && length == sizeof(uint8_t)) {
        uint8_t new_level = *((uint8_t*)value);
        // 安全地更新日志等级
        manual_log_level_changing = true;
        osDelay(5);  // 短暂延迟确保稳定
        lib_uart_log_set_level(&manual_log_obj, (log_level_t)new_level);
        osDelay(5);  // 额外延迟
        manual_log_level_changing = false;
    }
}



void button1_gpio_callback(st_gpio_input_ptr ptr, et_gpio_input_event event, void* arg) {
    // 检查当前日志等级，避免在日志等级变化时频繁输出
    if (manual_log_level <= LOG_LEVEL_NONE || manual_log_level_changing) {
        return;
    }
    
    switch (event) {
        case GPIO_EVENT_SHORT_PRESS:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Short Press on Pin %d", ptr->pin);
            break;
        case GPIO_EVENT_LONG_PRESS:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Long Press on Pin %d", ptr->pin);
            break;
        case GPIO_EVENT_RELEASE:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Released on Pin %d", ptr->pin);
            break;
        case GPIO_EVENT_LEVEL_CHANGE:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Level Changed to %d on Pin %d", ptr->current_level, ptr->pin);
            break;
    }
}

void button2_gpio_callback(st_gpio_input_ptr ptr, et_gpio_input_event event, void* arg) {
    // 检查当前日志等级，避免在日志等级变化时频繁输出
    if (manual_log_level <= LOG_LEVEL_NONE || manual_log_level_changing) {
        return;
    }
    
    switch (event) {
        case GPIO_EVENT_SHORT_PRESS:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Short Press on Pin %d", ptr->pin);
            break;
        case GPIO_EVENT_LONG_PRESS:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Long Press on Pin %d", ptr->pin);
            break;
        case GPIO_EVENT_RELEASE:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Released on Pin %d", ptr->pin);
            break;
        case GPIO_EVENT_LEVEL_CHANGE:
            MODULE_LOG_INFO_FROM_ISR(&manual_log_obj, "Level Changed to %d on Pin %d", ptr->current_level, ptr->pin);
            break;
    }
}

/**
  * @brief Function implementing the manual_task thread.
  * @param argument: Not used
  * @retval None
  */
void manual_task_func_poll(void *argument) {
    /* Infinite loop */
    for(;;) {
        // 处理消息队列，占位
        osDelay(10);
    }
}

// GPIO 输入实例
st_gpio_input button1_gpio;
st_gpio_input button2_gpio;

void manual_task_init(void) {
    uint32_t ms = 0;
    
    // 检查maintain模块是否已初始化
    if (!maintain_module_initialized()) {
        // 如果maintain模块未初始化，直接返回
        return;
    }
    
    // 初始化日志模块
    manual_log_obj.uart_dma_ptr = get_maintain_uart_instance();
    manual_log_obj.send = drv_uart_send_impl;
    lib_uart_log_init(&manual_log_obj, "MANUAL", manual_log_level);
    
    // 注册日志等级参数的回调函数
    com_param_manager_set_write_callback(0x07, manual_log_level_write_callback, NULL);
    
    MODULE_LOG_INFO(&manual_log_obj, "Manual module initializing...");
  
    drv_gpio_input_create(&button1_gpio, GPIOE, GPIO_PIN_12);
    button1_gpio.initialize(&button1_gpio, 
                            10, // 去抖时间（毫秒）
                            500, // 长按阈值（毫秒） 
                            GPIO_TRIGGER_LOW, // 触发电平
                            GPIO_EVENT_MASK_SHORT_PRESS | GPIO_EVENT_MASK_RELEASE, //事件掩码 
                            button1_gpio_callback, // 事件回调函数 
                            NULL); // 事件回调函数参数

    drv_gpio_input_create(&button2_gpio, GPIOE, GPIO_PIN_13);
    button2_gpio.initialize(&button2_gpio, 
                            10,  
                            500,  
                            GPIO_TRIGGER_LOW, 
                            GPIO_EVENT_MASK_SHORT_PRESS | GPIO_EVENT_MASK_RELEASE,
                            button2_gpio_callback, 
                            NULL);

    // 公共定时器
    gpio_input_shared_timer.SoftTime_id = SoftTime2;
    drv_timer_countdown_create(&gpio_input_shared_timer);
    ms = gpio_input_shared_poll_period_us/1000;
    if (ms== 0){
       ms = 1;
    }
    gpio_input_shared_timer.setr_call_back(&gpio_input_shared_timer, lib_gpio_input_polling, NULL);
    gpio_input_shared_timer.setr_argument(&gpio_input_shared_timer, PeriodicMode, ms);
    gpio_input_shared_timer.start(&gpio_input_shared_timer);

    //创建任务
    manual_taskHandle = osThreadNew(manual_task_func_poll, NULL, &manual_task_attributes);
    
    MODULE_LOG_INFO(&manual_log_obj, "Manual module initialized successfully");
}

/*---End of File----------------------------------------------------*/

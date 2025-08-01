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

#include "motor.h"
#include "drv_timer_pwm.h"
#include "lib_timer_pwm.h"
#include "lib_soft_timer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "stm32g4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 电机控制任务属性
osThreadId_t motor_taskHandle;
const osThreadAttr_t motor_task_attributes = {
    .name = "motor_task",
    .priority = (osPriority_t) osPriorityNormal4,
    .stack_size = 256 * 4
};

// 电机状态监控任务属性
osThreadId_t motor_monitor_taskHandle;
const osThreadAttr_t motor_monitor_task_attributes = {
    .name = "motor_monitor_task",
    .priority = (osPriority_t) osPriorityNormal3,
    .stack_size = 256 * 2
};

// 全局变量
static st_timer_pwm pwm_hw_instances[4];           // 硬件驱动层实例
static st_timer_pwm_protocol pwm_protocol_instances[4];     // 协议层实例
static QueueHandle_t motor_command_queue;          // 电机命令队列
static SemaphoreHandle_t motor_status_sem;         // 电机状态信号量
static st_soft_time motor_status_timer;            // 电机状态检查定时器

// 电机状态数组
static MotorStatus motor_status[4] = {0};
static MotorConfig motor_config[4] = {0};

// 软启动/软停止状态
typedef struct {
    bool active;
    uint32_t start_time;
    uint32_t duration;
    int32_t start_speed;
    int32_t target_speed;
    uint8_t motor_id;
} SoftControlState;

static SoftControlState soft_control[4] = {0};

// 错误码定义
typedef enum {
    MOTOR_ERROR_NONE = 0,
    MOTOR_ERROR_INVALID_MOTOR_ID,
    MOTOR_ERROR_INVALID_SPEED,
    MOTOR_ERROR_INVALID_FREQUENCY,
    MOTOR_ERROR_HARDWARE_ERROR,
    MOTOR_ERROR_OVERLOAD,
    MOTOR_ERROR_OVERTEMPERATURE,
    MOTOR_ERROR_COMMUNICATION_ERROR
} MotorError;

// 错误信息字符串
static const char* error_strings[] = {
    "无错误",
    "无效电机ID",
    "无效速度",
    "无效频率",
    "硬件错误",
    "过载",
    "过温",
    "通信错误"
};

// PWM协议回调函数
static void motor_pwm_protocol_callback(void* protocol, int error, uint32_t duty_cycle, 
                                       uint32_t frequency, void* arg) {
    uint8_t motor_id = *(uint8_t*)arg;
    
    if (error == PWM_PROTOCOL_ERROR_NONE) {
        motor_status[motor_id].duty_cycle = duty_cycle;
        motor_status[motor_id].frequency = frequency;
        motor_status[motor_id].error_count = 0;
    } else {
        motor_status[motor_id].error_count++;
        printf("电机%d PWM错误: %s\n", motor_id, lib_timer_pwm_get_error_string((PwmProtocolError)error));
    }
}

// PWM硬件更新回调函数
static void motor_pwm_hw_update_callback(void* pwm_ptr, void* arg) {
    uint8_t motor_id = *(uint8_t*)arg;
    motor_status[motor_id].running = true;
    
    // 更新运行时间和转数
    motor_status[motor_id].runtime_hours += 1; // 简化计算，实际应该基于时间
    motor_status[motor_id].total_revolutions += (motor_status[motor_id].frequency / 60); // 简化计算
}

// PWM硬件错误回调函数
static void motor_pwm_hw_error_callback(void* pwm_ptr, uint32_t error, void* arg) {
    uint8_t motor_id = *(uint8_t*)arg;
    motor_status[motor_id].error_count++;
    printf("电机%d硬件错误: %lu\n", motor_id, (unsigned long)error);
}

// 电机状态检查回调函数
static void motor_status_check_callback(void* arg) {
    xSemaphoreGive(motor_status_sem);
}

/**
 * @brief 初始化电机硬件驱动层
 * @return bool 初始化是否成功
 */
static bool motor_hw_init_internal(void) {
    // 初始化4个PWM通道（硬件已在main.c中初始化）
    for (int i = 0; i < 4; i++) {
        // 电机0,1使用TIM1，电机2,3使用TIM3
        TIM_TypeDef* tim_instance = (i < 2) ? TIM1 : TIM3;
        
        et_pwmDmaError error = drv_timer_pwm_create(&pwm_hw_instances[i], tim_instance, i);
        if (error != PWM_DMA_ERROR_NONE) {
            printf("电机%d PWM硬件创建失败，错误: %d\n", i, error);
            return false;
        }
        
        // 注册回调函数
        uint8_t* motor_id_arg = malloc(sizeof(uint8_t));
        *motor_id_arg = i;
        pwm_hw_instances[i].register_update_callback(&pwm_hw_instances[i], 
                                                   motor_pwm_hw_update_callback, motor_id_arg);
        pwm_hw_instances[i].register_error_callback(&pwm_hw_instances[i], 
                                                   motor_pwm_hw_error_callback, motor_id_arg);
    }
    
    return true;
}

/**
 * @brief 初始化电机协议层
 * @return bool 初始化是否成功
 */
static bool motor_protocol_init(void) {
    // 初始化4个PWM协议层实例
    for (int i = 0; i < 4; i++) {
        // 配置协议层
        lib_timer_pwm_configure(&pwm_protocol_instances[i]);
        
        // 绑定硬件驱动层
        pwm_protocol_instances[i].pwm_dma_ptr = &pwm_hw_instances[i];
        pwm_protocol_instances[i].set_duty_cycle = drv_timer_pwm_set_duty_cycle_impl;
        pwm_protocol_instances[i].set_frequency = drv_timer_pwm_set_frequency_impl;
        pwm_protocol_instances[i].set_polarity = drv_timer_pwm_set_polarity_impl;
        pwm_protocol_instances[i].set_dead_time = drv_timer_pwm_set_dead_time_impl;
        pwm_protocol_instances[i].enable_complementary_output = drv_timer_pwm_enable_complementary_output_impl;
        pwm_protocol_instances[i].start = drv_timer_pwm_start_impl;
        pwm_protocol_instances[i].stop = drv_timer_pwm_stop_impl;
        pwm_protocol_instances[i].configure = drv_timer_pwm_configure_impl;
        pwm_protocol_instances[i].register_update_callback = drv_timer_pwm_register_update_callback_impl;
        pwm_protocol_instances[i].register_error_callback = drv_timer_pwm_register_error_callback_impl;
        pwm_protocol_instances[i].get_id = drv_timer_pwm_get_id_impl;
        pwm_protocol_instances[i].get_hw_ptr = drv_timer_pwm_get_hw_ptr_impl;
        pwm_protocol_instances[i].get_channel = drv_timer_pwm_get_channel_impl;
        pwm_protocol_instances[i].get_duty_cycle = drv_timer_pwm_get_duty_cycle_impl;
        pwm_protocol_instances[i].get_frequency = drv_timer_pwm_get_frequency_impl;
        pwm_protocol_instances[i].get_state = drv_timer_pwm_get_state_impl;
        
        // 初始化协议层
        uint8_t* motor_id_arg = malloc(sizeof(uint8_t));
        *motor_id_arg = i;
        lib_timer_pwm_initialize(&pwm_protocol_instances[i], i + 1, 1000, 
                                PWM_PROTOCOL_POLARITY_HIGH, motor_pwm_protocol_callback, motor_id_arg);
        
        // 初始化电机状态
        motor_status[i].motor_id = i;
        motor_status[i].direction = MOTOR_DIRECTION_STOP;
        motor_status[i].speed = 0;
        motor_status[i].runtime_hours = 0;
        motor_status[i].total_revolutions = 0;
        
        // 初始化电机配置
        motor_config[i].motor_id = i;
        motor_config[i].max_speed = 3000;
        motor_config[i].min_speed = 0;
        motor_config[i].max_frequency = 10000;
        motor_config[i].min_frequency = 100;
        motor_config[i].acceleration_time = 1000;
        motor_config[i].deceleration_time = 1000;
        motor_config[i].enable_soft_start = true;
        motor_config[i].enable_brake = true;
    }
    
    return true;
}

/**
 * @brief 处理电机命令
 * @param cmd 电机命令结构体
 */
static void motor_process_command(MotorCommand* cmd) {
    if (cmd->motor_id >= 4) {
        printf("无效的电机ID: %d\n", cmd->motor_id);
        return;
    }
    
    st_timer_pwm_protocol* pwm = &pwm_protocol_instances[cmd->motor_id];
    
    switch (cmd->cmd_type) {
        case MOTOR_CMD_SET_DUTY_CYCLE:
            lib_timer_pwm_set_duty_cycle(pwm, cmd->duty_cycle);
            printf("设置电机%d占空比为%lu%%\n", cmd->motor_id, (unsigned long)cmd->duty_cycle);
            break;
            
        case MOTOR_CMD_SET_FREQUENCY:
            lib_timer_pwm_set_frequency(pwm, cmd->frequency);
            printf("设置电机%d频率为%luHz\n", cmd->motor_id, (unsigned long)cmd->frequency);
            break;
            
        case MOTOR_CMD_SET_POLARITY:
            lib_timer_pwm_set_polarity(pwm, (PwmProtocolPolarity)cmd->polarity);
            printf("设置电机%d极性为%d\n", cmd->motor_id, cmd->polarity);
            break;
            
        case MOTOR_CMD_START:
            lib_timer_pwm_start(pwm);
            motor_status[cmd->motor_id].running = true;
            printf("启动电机%d\n", cmd->motor_id);
            break;
            
        case MOTOR_CMD_STOP:
            lib_timer_pwm_stop(pwm);
            motor_status[cmd->motor_id].running = false;
            motor_status[cmd->motor_id].speed = 0;
            motor_status[cmd->motor_id].direction = MOTOR_DIRECTION_STOP;
            printf("停止电机%d\n", cmd->motor_id);
            break;
            
        case MOTOR_CMD_SET_DEAD_TIME:
            lib_timer_pwm_set_dead_time(pwm, cmd->duty_cycle);
            printf("设置电机%d死区时间为%luµs\n", cmd->motor_id, (unsigned long)cmd->duty_cycle);
            break;
            
        case MOTOR_CMD_ENABLE_COMPLEMENTARY:
            lib_timer_pwm_enable_complementary_output(pwm, cmd->enable);
            printf("%s电机%d互补输出\n", cmd->enable ? "使能" : "禁用", cmd->motor_id);
            break;
            
        case MOTOR_CMD_SET_SPEED:
            motor_set_speed(cmd->motor_id, cmd->speed);
            printf("设置电机%d速度为%ld\n", cmd->motor_id, (long)cmd->speed);
            break;
            
        case MOTOR_CMD_SET_DIRECTION:
            motor_set_direction(cmd->motor_id, cmd->direction);
            printf("设置电机%d方向为%d\n", cmd->motor_id, cmd->direction);
            break;
            
        default:
            printf("未知的电机命令类型: %d\n", cmd->cmd_type);
            break;
    }
}

/**
 * @brief 电机控制任务函数
 * @param argument 任务参数
 */
void motor_task_func(void *argument) {
    MotorCommand cmd;
    
    printf("电机控制任务启动\n");
    
    // 启动所有电机
    for (int i = 0; i < 4; i++) {
        lib_timer_pwm_start(&pwm_protocol_instances[i]);
        motor_status[i].running = true;
    }
    
    while (1) {
        // 等待电机命令
        if (xQueueReceive(motor_command_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            motor_process_command(&cmd);
        }
        
        // 处理协议层逻辑
        for (int i = 0; i < 4; i++) {
            lib_timer_pwm_process(&pwm_protocol_instances[i]);
        }
        
        // 处理软启动/软停止
        for (int i = 0; i < 4; i++) {
            if (soft_control[i].active) {
                uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                uint32_t elapsed = current_time - soft_control[i].start_time;
                
                if (elapsed >= soft_control[i].duration) {
                    // 软控制完成
                    soft_control[i].active = false;
                    motor_status[i].speed = soft_control[i].target_speed;
                    printf("电机%d软控制完成，目标速度: %ld\n", i, (long)soft_control[i].target_speed);
                } else {
                    // 计算当前速度
                    float progress = (float)elapsed / soft_control[i].duration;
                    int32_t current_speed = soft_control[i].start_speed + 
                                          (int32_t)((soft_control[i].target_speed - soft_control[i].start_speed) * progress);
                    
                    // 更新PWM占空比
                    uint32_t duty_cycle = abs(current_speed) * 100 / motor_config[i].max_speed;
                    if (duty_cycle > 100) duty_cycle = 100;
                    lib_timer_pwm_set_duty_cycle(&pwm_protocol_instances[i], duty_cycle);
                }
            }
        }
        
        // 任务延时
        osDelay(10);
    }
}

/**
 * @brief 电机状态监控任务函数
 * @param argument 任务参数
 */
void motor_monitor_task_func(void *argument) {
    printf("电机监控任务启动\n");
    
    while (1) {
        // 等待状态检查信号量
        if (xSemaphoreTake(motor_status_sem, portMAX_DELAY) == pdTRUE) {
            // 更新电机状态
            for (int i = 0; i < 4; i++) {
                lib_timer_pwm_update(&pwm_protocol_instances[i]);
                
                // 检查错误状态
                if (motor_status[i].error_count > 0) {
                    printf("电机%d错误计数: %lu\n", i, (unsigned long)motor_status[i].error_count);
                }
            }
            
            // 打印状态信息（每10次检查一次）
            static uint32_t status_counter = 0;
            status_counter++;
            if (status_counter % 10 == 0) {
                printf("电机状态: ");
                for (int i = 0; i < 4; i++) {
                    printf("M%d[%s,%ldRPM,%s] ", i, 
                           motor_status[i].running ? "ON" : "OFF",
                           motor_status[i].speed,
                           motor_status[i].direction == MOTOR_DIRECTION_FORWARD ? "FWD" : 
                           motor_status[i].direction == MOTOR_DIRECTION_BACKWARD ? "BWD" : "STOP");
                }
                printf("\n");
            }
        }
    }
}

/**
 * @brief 初始化电机任务
 */
void motor_task_init(void) {
    // 创建命令队列
    motor_command_queue = xQueueCreate(10, sizeof(MotorCommand));
    if (motor_command_queue == NULL) {
        printf("电机命令队列创建失败\n");
        return;
    }
    
    // 创建状态信号量
    motor_status_sem = xSemaphoreCreateBinary();
    if (motor_status_sem == NULL) {
        printf("电机状态信号量创建失败\n");
        return;
    }
    
    // 初始化硬件驱动层
    if (!motor_hw_init_internal()) {
        printf("电机硬件初始化失败\n");
        return;
    }
    
    // 初始化协议层
    if (!motor_protocol_init()) {
        printf("电机协议层初始化失败\n");
        return;
    }
    
    // 初始化状态检查定时器
    lib_softtimer_configure(&motor_status_timer);
    lib_softtimer_set_argument(&motor_status_timer, 0, 1000); // 1秒检查一次
    lib_softtimer_set_callback(&motor_status_timer, motor_status_check_callback, NULL);
    lib_softtimer_start(&motor_status_timer);
    
    // 创建电机控制任务
    motor_taskHandle = osThreadNew(motor_task_func, NULL, &motor_task_attributes);
    if (motor_taskHandle == NULL) {
        printf("电机控制任务创建失败\n");
        return;
    }
    
    // 创建电机监控任务
    motor_monitor_taskHandle = osThreadNew(motor_monitor_task_func, NULL, &motor_monitor_task_attributes);
    if (motor_monitor_taskHandle == NULL) {
        printf("电机监控任务创建失败\n");
        return;
    }
    
    printf("电机任务初始化完成\n");
}

/**
 * @brief 发送电机命令的接口函数
 * @param cmd 电机命令结构体
 * @return bool 是否成功发送
 */
bool motor_send_command(MotorCommand* cmd) {
    if (cmd == NULL || motor_command_queue == NULL) {
        return false;
    }
    
    return (xQueueSend(motor_command_queue, cmd, pdMS_TO_TICKS(100)) == pdTRUE);
}

/**
 * @brief 获取电机状态
 * @param motor_id 电机ID
 * @return MotorStatus* 电机状态指针
 */
MotorStatus* motor_get_status(uint8_t motor_id) {
    if (motor_id >= 4) {
        return NULL;
    }
    return &motor_status[motor_id];
}

/**
 * @brief 获取电机配置
 * @param motor_id 电机ID
 * @return MotorConfig* 电机配置指针
 */
MotorConfig* motor_get_config(uint8_t motor_id) {
    if (motor_id >= 4) {
        return NULL;
    }
    return &motor_config[motor_id];
}

/**
 * @brief 设置电机配置
 * @param motor_id 电机ID
 * @param config 电机配置
 * @return bool 是否成功
 */
bool motor_set_config(uint8_t motor_id, MotorConfig* config) {
    if (motor_id >= 4 || config == NULL) {
        return false;
    }
    
    motor_config[motor_id] = *config;
    motor_config[motor_id].motor_id = motor_id;
    return true;
}

/**
 * @brief 紧急停止所有电机
 */
void motor_emergency_stop(void) {
    MotorCommand cmd;
    cmd.cmd_type = MOTOR_CMD_STOP;
    
    for (int i = 0; i < 4; i++) {
        cmd.motor_id = i;
        motor_send_command(&cmd);
        soft_control[i].active = false; // 停止软控制
    }
    
    printf("紧急停止所有电机\n");
}

/**
 * @brief 电机软启动
 * @param motor_id 电机ID
 * @param target_speed 目标速度
 * @param duration_ms 持续时间（毫秒）
 */
void motor_soft_start(uint8_t motor_id, uint32_t target_speed, uint32_t duration_ms) {
    if (motor_id >= 4) return;
    
    soft_control[motor_id].active = true;
    soft_control[motor_id].start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    soft_control[motor_id].duration = duration_ms;
    soft_control[motor_id].start_speed = motor_status[motor_id].speed;
    soft_control[motor_id].target_speed = target_speed;
    soft_control[motor_id].motor_id = motor_id;
    
    printf("电机%d软启动:从%ld到%lu,持续时间%lums\n", 
           motor_id, soft_control[motor_id].start_speed, target_speed, duration_ms);
}

/**
 * @brief 电机软停止
 * @param motor_id 电机ID
 * @param duration_ms 持续时间（毫秒）
 */
void motor_soft_stop(uint8_t motor_id, uint32_t duration_ms) {
    motor_soft_start(motor_id, 0, duration_ms);
}

// 电机控制接口函数实现
bool motor_set_speed(uint8_t motor_id, int32_t speed) {
    if (motor_id >= 4) return false;
    
    MotorConfig* config = &motor_config[motor_id];
    if (abs(speed) > config->max_speed) {
        speed = (speed > 0) ? config->max_speed : -config->max_speed;
    }
    
    motor_status[motor_id].speed = speed;
    
    // 计算占空比
    uint32_t duty_cycle = abs(speed) * 100 / config->max_speed;
    if (duty_cycle > 100) duty_cycle = 100;
    
    // 设置PWM占空比
    lib_timer_pwm_set_duty_cycle(&pwm_protocol_instances[motor_id], duty_cycle);
    
    return true;
}

bool motor_set_direction(uint8_t motor_id, uint8_t direction) {
    if (motor_id >= 4) return false;
    
    motor_status[motor_id].direction = direction;
    
    // 根据方向设置极性
    PwmProtocolPolarity polarity = (direction == MOTOR_DIRECTION_FORWARD) ? 
                                   PWM_PROTOCOL_POLARITY_HIGH : PWM_PROTOCOL_POLARITY_LOW;
    lib_timer_pwm_set_polarity(&pwm_protocol_instances[motor_id], polarity);
    
    return true;
}

bool motor_start(uint8_t motor_id) {
    if (motor_id >= 4) return false;
    
    MotorCommand cmd;
    cmd.cmd_type = MOTOR_CMD_START;
    cmd.motor_id = motor_id;
    return motor_send_command(&cmd);
}

bool motor_stop(uint8_t motor_id) {
    if (motor_id >= 4) return false;
    
    MotorCommand cmd;
    cmd.cmd_type = MOTOR_CMD_STOP;
    cmd.motor_id = motor_id;
    return motor_send_command(&cmd);
}

bool motor_brake(uint8_t motor_id) {
    if (motor_id >= 4) return false;
    
    // 快速停止
    motor_soft_stop(motor_id, 100);
    return true;
}

// 电机状态查询函数实现
bool motor_is_running(uint8_t motor_id) {
    if (motor_id >= 4) return false;
    return motor_status[motor_id].running;
}

int32_t motor_get_current_speed(uint8_t motor_id) {
    if (motor_id >= 4) return 0;
    return motor_status[motor_id].speed;
}

uint8_t motor_get_current_direction(uint8_t motor_id) {
    if (motor_id >= 4) return MOTOR_DIRECTION_STOP;
    return motor_status[motor_id].direction;
}

uint32_t motor_get_runtime_hours(uint8_t motor_id) {
    if (motor_id >= 4) return 0;
    return motor_status[motor_id].runtime_hours;
}

uint32_t motor_get_total_revolutions(uint8_t motor_id) {
    if (motor_id >= 4) return 0;
    return motor_status[motor_id].total_revolutions;
}

// 电机错误处理函数实现
uint32_t motor_get_error_count(uint8_t motor_id) {
    if (motor_id >= 4) return 0;
    return motor_status[motor_id].error_count;
}

void motor_clear_error(uint8_t motor_id) {
    if (motor_id >= 4) return;
    motor_status[motor_id].error_count = 0;
}

const char* motor_get_error_string(uint32_t error_code) {
    if (error_code < sizeof(error_strings) / sizeof(error_strings[0])) {
        return error_strings[error_code];
    }
    return "未知错误";
}

// 中断处理函数
 
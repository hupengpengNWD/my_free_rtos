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

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// 电机控制任务属性
extern osThreadId_t motor_taskHandle;
extern const osThreadAttr_t motor_task_attributes;

// 电机状态监控任务属性
extern osThreadId_t motor_monitor_taskHandle;
extern const osThreadAttr_t motor_monitor_task_attributes;

// 电机命令结构
typedef struct {
    uint8_t cmd_type;        // 命令类型
    uint8_t motor_id;        // 电机ID
    uint32_t duty_cycle;     // 占空比
    uint32_t frequency;      // 频率
    uint8_t polarity;        // 极性
    bool enable;             // 使能状态
    int32_t speed;           // 速度值
    uint8_t direction;       // 方向
} MotorCommand;

// 命令类型定义
#define MOTOR_CMD_SET_DUTY_CYCLE    0x01
#define MOTOR_CMD_SET_FREQUENCY     0x02
#define MOTOR_CMD_SET_POLARITY      0x03
#define MOTOR_CMD_START             0x04
#define MOTOR_CMD_STOP              0x05
#define MOTOR_CMD_SET_DEAD_TIME     0x06
#define MOTOR_CMD_ENABLE_COMPLEMENTARY 0x07
#define MOTOR_CMD_SET_SPEED         0x08
#define MOTOR_CMD_SET_DIRECTION     0x09

// 电机方向定义
#define MOTOR_DIRECTION_FORWARD     0x01
#define MOTOR_DIRECTION_BACKWARD    0x02
#define MOTOR_DIRECTION_STOP        0x00

// 电机状态结构
typedef struct {
    uint8_t motor_id;
    uint32_t duty_cycle;
    uint32_t frequency;
    uint8_t polarity;
    bool running;
    uint32_t error_count;
    int32_t speed;
    uint8_t direction;
    uint32_t runtime_hours;      // 运行时间（小时）
    uint32_t total_revolutions;  // 总转数
} MotorStatus;

// 电机配置结构
typedef struct {
    uint8_t motor_id;
    uint32_t max_speed;          // 最大速度
    uint32_t min_speed;          // 最小速度
    uint32_t max_frequency;      // 最大频率
    uint32_t min_frequency;      // 最小频率
    uint32_t acceleration_time;  // 加速时间（ms）
    uint32_t deceleration_time;  // 减速时间（ms）
    bool enable_soft_start;      // 是否启用软启动
    bool enable_brake;           // 是否启用制动
} MotorConfig;

// 任务句柄声明
extern osThreadId_t motor_taskHandle;
extern osThreadId_t motor_monitor_taskHandle;

// 函数声明
void motor_task_func(void *argument);
void motor_monitor_task_func(void *argument);
void motor_task_init(void);


bool motor_send_command(MotorCommand* cmd);
MotorStatus* motor_get_status(uint8_t motor_id);
MotorConfig* motor_get_config(uint8_t motor_id);
bool motor_set_config(uint8_t motor_id, MotorConfig* config);
void motor_emergency_stop(void);
void motor_soft_start(uint8_t motor_id, uint32_t target_speed, uint32_t duration_ms);
void motor_soft_stop(uint8_t motor_id, uint32_t duration_ms);

// 电机控制接口函数
bool motor_set_speed(uint8_t motor_id, int32_t speed);
bool motor_set_direction(uint8_t motor_id, uint8_t direction);
bool motor_start(uint8_t motor_id);
bool motor_stop(uint8_t motor_id);
bool motor_brake(uint8_t motor_id);

// 电机状态查询函数
bool motor_is_running(uint8_t motor_id);
int32_t motor_get_current_speed(uint8_t motor_id);
uint8_t motor_get_current_direction(uint8_t motor_id);
uint32_t motor_get_runtime_hours(uint8_t motor_id);
uint32_t motor_get_total_revolutions(uint8_t motor_id);

// 电机错误处理函数
uint32_t motor_get_error_count(uint8_t motor_id);
void motor_clear_error(uint8_t motor_id);
const char* motor_get_error_string(uint32_t error_code);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_H 

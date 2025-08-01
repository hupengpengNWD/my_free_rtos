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

#include "com.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OP_READ
#define OP_READ 0x01
#endif
#ifndef OP_WRITE
#define OP_WRITE 0x02
#endif

// 设备控制任务属性
osThreadId_t com_taskHandle;
const osThreadAttr_t com_task_attributes = {
    .name = "com_task",
    .priority = (osPriority_t) osPriorityNormal5,
    .stack_size = 256 * 8
};

// UART 命令结构
typedef struct {
    uint8_t param_id;    // 参数编号
    uint8_t param_type;  // 参数类型
    char param_value[MAX_PARAM_VALUE_LEN]; // 参数值
} DeviceCommand;

// 全局变量
static st_uart_dma uart_dma_obj;
static st_uart_protocol uart_protocol_obj;
static QueueHandle_t command_queue;
static st_soft_time status_check_timer;

// 参数管理器
static ParamManager param_manager;
#define MAX_PARAMS 16
static ParamInfo param_list[MAX_PARAMS];

// 外部声明的数据变量（在lib_uart_protocol.c中定义）
extern int lib_uart_protocol_data1;
extern float lib_uart_protocol_data2;

// 外部声明的日志等级变量（在各模块中定义）
extern uint8_t maintain_log_level;
extern uint8_t manual_log_level;
extern uint8_t protector_log_level;

/**
 * @brief 初始化参数管理器
 * @details 将所有可访问的参数注册到参数管理器中
 * 
 * @return void
 */
void com_param_manager_init(void) {
    param_manager.param_list = param_list;
    param_manager.max_params = MAX_PARAMS;
    param_manager.param_count = 0;
    
    // 注册参数 lib_uart_protocol_data1 (整型，可读可写)
    param_list[0].param_id = 0x01;
    param_list[0].param_type = PARAM_TYPE_INT;
    param_list[0].permission = PARAM_PERMISSION_READ_WRITE;
    param_list[0].max_length = sizeof(int);
    param_list[0].param_ptr = &lib_uart_protocol_data1;
    param_list[0].param_name = "data1";
    param_list[0].param_desc = "Integer data parameter 1";
    param_manager.param_count++;
    
    // 注册参数 lib_uart_protocol_data2 (浮点型，可读可写)
    param_list[1].param_id = 0x02;
    param_list[1].param_type = PARAM_TYPE_FLOAT;
    param_list[1].permission = PARAM_PERMISSION_READ_WRITE;
    param_list[1].max_length = sizeof(float);
    param_list[1].param_ptr = &lib_uart_protocol_data2;
    param_list[1].param_name = "data2";
    param_list[1].param_desc = "Float data parameter 2";
    param_manager.param_count++;
    
    // 添加只读参数 - 系统版本号
    static uint16_t system_version = 0x0102; // 版本1.02
    param_list[2].param_id = 0x03;
    param_list[2].param_type = PARAM_TYPE_UINT16;
    param_list[2].permission = PARAM_PERMISSION_READ_ONLY;
    param_list[2].max_length = sizeof(uint16_t);
    param_list[2].param_ptr = &system_version;
    param_list[2].param_name = "version";
    param_list[2].param_desc = "System version (read-only)";
    param_manager.param_count++;
    
    // 添加布尔型参数 - 系统状态
    static bool system_status = true;
    param_list[3].param_id = 0x04;
    param_list[3].param_type = PARAM_TYPE_BOOL;
    param_list[3].permission = PARAM_PERMISSION_READ_WRITE;
    param_list[3].max_length = sizeof(bool);
    param_list[3].param_ptr = &system_status;
    param_list[3].param_name = "status";
    param_list[3].param_desc = "System running status";
    param_manager.param_count++;
    
    // 添加字符串参数 - 设备名称
    static char device_name[16] = "STM32G474";
    param_list[4].param_id = 0x05;
    param_list[4].param_type = PARAM_TYPE_STRING;
    param_list[4].permission = PARAM_PERMISSION_READ_WRITE;
    param_list[4].max_length = sizeof(device_name);
    param_list[4].param_ptr = device_name;
    param_list[4].param_name = "name";
    param_list[4].param_desc = "Device name";
    param_manager.param_count++;
    
    // 添加各模块的日志等级参数
    // Maintain模块日志等级
    param_list[5].param_id = 0x06;
    param_list[5].param_type = PARAM_TYPE_LOG_LEVEL;
    param_list[5].permission = PARAM_PERMISSION_READ_WRITE;
    param_list[5].max_length = sizeof(uint8_t);
    param_list[5].param_ptr = &maintain_log_level;
    param_list[5].param_name = "maintain_log_level";
    param_list[5].param_desc = "Maintain module log level (0-4)";
    param_list[5].write_callback = NULL;  // 不能设为NULL，这条语句执行前已经进行绑定，后期不会再绑定
    param_list[5].callback_user_data = NULL;
    param_manager.param_count++;
    
    // Manual模块日志等级
    param_list[6].param_id = 0x07;
    param_list[6].param_type = PARAM_TYPE_LOG_LEVEL;
    param_list[6].permission = PARAM_PERMISSION_READ_WRITE;
    param_list[6].max_length = sizeof(uint8_t);
    param_list[6].param_ptr = &manual_log_level;
    param_list[6].param_name = "manual_log_level";
    param_list[6].param_desc = "Manual module log level (0-4)";
    param_list[6].write_callback = NULL;  // 暂时设为NULL，后续由manual模块设置
    param_list[6].callback_user_data = NULL;
    param_manager.param_count++;
    
    // Protector模块日志等级
    param_list[7].param_id = 0x08;
    param_list[7].param_type = PARAM_TYPE_LOG_LEVEL;
    param_list[7].permission = PARAM_PERMISSION_READ_WRITE;
    param_list[7].max_length = sizeof(uint8_t);
    param_list[7].param_ptr = &protector_log_level;
    param_list[7].param_name = "protector_log_level";
    param_list[7].param_desc = "Protector module log level (0-4)";
    param_list[7].write_callback = NULL;  // 暂时设为NULL，后续由protector模块设置
    param_list[7].callback_user_data = NULL;
    param_manager.param_count++;
    
    // 在此处继续添加更多参数...
}

/**
 * @brief 根据参数ID获取参数信息
 * @param param_id 参数ID
 * @return ParamInfo* 参数信息指针，如果未找到返回NULL
 */
ParamInfo* com_param_manager_get_param(uint8_t param_id) {
    for (uint8_t i = 0; i < param_manager.param_count; i++) {
        if (param_manager.param_list[i].param_id == param_id) {
            return &param_manager.param_list[i];
        }
    }
    return NULL;
}

/**
 * @brief 根据参数名称获取参数信息
 * @param param_name 参数名称
 * @return ParamInfo* 参数信息指针，如果未找到返回NULL
 */
ParamInfo* com_param_manager_get_param_by_name(const char* param_name) {
    if (!param_name) {
        return NULL;
    }
    
    for (uint8_t i = 0; i < param_manager.param_count; i++) {
        if (strcmp(param_manager.param_list[i].param_name, param_name) == 0) {
            return &param_manager.param_list[i];
        }
    }
    return NULL;
}

/**
 * @brief 读取参数值
 * @param param_id 参数ID
 * @param value 输出缓冲区
 * @param length 输出长度
 * @return bool 是否成功
 */
bool com_param_manager_read_param(uint8_t param_id, void* value, uint8_t* length) {
    ParamInfo* param = com_param_manager_get_param(param_id);
    if (!param || !value || !length) {
        return false;
    }
    
    // 检查读权限
    if ((param->permission != PARAM_PERMISSION_READ_ONLY) && 
        (param->permission != PARAM_PERMISSION_READ_WRITE)) {
        return false;
    }
    
    // 根据参数类型读取数据
    switch (param->param_type) {
        case PARAM_TYPE_INT:
            *((int*)value) = *((int*)param->param_ptr);
            *length = sizeof(int);
            break;
        case PARAM_TYPE_FLOAT:
            *((float*)value) = *((float*)param->param_ptr);
            *length = sizeof(float);
            break;
        case PARAM_TYPE_BOOL:
            *((bool*)value) = *((bool*)param->param_ptr);
            *length = sizeof(bool);
            break;
        case PARAM_TYPE_UINT8:
            *((uint8_t*)value) = *((uint8_t*)param->param_ptr);
            *length = sizeof(uint8_t);
            break;
        case PARAM_TYPE_UINT16:
            *((uint16_t*)value) = *((uint16_t*)param->param_ptr);
            *length = sizeof(uint16_t);
            break;
        case PARAM_TYPE_UINT32:
            *((uint32_t*)value) = *((uint32_t*)param->param_ptr);
            *length = sizeof(uint32_t);
            break;
        case PARAM_TYPE_STRING:
            strncpy((char*)value, (char*)param->param_ptr, param->max_length);
            *length = strlen((char*)param->param_ptr);
            break;
        case PARAM_TYPE_LOG_LEVEL:
            *((uint8_t*)value) = *((uint8_t*)param->param_ptr);
            *length = sizeof(uint8_t);
            break;
        default:
            return false;
    }
    
    return true;
}

/**
 * @brief 写入参数值
 * @param param_id 参数ID
 * @param value 输入值
 * @param length 输入长度
 * @return bool 是否成功
 */
bool com_param_manager_write_param(uint8_t param_id, const void* value, uint8_t length) {
    ParamInfo* param = com_param_manager_get_param(param_id);
    if (!param || !value) {
        return false;
    }
    
    // 检查写权限
    if ((param->permission != PARAM_PERMISSION_WRITE_ONLY) && 
        (param->permission != PARAM_PERMISSION_READ_WRITE)) {
        return false;
    }
    
    bool write_success = false;
    
    // 根据参数类型写入数据
    switch (param->param_type) {
        case PARAM_TYPE_INT:
            if (length == sizeof(int)) {
                *((int*)param->param_ptr) = *((int*)value);
                write_success = true;
            }
            break;
        case PARAM_TYPE_FLOAT:
            if (length == sizeof(float)) {
                *((float*)param->param_ptr) = *((float*)value);
                write_success = true;
            }
            break;
        case PARAM_TYPE_BOOL:
            if (length == sizeof(bool)) {
                *((bool*)param->param_ptr) = *((bool*)value);
                write_success = true;
            }
            break;
        case PARAM_TYPE_UINT8:
            if (length == sizeof(uint8_t)) {
                *((uint8_t*)param->param_ptr) = *((uint8_t*)value);
                write_success = true;
            }
            break;
        case PARAM_TYPE_UINT16:
            if (length == sizeof(uint16_t)) {
                *((uint16_t*)param->param_ptr) = *((uint16_t*)value);
                write_success = true;
            }
            break;
        case PARAM_TYPE_UINT32:
            if (length == sizeof(uint32_t)) {
                *((uint32_t*)param->param_ptr) = *((uint32_t*)value);
                write_success = true;
            }
            break;
        case PARAM_TYPE_STRING:
            if (length <= param->max_length) {
                strncpy((char*)param->param_ptr, (char*)value, length);
                ((char*)param->param_ptr)[length] = '\0';
                write_success = true;
            }
            break;
        case PARAM_TYPE_LOG_LEVEL:
            if (length == sizeof(uint8_t)) {
                uint8_t log_level = *((uint8_t*)value);
                // 验证日志等级范围 (0-4)
                if (log_level <= LOG_LEVEL_DEBUG) {
                    *((uint8_t*)param->param_ptr) = log_level;
                    write_success = true;
                }
            }
            break;
        default:
            break;
    }
    
    // 如果写入成功且有回调函数，则调用回调函数
    if (write_success && param->write_callback != NULL) {
        param->write_callback(param_id, value, length, param->callback_user_data);
    }
    
    return write_success;
}

/**
 * @brief 获取参数管理器中的参数总数
 * @return uint8_t 参数总数
 */
uint8_t com_param_manager_get_param_count(void) {
    return param_manager.param_count;
}

/**
 * @brief 获取参数管理器中的参数列表
 * @return ParamInfo* 参数列表指针
 */
ParamInfo* com_param_manager_get_param_list(void) {
    return param_manager.param_list;
}

/**
 * @brief 设置参数的写回调函数
 * @param param_id 参数ID
 * @param callback 回调函数指针
 * @param user_data 用户数据指针
 * @return bool 设置成功返回true，失败返回false
 */
bool com_param_manager_set_write_callback(uint8_t param_id, ParamWriteCallback callback, void* user_data) {
    ParamInfo* param = com_param_manager_get_param(param_id);
    if (!param) {
        return false;
    }
    
    param->write_callback = callback;
    param->callback_user_data = user_data;
    return true;
}

/**
 * @brief UART协议回调函数
 * @details 处理UART协议层的接收数据，解析命令并执行相应操作
 *          支持读写操作：OP_READ(0x01)用于读取参数值，OP_WRITE(0x02)用于设置参数值
 * 
 * @param ptr 指向UART协议实例的指针 (st_uart_protocol_ptr)
 * @param error 错误代码，UART_PROTOCOL_ERROR_NONE表示无错误
 * @param data 接收到的数据指针 (当前未使用)
 * @param len 数据长度 (当前未使用)
 * @param arg 用户自定义参数指针 (当前未使用)
 * 
 * @note 该函数在UART协议层接收到完整帧时被调用
 * @note 对于写操作(OP_WRITE)：解析参数ID、类型和值，将命令加入队列等待处理
 * @note 对于读操作(OP_READ)：根据参数ID返回对应的参数值，并发送响应帧
 * 
 * @return void
 */
static void com_uart_protocol_callback(void* ptr, 
                                       int error, 
                                       uint8_t* data, 
                                       uint32_t len, 
                                       void* arg) {
    st_uart_protocol_ptr protocol = (st_uart_protocol_ptr)ptr;

    // 有错误直接返回
    if (error != UART_PROTOCOL_ERROR_NONE) {
        return;
    }
    
    // 直接解析param_buffer和rx_buffer
    if (protocol->rx_buffer[2] == OP_WRITE) {
        DeviceCommand cmd;
        cmd.param_id = protocol->rx_buffer[4];
        cmd.param_type = protocol->rx_buffer[5];
        if (protocol->rx_buffer[6] < MAX_PARAM_VALUE_LEN) {
            strncpy(cmd.param_value, (char*)protocol->param_buffer, protocol->rx_buffer[6]);
            cmd.param_value[protocol->rx_buffer[6]] = '\0';
        } else {
            cmd.param_value[0] = '\0';
        }
        xQueueSend(command_queue, &cmd, portMAX_DELAY);

        // 发送写操作响应帧
        uint8_t response_data[32];
        uint8_t response_len = 0;
        
        // 验证参数ID和类型
        ParamInfo* param = com_param_manager_get_param(cmd.param_id);
        if (param && param->param_type == cmd.param_type) {
            // 参数有效，返回成功响应
            response_len = sprintf((char*)response_data, "OK");
        } else {
            // 参数无效，返回错误响应
            response_len = sprintf((char*)response_data, "ERROR");
        }
        
        // 发送写操作响应帧
        lib_uart_protocol_send_frame(&uart_protocol_obj, OP_WRITE, cmd.param_id, cmd.param_type, response_data, response_len);

    } else if (protocol->rx_buffer[2] == OP_READ) {
        // 处理读请求并发送响应
        uint8_t param_id = protocol->rx_buffer[4];
        uint8_t response_data[32] = {0};
        uint8_t response_len = 0;
        
        ParamInfo* param = com_param_manager_get_param(param_id);
        if (param) {
            // 参数存在，读取值
            void* value_ptr = response_data;
            uint8_t value_len = 0;
            
            if (com_param_manager_read_param(param_id, value_ptr, &value_len)) {
                // 根据参数类型格式化响应数据
                switch (param->param_type) {
                    case PARAM_TYPE_INT:
                        response_len = sprintf((char*)response_data, "%d", *((int*)value_ptr));
                        break;
                    case PARAM_TYPE_FLOAT:
                        response_len = sprintf((char*)response_data, "%.2f", *((float*)value_ptr));
                        break;
                    case PARAM_TYPE_BOOL:
                        response_len = sprintf((char*)response_data, "%s", *((bool*)value_ptr) ? "true" : "false");
                        break;
                    case PARAM_TYPE_UINT8:
                        response_len = sprintf((char*)response_data, "%u", *((uint8_t*)value_ptr));
                        break;
                    case PARAM_TYPE_UINT16:
                        response_len = sprintf((char*)response_data, "%u", *((uint16_t*)value_ptr));
                        break;
                    case PARAM_TYPE_UINT32:
                        response_len = sprintf((char*)response_data, "%lu", *((uint32_t*)value_ptr));
                        break;
                    case PARAM_TYPE_STRING:
                        strcpy((char*)response_data, (char*)value_ptr);
                        response_len = strlen((char*)response_data);
                        break;
                    default:
                        response_len = sprintf((char*)response_data, "UNKNOWN_TYPE");
                        break;
                }
            } else {
                response_len = sprintf((char*)response_data, "READ_ERROR");
            }
        } else {
            // 无效参数ID
            response_len = sprintf((char*)response_data, "INVALID_ID");
        }
        
        // 发送响应帧
        lib_uart_protocol_send_frame(&uart_protocol_obj, OP_READ, param_id, param ? param->param_type : PARAM_TYPE_INT, response_data, response_len);
    }
}

// 定时器回调：周期性检查设备状态
__attribute__((unused)) static void status_check_callback(void* arg) {
    // 可扩展：检查设备状态，触发事件或日志
}

/**
 * @brief 通信任务主函数
 * @details 通信任务的核心处理函数，负责处理UART协议通信和命令队列
 *          该函数运行在独立的FreeRTOS任务中，优先级为osPriorityNormal5
 * 
 * @param argument 任务参数指针 (当前未使用，为NULL)
 * 
 * @note 该函数为无限循环，包含以下主要功能：
 *       1. 调用 lib_uart_protocol_process 处理UART协议队列
 *       2. 从命令队列中接收并处理写入命令
 *       3. 根据参数类型和ID更新对应的全局变量
 *       4. 每10ms执行一次循环，降低CPU使用率
 * 
 * @return void
 */
void com_task_func(void *argument) {
    DeviceCommand cmd;
    for (;;) {
        // 处理 UART 协议队列
        lib_uart_protocol_process(&uart_protocol_obj);

        // 处理写入命令
        if (xQueueReceive(command_queue, &cmd, 0) == pdTRUE) {
            // 使用参数管理器写入参数
            ParamInfo* param = com_param_manager_get_param(cmd.param_id);
            if (param) {
                // 根据参数类型转换字符串值
                switch (param->param_type) {
                    case PARAM_TYPE_INT: {
                        int value = atoi(cmd.param_value);
                        com_param_manager_write_param(cmd.param_id, &value, sizeof(int));
                        break;
                    }
                    case PARAM_TYPE_FLOAT: {
                        float value = atof(cmd.param_value);
                        com_param_manager_write_param(cmd.param_id, &value, sizeof(float));
                        break;
                    }
                    case PARAM_TYPE_BOOL: {
                        bool value = (strcmp(cmd.param_value, "true") == 0 || 
                                    strcmp(cmd.param_value, "1") == 0);
                        com_param_manager_write_param(cmd.param_id, &value, sizeof(bool));
                        break;
                    }
                    case PARAM_TYPE_UINT8: {
                        uint8_t value = (uint8_t)atoi(cmd.param_value);
                        com_param_manager_write_param(cmd.param_id, &value, sizeof(uint8_t));
                        break;
                    }
                    case PARAM_TYPE_UINT16: {
                        uint16_t value = (uint16_t)atoi(cmd.param_value);
                        com_param_manager_write_param(cmd.param_id, &value, sizeof(uint16_t));
                        break;
                    }
                    case PARAM_TYPE_UINT32: {
                        uint32_t value = (uint32_t)atol(cmd.param_value);
                        com_param_manager_write_param(cmd.param_id, &value, sizeof(uint32_t));
                        break;
                    }
                    case PARAM_TYPE_STRING: {
                        com_param_manager_write_param(cmd.param_id, cmd.param_value, strlen(cmd.param_value));
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        osDelay(10); // 10ms 延迟，降低 CPU 使用率
    }
}

/**
 * @brief 初始化通信任务和UART协议
 * @details 完成通信系统的完整初始化，包括UART硬件驱动、协议层配置和任务创建
 *          使用适配器模式连接硬件驱动层和协议层，实现解耦设计
 * 
 * @note 初始化流程：
 *       1. 初始化参数管理器
 *       2. 创建UART DMA实例 (USART1, Uart0)
 *       3. 配置协议层结构体的硬件相关函数指针
 *       4. 配置协议层结构体的定时器相关函数指针
 *       5. 配置协议层结构体的协议接口函数指针
 *       6. 调用协议层配置和初始化函数
 *       7. 创建命令队列 (容量10个DeviceCommand)
 *       8. 创建通信任务 (com_task_func)
 * 
 * @note 硬件配置：
 *       - UART实例: USART1, Uart0
 *       - 波特率: UART_DMA_DEFAULT_BAUDRATE
 *       - 校验位: UART_PROTOCOL_PARITY_NONE
 *       - 回调函数: com_uart_protocol_callback
 * 
 * @note 任务配置：
 *       - 任务名称: "com_task"
 *       - 优先级: osPriorityNormal5
 *       - 栈大小: 256 * 8 bytes
 * 
 * @return void
 */
void com_task_init(void) {
    // 初始化参数管理器
    com_param_manager_init();
    
    // 初始化 UART DMA 实例
    drv_uart_dma_create(&uart_dma_obj, USART1, Uart0);

    // 直接赋值协议层结构体的所有硬件相关函数指针
    uart_protocol_obj.uart_dma_ptr = &uart_dma_obj;
    uart_protocol_obj.send = drv_uart_send_impl;
    uart_protocol_obj.receive = drv_uart_receive_impl;
    uart_protocol_obj.set_baudrate = drv_uart_set_baudrate_impl;
    uart_protocol_obj.set_parity = drv_uart_set_parity_impl;
    uart_protocol_obj.enable_interrupts = drv_uart_enable_interrupts_impl;
    uart_protocol_obj.get_id = drv_uart_get_id_impl;
    uart_protocol_obj.get_hw_ptr = drv_uart_get_hw_ptr_impl;
    uart_protocol_obj.get_rx_buffer = drv_uart_get_rx_buffer_impl;
    uart_protocol_obj.register_tx_complete_callback = drv_uart_register_tx_complete_callback_impl;
    uart_protocol_obj.register_rx_complete_callback = drv_uart_register_rx_complete_callback_impl;
    uart_protocol_obj.register_idle_callback = drv_uart_register_idle_callback_impl;
    uart_protocol_obj.register_error_callback = drv_uart_register_error_callback_impl;

    // 定时器相关赋值
    uart_protocol_obj.timeout_timer = &status_check_timer;
    uart_protocol_obj.timer_set_argument = (TimerSetArgumentFunc)status_check_timer.setr_argument;
    uart_protocol_obj.timer_start = (TimerStartFunc)status_check_timer.start;
    uart_protocol_obj.timer_stop = (TimerStopFunc)status_check_timer.stop;
    uart_protocol_obj.timer_set_callback = (TimerSetCallbackFunc)status_check_timer.setr_call_back;

    // 协议层接口函数指针赋值
    uart_protocol_obj.configure = lib_uart_protocol_configure;
    uart_protocol_obj.initialize = lib_uart_protocol_initialize;
    uart_protocol_obj.process = lib_uart_protocol_process;
    uart_protocol_obj.send_frame = lib_uart_protocol_send_frame;
    uart_protocol_obj.receive_frame = lib_uart_protocol_receive_frame;

    // 配置和初始化协议层
    uart_protocol_obj.configure(&uart_protocol_obj);
    uart_protocol_obj.initialize(&uart_protocol_obj,
                                 UART_DMA_DEFAULT_BAUDRATE,
                                 UART_PROTOCOL_PARITY_NONE,
                                 com_uart_protocol_callback,
                                 NULL);

    // 创建命令队列
    command_queue = xQueueCreate(10, sizeof(DeviceCommand));
    
    // 创建设备控制任务
    com_taskHandle = osThreadNew(com_task_func, NULL, &com_task_attributes);
}

/*---End of File----------------------------------------------------*/

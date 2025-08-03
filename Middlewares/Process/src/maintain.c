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

#include "maintain.h"
#include "com.h"
#include <stdint.h>

// maintain 任务属性
osThreadId_t maintain_taskHandle;
const osThreadAttr_t maintain_task_attributes = {
  .name = "maintain_task",
  .priority = (osPriority_t) osPriorityNormal4,
  .stack_size = 512 * 4
};

// 维护计数器
// static uint32_t maintain_counter = 0;

// 模块初始化标志
static bool maintain_init_flag = false;

// 维护协议实例
static st_debug_protocol uart_debug_obj;

// 维护命令队列
static QueueHandle_t command_queue = NULL;

// 日志模块实例
static st_uart_log uart_log_obj;

// 日志等级变量（供com模块访问）
uint8_t maintain_log_level = LOG_LEVEL_INFO;

/**
 * @brief Maintain模块日志等级写回调函数
 * @param param_id 参数ID
 * @param value 新值指针
 * @param length 值长度
 * @param user_data 用户数据（未使用）
 */
void maintain_log_level_write_callback(uint8_t param_id, 
                                       const void* value, 
                                       uint8_t length, 
                                       void* user_data) {
                                        
    if (value && length == sizeof(uint8_t)) {
        uint8_t new_level = *((uint8_t*)value);
        // 安全地更新日志等级
        lib_uart_log_set_level(&uart_log_obj, (log_level_t)new_level);
    }
}

// UART DMA 实例
static st_uart_dma uart_maintain_dma_obj;

/**
 * @brief 维护协议回调函数
 * @param protocol 协议实例指针
 * @param error 错误代码
 * @param data 接收到的数据
 * @param len 数据长度
 * @param arg 用户参数
 */
void maintain_protocol_callback(void* protocol, 
                               int error, 
                               uint8_t* data, 
                               uint32_t len, 
                               void* arg) {
    if (error != 0) {
        return;
    }
    if (data && len > 0 && command_queue != NULL) {
        // 将命令放入队列，在任务中处理
        MaintainCommand cmd;
        strncpy(cmd.cmd_line, (char*)data, DEBUG_MAX_CMD_LEN - 1);
        cmd.cmd_line[DEBUG_MAX_CMD_LEN - 1] = '\0';
        cmd.is_valid = true;
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (xQueueSendFromISR(command_queue, &cmd, &xHigherPriorityTaskWoken) == pdTRUE) {
            // 队列发送成功
        } else {
            // 队列发送失败，可能是队列满了
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief 处理维护命令
 * @param cmd_line 命令字符串
 */
void maintain_process_command(const char* cmd_line) {
    char cmd_copy[DEBUG_MAX_CMD_LEN];
    char* token;
    char* saveptr;
    
    if (!cmd_line || strlen(cmd_line) == 0) {
        return;
    }
    
    
    // 复制命令字符串
    strncpy(cmd_copy, cmd_line, DEBUG_MAX_CMD_LEN - 1);
    cmd_copy[DEBUG_MAX_CMD_LEN - 1] = '\0';
    
    // 解析命令
    token = strtok_r(cmd_copy, " \t", &saveptr);
    if (!token || strcmp(token, "maintain") != 0) {
        MODULE_LOG_WARN(&uart_log_obj, "Invalid command format, should start with 'maintain'");
        return;
    }
    
    // 获取操作类型
    token = strtok_r(NULL, " \t", &saveptr);
    if (!token) {
        MODULE_LOG_WARN(&uart_log_obj, "Missing operation type, should be 'read', 'write', or 'parameter'");
        return;
    }
    
    if (strcmp(token, "read") == 0) {
        // 读取参数
        token = strtok_r(NULL, " \t", &saveptr);
        if (!token) {
            MODULE_LOG_WARN(&uart_log_obj, "Missing parameter name");
            return;
        }
        
        ParamInfo* param = com_param_manager_get_param_by_name(token);
        if (!param) {
            MODULE_LOG_ERROR(&uart_log_obj, "Parameter '%s' does not exist", token);
            return;
        }
        
        // 读取参数值
        uint8_t value_buffer[32];
        uint8_t length;
        if (com_param_manager_read_param(param->param_id, value_buffer, &length)) {
        
            // 根据参数类型格式化输出
            switch (param->param_type) {
                case PARAM_TYPE_INT:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = %d", param->param_name, *((int*)value_buffer));
                    break;
                case PARAM_TYPE_FLOAT:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = %.2f", param->param_name, *((float*)value_buffer));
                    break;
                case PARAM_TYPE_BOOL:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = %s", param->param_name, *((bool*)value_buffer) ? "true" : "false");
                    break;
                case PARAM_TYPE_UINT8:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = %u", param->param_name, *((uint8_t*)value_buffer));
                    break;
                case PARAM_TYPE_UINT16:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = %u", param->param_name, *((uint16_t*)value_buffer));
                    break;
                case PARAM_TYPE_UINT32:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = %lu", param->param_name, *((uint32_t*)value_buffer));
                    break;
                case PARAM_TYPE_STRING:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = '%s'", param->param_name, (char*)value_buffer);
                    break;
                case PARAM_TYPE_LOG_LEVEL:
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' = %u", param->param_name, *((uint8_t*)value_buffer));
                    break;
                default:
                    MODULE_LOG_WARN(&uart_log_obj, "Unknown parameter type");
                    break;
            }
        } else {
            MODULE_LOG_ERROR(&uart_log_obj, "Failed to read parameter '%s'", param->param_name);
        }
        
    } else if (strcmp(token, "parameter") == 0) {
        // 参数帮助功能
        token = strtok_r(NULL, " \t", &saveptr);
        if (!token) {
            MODULE_LOG_WARN(&uart_log_obj, "Missing parameter subcommand, should be 'help'");
            return;
        }
        
        if (strcmp(token, "help") == 0) {
            // 获取页码参数
            token = strtok_r(NULL, " \t", &saveptr);
            if (!token) {
                // 显示总览信息
                maintain_show_parameter_overview();
            } else if (strcmp(token, "page") == 0) {
                // 获取页码
                token = strtok_r(NULL, " \t", &saveptr);
                if (!token) {
                    MODULE_LOG_WARN(&uart_log_obj, "Missing page number");
                    return;
                }
                int page_num = atoi(token);
                if (page_num <= 0) {
                    MODULE_LOG_WARN(&uart_log_obj, "Invalid page number, should be > 0");
                    return;
                }
                maintain_show_parameter_page(page_num);
            } else {
                MODULE_LOG_WARN(&uart_log_obj, "Invalid parameter subcommand, should be 'help' or 'help page <number>'");
            }
        } else {
            MODULE_LOG_WARN(&uart_log_obj, "Invalid parameter subcommand, should be 'help'");
        }
        
    } else if (strcmp(token, "write") == 0) {
        // 写入参数
        token = strtok_r(NULL, " \t", &saveptr);
        if (!token) {
            MODULE_LOG_WARN(&uart_log_obj, "Missing parameter name");
            return;
        }
        
        ParamInfo* param = com_param_manager_get_param_by_name(token);
        if (!param) {
            MODULE_LOG_ERROR(&uart_log_obj, "Parameter '%s' does not exist", token);
            return;
        }
        
        // 获取参数值
        token = strtok_r(NULL, " \t", &saveptr);
        if (!token) {
            MODULE_LOG_WARN(&uart_log_obj, "Missing parameter value");
            return;
        }
        
        // 根据参数类型转换并写入
        bool write_success = false;
        switch (param->param_type) {
            case PARAM_TYPE_INT: {
                int value = atoi(token);
                write_success = com_param_manager_write_param(param->param_id, &value, sizeof(int));
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to %d", param->param_name, value);
                }
                break;
            }
            case PARAM_TYPE_FLOAT: {
                float value = atof(token);
                write_success = com_param_manager_write_param(param->param_id, &value, sizeof(float));
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to %.2f", param->param_name, value);
                }
                break;
            }
            case PARAM_TYPE_BOOL: {
                bool value = (strcmp(token, "true") == 0 || strcmp(token, "1") == 0);
                write_success = com_param_manager_write_param(param->param_id, &value, sizeof(bool));
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to %s", param->param_name, value ? "true" : "false");
                }
                break;
            }
            case PARAM_TYPE_UINT8: {
                uint8_t value = (uint8_t)atoi(token);
                write_success = com_param_manager_write_param(param->param_id, &value, sizeof(uint8_t));
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to %u", param->param_name, value);
                }
                break;
            }
            case PARAM_TYPE_UINT16: {
                uint16_t value = (uint16_t)atoi(token);
                write_success = com_param_manager_write_param(param->param_id, &value, sizeof(uint16_t));
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to %u", param->param_name, value);
                }
                break;
            }
            case PARAM_TYPE_UINT32: {
                uint32_t value = (uint32_t)atol(token);
                write_success = com_param_manager_write_param(param->param_id, &value, sizeof(uint32_t));
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to %lu", param->param_name, value);
                }
                break;
            }
            case PARAM_TYPE_STRING: {
                write_success = com_param_manager_write_param(param->param_id, token, strlen(token) + 1);
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to '%s'", param->param_name, token);
                }
                break;
            }
            case PARAM_TYPE_LOG_LEVEL: {
                uint8_t value = (uint8_t)atoi(token);
                write_success = com_param_manager_write_param(param->param_id, &value, sizeof(uint8_t));
                if (write_success) {
                    MODULE_LOG_INFO(&uart_log_obj, "Parameter '%s' set to %u", param->param_name, value);
                    // 日志等级更新现在由各模块的回调函数处理
                }
                break;
            }
            default:
                MODULE_LOG_WARN(&uart_log_obj, "Unsupported parameter type");
                break;
        }
        
        if (!write_success) {
            MODULE_LOG_ERROR(&uart_log_obj, "Failed to write parameter '%s'", param->param_name);
        }
    } else {
        MODULE_LOG_WARN(&uart_log_obj, "Invalid operation type '%s', should be 'read' or 'write'", token);
    }
}

/**
 * @brief Function implementing the maintain_task thread.
 * @param argument: Not used
 * @retval None
 */
void maintain_task_func(void *argument) {

    // 维护计数器
    static uint32_t maintain_counter = 0;
    
    for (;;) {
        // 定期打印系统状态维护信息
        maintain_counter++;
        if (maintain_counter % 100 == 0) {
            // 每1秒打印一次（100 * 10ms = 1s）
            // MODULE_LOG_INFO(&uart_log_obj, "testtesttesttesttest");
            // MODULE_LOG_INFO(&uart_log_obj, "TESTTESTTESTTESTTEST");
        }
        
        // 处理UART接收的命令（检查命令状态，但不直接处理）
        uart_debug_obj.process(&uart_debug_obj);
        
        // 处理命令队列中的命令（包括UART接收和任务发送的命令）
        MaintainCommand cmd;
        if (xQueueReceive(command_queue, &cmd, 0) == pdTRUE) {
            if (cmd.is_valid) {
                MODULE_LOG_INFO(&uart_log_obj, "Processing command: %s", cmd.cmd_line);
                maintain_process_command(cmd.cmd_line);
            }
        }

        // 处理日志队列中的消息
        lib_uart_log_process_queue(&uart_log_obj);

        osDelay(10); // 10ms delay, reduce CPU usage
    }
}

/**
 * @brief 初始化维护任务和UART协议
 * @details 完成维护系统的完整初始化，包括UART硬件驱动、协议层配置和任务创建
 *          参考com模块的初始化过程，使用适配器模式连接硬件驱动层和协议层
 * 
 * @note 初始化流程：
 *       1. 创建UART DMA实例 (USART2, Uart1)
 *       2. 配置维护协议模块的硬件相关函数指针
 *       3. 配置维护协议模块的协议接口函数指针
 *       4. 调用协议层配置和初始化函数
 *       5. 配置日志模块的硬件抽象层
 *       6. 初始化日志模块
 *       7. 创建命令队列 (容量10个MaintainCommand)
 *       8. 创建维护任务 (maintain_task_func)
 * 
 * @note 硬件配置：
 *       - UART实例: USART2, Uart1
 *       - 波特率: UART_DMA_DEFAULT_BAUDRATE
 *       - 回调函数: maintain_protocol_callback
 * 
 * @note 任务配置：
 *       - 任务名称: "maintain_task"
 *       - 优先级: osPriorityNormal4
 *       - 栈大小: 512 * 4 bytes
 * 
 * @return void
 */
void maintain_task_init(void) {

    for(volatile int i = 0; i < 1000; i++) {
        __NOP();
    }

    // 初始化 UART DMA 实例
    drv_uart_dma_create(&uart_maintain_dma_obj, USART2, Uart1);

    // 配置维护协议模块的硬件相关函数指针
    uart_debug_obj.uart_dma_ptr = &uart_maintain_dma_obj;
    uart_debug_obj.send = drv_uart_send_impl;
    uart_debug_obj.receive = drv_uart_receive_impl;
    uart_debug_obj.set_baudrate = drv_uart_set_baudrate_impl;
    uart_debug_obj.enable_interrupts = drv_uart_enable_interrupts_impl;
    uart_debug_obj.disable_interrupts = drv_uart_disable_interrupts_impl;
    uart_debug_obj.get_rx_buffer = drv_uart_get_rx_buffer_impl;
    uart_debug_obj.register_idle_callback = drv_uart_register_idle_callback_impl;
    uart_debug_obj.register_error_callback = drv_uart_register_error_callback_impl;
    uart_debug_obj.register_tx_complete_callback = drv_uart_register_tx_complete_callback_impl;

    // 配置维护协议模块的协议接口函数指针
    uart_debug_obj.configure = lib_debug_protocol_configure;
    uart_debug_obj.initialize = lib_debug_protocol_initialize;
    uart_debug_obj.process = lib_debug_protocol_process;
    uart_debug_obj.send_data = lib_debug_protocol_send_data;
    uart_debug_obj.is_cmd_ready = lib_debug_protocol_is_cmd_ready;
    uart_debug_obj.get_cmd = lib_debug_protocol_get_cmd;
    uart_debug_obj.clear_cmd = lib_debug_protocol_clear_cmd;

    // 创建命令队列（必须在回调函数注册之前创建）
    command_queue = xQueueCreate(10, sizeof(MaintainCommand));
    if (command_queue == NULL) {
        // 队列创建失败，输出错误信息
        // 注意：这里不能使用MODULE_LOG_INFO，因为日志模块可能还没初始化
    }

    // 配置和初始化维护协议
    uart_debug_obj.configure(&uart_debug_obj);
    uart_debug_obj.initialize(&uart_debug_obj, 
                                     UART_DMA_DEFAULT_BAUDRATE, 
                                     maintain_protocol_callback, 
                                     NULL);
    
    

    // 配置日志模块的硬件抽象层
    uart_log_obj.uart_dma_ptr = &uart_maintain_dma_obj;
    uart_log_obj.send = drv_uart_send_impl;
    uart_log_obj.register_tx_complete_callback = drv_uart_register_tx_complete_callback_impl;

    // 初始化日志模块
    lib_uart_log_init(&uart_log_obj, "MAINTAIN", maintain_log_level);
    
    // 注册日志等级参数的回调函数
    com_param_manager_set_write_callback(0x06, maintain_log_level_write_callback, NULL);
    
    // 创建维护任务
    maintain_taskHandle = osThreadNew(maintain_task_func, 
                                      NULL, 
                                      &maintain_task_attributes);
    
    // 设置初始化标志
    maintain_init_flag = true;
}

// 获取maintain模块的UART实例
st_uart_dma* get_maintain_uart_instance(void) {
    return &uart_maintain_dma_obj;
}

// 检查maintain模块是否已初始化
bool maintain_module_initialized(void) {
    return maintain_init_flag;
}

/**
 * @brief 显示参数总览信息
 * @details 显示可读写参数的总数和总页数
 */
void maintain_show_parameter_overview(void) {
    uint8_t total_params = com_param_manager_get_param_count();
    uint8_t params_per_page = 20;
    uint8_t total_pages = (total_params + params_per_page - 1) / params_per_page; // 向上取整
    
    MODULE_LOG_INFO(&uart_log_obj, "Parameter Overview:\r\n  Total parameters: %u\r\n  Parameters per page: %u\r\n  Total pages: %u\r\n  Usage: maintain parameter help page <number>", 
                   total_params, params_per_page, total_pages);
}

/**
 * @brief 显示指定页的参数列表
 * @param page_num 页码（从1开始）
 */
void maintain_show_parameter_page(int page_num) {
    uint8_t total_params = com_param_manager_get_param_count();
    uint8_t params_per_page = 20;
    uint8_t total_pages = (total_params + params_per_page - 1) / params_per_page;
    
    // 检查页码是否有效
    if (page_num > total_pages) {
        MODULE_LOG_WARN(&uart_log_obj, "Page %d does not exist. Total pages: %u", page_num, total_pages);
        return;
    }
    
    ParamInfo* param_list = com_param_manager_get_param_list();
    if (!param_list) {
        MODULE_LOG_ERROR(&uart_log_obj, "Parameter list is NULL");
        return;
    }
    
    uint8_t start_index = (page_num - 1) * params_per_page;
    uint8_t end_index = (start_index + params_per_page > total_params) ? total_params : start_index + params_per_page;
    
    // 构建参数列表字符串，限制在400字节以内（为日志前缀留空间）
    char param_list_buffer[400];
    int offset = 0;
    
    // 添加标题信息
    offset += snprintf(param_list_buffer + offset, sizeof(param_list_buffer) - offset,
                      "Page %d/%u, Params %u-%u/%u:\r\n  ", 
                      page_num, total_pages, start_index + 1, end_index, total_params);
    
    // 添加参数列表（简化格式，只显示ID和名称）
    for (uint8_t i = start_index; i < end_index; i++) {
        ParamInfo* param = &param_list[i];
        if (!param) {
            continue;
        }
        
        const char* param_name = param->param_name ? param->param_name : "UNKNOWN";
        
        // 检查剩余空间
        if (offset + 20 >= (int)sizeof(param_list_buffer)) {
            break; // 避免缓冲区溢出
        }
        
        offset += snprintf(param_list_buffer + offset, sizeof(param_list_buffer) - offset,
                          "[%02u]%s\r\n  ", param->param_id, param_name);
    }
    
    // 添加导航提示
    if (page_num < total_pages) {
        if (offset + 30 < (int)sizeof(param_list_buffer)) {
            offset += snprintf(param_list_buffer + offset, sizeof(param_list_buffer) - offset,
                              "\r\n  Next: maintain parameter help page %d", page_num + 1);
        }
    }
    if (page_num > 1) {
        if (offset + 30 < (int)sizeof(param_list_buffer)) {
            offset += snprintf(param_list_buffer + offset, sizeof(param_list_buffer) - offset,
                              "\r\n  Prev: maintain parameter help page %d", page_num - 1);
        }
    }
    
    // 一次性输出所有内容
    MODULE_LOG_INFO(&uart_log_obj, "%s", param_list_buffer);
}

/*---End of File----------------------------------------------------*/ 
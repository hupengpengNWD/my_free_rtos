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

#include "lib_soft_timer.h"

// 全局变量
st_soft_time_ptr timerCountdown_hendle_ptr = NULL; // 定时器链表头
static uint32_t global_tick_ms = 0; // 全局毫秒计数值

/**
  * @name     lib_softtimer_get_tick_count
  * @brief    获取当前软件定时器计数值 (ms)
  * @param    None
  * @return   uint32_t: 当前计数值
  * @remark   从全局毫秒计数读取
  */
uint32_t lib_softtimer_get_tick_count(void) {
    return global_tick_ms;
}

/**
  * @name     lib_softtimer_update_tick
  * @brief    更新全局计数值（由硬件定时器调用）
  * @param    None
  * @return   None
  * @remark   供 drv_timer_countdown 调用
  */
void lib_softtimer_update_tick(void) {
    global_tick_ms++;
}

/**
  * @name     lib_softtimer_fsm
  * @brief    软件定时器状态机
  * @param    ptr: 定时器指针
  * @return   None
  * @remark   处理超时、回调和状态转换
  */
void lib_softtimer_fsm(st_soft_time_ptr ptr) {
    switch (ptr->state) {
        case stoped_status:
            ptr->stop(ptr);
            break;

        case running_status:
            if (lib_softtimer_get_tick_count() >= ptr->deadline) {
                ptr->state = timeout_status;
                if (ptr->cb != NULL) {
                    ptr->cb(ptr->CbArgu);
                }
            }
            break;

        case timeout_status:
            if (ptr->mode == OnceMode) {
                ptr->state = stoped_status;
                ptr->stop(ptr);
            } else if (ptr->mode == PeriodicMode) {
                ptr->deadline += ptr->countdown;
                ptr->state = running_status;
            }
            break;

        default:
            break;
    }
}

/**
  * @name     lib_softtimer_get_state
  * @brief    获取定时器状态
  * @param    ptr: 定时器指针
  * @return   uint8_t: 定时器状态
  * @remark   
  */
uint8_t lib_softtimer_get_state(st_soft_time_ptr ptr) {
    return ptr->state;
}

/**
  * @name     lib_softtimer_set_argument
  * @brief    设置定时器参数 (模式和超时时间)
  * @param    ptr: 定时器指针
  * @param    mode: 定时模式 (OnceMode/PeriodicMode)
  * @param    countdown: 超时时间 (ms)
  * @return   None
  * @remark   
  */
void lib_softtimer_set_argument(st_soft_time_ptr ptr, uint8_t mode, uint64_t countdown) {
    ptr->deadline = lib_softtimer_get_tick_count() + countdown;
    ptr->countdown = countdown;
    ptr->state = running_status;
    ptr->mode = mode;
}

/**
  * @name     lib_softtimer_set_callback
  * @brief    设置定时器回调函数和参数
  * @param    ptr: 定时器指针
  * @param    cb: 回调函数
  * @param    CbArgu: 回调参数
  * @return   None
  * @remark   
  */
void lib_softtimer_set_callback(st_soft_time_ptr ptr, void (*cb)(void*), void *CbArgu) {
    ptr->cb = cb;
    ptr->CbArgu = CbArgu;
}

/**
  * @name     lib_softtimer_stop
  * @brief    停止定时器
  * @param    ptr: 定时器指针
  * @return   None
  * @remark   从链表移除
  */
void lib_softtimer_stop(st_soft_time_ptr ptr) {
    st_soft_time_ptr *curr;
    for (curr = &timerCountdown_hendle_ptr; *curr; curr = &(*curr)->next) {
        if (*curr == ptr) {
            *curr = ptr->next;
            break;
        }
    }
}

/**
  * @name     lib_softtimer_start
  * @brief    启动定时器
  * @param    ptr: 定时器指针
  * @return   int: 0 成功, -1 失败 (已存在)
  * @remark   添加到链表
  */
int lib_softtimer_start(st_soft_time_ptr ptr) {
    st_soft_time_ptr target = timerCountdown_hendle_ptr;
    while (target) {
        if (target == ptr) {
            return -1; // 已存在
        }
        target = target->next;
    }

    ptr->next = timerCountdown_hendle_ptr;
    timerCountdown_hendle_ptr = ptr;

    return 0;
}

/**
  * @name     lib_softtimer_initialize
  * @brief    初始化定时器结构体
  * @param    ptr: 定时器指针
  * @return   None
  * @remark   
  */
void lib_softtimer_initialize(st_soft_time_ptr ptr) {
    ptr->state = stoped_status;
    ptr->mode = 0;
    ptr->deadline = 0;
    ptr->countdown = 0;
    ptr->cb = NULL;
    ptr->CbArgu = NULL;
    ptr->next = NULL;
}

/**
  * @name     lib_softtimer_configure
  * @brief    配置定时器函数指针
  * @param    ptr: 定时器指针
  * @return   None
  * @remark   
  */
void lib_softtimer_configure(st_soft_time_ptr ptr) {
    ptr->stop = lib_softtimer_stop;
    ptr->start = lib_softtimer_start;
    ptr->setr_argument = lib_softtimer_set_argument;
    ptr->setr_call_back = lib_softtimer_set_callback;
    ptr->get_state = lib_softtimer_get_state;
    ptr->get_tick_count = lib_softtimer_get_tick_count;
    ptr->fsm = lib_softtimer_fsm;
    ptr->initialize = lib_softtimer_initialize;
    ptr->configure = lib_softtimer_configure;
}

/*---End of File----------------------------------------------------*/
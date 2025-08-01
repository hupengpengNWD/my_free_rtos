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

#include "lib_dc_motor.h"
#include "math.h"


/***************************************** 第一部分 基本驱动 *************************************************/

// extern TIM_HandleTypeDef g_atimx_cplm_pwm_handle;                 /* 定时器x句柄 */

/**
 * @brief       电机初始化
 * @param       无
 * @retval      无
 */
void dcmotor_init(void)
{
    // SHUTDOWN1_GPIO_CLK_ENABLE();
    // GPIO_InitTypeDef gpio_init_struct;
    
    // /* SD引脚设置，设置为推挽输出 */
    // gpio_init_struct.Pin = SHUTDOWN1_Pin;
    // gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    // gpio_init_struct.Pull = GPIO_NOPULL;
    // gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    // HAL_GPIO_Init(SHUTDOWN1_GPIO_Port, &gpio_init_struct);
    
    // HAL_GPIO_WritePin(SHUTDOWN1_GPIO_Port, SHUTDOWN1_Pin, GPIO_PIN_RESET);      /* SD拉低，关闭输出 */
    
    // dcmotor_stop();                 /* 停止电机 */
    // dcmotor_dir(0);                 /* 设置正转 */
    // dcmotor_speed(0);               /* 速度设置为0 */
    // dcmotor_start();                /* 开启电机 */
}

/**
 * @brief       电机开启
 * @param       无
 * @retval      无
 */
void dcmotor_start(void)
{
    ENABLE_MOTOR;                                                       /* 拉高SD引脚，开启电机 */
}

/**
 * @brief       电机停止
 * @param       无
 * @retval      无
 */
void dcmotor_stop(void)
{
    // HAL_TIM_PWM_Stop(&g_atimx_cplm_pwm_handle, TIM_CHANNEL_1);          /* 关闭主通道输出 */
    // HAL_TIMEx_PWMN_Stop(&g_atimx_cplm_pwm_handle, TIM_CHANNEL_1);       /* 关闭互补通道输出 */
    // DISABLE_MOTOR;                                                      /* 拉低SD引脚，停止电机 */
}

/**
 * @brief       电机旋转方向设置
 * @param       para:方向 0正转，1反转
 * @note        以电机正面，顺时针方向旋转为正转
 * @retval      无
 */
void dcmotor_dir(uint8_t para)
{
    // HAL_TIM_PWM_Stop(&g_atimx_cplm_pwm_handle, TIM_CHANNEL_1);          /* 关闭主通道输出 */
    // HAL_TIMEx_PWMN_Stop(&g_atimx_cplm_pwm_handle, TIM_CHANNEL_1);       /* 关闭互补通道输出 */

    // if (para == 0)                /* 正转 */
    // {
    //     HAL_TIM_PWM_Start(&g_atimx_cplm_pwm_handle, TIM_CHANNEL_1);     /* 开启主通道输出 */
    // } 
    // else if (para == 1)           /* 反转 */
    // {
    //     HAL_TIMEx_PWMN_Start(&g_atimx_cplm_pwm_handle, TIM_CHANNEL_1);  /* 开启互补通道输出 */
    // }
}

/**
 * @brief       电机速度设置
 * @param       para:比较寄存器值
 * @retval      无
 */
void dcmotor_speed(uint16_t para)
{
    // if (para < (__HAL_TIM_GetAutoreload(&g_atimx_cplm_pwm_handle) - 0x0F))  /* 限速 */
    // {  
    //     __HAL_TIM_SetCompare(&g_atimx_cplm_pwm_handle, TIM_CHANNEL_1, para);
    // }
}

/**
 * @brief       电机速度控制
 * @param       para: pwm比较值 ,正数电机为正转，负数为反转
 * @note        根据传入的参数控制电机的转向和速度
 * @retval      无
 */
void motor_pwm_set(float para)
{
    int val = (int)para;

    if (val >= 0) 
    {
        dcmotor_dir(0);           /* 正转 */
        dcmotor_speed(val);
    } 
    else 
    {
        dcmotor_dir(1);           /* 反转 */
        dcmotor_speed(-val);
    }
}


/************************************** 第二部分 电压电流温度采集 *********************************************/






/***************************************** 第三部分 编码器测速 ************************************************/

// Motor_TypeDef g_motor_data;  /*电机参数变量*/
// ENCODE_TypeDef g_encode;     /*编码器参数变量*/

/**
 * @brief       电机速度计算
 * @param       encode_now：当前编码器总的计数值
 *              ms：计算速度的间隔，中断1ms进入一次，例如ms = 5即5ms计算一次速度
 * @retval      无
 */
void speed_computer(int32_t encode_now, uint8_t ms)
{
    // uint8_t i = 0, j = 0;
    // float temp = 0.0;
    // static uint8_t sp_count = 0, k = 0;
    // static float speed_arr[10] = {0.0};                     /* 存储速度进行滤波运算 */

    // if (sp_count == ms)                                     /* 计算一次速度 */
    // {
    //     /* 计算电机转速 
    //        第一步 ：计算ms毫秒内计数变化量
    //        第二步 ；计算1min内计数变化量：g_encode.speed * ((1000 / ms) * 60 ，
    //        第三步 ：除以编码器旋转一圈的计数次数（倍频倍数 * 编码器分辨率）
    //        第四步 ：除以减速比即可得出电机转速
    //     */
    //     g_encode.encode_now = encode_now;                                /* 取出编码器当前计数值 */
    //     g_encode.speed = (g_encode.encode_now - g_encode.encode_old);    /* 计算编码器计数值的变化量 */
        
    //     speed_arr[k++] = (float)(g_encode.speed * ((1000 / ms) * 60.0) / REDUCTION_RATIO / ROTO_RATIO );    /* 保存电机转速 */
        
    //     g_encode.encode_old = g_encode.encode_now;          /* 保存当前编码器的值 */

    //     /* 累计10次速度值，后续进行滤波*/
    //     if (k == 10)
    //     {
    //         for (i = 10; i >= 1; i--)                       /* 冒泡排序*/
    //         {
    //             for (j = 0; j < (i - 1); j++) 
    //             {
    //                 if (speed_arr[j] > speed_arr[j + 1])    /* 数值比较 */
    //                 { 
    //                     temp = speed_arr[j];                /* 数值换位 */
    //                     speed_arr[j] = speed_arr[j + 1];
    //                     speed_arr[j + 1] = temp;
    //                 }
    //             }
    //         }
            
    //         temp = 0.0;
            
    //         for (i = 2; i < 8; i++)                         /* 去除两边高低数据 */
    //         {
    //             temp += speed_arr[i];                       /* 将中间数值累加 */
    //         }
            
    //         temp = (float)(temp / 6);                       /*求速度平均值*/
            
    //         /* 一阶低通滤波
    //          * 公式为：Y(n)= qX(n) + (1-q)Y(n-1)
    //          * 其中X(n)为本次采样值；Y(n-1)为上次滤波输出值；Y(n)为本次滤波输出值，q为滤波系数
    //          * q值越小则上一次输出对本次输出影响越大，整体曲线越平稳，但是对于速度变化的响应也会越慢
    //          */
    //         g_motor_data.speed = (float)( ((float)0.48 * temp) + (g_motor_data.speed * (float)0.52) );
    //         k = 0;
    //     }
    //     sp_count = 0;
    // }
    // sp_count ++;
}



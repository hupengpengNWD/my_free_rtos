
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

#ifndef __DCMOTOR_H
#define __DCMOTOR_H

#include "stm32g4xx.h"
#include "core_cm4.h"
#include "stm32g4xx_hal.h"

/********************************** 第一部分 基本驱动 *****************************************/

/* 停止引脚操作宏定义 
 * 此引脚控制H桥是否生效以达到开启和关闭电机的效果
 */
#define SHUTDOWN1_Pin                 GPIO_PIN_13
#define SHUTDOWN1_GPIO_Port           GPIOC
#define SHUTDOWN1_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC口时钟使能 */

#define SHUTDOWN2_Pin                 GPIO_PIN_11
#define SHUTDOWN2_GPIO_Port           GPIOD
#define SHUTDOWN2_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /* PD口时钟使能 */

/* 电机停止引脚定义 这里默认是接口1 */
#define ENABLE_MOTOR    HAL_GPIO_WritePin(SHUTDOWN1_GPIO_Port,SHUTDOWN1_Pin,GPIO_PIN_SET)
#define DISABLE_MOTOR   HAL_GPIO_WritePin(SHUTDOWN1_GPIO_Port,SHUTDOWN1_Pin,GPIO_PIN_RESET)

/********************************** 第二部分 电压电流温度采集 *********************************/

/* 电流计算公式：
 * I=（最终输出电压-初始参考电压）/（6*0.02）A
 * ADC值转换为电压值：电压=ADC值*3.3/4096，这里电压单位为V，我们换算成mV,4096/1000=4.096，后面就直接算出为mA
 * 整合公式可以得出电流 I= （当前ADC值-初始参考ADC值）* （3.3 / 4.096 / 0.12）
 */
#define ADC2CURT    (float)(3.3f / 4.096f / 0.12f)

/* 电压计算公式：
 * V_POWER = V_BUS * 25
 * ADC值转换为电压值：电压=ADC值*3.3/4096
 * 整合公式可以得出电压V_POWER= ADC值 *（3.3f * 25 / 4096）
 */
#define ADC2VBUS    (float)(3.3f * 25 / 4096)

/********************************** 第三部分 编码器测速 ***************************************/

#define ROTO_RATIO      44  /* 线数*倍频系数，即11*4=44 */
#define REDUCTION_RATIO 30  /* 减速比30:1 */

/* 电机参数结构体 */
typedef struct 
{
  uint8_t state;            /*电机状态*/
  float current;            /*电机电流*/
  float volatage;           /*电机电压*/
  float power;              /*电机功率*/
  float speed;              /*电机实际速度*/
  int32_t motor_pwm;        /*设置比较值大小 */
} Motor_TypeDef;

extern Motor_TypeDef  g_motor_data;     /*电机参数变量*/

/******************************************************************************************/

void dcmotor_init(void);                /* 直流有刷电机初始化 */
void dcmotor_start(void);               /* 开启电机 */
void dcmotor_stop(void);                /* 关闭电机 */  
void dcmotor_dir(uint8_t para);         /* 设置电机方向 */
void dcmotor_speed(uint16_t para);      /* 设置电机速度 */
void motor_pwm_set(float para);         /* 设置电机速度 */
void speed_computer(int32_t encode_now, uint8_t ms);/* 电机速度计算 */

#endif





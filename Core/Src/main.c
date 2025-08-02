/**
  ******************************************************************************
  * @file:    main.c
  * @brief:   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "main.h"
#include "cmsis_os.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "protector.h"
#include "manual.h"
#include "com.h"
#include "maintain.h"
#include "drv_gpio_output.h"
#include "drv_timer_countdown.h"
#include "drv_gpio_input.h"
#include "drv_timer_pwm.h"
#include "motor.h"

void MX_FREERTOS_Init(void);

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {

    /* 当前分支：motor分支 */
    /* 通用硬件初始化 */
    HAL_Init();
    SystemClock_Config();

    /* 外设硬件初始化 */
    drv_gpio_output_hw_init(GPIOE, GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0);
    drv_gpio_output_hw_init(GPIOE, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0);
    drv_timer_countdown_hw_init(TIM4, TIMER_COUNTDOWN_DEFAULT_PRESCALER);
    drv_gpio_input_hw_init(GPIOE, GPIO_PIN_12, GPIO_PULLUP, 0);
    drv_gpio_input_hw_init(GPIOE, GPIO_PIN_13, GPIO_PULLUP, 0);
    drv_uart_dma_hw_init(USART1, UART_DMA_DEFAULT_BAUDRATE, 0, DMA1_Channel4, DMA_REQUEST_USART1_TX, DMA1_Channel5, DMA_REQUEST_USART1_RX, Uart0);
    drv_uart_dma_hw_init(USART2, UART_DMA_DEFAULT_BAUDRATE, 1, DMA1_Channel7, DMA_REQUEST_USART2_TX, DMA1_Channel6, DMA_REQUEST_USART2_RX, Uart1);
                         
                         
  
    /* PWM硬件初始化 */
    // drv_timer_pwm_hw_init(TIM1, 1000, 0, 1, Pwm0);  // 电机通道1，TIM1，1kHz，引脚组0
    // drv_timer_pwm_hw_init(TIM1, 1000, 0, 2, Pwm1);  // 电机通道2，TIM1，1kHz，引脚组0
    // drv_timer_pwm_hw_init(TIM3, 1000, 0, 1, Pwm2);  // 电机通道3，TIM3，1kHz，引脚组0
    // drv_timer_pwm_hw_init(TIM3, 1000, 0, 2, Pwm3);  // 电机通道4，TIM3，1kHz，引脚组0


    /* rtos内核初始化 */
    osKernelInitialize();
    
    /* 初始化各个任务 */ 
    maintain_task_init();  // 初始化维护任务 - 必须先初始化
    com_task_init();
    protector_task_init();
    manual_task_init();
    
    /* rtos内核启动 */
    osKernelStart();

    for(;;);
}

/*---End of File----------------------------------------------------*/

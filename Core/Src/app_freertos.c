/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for switchs_task */
osThreadId_t switchs_taskHandle;
const osThreadAttr_t switchs_task_attributes = {
  .name = "switchs_task",
  .priority = (osPriority_t) osPriorityNormal5,
  .stack_size = 256 * 4
};
/* Definitions for debug_task */
osThreadId_t debug_taskHandle;
const osThreadAttr_t debug_task_attributes = {
  .name = "debug_task",
  .priority = (osPriority_t) osPriorityNormal6,
  .stack_size = 256 * 4
};
/* Definitions for button_task */
osThreadId_t button_taskHandle;
const osThreadAttr_t button_task_attributes = {
  .name = "button_task",
  .priority = (osPriority_t) osPriorityNormal7,
  .stack_size = 256 * 4
};
/* Definitions for myQueue01 */
osMessageQueueId_t myQueue01Handle;
const osMessageQueueAttr_t myQueue01_attributes = {
  .name = "myQueue01"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void switch_task_func_poll(void *argument);
void debug_task_func_poll(void *argument);
void button_task_func_poll(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of myQueue01 */
  myQueue01Handle = osMessageQueueNew (16, sizeof(uint16_t), &myQueue01_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of switchs_task */
  switchs_taskHandle = osThreadNew(switch_task_func_poll, NULL, &switchs_task_attributes);

  /* creation of debug_task */
  debug_taskHandle = osThreadNew(debug_task_func_poll, NULL, &debug_task_attributes);

  /* creation of button_task */
  button_taskHandle = osThreadNew(button_task_func_poll, NULL, &button_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_switch_task_func_poll */
/**
* @brief Function implementing the switchs_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_switch_task_func_poll */
void switch_task_func_poll(void *argument)
{
  /* USER CODE BEGIN switch_task_func_poll */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END switch_task_func_poll */
}

/* USER CODE BEGIN Header_debug_task_func_poll */
/**
* @brief Function implementing the debug_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_debug_task_func_poll */
void debug_task_func_poll(void *argument)
{
  /* USER CODE BEGIN debug_task_func_poll */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END debug_task_func_poll */
}

/* USER CODE BEGIN Header_button_task_func_poll */
/**
* @brief Function implementing the button_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_button_task_func_poll */
void button_task_func_poll(void *argument)
{
  /* USER CODE BEGIN button_task_func_poll */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END button_task_func_poll */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */


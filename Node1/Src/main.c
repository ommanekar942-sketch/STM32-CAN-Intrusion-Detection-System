/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

CAN_TxHeaderTypeDef TxHeader;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

UART_HandleTypeDef huart1;

/* Definitions for RPMTask */
osThreadId_t RPMTaskHandle;
const osThreadAttr_t RPMTask_attributes = {
  .name = "RPMTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SpeedTask */
osThreadId_t SpeedTaskHandle;
const osThreadAttr_t SpeedTask_attributes = {
  .name = "SpeedTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TempTask */
osThreadId_t TempTaskHandle;
const osThreadAttr_t TempTask_attributes = {
  .name = "TempTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for AttackTask */
osThreadId_t AttackTaskHandle;
const osThreadAttr_t AttackTask_attributes = {
  .name = "AttackTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* USER CODE BEGIN PV */
uint32_t TxMailbox;

volatile uint32_t lastButtonTick = 0;

volatile uint8_t attack_mode = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_USART1_UART_Init(void);
void StartRPMTask(void *argument);
void StartSpeedTask(void *argument);
void StartTempTask(void *argument);
void StartAttackTask(void *argument);

/* USER CODE BEGIN PFP */

void CAN_Filter_Config(void);

void CAN_Tx(uint16_t can_id, uint8_t *data, uint8_t len);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  // filters are applied immediately when CAN leaves initialization mode.
  CAN_Filter_Config();

  if(HAL_CAN_Start(&hcan) != HAL_OK)
  {
	  Error_Handler();
  }

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of RPMTask */
  RPMTaskHandle = osThreadNew(StartRPMTask, NULL, &RPMTask_attributes);

  /* creation of SpeedTask */
  SpeedTaskHandle = osThreadNew(StartSpeedTask, NULL, &SpeedTask_attributes);

  /* creation of TempTask */
  TempTaskHandle = osThreadNew(StartTempTask, NULL, &TempTask_attributes);

  /* creation of AttackTask */
  AttackTaskHandle = osThreadNew(StartAttackTask, NULL, &AttackTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void CAN_Filter_Config(void)
{
	CAN_FilterTypeDef filterInit;

	filterInit.FilterActivation = CAN_FILTER_ENABLE;
	filterInit.FilterBank = 0;
	filterInit.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	filterInit.FilterIdHigh = 0x0000;
	filterInit.FilterIdLow = 0x0000;
	filterInit.FilterMaskIdHigh = 0x0000;
	filterInit.FilterMaskIdLow = 0x0000;
	filterInit.FilterMode = CAN_FILTERMODE_IDMASK;
	filterInit.FilterScale = CAN_FILTERSCALE_32BIT;

	if(HAL_CAN_ConfigFilter(&hcan, &filterInit) != HAL_OK)
	{
		Error_Handler();
	}

}

void CAN_Tx(uint16_t can_id, uint8_t *data, uint8_t len)
{
	TxHeader.StdId = can_id;
	TxHeader.DLC = len;
	TxHeader.ExtId = 0;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.TransmitGlobalTime = DISABLE;

	if(HAL_CAN_AddTxMessage(&hcan, &TxHeader, data, &TxMailbox) != HAL_OK)
	{
		Error_Handler();
	}

	while(HAL_CAN_IsTxMessagePending(&hcan, TxMailbox));
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	BaseType_t pxHigherPriorityTaskWoken;

	if(GPIO_Pin == GPIO_PIN_0)
	{
		 uint32_t currentTick = HAL_GetTick();

		 // Ignore interrupts within 200 ms
		 if((currentTick - lastButtonTick) < 200)
		 {
			 return;
		 }

		 lastButtonTick = currentTick;

		attack_mode++;

		if(attack_mode > 3)
		{
			attack_mode = 1;
		}

		pxHigherPriorityTaskWoken = pdFALSE;

		// used whenever an interrupt needs to signal a FreeRTOS task that something has happened.
		vTaskNotifyGiveFromISR(AttackTaskHandle, &pxHigherPriorityTaskWoken);

		// Interrupt finishes context switching takes place
		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
	}
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartRPMTask */
/**
* @brief Function implementing the RPM_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRPMTask */
void StartRPMTask(void *argument)
{
  /* USER CODE BEGIN 5 */

	uint8_t TxData[8] = {0};
	uint16_t rpm = 3000;

	TickType_t xLastWakeTime = xTaskGetTickCount();

  /* Infinite loop */
  for(;;)
  {
	  TxData[0] = (rpm >> 8) & 0xFF;
	  TxData[1] = rpm & 0xFF;

	  CAN_Tx(0x100, TxData, 2);

	  vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartSpeedTask */
/**
* @brief Function implementing the SpeedTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSpeedTask */
void StartSpeedTask(void *argument)
{
  /* USER CODE BEGIN StartSpeedTask */

	uint8_t TxData[8] = {0};
	uint16_t speed = 60;

	TickType_t xLastWakeTime = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
	  TxData[0] = (speed >> 8) & 0xFF;
	  TxData[1] = speed & 0xFF;

	  CAN_Tx(0x200, TxData, 2);

	  vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));
  }
  /* USER CODE END StartSpeedTask */
}

/* USER CODE BEGIN Header_StartTempTask */
/**
* @brief Function implementing the TempTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTempTask */
void StartTempTask(void *argument)
{
  /* USER CODE BEGIN StartTempTask */

	uint8_t TxData[8] = {0};
	uint16_t temperature = 35;

	TickType_t xLastWakeTime = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {

	  TxData[0] = (temperature >> 8) & 0xFF;
	  TxData[1] = temperature & 0xFF;

	  CAN_Tx(0x300, TxData, 2);

	  vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(30));
  }
  /* USER CODE END StartTempTask */
}

/* USER CODE BEGIN Header_StartAttackTask */
/**
* @brief Function implementing the AttackTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAttackTask */
void StartAttackTask(void *argument)
{
  /* USER CODE BEGIN StartAttackTask */

  /* Infinite loop */
  for(;;)
  {
	  // Wait for a notification.
	  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	  uint8_t TxData[8] = {0};

	  switch(attack_mode)
	  {
	  case 1 :
	  {
		  // Unknown ID

			CAN_Tx(0x666, TxData, 2);

		  break;
	  }

	  case 2 :
	  {
		  // Invalid RPM

		  uint16_t rpm = 15000;

		  TxData[0] = (rpm  >>  8) & 0xFF;
		  TxData[1] = rpm & 0xFF;

		  CAN_Tx(0x100, TxData, 2);

		  break;
	  }

	  case 3 :
	  {
		  // Flood attack

		  uint16_t rpm = 3000;

		  TxData[0] = (rpm  >>  8) & 0xFF;
		  TxData[1] = rpm & 0xFF;

		  for(int i=0; i<100; i++)
		  {
			  CAN_Tx(0x100, TxData, 2);
			  osDelay(1);
		  }

		  break;
	  }

	  default :
		  break;

	  }
  }
  /* USER CODE END StartAttackTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

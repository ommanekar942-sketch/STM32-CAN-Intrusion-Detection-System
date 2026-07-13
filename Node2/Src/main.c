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
#include "ssd1306.h"
#include "fonts.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MAX_RPM	8000
#define FLOOD_THRESHOLD_ms	5

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* Definitions for CANReceiveTask */
osThreadId_t CANReceiveTaskHandle;
const osThreadAttr_t CANReceiveTask_attributes = {
  .name = "CANReceiveTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for IDSDetectionTas */
osThreadId_t IDSDetectionTasHandle;
const osThreadAttr_t IDSDetectionTas_attributes = {
  .name = "IDSDetectionTas",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for AlertTask */
osThreadId_t AlertTaskHandle;
const osThreadAttr_t AlertTask_attributes = {
  .name = "AlertTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for OLEDTask */
osThreadId_t OLEDTaskHandle;
const osThreadAttr_t OLEDTask_attributes = {
  .name = "OLEDTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Queue1 */
osMessageQueueId_t Queue1Handle;
const osMessageQueueAttr_t Queue1_attributes = {
  .name = "Queue1"
};
/* Definitions for Queue2 */
osMessageQueueId_t Queue2Handle;
const osMessageQueueAttr_t Queue2_attributes = {
  .name = "Queue2"
};
/* Definitions for Queue3 */
osMessageQueueId_t Queue3Handle;
const osMessageQueueAttr_t Queue3_attributes = {
  .name = "Queue3"
};
/* Definitions for Queue4 */
osMessageQueueId_t Queue4Handle;
const osMessageQueueAttr_t Queue4_attributes = {
  .name = "Queue4"
};
/* Definitions for UARTmutex */
osMutexId_t UARTmutexHandle;
const osMutexAttr_t UARTmutex_attributes = {
  .name = "UARTmutex"
};
/* USER CODE BEGIN PV */

uint16_t CurrentRPM = 0;
uint16_t CurrentSpeed = 0;
uint16_t CurrentTemp = 0;

uint32_t lastRPMTime = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
void Function_CANReceiveTask(void *argument);
void Function_IDSDetectionTask(void *argument);
void Function_AlertTask(void *argument);
void Function_OLEDTask(void *argument);

/* USER CODE BEGIN PFP */

void CAN_Filter_Config(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

typedef struct
{
	uint32_t id;
	uint8_t  dlc;
	uint8_t  data[8];
	uint32_t timestamp;
}CAN_Frame_t;

typedef enum{
	ATTACK_UNKNOWN_ID = 0,
	ATTACK_INVALID_RPM,
	ATTACK_INVALID_SPEED,
	ATTACK_INVALID_TEMP,
	ATTACK_FLOOD
}AttackType_t;

typedef struct
{
	uint32_t id;
	uint16_t value;
	AttackType_t attackType;
	uint32_t timestamp;
}IDS_Event_t;	 // IDS --> Intrusion detection system

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
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  // filters are applied immediately when CAN leaves initialization mode.
  CAN_Filter_Config();

  if(HAL_CAN_Start(&hcan1) != HAL_OK)
  {
	  Error_Handler();
  }

	// API is use for enabling interrupt
	if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
	{
		Error_Handler();
	}

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of UARTmutex */
  UARTmutexHandle = osMutexNew(&UARTmutex_attributes);

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
  /* creation of Queue1 */
  Queue1Handle = osMessageQueueNew (16, sizeof(CAN_Frame_t), &Queue1_attributes);

  /* creation of Queue2 */
  Queue2Handle = osMessageQueueNew (16, sizeof(CAN_Frame_t), &Queue2_attributes);

  /* creation of Queue3 */
  Queue3Handle = osMessageQueueNew (16, sizeof(IDS_Event_t), &Queue3_attributes);

  /* creation of Queue4 */
  Queue4Handle = osMessageQueueNew (16, sizeof(CAN_Frame_t), &Queue4_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of CANReceiveTask */
  CANReceiveTaskHandle = osThreadNew(Function_CANReceiveTask, NULL, &CANReceiveTask_attributes);

  /* creation of IDSDetectionTas */
  IDSDetectionTasHandle = osThreadNew(Function_IDSDetectionTask, NULL, &IDSDetectionTas_attributes);

  /* creation of AlertTask */
  AlertTaskHandle = osThreadNew(Function_AlertTask, NULL, &AlertTask_attributes);

  /* creation of OLEDTask */
  OLEDTaskHandle = osThreadNew(Function_OLEDTask, NULL, &OLEDTask_attributes);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 6;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CS_I2C_SPI_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PDM_OUT_Pin */
  GPIO_InitStruct.Pin = PDM_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : I2S3_WS_Pin */
  GPIO_InitStruct.Pin = I2S3_WS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(I2S3_WS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_SCK_Pin SPI1_MISO_Pin SPI1_MOSI_Pin */
  GPIO_InitStruct.Pin = SPI1_SCK_Pin|SPI1_MISO_Pin|SPI1_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CLK_IN_Pin */
  GPIO_InitStruct.Pin = CLK_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : I2S3_MCK_Pin I2S3_SCK_Pin I2S3_SD_Pin */
  GPIO_InitStruct.Pin = I2S3_MCK_Pin|I2S3_SCK_Pin|I2S3_SD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : VBUS_FS_Pin */
  GPIO_InitStruct.Pin = VBUS_FS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(VBUS_FS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OTG_FS_ID_Pin OTG_FS_DM_Pin OTG_FS_DP_Pin */
  GPIO_InitStruct.Pin = OTG_FS_ID_Pin|OTG_FS_DM_Pin|OTG_FS_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

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

	if(HAL_CAN_ConfigFilter(&hcan1, &filterInit) != HAL_OK)
	{
		Error_Handler();
	}

}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_Frame_t frame;

	CAN_RxHeaderTypeDef RxHeader;
	uint8_t RxData[8];

	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxHeader, RxData);

	frame.id = RxHeader.StdId;
	frame.dlc = RxHeader.DLC;
	memcpy(frame.data, RxData, 8);
	frame.timestamp = HAL_GetTick();

	// Send Queue 1 -->
	osMessageQueuePut(Queue1Handle, &frame, 0, 0);
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_Function_CANReceiveTask */
/**
  * @brief  Function implementing the CANReceiveTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Function_CANReceiveTask */
void Function_CANReceiveTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	CAN_Frame_t frame;

  /* Infinite loop */
  for(;;)
  {
	  if(osMessageQueueGet(Queue1Handle, &frame, 0, osWaitForever) == osOK)
	  {
		  // Send to IDS
		  osMessageQueuePut(Queue2Handle, &frame, 0, 0);

		  // Send to OLED
		  osMessageQueuePut(Queue4Handle, &frame, 0, 0);
	  }
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_Function_IDSDetectionTask */
/**
* @brief Function implementing the IDSDetectionTas thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Function_IDSDetectionTask */
void Function_IDSDetectionTask(void *argument)
{
  /* USER CODE BEGIN Function_IDSDetectionTask */

	char msg[50];

	CAN_Frame_t frame;
	IDS_Event_t event;
	uint16_t value;

  /* Infinite loop */
  for(;;)
  {
	  if(osMessageQueueGet(Queue2Handle, &frame, 0, osWaitForever) == osOK)
	  {
		  value = (frame.data[0] << 8) | frame.data[1];

		  event.id = frame.id;
		  event.value = value;
		  event.timestamp = frame.timestamp;

		  uint8_t printuart = 0;

		  switch(frame.id)
		  {

		  case 0x100 :
		  {
			  uint32_t CurrentTime = HAL_GetTick();

			  if(lastRPMTime!=0 && (CurrentTime - lastRPMTime) < FLOOD_THRESHOLD_ms)
			  {
				  event.attackType = ATTACK_FLOOD;
				  osMessageQueuePut(Queue3Handle, &event, 0, 0);
			  }
			  else if(value > MAX_RPM)
			  {
				  event.attackType = ATTACK_INVALID_RPM;
				  osMessageQueuePut(Queue3Handle, &event, 0, 0);
			  }
			  else
			  {
				  sprintf(msg, "ID:0x100 RPM:%u\r\n", value);
				  printuart = 1;
			  }

			  // Update for next Tick
			  lastRPMTime = CurrentTime;

			  break;
		  }

		  case 0x200 :
		  {
			  sprintf(msg, "ID:0x200 SPEED:%u\r\n", value);
			  printuart = 1;
			  break;
		  }

		  case 0x300 :
		  {
			  sprintf(msg, "ID:0x300 TEMP:%u\r\n", value);
			  printuart = 1;
			  break;
		  }

		  default :
		  {
			  event.attackType = ATTACK_UNKNOWN_ID;
			  osMessageQueuePut(Queue3Handle, &event, 0, 0);
			  break;
		  }

		  }

		  if(printuart)
		  {
			  osMutexAcquire(UARTmutexHandle, osWaitForever);

			  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

			  osMutexRelease(UARTmutexHandle);
		  }


	  }
  }
  /* USER CODE END Function_IDSDetectionTask */
}

/* USER CODE BEGIN Header_Function_AlertTask */
/**
* @brief Function implementing the AlertTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Function_AlertTask */
void Function_AlertTask(void *argument)
{
  /* USER CODE BEGIN Function_AlertTask */

	char msg[50];

	IDS_Event_t event;

  /* Infinite loop */
  for(;;)
  {
	  if(osMessageQueueGet(Queue3Handle, &event, 0, osWaitForever) == osOK)
	  {
		  switch(event.attackType)
		  {

		  case ATTACK_UNKNOWN_ID :
		  {
			  sprintf(msg, "****ATTACK UNKOWN ID:0x%03lX****\r\n", event.id);
			  break;
		  }

		  case ATTACK_INVALID_RPM :
		  {
			  sprintf(msg, "****ATTACK INVALID RPM:%u****\r\n", event.value);
			  break;
		  }

		  case ATTACK_FLOOD :
		  {
			  sprintf(msg, "****FLOOD ATTACK****\r\n");
			  break;
		  }

		  default:
			  break;

		  }

		  osMutexAcquire(UARTmutexHandle, osWaitForever);

		  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

		  osMutexRelease(UARTmutexHandle);

		  // Turn ON LED
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);

		  osDelay(500);

		  // Turn OFF LED
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
	  }
  }
  /* USER CODE END Function_AlertTask */
}

/* USER CODE BEGIN Header_Function_OLEDTask */
/**
* @brief Function implementing the OLEDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Function_OLEDTask */
void Function_OLEDTask(void *argument)
{
  /* USER CODE BEGIN Function_OLEDTask */

	CAN_Frame_t frame;
	uint16_t value;

  /* Infinite loop */
  for(;;)
  {
	  if(osMessageQueueGet(Queue4Handle, &frame, 0, osWaitForever) == osOK)
	  {
		  value = (frame.data[0] << 8) | frame.data[1];

		  switch(frame.id)
		  {
		  case 0x100 :
			  CurrentRPM = value;
			  break;

		  case 0x200 :
			  CurrentSpeed = value;
			  break;

		  case 0x300 :
			  CurrentTemp = value;
			  break;
		  }

          SSD1306_Fill(SSD1306_COLOR_BLACK);

          char line[20];

          sprintf(line, "RPM : %u", CurrentRPM);
          SSD1306_GotoXY(0, 0);
          SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

          sprintf(line, "SPD : %u", CurrentSpeed);
          SSD1306_GotoXY(0, 16);
          SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

          sprintf(line, "TMP : %u", CurrentTemp);
          SSD1306_GotoXY(0, 32);
          SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

          SSD1306_GotoXY(0, 48);
          SSD1306_Puts("STATUS: NORMAL", &Font_7x10, SSD1306_COLOR_WHITE);

          SSD1306_UpdateScreen();
	  }
  }
  /* USER CODE END Function_OLEDTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
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

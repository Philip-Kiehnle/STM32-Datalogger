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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>


#include <config.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define DATALOGGER_VERSION_MAJOR 1
#define DATALOGGER_VERSION_MINOR 2
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc2;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
uint16_t ADC2ConvertedData[MAX_CHANNELS*FRAMES];

volatile bool fast_mon_vars_en;
uint8_t decimation_factor;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void UARTprintf(const char *pcString, ...)
{
#define BUF_MAXLEN 1024
    char buf[BUF_MAXLEN];
    va_list args;
    va_start(args, pcString);
    int len = vsnprintf(buf, BUF_MAXLEN, pcString, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 10);
}


void print_help()
{
	UARTprintf("\n");
	UARTprintf("Datalogger v%d.%d\n", DATALOGGER_VERSION_MAJOR, DATALOGGER_VERSION_MINOR);
	UARTprintf("Build: %s %s\n", __DATE__, __TIME__);
	UARTprintf("Commands:\n");
	UARTprintf("s: Start monitoring stream\n");
	UARTprintf("x: Stop monitoring stream\n");
	UARTprintf("t: Two channel mode\n");
	UARTprintf("f: Four channel mode\n");
	UARTprintf("d: Set decimation factor + number\n");
	UARTprintf("o: Set output state + number\n");
	UARTprintf("0-9: Set decimation factor or output state\n");
	UARTprintf("r: Read settings\n");
	UARTprintf("h: Print help (this text)\n");
}


void print_output_state()
{
	if (fast_mon_vars_en == false) {
		UARTprintf("Output state: PA10=%d PA9=%d\n", HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10), HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
	}
}


void start_stream()
{
	/* Start ADC group regular conversion with DMA */
	if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&ADC2ConvertedData, sizeof(ADC2ConvertedData)/sizeof(ADC2ConvertedData[0])) != HAL_OK)
	{
	  /* ADC conversion start error */
	  Error_Handler();
	}
	fast_mon_vars_en = true;
}


void stop_stream()
{
	if (HAL_ADC_Stop_DMA(&hadc2) != HAL_OK)
	{
	  Error_Handler();
	}
	fast_mon_vars_en = false;
}



/**
  * @brief  Tx Transfer completed callback
  * @param  huart: UART handle.
  * @note   This example shows a simple way to report end of DMA Tx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
}


/**
  * @brief  UART error callbacks
  * @param  huart: UART handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  /* Turn LED off: Transfer error in reception/transmission process */
  GPIOB->BRR = (1<<8);  // disable green LED
  while(1) {

  }
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	DEBUG_ISR GPIOA->BSRR = (1<<12);  // set Testpin PA12 (yellow LED) T0

	/********************************************************/
	/* Highest priority: Get ADC samples					*/
	/* 	execution rate: 50Hz (4chn FRAMES=443)				*/
	/* 	runtime: 3.24us										*/
	/********************************************************/

	// ADC2: four channels
	// main triggers ADC and DMA calls this callback if ADC2ConvertedData[] is full
	//if (hadc == &hadc1) {  // only one ADC is used, so dont need to check

	if (fast_mon_vars_en) {
		if (HAL_UART_Transmit_DMA( &huart2, (uint8_t*)ADC2ConvertedData, sizeof(ADC2ConvertedData)) == HAL_OK) {  // DMA config: Byte<->Byte -> working
		}
//		if (HAL_UART_Transmit_DMA( &huart2, (uint8_t*)ADC2ConvertedData, sizeof(ADC2ConvertedData)/2) == HAL_OK) {  // DMA config: Periph:Byte<->Mem:Half-Word -> does not work
//		}
	}

//	while (HAL_UART_GetState(&huart2) != HAL_UART_STATE_READY)
//	{
//	}
	DEBUG_ISR GPIOA->BRR = (1<<12);  // reset Testpin PA12 (yellow LED) T1: ~xxus after T0

}
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  GPIOB->BSRR = (1<<8);  // enable green LED
  print_help();
  fast_mon_vars_en = false;
  decimation_factor = 1;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	// UART RX
	uint8_t rx_buf = 0;
	HAL_UART_Receive(&huart2, &rx_buf, 1, 1);
	static bool set_decimation = false;
	static bool set_output_state = false;
	static bool two_chn_mode = false;

	if (rx_buf == 's') {
		UARTprintf("Monitoring stream START\n");
		start_stream();

	} else if (rx_buf == 'x') {  // stop monitoring stream
		stop_stream();

	} else if (rx_buf == 't') {
		two_chn_mode = true;
		stop_stream();
		HAL_Delay(100);
		hadc2.Init.NbrOfConversion = 2;
		if (HAL_ADC_Init(&hadc2) != HAL_OK)
		{
			Error_Handler();
		}

	} else if (rx_buf == 'q') {
		two_chn_mode = false;
		stop_stream();
		HAL_Delay(100);
		hadc2.Init.NbrOfConversion = 4;
		if (HAL_ADC_Init(&hadc2) != HAL_OK)
		{
			Error_Handler();
		}

	} else if (rx_buf == 'd') {
		set_decimation = true;
		set_output_state = false;
		UARTprintf("Enter decimation factor\n");

	} else if (rx_buf == 'o') {
		set_decimation = false;
		set_output_state = true;
		UARTprintf("Enter output state:\n"
				"0: PA10=0 PA9=0\n"
				"1: PA10=0 PA9=1\n"
				"2: PA10=1 PA9=0\n"
				"3: PA10=1 PA9=1\n");

	} else if (set_output_state) {

		if (rx_buf == '0') {        // GPIO 00 PA10=0 PA9=0
			GPIOA->BRR = (1<<9);
			GPIOA->BRR = (1<<10);
			print_output_state();
			set_output_state = false;

		} else if (rx_buf == '1') {  // GPIO 01 PA10=0 PA9=1
			GPIOA->BSRR = (1<<9);
			GPIOA->BRR = (1<<10);
			print_output_state();
			set_output_state = false;

		} else if (rx_buf == '2') {  // GPIO 10 PA10=1 PA9=0
			GPIOA->BRR = (1<<9);
			GPIOA->BSRR = (1<<10);
			print_output_state();
			set_output_state = false;

		} else if (rx_buf == '3') {  // GPIO 11 PA10=1 PA9=1
			GPIOA->BSRR = (1<<9);
			GPIOA->BSRR = (1<<10);
			print_output_state();
			set_output_state = false;
  		} else if (rx_buf != 0) {
  			set_output_state = false;
  		}

	} else if (set_decimation) {
		int8_t new_decimation_factor = -1;

		if (rx_buf == '0' || rx_buf == '1' || rx_buf == '2' || rx_buf == '4' || rx_buf == '8') {
			new_decimation_factor = rx_buf - '0';
			if (new_decimation_factor == 0)
				new_decimation_factor = 1;
			set_decimation = false;
			UARTprintf("Decimation factor = %d\n", new_decimation_factor);

  		} else if (rx_buf >= '0' && rx_buf <= '9') {
  			UARTprintf("Supported decimation factors: 0/1,2,4,8\n");

  		} else if (rx_buf != 0) {
  			set_decimation = false;
  		}

  		if (   new_decimation_factor != -1  // valid decimation factor received
  			&& new_decimation_factor != decimation_factor
		   ){

			stop_stream();
			HAL_Delay(100);

			if (new_decimation_factor == 1) {  // 4chn: 22.1 ksps; 2chn: 44.27 ksps
				hadc2.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_32;
				hadc2.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_1;
			} else if (new_decimation_factor == 2) {  // 4chn: 11.1 ksps; 2chn: 22.1 ksps
				hadc2.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_64;
				hadc2.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_2;
			} else if (new_decimation_factor == 4) {  // 4chn: 5.54 ksps; 2chn: 11.1 ksps
				hadc2.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_128;
				hadc2.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_3;
			} else if (new_decimation_factor == 8) {  // 4chn: 2.77 ksps; 2chn: 5.54 ksps
				hadc2.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_256;
				hadc2.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_4;
			}

			if (HAL_ADC_Init(&hadc2) != HAL_OK)
			{
				Error_Handler();
			}
			decimation_factor = new_decimation_factor;
  		}

	} else if (rx_buf == 'r' && fast_mon_vars_en == false ) {
		UARTprintf("Decimation factor = %d\n", decimation_factor);
		print_output_state();

	} else if (rx_buf == 'h' && fast_mon_vars_en == false) {
		print_help();
  	}
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.GainCompensation = 0;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = ENABLE;
  hadc2.Init.NbrOfConversion = 4;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.DMAContinuousRequests = ENABLE;
  hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc2.Init.OversamplingMode = ENABLE;
  hadc2.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_32;
  hadc2.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_1;
  hadc2.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc2.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_DIFFERENTIAL_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_17;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

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
  huart2.Init.BaudRate = 2000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_EnableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA9 PA10 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
	/* Toggle LED for error */
	GPIOB->ODR ^= (1 << 8);  // toggle green LED
	HAL_Delay(500);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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

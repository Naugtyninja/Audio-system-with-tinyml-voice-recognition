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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define WM8960_ADDR (0x1A << 1)
#define SAMPLE_RATE 48000
#define SINE_FREQ   1000
#define SINE_SAMPLES 48
#define AUDIO_BUFFER_SIZE 96

int16_t audio_buffer[AUDIO_BUFFER_SIZE];






void scan_once(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef result;

    printf("Single scan...\r\n");

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        result = HAL_I2C_IsDeviceReady(hi2c, (addr << 1), 2, 50);

        if (result == HAL_OK)
        {
            printf("FOUND at 0x%02X\r\n", addr);
            return;
        }
    }

    printf("Nothing found\r\n");
}

 // 1 cycle simplified

int16_t sine_table[SINE_SAMPLES] = {
     0,  4276,  8480, 12539, 16383, 19947, 23169, 25995,
 28377, 30272, 31650, 32487, 32767, 32487, 31650, 30272,
 28377, 25995, 23169, 19947, 16383, 12539,  8480,  4276,
     0, -4276, -8480,-12539,-16383,-19947,-23169,-25995,
-28377,-30272,-31650,-32487,-32767,-32487,-31650,-30272,
-28377,-25995,-23169,-19947,-16383,-12539, -8480, -4276
};

void fill_audio_buffer(void)
{
    static int index = 0;

    for (int i = 0; i < AUDIO_BUFFER_SIZE; i += 2)
    {
        int16_t sample = sine_table[index];

        audio_buffer[i] = sample;     // Left
        audio_buffer[i+1] = sample;   // Right

        index++;
        if (index >= SINE_SAMPLES)
            index = 0;
    }
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S1_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

HAL_StatusTypeDef WM8960_Write(uint8_t reg, uint16_t value)
{
    uint8_t data[2];

    data[0] = (reg << 1) | ((value >> 8) & 0x01);
    data[1] = value & 0xFF;

    return HAL_I2C_Master_Transmit(&hi2c1, 0x1A << 1, data, 2, 100);
}

void WM8960_Init(void)
{
    HAL_StatusTypeDef ret;

    ret = WM8960_Write(0x0F, 0x000); // Reset
    printf("Reset: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    HAL_Delay(10);

    // Clocking: SYSCLK = MCLK directly, no PLL, no divider
    // ADCDIV=000, DACDIV=000, SYSCLKDIV=00, CLKSEL=0
    ret = WM8960_Write(0x04, 0x000);
    printf("Clocking1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // Clocking2: DCLKDIV=111 (SYSCLK/16), BCLKDIV=0000
    // WM8960 is SLAVE so BCLKDIV doesn't matter, but set safe value
    ret = WM8960_Write(0x08, 0x1C0);
    printf("Clocking2: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // Power 1: VMIDSEL=01, VREF=1
    ret = WM8960_Write(0x19, 0x0C0);
    printf("Power1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    HAL_Delay(100); // Let VMID charge up

    // Power 2: DACL, DACR, LOUT1, ROUT1
    ret = WM8960_Write(0x1A, 0x1E0);
    printf("Power2: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // Power 3: LOMIX, ROMIX
    ret = WM8960_Write(0x2F, 0x00C);
    printf("Power3: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // Audio format: I2S, 16-bit, SLAVE mode (MS=0)
    ret = WM8960_Write(0x07, 0x002);
    printf("Format: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // DAC unmute
    ret = WM8960_Write(0x05, 0x000);
    printf("DAC unmute: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // DAC digital volume max + update
    ret = WM8960_Write(0x0A, 0x1FF);
    printf("LDAC vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    ret = WM8960_Write(0x0B, 0x1FF);
    printf("RDAC vol: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // Output mixer: DAC -> output
    ret = WM8960_Write(0x22, 0x100);
    printf("LMIX: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    ret = WM8960_Write(0x25, 0x100);
    printf("RMIX: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    // Headphone volume 0dB + update
    ret = WM8960_Write(0x02, 0x179);
    printf("LOUT1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");
    ret = WM8960_Write(0x03, 0x179);
    printf("ROUT1: %s\r\n", ret == HAL_OK ? "OK" : "FAIL");

    printf("WM8960 init done\r\n");
}

void WM8960_Enable_Speaker(void)
{
    // Power 2: add SPKL and SPKR bits to existing DAC+LOUT1+ROUT1
    // DACL=1, DACR=1, LOUT1=1, ROUT1=1, SPKL=1, SPKR=1
    WM8960_Write(0x1A, 0x1F8);

    // Speaker volume: 0dB + update bit (SPKVU=1)
    // 0x179 = SPKVU(bit8)=1, volume=0x79 (0dB)
    WM8960_Write(0x28, 0x179);  // Left speaker volume
    WM8960_Write(0x29, 0x179);  // Right speaker volume

    // Enable Class D outputs: left + right
    WM8960_Write(0x31, 0x0C0);  // SPK_OP_EN[1:0] = 11

    printf("Speaker enabled\r\n");
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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_BLUE);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  scan_once(&hi2c1);
  WM8960_Init();
  HAL_Delay(100);
  WM8960_Enable_Speaker();
  HAL_Delay(100);
  if (HAL_I2S_GetState(&hi2s1) == HAL_I2S_STATE_READY)
  {
      printf("I2S ready\r\n");
  }
  else
  {
      printf("I2S NOT ready\r\n");
  }

//
//  HAL_I2S_Receive(&hi2s1,(uint16_t*)audio_buffer,
//  	                                     AUDIO_BUFFER_SIZE,
//  	                                     HAL_MAX_DELAY);

  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */



	      fill_audio_buffer();
	      HAL_I2S_Transmit(&hi2s1,(uint16_t*)audio_buffer,
	                                     AUDIO_BUFFER_SIZE,
	                                     HAL_MAX_DELAY);




	  	     // BSP_LED_Toggle(LED_RED);



//	  fill_audio_buffer();
//	  HAL_I2S_Transmit(&hi2s1,
//	                  (uint16_t*)audio_buffer,
//	                        AUDIO_BUFFER_SIZE,
//	                        HAL_MAX_DELAY);

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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 9;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 1;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_TIM;
  PeriphClkInitStruct.TIMPresSelection = RCC_TIMPRES_ACTIVATED;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
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
  hi2c1.Init.Timing = 0x10707DBC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_DISABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2S1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S1_Init(void)
{

  /* USER CODE BEGIN I2S1_Init 0 */

  /* USER CODE END I2S1_Init 0 */

  /* USER CODE BEGIN I2S1_Init 1 */

  /* USER CODE END I2S1_Init 1 */
  hi2s1.Instance = SPI1;
  hi2s1.Init.Mode = I2S_MODE_MASTER_FULLDUPLEX;
  hi2s1.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s1.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s1.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s1.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s1.Init.CPOL = I2S_CPOL_LOW;
  hi2s1.Init.FirstBit = I2S_FIRSTBIT_MSB;
  hi2s1.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
  hi2s1.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
  hi2s1.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;
  if (HAL_I2S_Init(&hi2s1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S1_Init 2 */

  /* USER CODE END I2S1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

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

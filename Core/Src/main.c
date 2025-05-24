/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file               : main.c
  * @brief              : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// #include "usbd_cdc_if.h" // USB CDCを廃止
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// SWState_t は直接フラグで管理するため、必須ではなくなりました。
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AUDIO_SAMPLING_FREQUENCY 16000
#define AUDIO_BUFFER_SIZE (AUDIO_SAMPLING_FREQUENCY * 1) // 1秒分のバッファサイズ
#define SOUND_AVOID_DELAY_MS 150
#define PRINT_DATA_COUNT 8//000 // 表示する先頭データの数
#define PRINT_DATA_OFFSET 0 // 表示する先頭データ
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

DFSDM_Filter_HandleTypeDef hdfsdm1_filter0;
DFSDM_Channel_HandleTypeDef hdfsdm1_channel0;
DMA_HandleTypeDef hdma_dfsdm1_flt0;

/* USER CODE BEGIN PV */
int cnt_it; // デバッグ用カウンタ
volatile uint32_t last_interrupt_time = 0; // 最後に割り込みが発生した時刻
const uint32_t debounce_delay = 300;      // ディバウンス時間 (ms)
int32_t audio_buffer[AUDIO_BUFFER_SIZE]; // オーディオデータバッファ

// SW押下によるDMA開始要求フラグ
volatile uint8_t transmit_request = 0;
// DMA完了によるデータ表示要求フラグ
volatile uint8_t data_ready_to_print = 0;

// DFSDM DMA操作のステータスを保持する変数
HAL_StatusTypeDef dfsm_start_dma_status;
HAL_StatusTypeDef dfsm_stop_dma_status;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DFSDM1_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_DMA_Init();
  MX_DFSDM1_Init();
  /* USER CODE BEGIN 2 */
//  // DFSDM+DMAはここでは開始しない。SW押下時に開始する。
//  printf("System Initialized. Waiting for button press...\r\n");
  // MX_USB_DEVICE_Init(); // USB CDCを廃止
//  printf("System Initialized. Performing initial DFSDM DMA start test...\r\n");
//  dfsm_start_dma_status = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0, audio_buffer, AUDIO_BUFFER_SIZE);
//  printf("Initial HAL_DFSDM_FilterRegularStart_DMA status: %d\r\n", dfsm_start_dma_status);
//  if (dfsm_start_dma_status != HAL_OK) {
//      printf("Initial DFSDM DMA Start Error!\r\n");
//      // Error_Handler(); // ここではエラーハンドラを呼ばずに継続させる
//  } else {
//      printf("Initial DFSDM DMA started successfully. Stopping DMA...\r\n");
//      HAL_DFSDM_FilterRegularStop_DMA(&hdfsdm1_filter0); // テストなので止めておく
//  }
  printf("Waiting for button press...\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // SW押下によるDMA開始要求をチェック
    if (transmit_request) {
      transmit_request = 0; // フラグをクリア
      HAL_Delay(SOUND_AVOID_DELAY_MS); // スイッチ音回避のための遅延

      printf("Button pressed. Re-initializing peripherals and starting DFSDM DMA...\r\n");

      // DFSDMとDMAをDeInit
      // DFSDM Filter DeInit (これにより関連するMSP DeInitが呼ばれ、DMA DeInitも行われるはず)
      if (HAL_DFSDM_FilterDeInit(&hdfsdm1_filter0) != HAL_OK) {
        printf("DFSDM Filter DeInit Error!\r\n");
        Error_Handler();
      }
      //念のためDMAストリームもDeInit (DFSDMのMSP DeInitでカバーされていれば不要な場合もある)
      //hdfsdm1_filter0.hdmaReg は HAL_DFSDM_FilterInit 時にリンクされるDMAハンドル
      //DeInit前にこのハンドルが有効であることを確認
      if (hdfsdm1_filter0.hdmaReg != NULL && HAL_DMA_DeInit(hdfsdm1_filter0.hdmaReg) != HAL_OK) {
          printf("DMA DeInit Error!\r\n");
          // Error_Handler(); // DMA DeInitエラーは必ずしも致命的ではない場合もある
      }
      // ★★★ チャンネルのDeInitを追加 ★★★
      if (HAL_DFSDM_ChannelDeInit(&hdfsdm1_channel0) != HAL_OK) {
        printf("DFSDM Channel DeInit Error!\r\n");
        Error_Handler();
      }
      // ★★★-------------------------★★★

      // DFSDMとDMAを再初期化
      // MX_DMA_Init() はDMAコントローラのクロックやIRQを再設定します。
      // DMAハンドル(hdma_dfsdm1_flt0)の再設定はHAL_DFSDM_FilterMspInitで行われます。
      MX_DMA_Init();

      hdma_dfsdm1_flt0.Instance = DMA2_Stream0; // 念のためインスタンスも再設定
      hdma_dfsdm1_flt0.Init.Mode = DMA_NORMAL;  // DMAモードをノーマルに設定

      // MX_DFSDM1_Init() はDFSDMのレジスタを再設定し、HAL_DFSDM_FilterMspInitを呼び出して
      // GPIOやDMAの再設定を行います。
      MX_DFSDM1_Init();

      // DFSDM+DMA をノーマルモードで開始
      // audio_bufferにAUDIO_BUFFER_SIZE分のデータを格納
      dfsm_start_dma_status = HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0, audio_buffer, AUDIO_BUFFER_SIZE);
      printf("HAL_DFSDM_FilterRegularStart_DMA status: %d (0=OK, 1=ERROR, 2=BUSY, 3=TIMEOUT)\r\n", dfsm_start_dma_status);

      if (dfsm_start_dma_status != HAL_OK)
      {
        printf("DFSDM DMA Start Error! Status: %d\r\n", dfsm_start_dma_status);
        // Error_Handler(); // エラー内容に応じて処理
      } else {
        printf("DFSDM DMA started. Acquiring audio data...\r\n");
      }
    }

    // DMA完了によるデータ表示要求をチェック
    if (data_ready_to_print) {
      data_ready_to_print = 0; // フラグをクリア

      printf("DFSDM DMA Stop status from callback: %d (0=OK, 1=ERROR, 2=BUSY, 3=TIMEOUT)\r\n", dfsm_stop_dma_status);
      if (dfsm_stop_dma_status != HAL_OK && dfsm_stop_dma_status != HAL_BUSY) { // BUSYはDMAが既に止まっている場合など
          printf("DFSDM DMA Stop Error occurred in callback! Status: %d\r\n", dfsm_stop_dma_status);
      }

      printf("Audio data acquisition complete. First %d samples:\r\n", PRINT_DATA_COUNT);
      for (int i = PRINT_DATA_OFFSET; i < PRINT_DATA_OFFSET+PRINT_DATA_COUNT && i < AUDIO_BUFFER_SIZE; i++) {
        printf("Sample %2d: %ld\r\n", i, audio_buffer[i]);
      }
      printf("\nData printing complete. Waiting for next button press...\r\n");
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DFSDM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DFSDM1_Init(void)
{

  /* USER CODE BEGIN DFSDM1_Init 0 */
  // この関数はDeInit後に再呼び出しされるため、毎回設定が適用される
  /* USER CODE END DFSDM1_Init 0 */

  /* USER CODE BEGIN DFSDM1_Init 1 */

  /* USER CODE END DFSDM1_Init 1 */
  hdfsdm1_filter0.Instance = DFSDM1_Filter0;
  hdfsdm1_filter0.Init.RegularParam.Trigger = DFSDM_FILTER_SW_TRIGGER;
  hdfsdm1_filter0.Init.RegularParam.FastMode = ENABLE;
  hdfsdm1_filter0.Init.RegularParam.DmaMode = ENABLE;
  hdfsdm1_filter0.Init.FilterParam.SincOrder = DFSDM_FILTER_SINC3_ORDER;
  hdfsdm1_filter0.Init.FilterParam.Oversampling = 125;
  hdfsdm1_filter0.Init.FilterParam.IntOversampling = 1;
  HAL_DFSDM_FilterInit(&hdfsdm1_filter0);
  hdfsdm1_channel0.Instance = DFSDM1_Channel0;
  hdfsdm1_channel0.Init.OutputClock.Activation = ENABLE;
  hdfsdm1_channel0.Init.OutputClock.Selection = DFSDM_CHANNEL_OUTPUT_CLOCK_SYSTEM;
  hdfsdm1_channel0.Init.OutputClock.Divider = 8;
  hdfsdm1_channel0.Init.Input.Multiplexer = DFSDM_CHANNEL_EXTERNAL_INPUTS;
  hdfsdm1_channel0.Init.Input.DataPacking = DFSDM_CHANNEL_STANDARD_MODE;
  hdfsdm1_channel0.Init.Input.Pins = DFSDM_CHANNEL_SAME_CHANNEL_PINS;
  hdfsdm1_channel0.Init.SerialInterface.Type = DFSDM_CHANNEL_SPI_FALLING;
  hdfsdm1_channel0.Init.SerialInterface.SpiClock = DFSDM_CHANNEL_SPI_CLOCK_INTERNAL;
  hdfsdm1_channel0.Init.Awd.FilterOrder = DFSDM_CHANNEL_FASTSINC_ORDER;
  hdfsdm1_channel0.Init.Awd.Oversampling = 1;
  hdfsdm1_channel0.Init.Offset = 0;
  hdfsdm1_channel0.Init.RightBitShift = 0x02;
  if (HAL_DFSDM_ChannelInit(&hdfsdm1_channel0) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_DFSDM_FilterConfigRegChannel(&hdfsdm1_filter0, DFSDM_CHANNEL_0, DFSDM_CONTINUOUS_CONV_ON);
  /* USER CODE BEGIN DFSDM1_Init 2 */
  // HAL_DFSDM_FilterInitの中でMSPInitが呼ばれ、その中でDMAの初期化とリンクが行われる。
  // hdma_dfsdm1_flt0 がこのDFSDMフィルターにリンクされる。
  /* USER CODE END DFSDM1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_REF_CLK_Pin RMII_MDIO_Pin RMII_CRS_DV_Pin */
  GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_RXD0_Pin RMII_RXD1_Pin */
  GPIO_InitStruct.Pin = RMII_RXD0_Pin|RMII_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_TXD1_Pin */
  GPIO_InitStruct.Pin = RMII_TXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : STLK_RX_Pin STLK_TX_Pin */
  GPIO_InitStruct.Pin = STLK_RX_Pin|STLK_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_SOF_Pin USB_ID_Pin USB_DM_Pin USB_DP_Pin */
  GPIO_InitStruct.Pin = USB_SOF_Pin|USB_ID_Pin|USB_DM_Pin|USB_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_TX_EN_Pin RMII_TXD0_Pin */
  GPIO_InitStruct.Pin = RMII_TX_EN_Pin|RMII_TXD0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief EXTI Line Detection Callback
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == USER_Btn_Pin) {
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_interrupt_time > debounce_delay) {
      // 前の処理 (DMA開始要求 or データ表示要求) が完了しているか確認
      if (transmit_request == 0 && data_ready_to_print == 0) {
          transmit_request = 1; // DMA開始要求フラグをセット
          last_interrupt_time = current_time;
          cnt_it++; // デバッグ用カウンタ
          HAL_GPIO_TogglePin(GPIOB, LD1_Pin); // ボタン反応表示用LED (緑)
      } else {
          // 前の処理がまだ終わっていない場合は、チャタリングによる連続押下とみなし、
          // フラグは立てずにデバウンス時間のみ更新する
          printf("Button press ignored, previous operation still in progress or finishing.\r\n");
          last_interrupt_time = current_time;
      }
    }
  }
}

/**
  * @brief Regular conversion half DMA transfer complete callback.
  * @note This callback is not typically used in Normal DMA mode for a single buffer.
  * @param hdfsdm_filter : DFSDM filter handle.
  * @retval None
  */
void HAL_DFSDM_FilterRegConvHalfCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
  /* USER CODE BEGIN FilterRegConvHalfCpltCallback */
  // ノーマルモードで単一バッファの場合、このコールバックは通常使用しません。
  if (hdfsdm_filter == &hdfsdm1_filter0) {
    // 何もしない
  }
  /* USER CODE END FilterRegConvHalfCpltCallback */
}

/**
  * @brief Regular conversion complete DMA transfer callback.
  * @param hdfsdm_filter : DFSDM filter handle.
  * @retval None
  */
void HAL_DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
  printf("!!! HAL_DFSDM_FilterRegConvCpltCallback Entered !!!\r\n"); // ★追加
  HAL_GPIO_TogglePin(GPIOB, LD2_Pin); // ★追加 (LD2が青色LEDの場合など、別のLEDで試す)
  /* USER CODE BEGIN FilterRegConvCpltCallback */
  if (hdfsdm_filter == &hdfsdm1_filter0) {
    // DMA転送が完了したので、DFSDMのDMAを停止する
    // HAL_DFSDM_FilterRegularStop_DMA() はブロッキング関数であることに注意。
    // ノーマルモードなのでDMAは既に停止しているはずだが、状態をクリーンにするために呼び出す。
    dfsm_stop_dma_status = HAL_DFSDM_FilterRegularStop_DMA(hdfsdm_filter);
    // dfsm_stop_dma_status はグローバル変数に保存し、メインループでその値を表示。

    data_ready_to_print = 1; // メインループでデータを表示するためのフラグをセット

    // 注意: 割り込みハンドラ内での printf のような時間のかかる処理は避けるのが一般的です。
    // ここではフラグを立て、メインループで実際の表示処理を行います。
  }
  /* USER CODE END FilterRegConvCpltCallback */
}


void HAL_DFSDM_FilterErrorCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter) {
    if (hdfsdm_filter == &hdfsdm1_filter0) {
        printf("!!! DFSDM Filter Error Callback! ErrorCode: 0x%lX !!!\r\n", (unsigned long)hdfsdm_filter->ErrorCode);
        HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);
        HAL_DFSDM_FilterRegularStop_DMA(&hdfsdm1_filter0);
    }
}


/**
  * @brief  Retargets the C library printf function to the ITM.
  * @param  file: Not used.
  * @param  ptr: Pointer to characters to output.
  * @param  len: Number of characters to output.
  * @retval The number of characters output.
  */
int _write(int file, char *ptr, int len)
{
  int DataIdx;
  for(DataIdx=0; DataIdx<len; DataIdx++)
  {
    ITM_SendChar(*ptr++);
  }
  return len;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  printf("\r\n!!! System Error: Entered Error_Handler() !!!\r\n");
  __disable_irq(); // 割り込みを無効化
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOB, LD3_Pin); // エラー表示用LED (例: LD3 赤色) を点滅
    HAL_Delay(200); // 点滅間隔
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
  printf("Wrong parameters value: file %s on line %lu\r\n", file, line);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

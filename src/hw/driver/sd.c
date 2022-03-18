/*
 * sd.c
 *
 *  Created on: 2021. 11. 16.
 *      Author: 82109
 */




#include "sd.h"
#include "gpio.h"
#include "cli.h"



#ifdef _USE_HW_SD





static bool is_init = false;
static bool is_detected = false;
static volatile bool is_rx_done = false;
static volatile bool is_tx_done = false;
static uint8_t is_try = 0;
static sd_state_t sd_state = SDCARD_IDLE;


SD_HandleTypeDef hsd1;
//DMA_HandleTypeDef hdma_sdio_rx;
//DMA_HandleTypeDef hdma_sdio_tx;

#define hsd hsd1

#ifdef _USE_HW_CLI
static void cliSd(cli_args_t *args);
#endif


bool sdInit(void)
{
  bool ret = false;

   // From Sdio.c //
  hsd.Instance                 = SDMMC1;
  hsd.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
  hsd.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide             = SDMMC_BUS_WIDE_4B;
  hsd.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv            = SDMMC_NSpeed_CLK_DIV;
  hsd.Init.TranceiverPresent 	 = SDMMC_TRANSCEIVER_NOT_PRESENT;


  is_detected = sdIsDetected();

  if (is_detected == true) // sd가 검출 될 때만 초기화//
  {

  	 cliPrintf("sdCard     \t\t: connected\r\n");

  	 HAL_SD_DeInit(&hsd);

    if (HAL_SD_Init(&hsd) == HAL_OK)
    {

        ret = true;
        cliPrintf("sdCardInit  \t\t: OK\r\n");

    }
  }
  else
	{
  	cliPrintf("sdCard     \t\t: not connected\r\n");
	}

  is_init = ret;


#ifdef _USE_HW_CLI
  cliAdd("sd", cliSd);
#endif

  return ret;
}

bool sdReInit(void)
{
  bool ret = false;

  HAL_SD_DeInit(&hsd);  // 초기화 해체
  if (HAL_SD_Init(&hsd) == HAL_OK)  // SD 초기화
  {
      ret = true;
  }

  is_init = ret;

  return ret;
}

bool sdDeInit(void)
{
  bool ret = false;

  if (is_init == true)
  {
    is_init = false;
    if (HAL_SD_DeInit(&hsd) == HAL_OK)
    {
      ret = true;
    }

    HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
    __HAL_RCC_SDMMC1_CLK_DISABLE();

  }

  return ret;
}

bool sdIsInit(void)
{
  return is_init;
}

bool sdIsDetected(void)
{
  if (gpioPinRead(_PIN_GPIO_SDCARD_DETECT) == true)
  {
    is_detected = true;
  }
  else
  {
    is_detected = false;
  }

  return is_detected;
}

sd_state_t sdUpdate(void)
{
  sd_state_t ret_state = SDCARD_IDLE;
  static uint32_t pre_time;


  switch(sd_state)
  {
    case SDCARD_IDLE:
      if (sdIsDetected() == true)
      {
        if (is_init)
        {
          sd_state = SDCARD_CONNECTED;
        }
        else
        {
          sd_state = SDCARD_CONNECTTING;
          pre_time = millis();
        }
      }
      else
      {
        is_init = false;
        sd_state  = SDCARD_DISCONNECTED;
        ret_state = SDCARD_DISCONNECTED;
      }
      break;

    case SDCARD_CONNECTTING:
      if (millis()-pre_time >= 100)
      {
        if (sdReInit())
        {
          sd_state  = SDCARD_CONNECTED;
          ret_state = SDCARD_CONNECTED;
        }
        else
        {
          sd_state = SDCARD_IDLE;
          is_try++;

          if (is_try >= 3)
          {
            sd_state = SDCARD_ERROR;
          }
        }
      }
      break;

    case SDCARD_CONNECTED:
      if (sdIsDetected() != true)
      {
        is_try = 0;
        sd_state = SDCARD_IDLE;
      }
      break;

    case SDCARD_DISCONNECTED:
      if (sdIsDetected() == true)
      {
        sd_state = SDCARD_IDLE;
      }
      break;

    case SDCARD_ERROR:
      break;
  }

  return ret_state;
}

bool sdGetInfo(sd_info_t *p_info)
{
  bool ret = false;
  sd_info_t *p_sd_info = (sd_info_t *)p_info;  // 새로운 구조체에 재정의  //

  HAL_SD_CardInfoTypeDef card_info;


  if (is_init == true)
  {
    HAL_SD_GetCardInfo(&hsd, &card_info);

    p_sd_info->card_type          = card_info.CardType;
    p_sd_info->card_version       = card_info.CardVersion;
    p_sd_info->card_class         = card_info.Class;
    p_sd_info->rel_card_Add       = card_info.RelCardAdd;
    p_sd_info->block_numbers      = card_info.BlockNbr;
    p_sd_info->block_size         = card_info.BlockSize;
    p_sd_info->log_block_numbers  = card_info.LogBlockNbr;
    p_sd_info->log_block_size     = card_info.LogBlockSize;
    p_sd_info->card_size          =  (uint32_t)((uint64_t)p_sd_info->block_numbers * (uint64_t)p_sd_info->block_size / (uint64_t)1024 / (uint64_t)1024);
    ret = true;                       // 블럭 * 블럭 사이즈 = 용량  --> 용량 / 1024 = kByte   용량  /1024 /1024 = Mbyte
  }

  return ret;
}

bool sdIsBusy(void)  // sd 동작 여부 상태 확인용 //
{
  bool is_busy;

  if (HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER )
  {
    is_busy = false;
  }
  else
  {
    is_busy = true;
  }

  return is_busy;
}

bool sdIsReady(uint32_t timeout) // Busy 반대 // 동작 가능 //
{
  uint32_t pre_time;

  pre_time = millis();

  while(millis() - pre_time < timeout)
  {
    if (sdIsBusy() == false)
    {
      return true;
    }
  }

  return false;
}

bool sdReadBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms)
{
  bool ret = false;
  //uint32_t pre_time;


#if 1

  if(HAL_SD_ReadBlocks(&hsd, (uint8_t *)p_data, block_addr, num_of_blocks, timeout_ms) == HAL_OK)
   {
     while(sdIsBusy() == true);
     ret = true;
   }

#else
  is_rx_done = false; // read 하기 전에 초기화 //

  if(HAL_SD_ReadBlocks_DMA(&hsd, (uint8_t *)p_data, block_addr, num_of_blocks) == HAL_OK) // READ DMA 명령어 //
  {

    pre_time = millis();
    while(is_rx_done == false) // callback 에서 수신 완료 되었는지 대기 //
    {
      if (millis()-pre_time >= timeout_ms)
      {
        break;
      }
    }
    while(sdIsBusy() == true) // 동작 가능한 상태인지 확인
    {
      if (millis()-pre_time >= timeout_ms)
      {
        is_rx_done = false;
        break;
      }
    }
    ret = is_rx_done;  // 위 두 조건을 줘서 충분히 수신 및 동작 가능한 상태까지 대기 //
  }
#endif

  return ret; // 정상 수신시 "1" 리턴
}

bool sdWriteBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms)
{
  bool ret = false;
  //uint32_t pre_time;

  if (is_init == false) return false;

#if 1
  if(HAL_SD_WriteBlocks(&hsd, (uint8_t *)p_data, block_addr, num_of_blocks, timeout_ms) == HAL_OK)
   {
     ret = true;
   }
#else
  is_tx_done = false;
  if(HAL_SD_WriteBlocks_DMA(&hsd, (uint8_t *)p_data, block_addr, num_of_blocks) == HAL_OK)
  {
    pre_time = millis();
    while(is_tx_done == false)
    {
      if (millis()-pre_time >= timeout_ms)
      {
        break;
      }
    }
    pre_time = millis();
    while(sdIsBusy() == true)
    {
      if (millis()-pre_time >= timeout_ms)
      {
        is_tx_done = false;
        break;
      }
    }
    ret = is_tx_done;
  }
#endif

  return ret;
}

bool sdEraseBlocks(uint32_t start_addr, uint32_t end_addr)
{
  bool ret = false;

  if (is_init == false) return false;

  if(HAL_SD_Erase(&hsd, start_addr, end_addr) == HAL_OK)
  {
    ret = true;
  }

  return ret;
}




void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  is_rx_done = true;
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  is_tx_done = true;
}

// From sdio.c //

void HAL_SD_MspInit(SD_HandleTypeDef* sdHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(sdHandle->Instance==SDMMC1)
  {
  /* USER CODE BEGIN SDMMC1_MspInit 0 */

  /* USER CODE END SDMMC1_MspInit 0 */
    /* SDMMC1 clock enable */
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**SDMMC1 GPIO Configuration
    PC10     ------> SDMMC1_D2
    PC11     ------> SDMMC1_D3
    PC12     ------> SDMMC1_CK
    PD2     ------> SDMMC1_CMD
    PC8     ------> SDMMC1_D0
    PC9     ------> SDMMC1_D1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_8
                          |GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SDMMC1 interrupt Init */
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
  /* USER CODE BEGIN SDMMC1_MspInit 1 */

  /* USER CODE END SDMMC1_MspInit 1 */
  }
}

void HAL_SD_MspDeInit(SD_HandleTypeDef* sdHandle)
{

  if(sdHandle->Instance==SDMMC1)
  {
  /* USER CODE BEGIN SDMMC1_MspDeInit 0 */

  /* USER CODE END SDMMC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SDMMC1_CLK_DISABLE();

    /**SDMMC1 GPIO Configuration
    PC10     ------> SDMMC1_D2
    PC11     ------> SDMMC1_D3
    PC12     ------> SDMMC1_CK
    PD2     ------> SDMMC1_CMD
    PC8     ------> SDMMC1_D0
    PC9     ------> SDMMC1_D1
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_8
                          |GPIO_PIN_9);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

    /* SDMMC1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
  /* USER CODE BEGIN SDMMC1_MspDeInit 1 */

  /* USER CODE END SDMMC1_MspDeInit 1 */
  }
}


#ifdef _USE_HW_CLI
void cliSd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    sd_info_t sd_info;

    cliPrintf("sd init      : %d\n", is_init);
    cliPrintf("sd connected : %d\n", is_detected);

    if (is_init == true)
    {
      if (sdGetInfo(&sd_info) == true)
      {
        cliPrintf("   card_type            : %d\n", sd_info.card_type);
        cliPrintf("   card_version         : %d\n", sd_info.card_version);
        cliPrintf("   card_class           : %d\n", sd_info.card_class);
        cliPrintf("   rel_card_Add         : %d\n", sd_info.rel_card_Add);
        cliPrintf("   block_numbers        : %d\n", sd_info.block_numbers);
        cliPrintf("   block_size           : %d\n", sd_info.block_size);
        cliPrintf("   log_block_numbers    : %d\n", sd_info.log_block_numbers);
        cliPrintf("   log_block_size       : %d\n", sd_info.log_block_size);
        cliPrintf("   card_size            : %d MB, %d.%d GB\n", sd_info.card_size, sd_info.card_size/1024, ((sd_info.card_size * 10)/1024) % 10);
      }
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read") == true)
  {
    uint32_t number;
    uint32_t buf[512/4];

    number = args->getData(1);

    if (sdReadBlocks(number, (uint8_t *)buf, 1, 100) == true) //1블럭은 512BYTE//
    {
      for (int i=0; i<512/4; i++)
      {
        cliPrintf("%d:%04d : 0x%08X\n", number, i*4, buf[i]);
      }
    }
    else
    {
      cliPrintf("sdRead Fail\n");
    }

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("sd info\n");                 // CLI sd 정보 출력 함수

    if (is_init == true)
    {
      cliPrintf("sd read block_number\n");  // CLI sd 블럭메모리(512K Byte)넘버 Read 함수
    }
  }
}
#endif


#endif

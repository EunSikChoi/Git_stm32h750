/*
 * uart.c
 *
 *  Created on: 2022. 1. 3.
 *      Author: 82109
 */




#include "uart.h"
#include "qbuffer.h"


uart_tbl_t uart_tbl[UART_MAX_CH];         // static 선언 하면 hw falut 생성됨 // 모드버스에서 사용됨 //

#define UART_MAX_BUF_SIZE    256
static uint8_t rx_buf[UART_MAX_CH -1][UART_MAX_BUF_SIZE];; //  buf for qbuffer    // CLI
static uint8_t tx_buf[UART_MAX_CH -1][UART_MAX_BUF_SIZE];


//static uint8_t rx_data[UART_MAX_CH];      //  buf for interrupt  // CLI

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;


#ifdef _USE_HW_CLI

static void cliUart(cli_args_t *args);

#endif

data_t data;


bool uartInit(void)
{
  for (int i =0; i < UART_MAX_CH; i++)
  {
		uart_tbl[i].is_open 	  = false;
		uart_tbl[i].baud 			  = 57600;
		uart_tbl[i].is_tx_done 	= true;
		uart_tbl[i].is_tx_error = false;
		uart_tbl[i].ch				  = i;
  }


	#ifdef _USE_HW_CLI

		cliAdd("uart",cliUart);

	#endif

  return true;
}


bool uartOpen(uint8_t ch, uint32_t baud)
{

  bool ret = false;
  switch(ch)
  {
    case _DEF_UART1:

    	ret = false;

    	uart_tbl[ch].p_huart 			= &huart2;
    	uart_tbl[ch].p_hdma_rx		= &hdma_usart2_rx;
    	uart_tbl[ch].p_hdma_tx		= &hdma_usart2_tx;

    	uart_tbl[ch].is_open 		 = true;

      huart2.Instance					 = USART2;
      huart2.Init.BaudRate 		 = baud;
      huart2.Init.WordLength 	 = UART_WORDLENGTH_8B;
      huart2.Init.StopBits 		 = UART_STOPBITS_1;
      huart2.Init.Parity 			 = UART_PARITY_NONE;
      huart2.Init.Mode 				 = UART_MODE_TX_RX;
      huart2.Init.HwFlowCtl 	 = UART_HWCONTROL_NONE;
      huart2.Init.OverSampling = UART_OVERSAMPLING_16;

      HAL_UART_DeInit(&huart2);

      qbufferCreate(&uart_tbl[ch].qbuffer, &rx_buf[ch][0], UART_MAX_BUF_SIZE);

			__485_CLI_RX_ENB;

      __HAL_RCC_DMA1_CLK_ENABLE();

      __HAL_UART_ENABLE_IT(uart_tbl[ch].p_huart, UART_IT_RXNE); //UART RX INT Enable//

      if (HAL_UART_Init(uart_tbl[ch].p_huart) != HAL_OK)
			{
      	ret = false;
			}
			else
			{

				 if(HAL_UART_Receive_DMA(uart_tbl[ch].p_huart, (uint8_t *)&rx_buf[ch][0], UART_MAX_BUF_SIZE) != HAL_OK) // start IT
				 {
					 ret = false;
				 }

	       uart_tbl[ch].qbuffer.in  = uart_tbl[ch].qbuffer.len - uart_tbl[ch].p_hdma_rx->Instance->NDTR;
	       uart_tbl[ch].qbuffer.out = uart_tbl[ch].qbuffer.in;

	       HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 6, 0);
	       HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

	       HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 1, 0);
	       HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

				 ret = true;

				 logPrintf("uartOpen     \t\t: DEF_UART3\r\n");

			}
	  break;
  }

  return ret;
}


uint32_t uartAvailable(uint8_t ch)
{
  uint32_t ret = 0;

  switch(ch)
  {
    case _DEF_UART1:
    	// update in count //
      uart_tbl[ch].qbuffer.in = (uart_tbl[ch].qbuffer.len - uart_tbl[ch].p_hdma_rx->Instance->NDTR); //== in++
    	ret = qbufferAvailable(&uart_tbl[ch].qbuffer);
	  break;
  }

  return ret;
}


uint8_t  uartRead(uint8_t ch)
{
  uint8_t ret = 0;

   switch(ch)
   {
     case _DEF_UART1:
    	 qbufferRead(&uart_tbl[ch].qbuffer, &ret, 1);
 	   break;

   }

   return ret;

}


uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  uint32_t ret = 0;
  HAL_StatusTypeDef status;
  uint32_t pre_time;

  switch(ch)
  {

    case _DEF_UART1:

	#if 0 // case 1  // Tx polling
    	status = HAL_UART_Transmit(uart_tbl[ch].p_huart, p_data, length, 100);

		  if (status == HAL_OK)
		  {
			  ret = length;
		  }
		  break;
	#endif

	#if 0 // case 2  // No wait Tx,  DMA , no buffer
  	status = HAL_UART_Transmit_DMA(uart_tbl[ch].p_huart, p_data, length);

	  if (status == HAL_OK)
	  {
		  ret = length;
	  }
	  break;
	#endif

	#if 0 // case 3  //  wait Tx, DMA, no buffer

	if(uart_tbl[ch].p_huart->gState == HAL_UART_STATE_READY)
	{
		status = HAL_UART_Transmit_DMA(&huart2, p_data, length);
	}

	if (status == HAL_OK)
	{
		ret = length;
	}
	break;
#endif

	#if 1 // case 4  //  wait Tx, DMA, make buffer

  pre_time = millis();

  while(millis()- pre_time < 100)
  {
	  //if (uart_tbl[ch].p_huart->gState == HAL_UART_STATE_READY)  // huart2 상태변수 이용
	  if( uart_tbl[ch].is_tx_done == true)   // Txcplt 콜백에 변수선언  및 이용
	  {
		  uart_tbl[ch].is_tx_done  = false;

		  for(int i = 0; i < length ; i++)
		  {
		  	tx_buf[0][i] = p_data[i];
		  }

		  __485_CLI_TX_ENB;

		  status = HAL_UART_Transmit_DMA(uart_tbl[ch].p_huart, &tx_buf[0][0], length);

		  if (status == HAL_OK)
		  {
			  ret = length;
		  }
		  break;
	  }

  }
	#endif

	  break;
  }

  return ret;

}


uint32_t uartPrintf(uint8_t ch, char *fmt, ...)  // #include <stdarg.h> 추가해야 가능 //
{
	char buf[256];
  va_list args;  						// memory address , args 이름으로 포인트 설정 // 포인터 변수 생성
  int len;
  uint32_t ret;

  va_start(args, fmt); 					// 가변인자중 첫번째 인자의 주소를 가르킴 // 즉 시작 주소 지정 //
  len = vsnprintf(buf, 256, fmt, args); // 가변인자 문자 출력  //(fmt에 만들어진 내용이 담길 버퍼, 최대크키 , 포멧, 가변인자 시작주소) // 리턴값은 길이 //
  ret = uartWrite(ch, (uint8_t *)buf, len);

  va_end(args); 						//args 목록 초기화

  return ret;

}

uint32_t uartGetBaud(uint8_t ch)
{
  uint32_t ret = 0;

  switch(ch)
  {
    case _DEF_UART1:
    	ret = huart2.Init.BaudRate;
		break;
  }

  return ret;


}


/* RTU USART1
__HAL_RCC_USART1_CLK_ENABLE();
__HAL_RCC_DMA2_CLK_ENABLE();
*/
/**USART1 GPIO Configuration
PA15     ------> USART1_TX
PB3     ------> USART1_RX

HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 4, 0);
HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
*/

/*  CLI USART2

  __HAL_RCC_DMA1_CLK_ENABLE();


  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  */



void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA15     ------> USART1_TX
    PB3     ------> USART1_RX
    */

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */

    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Stream5;
    hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}


void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA15     ------> USART1_TX
    PB3     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

static void cliUart(cli_args_t *args)
{

  bool ret = false;
  uint32_t baudrate = 0 ;

  if(args->argc == 1 && args->isStr(0, "info") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
  {

  	uartPrintf(_DEF_UART1, "Uart(485) BaudRate : %d\n", uartGetBaud(_DEF_UART2));
  	uartPrintf(_DEF_UART1, "ID    : DEC: %d, HEX: %x\n", STATION_ID_HMI, STATION_ID_HMI);

  	ret = true;
  }

  if(args->argc == 2 && args->isStr(0, "set") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
  {

  	baudrate  = (uint32_t) args->getData(1);

  	uartOpen(_DEF_UART2, baudrate);

  	ret = true;
  }

  if(args->argc == 1 && args->isStr(0, "reset") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
  {

    HAL_UART_Init(uart_tbl[1].p_huart);
  	uartPrintf(_DEF_UART1, "Uart(485) reset OK\n");

  	ret = true;
  }



  if(ret != true )
  {

    cliPrintf("uart info\n");
    cliPrintf("uart set baud\n");
  }

}

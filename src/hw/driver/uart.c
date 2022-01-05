/*
 * uart.c
 *
 *  Created on: 2022. 1. 3.
 *      Author: 82109
 */




#include "uart.h"
#include "qbuffer.h"



#define UART_RX_BUF_LENGTH      1024





typedef struct
{
  bool     is_open;
  uint32_t baud;


  uint8_t  rx_buf[UART_RX_BUF_LENGTH];
  qbuffer_t qbuffer;
  UART_HandleTypeDef *p_huart;
  DMA_HandleTypeDef  *p_hdma_rx;

} uart_tbl_t;

#if 1
uart_tbl_t uart_tbl[UART_MAX_CH];
#else
static __attribute__((section(".non_cache"))) uart_tbl_t uart_tbl[UART_MAX_CH];
#endif



 DMA_HandleTypeDef hdma_uart4_rx;
 UART_HandleTypeDef huart4;

 static uint8_t rx_data[UART_MAX_CH];  // rx INT buf


#ifdef _USE_HW_CLI

static void cliUart(cli_args_t *args);

#endif


bool uartInit(void)
{
  for (int i =0; i < UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;
    uart_tbl[i].baud = 57600;
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

    	uart_tbl[ch].p_huart   = &huart4;
    	uart_tbl[ch].p_hdma_rx = &hdma_uart4_rx;

    	uart_tbl[ch].p_huart->Instance    = UART4;
			uart_tbl[ch].p_huart->Init.BaudRate    = baud;
			uart_tbl[ch].p_huart->Init.WordLength  = UART_WORDLENGTH_8B;
			uart_tbl[ch].p_huart->Init.StopBits    = UART_STOPBITS_1;
			uart_tbl[ch].p_huart->Init.Parity      = UART_PARITY_NONE;
			uart_tbl[ch].p_huart->Init.Mode        = UART_MODE_TX_RX;
			uart_tbl[ch].p_huart->Init.HwFlowCtl   = UART_HWCONTROL_NONE;
			uart_tbl[ch].p_huart->Init.OverSampling= UART_OVERSAMPLING_16;
			uart_tbl[ch].p_huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
			uart_tbl[ch].p_huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
			uart_tbl[ch].p_huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;


			HAL_UART_DeInit(uart_tbl[ch].p_huart);


			qbufferCreate(&uart_tbl[ch].qbuffer, &uart_tbl[ch].rx_buf[0], UART_RX_BUF_LENGTH);

			__HAL_RCC_DMA1_CLK_ENABLE();

      if (HAL_UART_Init(uart_tbl[ch].p_huart) != HAL_OK)
      {
        ret = false;
      }
      else
      {
        ret = true;
        uart_tbl[ch].is_open = true;

        if(HAL_UART_Receive_DMA(uart_tbl[ch].p_huart, (uint8_t *)&uart_tbl[ch].rx_buf[0], UART_RX_BUF_LENGTH) != HAL_OK)
        {
          ret = false;
        }

        uart_tbl[ch].qbuffer.in  = uart_tbl[ch].qbuffer.len - ((DMA_Stream_TypeDef *)uart_tbl[ch].p_huart->hdmarx->Instance)->NDTR;
        uart_tbl[ch].qbuffer.out = uart_tbl[ch].qbuffer.in;
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
			uart_tbl[ch].qbuffer.in = (uart_tbl[ch].qbuffer.len - ((DMA_Stream_TypeDef *)uart_tbl[ch].p_hdma_rx->Instance)->NDTR);
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

	#if 1 // case 1  // Tx polling
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

	#if 0 // case 4  //  wait Tx, DMA, make buffer

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

		  status = HAL_UART_Transmit_DMA(uart_tbl[ch].handle, &tx_buf[0][0], length);

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
    	ret = uart_tbl[ch].baud;;
		break;
  }

  return ret;


}


//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//  if(huart->Instance == UART4)
//  {
//    qbufferWrite(&uart_tbl[_DEF_UART1].qbuffer, &rx_data[_DEF_UART1] , 1);
//
//    HAL_UART_Receive_IT(&huart4, (uint8_t *)&rx_data[_DEF_UART1], 1) ; // Re enable IT
//
//  }
//}
//
//
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
//{
//
//   if(huart->Instance == UART4)
//  {
//	   __HAL_UART_CLEAR_FLAG(&huart4,UART_FLAG_RXNE);
//		 __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
//
// 	 //is_tx_done[_DEF_UART2] = true; // 수신 준비 됬다고 알려주는 신호
//  	 uart_tbl[_DEF_UART1].is_tx_done  = true;
//
//  	 return;
// }
//
//
//}





void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspInit 0 */

  /* USER CODE END UART4_MspInit 0 */
    /* UART4 clock enable */
    __HAL_RCC_UART4_CLK_ENABLE();

    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**UART4 GPIO Configuration
    PI9     ------> UART4_RX
    PA0     ------> UART4_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* UART4 DMA Init */
    /* UART4_RX Init */
    hdma_uart4_rx.Instance = DMA1_Stream0;
    hdma_uart4_rx.Init.Request = DMA_REQUEST_UART4_RX;
    hdma_uart4_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart4_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart4_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart4_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart4_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart4_rx.Init.Mode = DMA_CIRCULAR;
    hdma_uart4_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart4_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart4_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_uart4_rx);

    /* UART4 interrupt Init */
    HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspInit 1 */

  /* USER CODE END UART4_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspDeInit 0 */

  /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART4_CLK_DISABLE();

    /**UART4 GPIO Configuration
    PI9     ------> UART4_RX
    PA0     ------> UART4_TX
    */
    HAL_GPIO_DeInit(GPIOI, GPIO_PIN_9);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0);

    /* UART4 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* UART4 interrupt Deinit */
    HAL_NVIC_DisableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspDeInit 1 */

  /* USER CODE END UART4_MspDeInit 1 */
  }
}

#ifdef _USE_HW_CLI

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

#endif

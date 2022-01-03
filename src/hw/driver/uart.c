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


 DMA_HandleTypeDef hdma_uart4_rx;
 DMA_HandleTypeDef hdma_uart4_tx;
 UART_HandleTypeDef huart4;

 static uint8_t rx_data[UART_MAX_CH];  // rx INT buf


#ifdef _USE_HW_CLI

static void cliUart(cli_args_t *args);

#endif


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


  uart_tbl_t *p_uart;

  switch(ch)
  {
    case _DEF_UART1:
    	p_uart = &uart_tbl[ch];

    	ret = false;

    	p_uart->handle 		= &huart4;
    	p_uart->p_hdma_rx		= &hdma_uart4_rx;
    	p_uart->p_hdma_tx		= &hdma_uart4_tx;

    	p_uart->is_open 		 = true;


    	p_uart->handle->Instance										= UART4;
    	p_uart->handle->Init.BaudRate 							= baud;
    	p_uart->handle->Init.WordLength 						= UART_WORDLENGTH_8B;
    	p_uart->handle->Init.StopBits 							= UART_STOPBITS_1;
    	p_uart->handle->Init.Parity 								= UART_PARITY_NONE;
    	p_uart->handle->Init.HwFlowCtl 							= UART_HWCONTROL_NONE;
    	p_uart->handle->Init.Mode 									= UART_MODE_TX_RX;
    	p_uart->handle->Init.OverSampling 					= UART_OVERSAMPLING_16;
    	p_uart->handle->Init.OneBitSampling 				= UART_ONE_BIT_SAMPLE_DISABLE;
    	p_uart->handle->Init.ClockPrescaler  				= UART_PRESCALER_DIV1;
    	p_uart->handle->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    	p_uart->handle->AdvancedInit.AutoBaudRateEnable = UART_ADVFEATURE_AUTOBAUDRATE_ENABLE;

			HAL_UART_DeInit(p_uart->handle);

     // qbufferCreate(&uart_tbl[ch].qbuffer, &rx_buf[0], 256);
      qbufferCreate(&p_uart->qbuffer, &rx_buf[0], 256);

      __HAL_UART_ENABLE_IT(p_uart->handle, UART_IT_RXNE); //UART RX INT Enable//

      if (HAL_UART_Init(p_uart->handle) != HAL_OK)
			{
      	ret = false;
			}
			else
			{

				// if(HAL_UART_Receive_DMA(uart_tbl[ch].handle, (uint8_t *)&rx_buf[ch][0], UART_MAX_BUF_SIZE) != HAL_OK) // start IT
				 if(HAL_UART_Receive_IT(p_uart->handle, (uint8_t *)&rx_data[_DEF_UART1], 1) != HAL_OK)
				 {
					 ret = false;
				 }

				 ret = true;

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
    	status = HAL_UART_Transmit(uart_tbl[ch].handle, p_data, length, 100);

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
    	ret = huart4.Init.BaudRate;
		break;
  }

  return ret;


}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == UART4)
  {
    qbufferWrite(&uart_tbl[_DEF_UART1].qbuffer, &rx_data[_DEF_UART1] , 1);

    HAL_UART_Receive_IT(&huart4, (uint8_t *)&rx_data[_DEF_UART1], 1) ; // Re enable IT

  }
}


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



/*
     // DMA controller clock enable //
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

*/


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

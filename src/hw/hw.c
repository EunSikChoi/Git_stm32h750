/*
 * hw.c
 *
 *  Created on: Nov 15, 2021
 *      Author: 82109
 */


#include "hw.h"
#include "uart.h"

void hwInit(void)
{

	bspInit();

	cliInit();
  ledInit();

	__disable_irq();


	uartInit();
  uartOpen(_DEF_UART1, 57600);

	logInit();
	logPrintf("\n\n");
	logPrintf("[ FW Begin... ]\r\n");
	logPrintf("Booting..Clock\t\t: %d Mhz\r\n", (int)HAL_RCC_GetSysClockFreq()/1000000);


	buttonInit();

  //sdramInit();

  //qspiInit(); /* App 펌웨어 자체가 QSPI 영역에 존재 하기때문 초기화 하면 안됨  */
  flashInit();

	gpioInit();

	//if (sdInit() == true)
	//{
	//  fatfsInit();
	//}

  //qspiEnableMemoryMappedMode(); // CLI md 사용하여면 해당 함수 주석 없어야함 //
                                  // map 모드시 indirect 방식은 사용 안됨 // 모드 변경을 해줘햐함


}

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

	uartInit();
  uartOpen(_DEF_UART1, 57600);

	logInit();
	logPrintf("\n\n");
	logPrintf("[ FSBL Begin... ]\r\n");
	logPrintf("Booting..Clock\t\t: %d Mhz\r\n", (int)HAL_RCC_GetSysClockFreq()/1000000);


	buttonInit();

  sdramInit();

  qspiInit();
  flashInit();

	gpioInit();

	if (sdInit() == true)
	{
	  fatfsInit();
	}

  qspiEnableMemoryMappedMode(); // CLI md 사용하여면 해당 함수 주석 없어야함 //
                                  // map 모드시 indirect 방식은 사용 안됨 // 모드 변경을 해줘햐함


}

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
	logPrintf("[ FW Begin... ]\r\n");


	buttonInit();

  qspiInit();
  flashInit();

 // qspiEnableMemoryMappedMode();


}

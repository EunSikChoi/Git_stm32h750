/*
 * ap.cpp
 *
 *  Created on: Nov 15, 2021
 *      Author: 82109
 */


#include "ap.h"
#include "led.h"
#include "uart.h"



void apInit(void)
{
	uartOpen(_DEF_UART1, 57600);
}

void apMain(void)
{
  uint32_t pre_time;
  uint8_t ces;

  pre_time = millis();

	while(1)
	{
		if(millis()-pre_time >= 2000) //
		{
			pre_time = millis();

			ledToggle(_DEF_LED1); // LED
			ledToggle(_DEF_LED2); // BLUE

		}

    if (uartAvailable(_DEF_UART1) > 0)
    {
    	ces = uartRead(_DEF_UART1);
      uartPrintf(_DEF_UART1, "rx : 0x%X\n", ces);
    }

	}


}


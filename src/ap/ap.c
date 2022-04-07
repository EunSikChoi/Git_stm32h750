/*
 * ap.cpp
 *
 *  Created on: Nov 15, 2021
 *      Author: 82109
 */


#include "ap.h"


void apInit(void)
{
	cliOpen(_DEF_UART1, 57600);
}

void apMain(void)
{
  uint32_t pre_time;

  pre_time = millis();


	while(1)
	{
		if(millis()-pre_time >= 500) //
		{
			pre_time = millis();

			cliPrintf("Time %d\n" , pre_time);

			ledToggle(_DEF_LED1); // LED
			ledToggle(_DEF_LED2); // BLUE

		}

#ifdef _USE_HW_CLI
    cliMain();
#endif

	}

}






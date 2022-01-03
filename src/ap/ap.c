/*
 * ap.cpp
 *
 *  Created on: Nov 15, 2021
 *      Author: 82109
 */


#include "ap.h"
#include "led.h"



void apInit(void)
{


}

void apMain(void)
{
  uint32_t pre_time;

  pre_time = millis();

	while(1)
	{
		if(millis()-pre_time >= 1000) //
		{
			pre_time = millis();

			//ledToggle(_DEF_LED1); // LED
			ledToggle(_DEF_LED2); // BLUE

		}
	}


}


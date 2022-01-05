/*
 * ap.cpp
 *
 *  Created on: Nov 15, 2021
 *      Author: 82109
 */


#include "ap.h"
#include "led.h"
#include "uart.h"

#if 1
// RAM 함수 복사

extern uint32_t  _siram_code;
extern uint32_t  _sram_code;
extern uint32_t  _eram_code;

void ramfuncInit(void)
{
	uint32_t *p_siram_code = &_siram_code; // 플래시 주소
	uint32_t *p_sram_code  = &_sram_code;  // RAM 시작 주소
	uint32_t *p_eram_code  = &_eram_code;

	uint32_t length;

	length = (uint32_t)(p_eram_code- p_sram_code);

	for(int i = 0 ; i < length ; i++)
	{
		p_sram_code[i] = p_siram_code[i];
	}
}

__attribute__((section(".ram_code"))) void funcRam(void)
{
	volatile float sum, a, b;

	sum = 0.;
	a		= 0.;
	b 	= 0.;

	for(int i = 0 ; i< 1024*1024 ; i++)
	{
		sum += a+b;
		a 	+=	1.;
		b 	+= 	2.;
	}
}

 void funcFlash(void)
{
	volatile float sum, a, b;

	sum = 0.;
	a		= 0.;
	b 	= 0.;

	for(int i = 0 ; i< 1024*1024 ; i++)
	{
		sum += a+b;
		a 	+=	1.;
		b 	+= 	2.;
	}
}
#endif

void apInit(void)
{
	uartOpen(_DEF_UART1, 57600);
#if 1
	ramfuncInit();
#endif
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
#if 1 // TEST
			uint32_t pre_time_func;
			uint32_t exe_time;

			pre_time_func =  millis();
			funcRam();
			exe_time = millis() - pre_time_func;

			uartPrintf(_DEF_UART1, "funcRam time : %d\n", exe_time);

			pre_time_func =  millis();
			funcFlash();
			exe_time = millis() - pre_time_func;

			uartPrintf(_DEF_UART1, "funcFlash time : %d\n", exe_time);

#endif
		}

    if (uartAvailable(_DEF_UART1) > 0)
    {
    	ces = uartRead(_DEF_UART1);
      uartPrintf(_DEF_UART1, "rx : 0x%X\n", ces);
    }

	}


}


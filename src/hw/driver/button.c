/*
 * button.c
 *
 *  Created on: 2022. 1. 6.
 *      Author: 82109
 */



#include "button.h"
#include "cli.h"

typedef struct
{
	GPIO_TypeDef* 	port;
	uint32_t			 	pin;
	GPIO_PinState 	on_state;

}button_tbl_t;


button_tbl_t button_tbl[BUTTON_MAX_CH] =
{
				{GPIOH ,GPIO_PIN_4, GPIO_PIN_RESET}
};

#ifdef _USE_HW_CLI
static void cliButton(cli_args_t *args);
#endif


bool buttonInit(void)
{
	bool ret = true;

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  for(int i = 0 ; i < BUTTON_MAX_CH ; i++)
  {
  	 GPIO_InitStruct.Pin = button_tbl[i].pin;
  	 HAL_GPIO_Init(button_tbl[i].port,&GPIO_InitStruct);
  }



	#ifdef _USE_HW_CLI
		cliAdd("button",cliButton);
	#endif
	return ret;
}

bool buttonGetPressed(uint8_t ch)
{
	bool ret = false;

	if(ch > BUTTON_MAX_CH)
	{
		return false;
	}

	if( HAL_GPIO_ReadPin(button_tbl[ch].port, button_tbl[ch].pin) == button_tbl[ch].on_state ) //읽은값이 0 이면//
	{
		ret = true;
	}

	return ret;

}

	#ifdef _USE_HW_CLI
	void cliButton(cli_args_t *args)
	{
		bool  ret = false;
		uint16_t pre_time;

		if(args->argc == 1 && args->isStr(0, "show") == true)
		{
			while(cliKeepLoop())
			{

			  if( millis() - pre_time > 500)
			  {
			     pre_time = millis();

           for( int i = 0 ; i < BUTTON_MAX_CH ; i++)
           {
            cliPrintf("Button  : %d \n", buttonGetPressed(i));
           }
			  }


			  // delay(100);
			}
		}


		if(ret != true )
		{
		  cliPrintf("button show\n");
		}

	}
	#endif

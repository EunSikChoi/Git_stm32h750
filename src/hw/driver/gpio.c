/*
 * gpio.c
 *
 *  Created on: 2021. 11. 16.
 *      Author: 82109
 */




#include "gpio.h"
#include "cli.h"

typedef struct
{
	GPIO_TypeDef  	*port;
	uint32_t      	pin;
	uint8_t		  	  mode;
	GPIO_PinState	  on_state;
	GPIO_PinState	  off_state;
	bool			      init_value;
} gpio_tbl_t;

#ifdef _USE_HW_CLI

static void cliGpio(cli_args_t *args);

#endif


gpio_tbl_t gpio_tbl[GPIO_MAX_CH] =
{
		{GPIOD , GPIO_PIN_5, _DEF_INPUT_PULLUP ,GPIO_PIN_SET, GPIO_PIN_RESET, 1 },    // DI모드  // SD 삽입시  low 떨어짐. 초기값"1" 별 의미 없음
		//{GPIOB , GPIO_PIN_9, _DEF_OUTPUT ,GPIO_PIN_RESET, GPIO_PIN_SET, 1 }     	  // DO 모드 // 테스트용


		//{GPIOC , GPIO_PIN_8, _DEF_OUTPUT ,GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH },  // MCP2515 CH0 CS
		//{GPIOC , GPIO_PIN_9, _DEF_OUTPUT ,GPIO_PIN_SET, GPIO_PIN_RESET, _DEF_HIGH }   // Wiz5500 CH0 CS

};

bool gpioInit(void)
{
	bool ret = true;

	  __HAL_RCC_GPIOB_CLK_ENABLE();

	for (int i = 0 ; i < GPIO_MAX_CH ; i++)
	{
		gpioPinMode (i , gpio_tbl[i].mode);
		gpioPinWrite(i , gpio_tbl[i].init_value);
	}

	#ifdef _USE_HW_CLI

		cliAdd("gpio",cliGpio);

	#endif

	return ret;
}

bool gpioPinMode (uint8_t ch, uint8_t mode)
{
	bool ret = true;

	  GPIO_InitTypeDef GPIO_InitStruct = {0};

	  switch(mode)
	  {
	  	  case _DEF_INPUT :
	  		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  		GPIO_InitStruct.Pull = GPIO_NOPULL;
	  	  break;

	  	  case _DEF_INPUT_PULLUP :
	  		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  		GPIO_InitStruct.Pull = GPIO_PULLUP;
	  	  break;

	  	  case _DEF_INPUT_PULLDOWN :
	  		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	  	  break;

	  	  case _DEF_OUTPUT :
	  		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  		GPIO_InitStruct.Pull = GPIO_NOPULL;
	  	  break;

	  	  case _DEF_OUTPUT_PULLUP :
	  		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  		GPIO_InitStruct.Pull = GPIO_PULLUP;
	  	  break;

	  	  case _DEF_OUTPUT_PULLDOWN :
	  		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	  	  break;

	  }

	  GPIO_InitStruct.Pin = gpio_tbl[ch].pin;
	  HAL_GPIO_Init(gpio_tbl[ch].port, &GPIO_InitStruct);


	return ret;
}

void gpioPinWrite (uint8_t ch, bool value)
{
	if( ch > GPIO_MAX_CH)
	{
		return ;
	}

	if(value == true)
	{
		HAL_GPIO_WritePin(gpio_tbl[ch].port, gpio_tbl[ch].pin,  gpio_tbl[ch].on_state);
	}
	else
	{
		HAL_GPIO_WritePin(gpio_tbl[ch].port, gpio_tbl[ch].pin,  gpio_tbl[ch].off_state);
	}
}

bool gpioPinRead (uint8_t ch)
{
	bool ret = false;
	if( ch > GPIO_MAX_CH)
	{
		return false ;
	}

	if ( HAL_GPIO_ReadPin(gpio_tbl[ch].port, gpio_tbl[ch].pin) == gpio_tbl[ch].on_state)
	{
		ret = true;
	}

	return ret;
}

void gpioPinToggle (uint8_t ch)
{
	if( ch > GPIO_MAX_CH)
	{
		return  ;
	}

	HAL_GPIO_TogglePin(gpio_tbl[ch].port, gpio_tbl[ch].pin);
}

#ifdef _USE_HW_CLI



static void cliGpio(cli_args_t *args)
{

  bool ret = false;

  if(args->argc == 1 && args->isStr(0, "show") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
  {

    while(cliKeepLoop()) // 다음 통신 오기전까지 일시적으로만 도는 루프//
    {
    	for(int i =0 ; i < GPIO_MAX_CH ; i++)
    	{
    		cliPrintf("%d", gpioPinRead(i));
    	}
    	cliPrintf("\n");
    	delay(100);
    }

    ret = true;

  }

  if(args->argc == 2 && args->isStr(0, "read") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
  {
	  uint8_t ch;

	  ch = (uint8_t) args->getData(1);

    while(cliKeepLoop()) // 다음 통신 오기전까지 일시적으로만 도는 루프//
    {
    	cliPrintf("gpio read %d : %d \n", ch, gpioPinRead(ch));
    	delay(100);
    }

    ret = true;

  }

  if(args->argc == 3 && args->isStr(0, "write") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
  {

	  uint8_t ch , value;

	  ch 	= (uint8_t) args->getData(1);
	  value = (uint8_t) args->getData(2);
	  gpioPinWrite(ch, value);
	  cliPrintf("gpio write %d : %d \n", ch, value );

    ret = true;

  }

  if(ret != true )
  {
	for (int i = 0 ; i < GPIO_MAX_CH ; i++)
	{
		if(gpio_tbl[i].mode >= 3)
		{
			 cliPrintf("== gpio %d Write Mode == \n", i);// for check Mode//
		}
		else
		{
			cliPrintf("== gpio %d Read Mode == \n", i);
		}
	}

    cliPrintf("gpio show\n");
    cliPrintf("gpio read ch [0~%d]\n", GPIO_MAX_CH -1);
    cliPrintf("gpio write ch [0~%d] 0:1 \n", GPIO_MAX_CH -1);
  }



}

#endif



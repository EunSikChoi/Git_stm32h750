/*
 * led.c
 *
 *  Created on: 2021. 11. 16.
 *      Author: 82109
 */


#include"led.h"
//#include "cli.h"


typedef struct
{
	GPIO_TypeDef  	*port;
	uint32_t      	pin;
	GPIO_PinState	  on_state;
	GPIO_PinState	  off_state;
} led_tbl_t;


#ifdef _USE_HW_CLI

static void cliled(cli_args_t *args);

#endif

led_tbl_t led_tbl[LED_MAX_CH] =
{
		{LED_RED_GPIO_Port , LED_RED_Pin  , GPIO_PIN_SET , GPIO_PIN_RESET },
		{LED_BLUE_GPIO_Port, LED_BLUE_Pin , GPIO_PIN_SET , GPIO_PIN_RESET }
};

bool ledInit(void)
{

  bool ret = true;

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  for(int i = 0 ; i < LED_MAX_CH ; i++)
  {
  	GPIO_InitStruct.Pin =led_tbl[i].pin;
  	HAL_GPIO_Init(led_tbl[i].port, &GPIO_InitStruct);

  	ledOff(i);
  }

  return ret;
}

void ledOn(uint8_t ch)
{
	HAL_GPIO_WritePin(led_tbl[ch].port, led_tbl[ch].pin, GPIO_PIN_RESET);
}

void ledOff(uint8_t ch)
{
	HAL_GPIO_WritePin(led_tbl[ch].port, led_tbl[ch].pin, GPIO_PIN_SET);
}

void ledToggle(uint8_t ch)
{
	HAL_GPIO_TogglePin(led_tbl[ch].port, led_tbl[ch].pin);
}


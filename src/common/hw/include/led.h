/*
 * led.h
 *
 *  Created on: 2021. 11. 16.
 *      Author: 82109
 */

#ifndef SRC_COMMON_HW_INCLUDE_LED_H_
#define SRC_COMMON_HW_INCLUDE_LED_H_


#include "hw_def.h"

#ifdef _USE_HW_LED

bool ledInit(void);
void ledOn(uint8_t ch);
void ledOff(uint8_t ch);
void ledToggle(uint8_t ch);
#endif


#define LED_RED_Pin 			 GPIO_PIN_15
#define LED_RED_GPIO_Port  GPIOC
#define LED_BLUE_Pin 			 GPIO_PIN_8
#define LED_BLUE_GPIO_Port GPIOI



#endif /* SRC_COMMON_HW_INCLUDE_LED_H_ */

/*
 * bsp.h
 *
 *  Created on: Nov 15, 2021
 *      Author: 82109
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_



#include "def.h"
#include "stm32h7xx_hal.h"

void bspInit(void);
void bspDeInit(void);

void delay(uint32_t ms);
uint32_t millis(void);

void Error_Handler(void);


#endif /* SRC_BSP_BSP_H_ */

/*
 * hw.c
 *
 *  Created on: Nov 15, 2021
 *      Author: 82109
 */


#include "hw.h"


void hwInit(void)
{

	bspInit();

	ledInit();

  qspiInit();

  flashInit();

}

/*
 * log.h
 *
 *  Created on: 2022. 1. 11.
 *      Author: 82109
 */

#ifndef SRC_COMMON_HW_INCLUDE_LOG_H_
#define SRC_COMMON_HW_INCLUDE_LOG_H_



#include "hw_def.h"


#ifdef _USE_HW_LOG

#define LOG_CH            HW_LOG_CH


bool logInit(void);
void logPrintf(const char *fmt, ...);

#endif




#endif /* SRC_COMMON_HW_INCLUDE_LOG_H_ */

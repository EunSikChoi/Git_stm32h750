/*
 * cli.h
 *
 *  Created on: 2022. 1. 6.
 *      Author: 82109
 */

#ifndef SRC_COMMON_HW_INCLUDE_CLI_H_
#define SRC_COMMON_HW_INCLUDE_CLI_H_


#include "hw_def.h"


#ifdef _USE_HW_CLI

#define CLI_CMD_LIST_MAX      HW_CLI_CMD_LIST_MAX  // CMD 명령어의 최대 갯수 (16)
#define CLI_CMD_NAME_MAX      HW_CLI_CMD_NAME_MAX  //  CMD 명령어를 몇 글자까지 허용할껀지 (지금16글자)

#define CLI_LINE_HIS_MAX      HW_CLI_LINE_HIS_MAX  //
#define CLI_LINE_BUF_MAX      HW_CLI_LINE_BUF_MAX




typedef struct
{
  uint16_t   argc;
  char     **argv;

  int32_t  (*getData)(uint8_t index);
  float    (*getFloat)(uint8_t index);
  char    *(*getStr)(uint8_t index);
  bool     (*isStr)(uint8_t index, char *p_str);
} cli_args_t;


bool cliInit(void);
bool cliOpen(uint8_t ch, uint32_t baud);
bool cliOpenLog(uint8_t ch, uint32_t baud);
bool cliMain(void);
void cliPrintf(const char *fmt, ...);
bool cliAdd(const char *cmd_str, void (*p_func)(cli_args_t *)); // CMD 명령을 치면 뒤에 펑션이 콜백되는 기능//
bool cliKeepLoop(void);
uint32_t cliAvailable(void);
uint8_t  cliRead(void);
uint32_t cliWrite(uint8_t *p_data, uint32_t length);


#endif



#endif /* SRC_COMMON_HW_INCLUDE_CLI_H_ */

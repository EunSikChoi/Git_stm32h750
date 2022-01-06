/*
 * cli.c
 *
 *  Created on: 2022. 1. 6.
 *      Author: 82109
 */






#include "cli.h"
#include "uart.h"


#ifdef _USE_HW_CLI

#define CLI_KEY_BACK              0x08
#define CLI_KEY_DEL               0x7F
#define CLI_KEY_ENTER             0x0D  //CR (13)
#define CLI_KEY_ESC               0x1B
#define CLI_KEY_LEFT              0x44
#define CLI_KEY_RIGHT             0x43
#define CLI_KEY_UP                0x41
#define CLI_KEY_DOWN              0x42
#define CLI_KEY_HOME              0x31
#define CLI_KEY_END               0x34

#define CLI_PROMPT_STR            "cli# "

#define CLI_ARGS_MAX              32
#define CLI_PRINT_BUF_MAX         256


enum
{
  CLI_RX_IDLE,
  CLI_RX_SP1,
  CLI_RX_SP2,
  CLI_RX_SP3,
  CLI_RX_SP4,
};


typedef struct
{
  char   cmd_str[CLI_CMD_NAME_MAX];
  void (*cmd_func)(cli_args_t *);
} cli_cmd_t;


typedef struct
{
  uint8_t buf[CLI_LINE_BUF_MAX];
  uint8_t buf_len;
  uint8_t cursor;
  uint8_t count;
} cli_line_t;


typedef struct
{
  uint8_t  ch;
  uint32_t baud;
  bool     is_open;
  bool     is_log;
  uint8_t  log_ch;
  uint32_t log_baud;
  uint8_t  state;
  char     print_buffer[CLI_PRINT_BUF_MAX];
  uint16_t  argc;
  char     *argv[CLI_ARGS_MAX];


  bool        hist_line_new;
  int8_t      hist_line_i;
  uint8_t     hist_line_last;
  uint8_t     hist_line_count;

  cli_line_t  line_buf[CLI_LINE_HIS_MAX];
  cli_line_t  line;

  uint16_t    cmd_count;
  cli_cmd_t   cmd_list[CLI_CMD_LIST_MAX];
  cli_args_t  cmd_args;
} cli_t;



cli_t   cli_node;


static bool cliUpdate(cli_t *p_cli, uint8_t rx_data);    // 한바이트씩 받아서 한 라인 업데이트 하는 기능//
static void cliLineClean(cli_t *p_cli);                  // 엔터를 쳣을때 클리어 시키는 기능
static void cliLineAdd(cli_t *p_cli);                    // 엔터 쳣을때 버퍼로 저장
static void cliLineChange(cli_t *p_cli, int8_t key_up);  // 히스토리 버퍼 데이터를 업 다운 했을때 바꿔주는 기능
static void cliShowPrompt(cli_t *p_cli);                 // 기본 스트링 함수 출력 함수//
static void cliToUpper(char *str);                       // 소문자 대문자 변경함수
static bool cliRunCmd(cli_t *p_cli);                     // ENTER키 입력후 cli 실행하는 시작 함수
static bool cliParseArgs(cli_t *p_cli);                  // 입력 문자 파싱함수

static int32_t  cliArgsGetData(uint8_t index);           // 함수인수(argument) 입력된 정수형 문자 GET 합수
static float    cliArgsGetFloat(uint8_t index);          // 함수인수(argument) 입력된 소수형 문자 GET 합수
static char    *cliArgsGetStr(uint8_t index);
static bool     cliArgsIsStr(uint8_t index, char *p_str);// 저장된 argument 를 입력문자와 비교해서 같으면 "0" 리턴해줌


void cliShowList(cli_args_t *args);                      // "HELP"를 치면 실행되믄 함수. 저장된 명령어 List 출력함수
void cliMemoryDump(cli_args_t *args);                    // "MD"를 치면 지정된 메모리주소, 크기 만금 출력하는 함수


bool cliInit(void)
{
  cli_node.is_open = false;
  cli_node.is_log  = false;
  cli_node.state   = CLI_RX_IDLE;

  cli_node.hist_line_i     = 0;
  cli_node.hist_line_last  = 0;
  cli_node.hist_line_count = 0;
  cli_node.hist_line_new   = false;

  cli_node.cmd_args.getData  = cliArgsGetData;   // 입력 인수의 정수형 숫자 데이터 GET
  cli_node.cmd_args.getFloat = cliArgsGetFloat;  // 입력 인수의 플롯형 숫자 데이터 GET
  cli_node.cmd_args.getStr   = cliArgsGetStr;    // 입력된 string 문자 GET
  cli_node.cmd_args.isStr    = cliArgsIsStr;     // 입력된 string 문자 비교해서 동일하면 "1" 리턴

  cliLineClean(&cli_node);


  cliAdd("help", cliShowList);   // 현재 저장된 명령어 출력
  cliAdd("md"  , cliMemoryDump); // 특정주소 메모리 출력

  return true;
}

bool cliOpen(uint8_t ch, uint32_t baud)
{
  cli_node.ch = ch;
  cli_node.baud = baud;

  cli_node.is_open = uartOpen(ch, baud);

  return cli_node.is_open;
}

bool cliOpenLog(uint8_t ch, uint32_t baud)
{
  bool ret;

  cli_node.log_ch = ch;
  cli_node.log_baud = baud;

  ret = uartOpen(ch, baud);

  if (ret == true)
  {
    cli_node.is_log = true;
  }
  return ret;
}

bool cliLogClose(void)
{
  cli_node.is_log = false;
  return true;
}

void cliShowLog(cli_t *p_cli)
{
  if (cli_node.is_log == true)
  {
    uartPrintf(p_cli->log_ch, "Cursor  : %d\n", p_cli->line.cursor);    // 현재 커서 위치
    uartPrintf(p_cli->log_ch, "Count   : %d\n", p_cli->line.count);     // 입력된 문자 갯수
    uartPrintf(p_cli->log_ch, "buf_len : %d\n", p_cli->line.buf_len);   // 버퍼크기
    uartPrintf(p_cli->log_ch, "buf     : %s\n", p_cli->line.buf);       // 버퍼
    uartPrintf(p_cli->log_ch, "line_i  : %d\n", p_cli->hist_line_i);    // 이전에 저장된 배열 index 번호 ( 사용자가 정하면 늘어남. 현재는 4개만 선언
    uartPrintf(p_cli->log_ch, "line_lt : %d\n", p_cli->hist_line_last); // 다음에 써야할 배열 index 번호
    uartPrintf(p_cli->log_ch, "line_c  : %d\n", p_cli->hist_line_count);// 히스토리 카운트. 저장 몇번 했는지 알려줌// 현재 4 다차면 4로 유지됨

    for (int i=0; i<p_cli->hist_line_count; i++)
    {
      uartPrintf(p_cli->log_ch, "buf %d   : %s\n", i, p_cli->line_buf[i].buf);
    }
    uartPrintf(p_cli->log_ch, "\n");
  }
}

void cliShowPrompt(cli_t *p_cli)
{
  uartPrintf(p_cli->ch, "\n\r");
  uartPrintf(p_cli->ch, CLI_PROMPT_STR);
}


bool cliMain(void)
{
  if (cli_node.is_open != true)
  {
    return false;
  }

	if (uartAvailable(cli_node.ch) > 0) // 실제적으로 한바이트 이상 들어오면 Update 함수를 실행해서 계속 업데이트 해주는 역활
	{
		cliUpdate(&cli_node, uartRead(cli_node.ch));
	}

  return true;
}

uint32_t cliAvailable(void)
{
  return uartAvailable(cli_node.ch);
}

uint8_t cliRead(void)
{
  return uartRead(cli_node.ch);
}

uint32_t cliWrite(uint8_t *p_data, uint32_t length)
{
  return uartWrite(cli_node.ch, p_data, length);
}


bool cliUpdate(cli_t *p_cli, uint8_t rx_data)
{
  bool ret = false;
  uint8_t tx_buf[8];
  cli_line_t *line; // 구조체는 주소를 반환하기 때문에 포인터로 선언해서 접근해야됨//

  line = &p_cli->line; // 좀더 편하게 맴버에 접근 하기 위함//


  if (p_cli->state == CLI_RX_IDLE)  // 조합키여부 판단, 처음에는 무조건 실행되서 조합키(여러개 문자)인지 판별함
  {                                 // CLI_RX_IDLE : 1바이트인 문자키 일때는 IDLE임
    switch(rx_data)
    {
      // 엔터
      //
      case CLI_KEY_ENTER:
        if (line->count > 0) // 라인상 데이터 있으면//
        {
          cliLineAdd(p_cli); // 히스토리에 라인에 저장을 해라//
          cliRunCmd(p_cli);  // cli RunCmd 함수// 파싱부터 입력문자에 따라 할 일 정의되 있음//
        }

       // p_cli->line.count =0; // 아래와 같은 표현이지만 *line 선언해서 좀더 간결하게 맴버에 접근함//
        line->count = 0;
        line->cursor = 0;
        line->buf[0] = 0;
        cliShowPrompt(p_cli);
        break;


      case CLI_KEY_ESC:
        p_cli->state = CLI_RX_SP1; // 스페셜키 처리해주세요~
        break;


      // DEL
      //
      case CLI_KEY_DEL:
        if (line->cursor < line->count) // 커서위치가 입력된 문자열보다 작아야 Del 키 동작//
        {
          uint8_t mov_len;

          mov_len = line->count - line->cursor;
          for (int i=1; i<mov_len; i++)
          {
            line->buf[line->cursor + i - 1] = line->buf[line->cursor + i];// 라인커서 위치에서  +1 된 테이터를 왼쪽으로 하나씩 미루는 구문//
          }

          line->count--;
          line->buf[line->count] = 0;

          uartPrintf(p_cli->ch, "\x1B[1P");//VT100 에서 커서위치 문자 삭제하라는 명령어임
        }
        break;


      // 백스페이스
      //
      case CLI_KEY_BACK:
        if (line->count > 0 && line->cursor > 0)
        {
          if (line->cursor == line->count)// 커서가 맨뒤에 있을때
          {
            line->count--;
            line->buf[line->count] = 0;// 마지막에 NULL문자 추가
          }

          if (line->cursor < line->count)//입력된 문자 안에 커서 있을때
          {
            uint8_t mov_len;

            mov_len = line->count - line->cursor;

            for (int i=0; i<mov_len; i++)
            {
              line->buf[line->cursor + i - 1] = line->buf[line->cursor + i];
            }

            line->count--;
            line->buf[line->count] = 0;
          }
        }

        if (line->cursor > 0)
        {
          line->cursor--;
          uartPrintf(p_cli->ch, "\b \b\x1B[1P");//백스페이스 \b \b  VT100
        }
        break;


      default:// 위에 스페셜키가 아닐경우 처리//
        if ((line->count + 1) < line->buf_len)
        {
          if (line->cursor == line->count)// 맨뒤에서 문자가 삽입되면
          {
            uartWrite(p_cli->ch, &rx_data, 1);

            line->buf[line->cursor] = rx_data;
            line->count++;
            line->cursor++;
            line->buf[line->count] = 0;
          }
          if (line->cursor < line->count)// 중간에 문자가 삽입되면
          {
            uint8_t mov_len;

            mov_len = line->count - line->cursor;
            for (int i=0; i<mov_len; i++)
            {
              line->buf[line->count - i] = line->buf[line->count - i - 1];
            }
            line->buf[line->cursor] = rx_data;
            line->count++;
            line->cursor++;
            line->buf[line->count] = 0;

            uartPrintf(p_cli->ch, "\x1B[4h%c\x1B[4l", rx_data);
          }
        }
        break;
    }
  }

  switch(p_cli->state)// 조합키 처리
  {
    case CLI_RX_SP1:
      p_cli->state = CLI_RX_SP2;// 조합키 처리하니 단축키 처리 하지 마셈 의미//
      break;

    case CLI_RX_SP2:
      p_cli->state = CLI_RX_SP3;
      break;

    case CLI_RX_SP3:
      p_cli->state = CLI_RX_IDLE;// 조합키를 처리하는 단계가 아닐때만 수행

      if (rx_data == CLI_KEY_LEFT)
      {
        if (line->cursor > 0)
        {
          line->cursor--;// 커서 왼쪽
          tx_buf[0] = 0x1B;
          tx_buf[1] = 0x5B;
          tx_buf[2] = rx_data;
          uartWrite(p_cli->ch, tx_buf, 3);//VT100 명령어 출력 3개
        }
      }

      if (rx_data == CLI_KEY_RIGHT)
      {
        if (line->cursor < line->buf_len)
        {
          line->cursor++;// 커서 오른쪽
        }
        tx_buf[0] = 0x1B;
        tx_buf[1] = 0x5B;
        tx_buf[2] = rx_data;
        uartWrite(p_cli->ch, tx_buf, 3);//VT100 명령어 출력 3개
      }

      if (rx_data == CLI_KEY_UP)
      {
        cliLineChange(p_cli, true);// 버퍼에 저장되어 있는 값을 호출//
        uartPrintf(p_cli->ch, (char *)p_cli->line.buf);
      }

      if (rx_data == CLI_KEY_DOWN)
      {
        cliLineChange(p_cli, false);
        uartPrintf(p_cli->ch, (char *)p_cli->line.buf);
      }

      if (rx_data == CLI_KEY_HOME)
      {
        uartPrintf(p_cli->ch, "\x1B[%dD", line->cursor);//VT100 명령어
        line->cursor = 0;

        p_cli->state = CLI_RX_SP4;
      }

      if (rx_data == CLI_KEY_END)
      {
        uint16_t mov_len;

        if (line->cursor < line->count)
        {
          mov_len = line->count - line->cursor;
          uartPrintf(p_cli->ch, "\x1B[%dC", mov_len);//VT100 명령어
        }
        if (line->cursor > line->count)
        {
          mov_len = line->cursor - line->count;
          uartPrintf(p_cli->ch, "\x1B[%dD", mov_len);//VT100 명령어
        }
        line->cursor = line->count;
        p_cli->state = CLI_RX_SP4;
      }
      break;

    case CLI_RX_SP4:
      p_cli->state = CLI_RX_IDLE;
      break;
  }



  cliShowLog(p_cli); // log용 미러 화면을 추가시 관련 내용을 뿌리는 함수 //

  return ret;
}

void cliLineClean(cli_t *p_cli)
{
  p_cli->line.count   = 0;
  p_cli->line.cursor  = 0;
  p_cli->line.buf_len = CLI_LINE_BUF_MAX - 1;
  p_cli->line.buf[0]  = 0;
}

void cliLineAdd(cli_t *p_cli)
{

  p_cli->line_buf[p_cli->hist_line_last] = p_cli->line;

  if (p_cli->hist_line_count < CLI_LINE_HIS_MAX)
  {
    p_cli->hist_line_count++;
  }

  p_cli->hist_line_i    = p_cli->hist_line_last;
  p_cli->hist_line_last = (p_cli->hist_line_last + 1) % CLI_LINE_HIS_MAX;
  p_cli->hist_line_new  = true;
}

void cliLineChange(cli_t *p_cli, int8_t key_up)
{
  uint8_t change_i;


  if (p_cli->hist_line_count == 0)
  {
    return;
  }


  if (p_cli->line.cursor > 0)
  {
    uartPrintf(p_cli->ch, "\x1B[%dD", p_cli->line.cursor);
  }
  if (p_cli->line.count > 0)
  {
    uartPrintf(p_cli->ch, "\x1B[%dP", p_cli->line.count);
  }


  if (key_up == true)
  {
    if (p_cli->hist_line_new == true)
    {
      p_cli->hist_line_i = p_cli->hist_line_last;
    }
    p_cli->hist_line_i = (p_cli->hist_line_i + p_cli->hist_line_count - 1) % p_cli->hist_line_count;
    // 최근에 저장된 인댁스 에서 (-1)전꺼면 상위로 올라가는 의미 UP일떄는 -1//
    change_i = p_cli->hist_line_i;//즉 최종 출력할 인덱스 번호를 저장
  }
  else
  {
    p_cli->hist_line_i = (p_cli->hist_line_i + 1) % p_cli->hist_line_count;
    change_i = p_cli->hist_line_i;
  }

  p_cli->line = p_cli->line_buf[change_i]; // 현재 라인에 최종 출력할 인덱스 버퍼를 출력시켜줌
  p_cli->line.cursor = p_cli->line.count;  // 커서 위치를 맨뒤로 보내줌

  p_cli->hist_line_new = false;
}

bool cliRunCmd(cli_t *p_cli)
{
  bool ret = false;


  if (cliParseArgs(p_cli) == true)
  {
    cliPrintf("\r\n");

    cliToUpper(p_cli->argv[0]); // 0번째는 명령어, 대문자로 변경하는 함수//

    for (int i=0; i < p_cli->cmd_count; i++) // List에 저장된 CMD 만큼 루프 돌려줌
    {
      if (strcmp(p_cli->argv[0], p_cli->cmd_list[i].cmd_str) == 0) // 입력된 명령어(HELP, MD, LED)와 저장된 명령어가 같으면 아래 실행//
      {
        p_cli->cmd_args.argc =  p_cli->argc - 1;     // 입력된 문자열 갯수는 하나 뺴줌. CMD는 제외
        p_cli->cmd_args.argv = &p_cli->argv[1];      // 입력된 문자열 배열 0 뺴줌. CMD는 제외
        p_cli->cmd_list[i].cmd_func(&p_cli->cmd_args);  // 해당하는 명령어의 콜백 함수를 실행//HELP, MD, LED 가르키는 함수 실행//
        break;
      }
    }
  }

  return ret;
}

bool cliParseArgs(cli_t *p_cli)
{
  bool ret = false;
  char *tok;
  char *next_ptr;
  uint16_t argc = 0;
  static const char *delim = " \f\n\r\t\v"; //\r 열복귀 \n 줄바꿈\t 탭문자
  char *cmdline;
  char **argv;

  p_cli->argc = 0;

  cmdline = (char *)p_cli->line.buf;
  argv    = p_cli->argv;

  argv[argc] = NULL;

  // 입력 CMD 를 파싱하는 부분//
  for (tok = strtok_r(cmdline, delim, &next_ptr); tok; tok = strtok_r(NULL, delim, &next_ptr))
  {
    argv[argc++] = tok;  // 입력된 명령어 문자열 배열형태로 argv 에  저장//
  }

  // ex test 0 1 2 3 입력 //  argv[0] = test,  argv[1] = 0, //

  p_cli->argc = argc; // 입력된 파라미터 문자열의 갯수

  if (argc > 0)
  {
    ret = true;
  }

  return ret;
}

void cliPrintf(const char *fmt, ...)
{
  va_list arg;
  va_start (arg, fmt);
  int32_t len;
  cli_t *p_cli = &cli_node;


  len = vsnprintf(p_cli->print_buffer, 256, fmt, arg);
  va_end (arg);

  uartWrite(p_cli->ch, (uint8_t *)p_cli->print_buffer, len);
}

void cliToUpper(char *str)
{
  uint16_t i;
  uint8_t  str_ch;

  for (i=0; i<CLI_CMD_NAME_MAX; i++)
  {
    str_ch = str[i];

    if (str_ch == 0)
    {
      break;
    }

    if ((str_ch >= 'a') && (str_ch <= 'z'))
    {
      str_ch = str_ch - 'a' + 'A';
    }
    str[i] = str_ch;
  }

  if (i == CLI_CMD_NAME_MAX)
  {
    str[i-1] = 0;
  }
}

int32_t cliArgsGetData(uint8_t index)
{
  int32_t ret = 0;
  cli_t *p_cli = &cli_node;


  if (index >= p_cli->cmd_args.argc)
  {
    return 0;
  }

  ret = (int32_t)strtoul((const char * ) p_cli->cmd_args.argv[index], (char **)NULL, (int) 0);
  // strtoul()은 정수 문자열 진수를 선택하여 unsigned long 타입형 정수로 변환함수 (진수도 선택가능)
  // 숫자가 아닌 문자를 만나면 그 포인터 위치도 구해줌(2번쨰 변수에 저장)
  // strtoul( 입려문자 , 문자열위치(숫자로변경못하는문자열) ,  진수 선택(10진수..등등)
  // 진수가 NULL 이면 2번째 변수의 시작위치를 구하지 않음
  // 즉 파싱한 p_cli->cmd_args.argv[index] 의 문자를 입력받아 strtoul 을 이용하여 unsigned long 으로 변환하여 ret에 리턴시킴//

  return ret;
}

float cliArgsGetFloat(uint8_t index)
{
  float ret = 0.0;
  cli_t *p_cli = &cli_node;


  if (index >= p_cli->cmd_args.argc)
  {
    return 0;
  }

  ret = (float)strtof((const char * ) p_cli->cmd_args.argv[index], (char **)NULL);

  return ret;
}

char *cliArgsGetStr(uint8_t index)
{
  char *ret = NULL;
  cli_t *p_cli = &cli_node;


  if (index >= p_cli->cmd_args.argc)
  {
    return 0;
  }

  ret = p_cli->cmd_args.argv[index];

  return ret;
}

bool cliArgsIsStr(uint8_t index, char *p_str)
{
  bool ret = false;
  cli_t *p_cli = &cli_node;


  if (index >= p_cli->cmd_args.argc)
  {
    return 0;
  }

  if(strcmp(p_str, p_cli->cmd_args.argv[index]) == 0)
  {
    ret = true;
  }

  return ret;
}

bool cliKeepLoop(void)
{
  cli_t *p_cli = &cli_node;


  if (uartAvailable(p_cli->ch) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool cliAdd(const char *cmd_str, void (*p_func)(cli_args_t *))
{
  bool ret = true;
  cli_t *p_cli = &cli_node;
  uint16_t index;

  if (p_cli->cmd_count >= CLI_CMD_LIST_MAX)
  {
    return false;
  }

  index = p_cli->cmd_count;

  strcpy(p_cli->cmd_list[index].cmd_str, cmd_str); //입력된 명령어를(cmd_str) CMD LIST에 복사해서 등록해줌
  p_cli->cmd_list[index].cmd_func = p_func; // 명령어의 콜백 함수를 등록해줌

  cliToUpper(p_cli->cmd_list[index].cmd_str);  // 명령어를 대문자 변환//

  p_cli->cmd_count++;

  return ret;
}

void cliShowList(cli_args_t *args)
{
  cli_t *p_cli = &cli_node;


  cliPrintf("\r\n");
  cliPrintf("---------- cmd list ---------\r\n");

  for (int i=0; i<p_cli->cmd_count; i++)
  {
    cliPrintf(p_cli->cmd_list[i].cmd_str);
    cliPrintf("\r\n");
  }

  cliPrintf("-----------------------------\r\n");
}

void cliMemoryDump(cli_args_t *args)
{
  int idx, size = 16;
  unsigned int *addr;
  int idx1, i;
  unsigned int *ascptr;
  unsigned char asc[4];
  uint32_t pre_time;

  int    argc = args->argc;
  char **argv = args->argv;


  if(args->argc < 1)
  {
    cliPrintf(">> md addr [size] \n");
    return;
  }

  if(argc > 1)
  {
    size = (int)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  }
  addr   = (unsigned int *)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  ascptr = (unsigned int *)addr;

  cliPrintf("\n   ");

  pre_time = millis();

  for (idx = 0; idx<size; idx++)
  {
    if((idx%4) == 0)
    {
      cliPrintf(" 0x%08X: ", (unsigned int)addr);
    }
    cliPrintf(" 0x%08X", *(addr));

    if ((idx%4) == 3)
    {
      cliPrintf ("  |");
      for (idx1= 0; idx1< 4; idx1++)
      {
        memcpy((char *)asc, (char *)ascptr, 4);
        for (i=0;i<4;i++)
        {
          if (asc[i] > 0x1f && asc[i] < 0x7f)
          {
            cliPrintf("%c", asc[i]);
          }
          else
          {
            cliPrintf(".");
          }
        }
        ascptr+=1;
      }
      cliPrintf("|\n   ");
    }
    addr++;
  }

  cliPrintf("  time MD : %d ms\n", millis() - pre_time);
}

#endif

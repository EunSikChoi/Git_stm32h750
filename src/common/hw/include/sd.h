/*
 * sd.h
 *
 *  Created on: 2021. 11. 16.
 *      Author: 82109
 */

#ifndef SRC_COMMON_HW_INCLUDE_SD_H_
#define SRC_COMMON_HW_INCLUDE_SD_H_





#include "hw_def.h"

#ifdef _USE_HW_SD


typedef enum
{
  SDCARD_IDLE,
  SDCARD_CONNECTTING,
  SDCARD_CONNECTED,
  SDCARD_DISCONNECTED,
  SDCARD_ERROR
} sd_state_t;


typedef struct
{
  uint32_t card_type;                    /*!< Specifies the card Type                         */
  uint32_t card_version;                 /*!< Specifies the card version                      */
  uint32_t card_class;                   /*!< Specifies the class of the card class           */
  uint32_t rel_card_Add;                 /*!< Specifies the Relative Card Address             */
  uint32_t block_numbers;                /*!< Specifies the Card Capacity in blocks           */
  uint32_t block_size;                   /*!< Specifies one block size in bytes               */
  uint32_t log_block_numbers;            /*!< Specifies the Card logical Capacity in blocks   */
  uint32_t log_block_size;               /*!< Specifies logical block size in bytes           */
  uint32_t card_size;
} sd_info_t;


bool sdInit(void);        // sd 초기화 함수
bool sdReInit(void);      // sd 재 삽입시 초기화 함수
bool sdDeInit(void);      // sd 초기화 해제 함수
bool sdIsInit(void);      // sd 초기화 중
bool sdIsDetected(void);  // sd 카드 삽입 상태 확인 함수
bool sdGetInfo(sd_info_t *p_info); // sd 정보 GET 함수
bool sdIsBusy(void);               // sd 현재 전송 진행 확인 함수
bool sdIsReady(uint32_t timeout);  // sd 현재 전송 종료 확인 함수

sd_state_t sdUpdate(void);  // sd 삽입 여부 확인 하는 함수

bool sdReadBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms);  // sd 메모리 Read 함수
bool sdWriteBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms); // sd 메모리 write 함수
bool sdEraseBlocks(uint32_t start_addr, uint32_t end_addr);                                            // sd 메모리 erase 함수


#endif


#endif /* SRC_COMMON_HW_INCLUDE_SD_H_ */

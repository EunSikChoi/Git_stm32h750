/*
 * flash.c
 *
 *  Created on: 2022. 1. 10.
 *      Author: 82109
 */



#include "flash.h"
#include "cli.h"

#define FLASH_SECTOR_MAX        4
#define FLASH_SECTOR_OFFSET     4

typedef struct
{
  uint32_t addr;
  uint32_t length;
}flash_tbl_t;

static flash_tbl_t flash_tbl[FLASH_SECTOR_MAX] =
{
//		{ 0x08000000, 16*1024},
//		{ 0x08004000, 16*1024},
//		{ 0x08008000, 16*1024},
//		{ 0x0800C000, 16*1024},
		{ 0x08010000, 64*1024},
    { 0x08020000, 128*1024},
    { 0x08044000, 128*1024},
    { 0x08060000, 128*1024}
};

static flash_tbl_t flash_tbl[FLASH_SECTOR_MAX];

static bool flashInSector(uint16_t sector_num ,uint32_t addr, uint32_t length);

#ifdef _USE_HW_CLI
  static void cliFlash(cli_args_t *args);
#endif

bool flashInit(void)
{

	#ifdef _USE_HW_CLI
  	cliAdd("flash", cliFlash);
	#endif

  return true;
}


bool flashErase(uint32_t addr, uint32_t length)
{
  bool ret = false;
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef init;
  uint32_t page_error;

  int16_t  start_sector_num = -1;
  uint32_t sector_count = 0;
  int16_t  ces = 0;


  for (int i=0; i<FLASH_SECTOR_MAX; i++)
  {
    if (flashInSector(i, addr, length) == true) // 총 섹터크기만큼 루프 수행
    {
      if (start_sector_num < 0) //flashInSector() 조건 만족시 1번만 진입. 시작 섹터 주소 저장 //
      {
        start_sector_num = i +FLASH_SECTOR_OFFSET ;
        //cliPrintf("sector NUM %d : \n", start_sector_num);
      }
      sector_count++; // flashInSector() 조건 만족시 매번 ++1 수행. 결국 지워야할 섹터 갯수 정보임  //
    }
  }

  ces = sector_count;

  if (sector_count > 0)
  {
    HAL_FLASH_Unlock();

    init.TypeErase   	= FLASH_TYPEERASE_SECTORS;
    init.Banks       	= FLASH_BANK_1;
    init.Sector		 		= start_sector_num;//start_sector_num ;		  // 기존은 주소였는데 Sector로 변경
    init.NbSectors    = sector_count;
    init.VoltageRange	= FLASH_VOLTAGE_RANGE_3;  // 전원 범위 선택

    status = HAL_FLASHEx_Erase(&init, &page_error);
    if (status == HAL_OK)
    {
      ret = true;
    }

    HAL_FLASH_Lock();
  }

  return ret;
}


bool flashWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
   bool ret = true;

   HAL_StatusTypeDef status;

   HAL_FLASH_Unlock();

   for(int i = 0 ; i < length ; i++ )
   {
     uint32_t  data;
     data  = p_data[i + 0] << 0 ;

      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr+i, (uint64_t) data);

      if(status != HAL_OK)
      {
        ret = false;
        break;

      }
   }

   HAL_FLASH_Lock();

   return ret;

}


bool flashRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  uint8_t *p_byte = (uint8_t *)addr;


  for (int i=0; i<length; i++)
  {
    p_data[i] = p_byte[i];
  }

  return ret;
}

bool flashInSector(uint16_t sector_num ,uint32_t addr, uint32_t length)
{
  bool ret = false;

  uint32_t sector_start;
  uint32_t sector_end;
  uint32_t flash_start;
  uint32_t flash_end;


  sector_start = flash_tbl[sector_num].addr; // load addr // save at flashinit //
  sector_end   = flash_tbl[sector_num].addr + flash_tbl[sector_num].length - 1;
  flash_start  = addr;              // 지우고자 하는 시작주소
  flash_end    = addr + length - 1; // 지우고자 하는 메모리 크기


  if (sector_start >= flash_start && sector_start <= flash_end)
  {
    ret = true;
    //cliPrintf("sector_start 01x%d : \n", ret);

  }

  if (sector_end >= flash_start && sector_end <= flash_end)
  {
    ret = true;
    //cliPrintf("sector_start 02x%d : \n", ret);
  }

  if (flash_start >= sector_start && flash_start <= sector_end)
  {
    ret = true;
    //cliPrintf("sector_start 03x%d : \n", ret);
  }

  if (flash_end >= sector_start && flash_end <= sector_end)
  {
    ret = true;
    //cliPrintf("sector_start 04x%d : \n", ret);
  }


  return ret;
}

#ifdef _USE_HW_CLI
void cliFlash(cli_args_t *args)
{

	bool ret = false;

    if(args->argc == 1 && args->isStr(0, "info") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
		{
				for( int i = 0 ; i < FLASH_SECTOR_MAX ; i++)
				{
					cliPrintf("0x%X : %d KB\n", flash_tbl[i].addr ,flash_tbl[i].length);
				}

				ret = true;
		}

    if(args->argc == 3 && args->isStr(0, "read") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
		{

				uint32_t addr;
				uint32_t length;

				addr   = (uint32_t)args->getData(1);
				length = (uint32_t)args->getData(2);

				if ((addr >= 0x08080001) || (addr < 0x08010000))
				{

					cliPrintf("<< flash addr ERR >> \n");
					ret = false;

				}
				else
				{

					 for (int i =0; i < length ; i++)
					 {
						 cliPrintf("0x%X : 0x%X \n", addr+i ,  *((uint8_t *)addr+i));
					 }

					 ret = true;

					 if( ret == true)
					 {
						 cliPrintf("flash read OK\n");
					 }
				}
		}

    if(args->argc == 3 && args->isStr(0, "erase") == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
		{
					uint32_t addr;
					uint32_t length;

					addr   = (uint32_t) args->getData(1);
					length = (uint32_t) args->getData(2);

					if ((addr >= 0x08080001) || (addr < 0x08000000))
					{

						cliPrintf("<< flash addr ERR >> \n");
						ret = false;

					}
					else
					{
							 ret = flashErase(addr, length);

							if( ret == true)
							{
								cliPrintf("flash erase OK\n");
							}
					}

		}

    if((args->argc == 4 && args->isStr(0, "write") && args->getData(3) <= 4) == true) //여기서 설정한 toggle 과 동일 문자가 입력하면 아래 Loop가 실행됨//
    {

        uint32_t addr;
        uint32_t data;
        uint32_t length;  //4바이트면 32비트 데이터 오버됨//


        addr    = (uint32_t) args->getData(1);
        data    = (uint32_t) args->getData(2);
        length  = (uint32_t) args->getData(3);


        if ((addr >= 0x08080001) || (addr < 0x08000000))
        {
             cliPrintf("<< flash addr ERR >> \n");
             ret = false;
        }
        else
        {
            if( flashWrite(addr, (uint8_t *)&data, length) == true )
            {
              cliPrintf("flash write OK\n");
              ret = true;
            }
            else
            {
              cliPrintf("flash write fail\n");
            }
        }

    }

		if(ret != true)
		{
			cliPrintf("flash info\n");
			cliPrintf("flash read  addr length\n");
			cliPrintf("flash erase addr length\n");
			cliPrintf("flash write addr data length<=4\n");

		}

}

#endif

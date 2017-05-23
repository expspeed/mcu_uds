/***************************************************************************//**
    \file          uds-program.c
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0.00
    \date          2017-05-12
    \description   uds program code, include sdram program and flash program
*******************************************************************************/

/*******************************************************************************
    Include Files
*******************************************************************************/
#include "uds_program.h"
#include "uds_type.h"
#include "stm32f10x_flash.h"
#include "uds_util.h"
#include "iap_util.h"
/*******************************************************************************
    Function  Definition
*******************************************************************************/


/**
 * flash_erase - erase flash
 *
 * @write_addr:
 * @write_len :
 *
 * returns:
 *     erase len
 */
uint16_t
flash_erase (uint32_t erase_addr, uint16_t erase_len)
{
  uint32_t end_addr;
  FLASH_Status FLASHStatus = FLASH_COMPLETE;

  end_addr = erase_addr + erase_len -1;
	/* Clears the FLASH's pending flags*/
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	
  for (; erase_addr <= end_addr; )
  {
    /* This function will erase the page that contains the addr */
    FLASHStatus = FLASH_ErasePage(erase_addr);
    if (FLASHStatus != FLASH_COMPLETE)
      return 0;
    erase_addr += (P_FLASH_PAGE_SIZE - erase_addr%P_FLASH_PAGE_SIZE);
  }
  return (erase_len + (P_FLASH_PAGE_SIZE - end_addr%P_FLASH_PAGE_SIZE - 1));
}
/**
 * sdram_program - program the sdram, write flash driver code to ram
 *
 * @write_addr:
 * @msg_buf   :
 * @write_len :
 *
 * returns:
 *     program len
 */
uint16_t
sdram_program (uint32_t w_addr, const uint8_t src_buf[], uint16_t w_len)
{
    return w_len;
}

/**
 * flash_program - program the sdram, write flash driver code to ram
 *
 * @write_addr:
 * @msg_buf   :
 * @write_len :
 *
 * returns:
 *     program len
 */
uint16_t
flash_program (uint32_t w_addr, const uint8_t src_buf[], uint16_t w_len)
{

  uint16_t w_cnt;
  uint32_t w_val;
	
  for (w_cnt = 0; w_cnt < w_len; w_cnt += 4)
  {
    w_val = can_to_littl(&src_buf[w_cnt]);
    /* Clears all pending flags */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    /* Program the data received into STM32F10x Flash */
    FLASH_ProgramWord(w_addr+w_cnt, w_val);

    if (*(uint32_t*)(w_addr+w_cnt) != w_val)
	{
		IAPWait1ms(1);
		if (*(uint32_t*)(w_addr+w_cnt) != w_val)
			break;
	}
  }
  return w_cnt;
}

/****************EOF****************/

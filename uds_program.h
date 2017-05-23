/***************************************************************************//**
    \file          uds-program.h
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0
    \date          2017-05-04
    \description   uds service
*******************************************************************************/
#ifndef	__UDS_PROGRAM_H_
#define	__UDS_PROGRAM_H_
/*******************************************************************************
    Include Files
*******************************************************************************/
#include <stdint.h>
#include "stm32f10x_flash.h"
#include "uds_type.h"
#include "main.h"
/*******************************************************************************
    Type Definition
*******************************************************************************/

/* uds programming status */
typedef enum __UDS_PROG_T_
{
    UDS_PROG_NONE = 0,
    UDS_PROG_FINGERPRINT_COMPLETE,
    UDS_PROG_FLASH_DRIVER_DOWNLOADING,
    UDS_PROG_FLASH_DRIVER_DOWNLOAD_COMPLETE,
    UDS_PROG_FLASH_DRIVER_CRC32_VALID,
    UDS_PROG_ERASE_MEMORY_COMPLETE,
    UDS_PROG_APP_DOWNLOADING,
    UDS_PROG_APP_DOWNLOAD_COMPLETE,
    UDS_PROG_APP_CRC32_VALID,
    UDS_PROG_ENUM_MAX
}uds_prog_t;


typedef enum __UDS_DOWNLOAD_T_
{
    UDS_DOWN_NONE = 0,
    UDS_DOWN_FLASH_DRIVER,
    UDS_DOWN_DATA,
    UDS_DOWN_INVLID
}uds_down_st;



/* for uds data update/download */
#define UDS_VALID_DATA_FORMAT             (0x00)
#define UDS_VALID_ADDR_LEN_FORMAT         (0x44)
#define UDS_MAX_NUM_BLOCK_LEN             (0x20) /* maxNumberOfBlockLength 32 */

#define UDS_VALID_START_MEM_ADDR          (0x8000400ul)
#define UDS_MAX_MEM_SIZE

#define UDS_LENGTH_FORMAT_ID              (0x40) /* lengthFormatIdentifier */



/* bootloader and app memory map */
#define P_IAP_START_ADDR      (0x08000000ul)
#define P_DRIVER_START_ADDR   (0x08000c70ul)
#define P_DRIVER_ENDxx_ADDR   (0x08000df7ul)
#define FLASH_DRIVER_LENGTH   (0x188)
#define P_APP_START_ADDR      (0x08004000ul)
#define P_APP_ENDxx_ADDR      (P_IAP_START_ADDR + P_FLASH_SIZE - P_FLASH_PAGE_SIZE - 1)


/* App Preset Id For Check Dependencies */
#define APP_MARK_ID        (0x12A00ABCul)
#define APP_MARK_ID_ADDR   (0x080047fcul)
#define GET_APP_MARK_ID()  (*(uint32_t*)APP_MARK_ID_ADDR)





#define P_FLASH_PAGE_SIZE     (0x800)
#define P_FLASH_SIZE          (0x40000)   /* 256K */
#define P_FLASH_PAGE_NUM      (P_FLASH_SIZE/P_FLASH_PAGE_SIZE -1)       /* 128 - 1 */




/**
 By default,  stack top pointer of the program is stored in 
 the first four bytes，and Reset vector address is stored in 
 4th to 7th bytes
 But I have specify the app start id at the first four bytes,
 So stack ptr and reset vector offset 4 bytes more */
#define P_APP_STACK_PTR_ADDR  (P_APP_START_ADDR)
#define P_APP_RESET_PTR_ADDR  (P_APP_START_ADDR+4)


#define UDS_DATA_ADDR     (P_IAP_START_ADDR + P_FLASH_SIZE - P_FLASH_PAGE_SIZE) /* update request flag stored in the last page */

/* board debug macro */

#define CONF_STAY_IN_BOOT     1


#define NON_UPDATE_CODE       0
#define USB_UPDATE_CODE       (0xc3a55a3cul)
#define UDS_UPDATE_CODE       (0xa5c33c5aul)



/*******************************************************************************
    extern Varaibles
*******************************************************************************/
extern uds_prog_t uds_prog_st;
extern uint32_t   crc32_calc;


/*******************************************************************************
    Function  Extern
*******************************************************************************/
#define FLASH_UNLOCK FLASH_Unlock();
#define FLASH_LOCK   FLASH_Lock();

uint16_t
flash_erase (uint32_t w_addr, uint16_t w_len);

uint16_t
sdram_program (uint32_t w_addr, const uint8_t src_buf[], uint16_t w_len);


uint16_t
flash_program (uint32_t w_addr, const uint8_t src_buf[], uint16_t w_len);

#endif
/* end of file*/

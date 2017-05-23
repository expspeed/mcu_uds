/***************************************************************************//**
    \file          uds-util.h
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0
    \date          2016-09-28
    \description   uds util
*******************************************************************************/
#ifndef	__UDS_UTIL_H_
#define	__UDS_UTIL_H_
/*******************************************************************************
    Include Files
*******************************************************************************/
#include <stdint.h>
#include "uds_type.h"
/*******************************************************************************
    Type Definition
*******************************************************************************/
#define CRC32_INIT 0xFFFFFFFFul
#define CRC32_POLY 0x04c11db7ul

/*******************************************************************************
    Function  Definition
*******************************************************************************/

uint8_t
rand_u8 (uint8_t id);

int
host_to_canl (uint8_t buf[], uint32_t val);

int
host_to_cans (uint8_t buf[], uint16_t val);

uint32_t
can_to_hostl (const uint8_t buf[]);

uint32_t
can_to_littl (const uint8_t buf[]);
/**
 * crc32_continue - calculate crc32 once time
 *
 * @data: start position to be transformed
 * @len :
 * returns:
 *     transformed value
 */
uint32_t
crc32_continue (uint8_t *data, uint32_t len);

/**
 * crc32_discontinue - calculate 
 *
 * @buf: ther buffer to be transformed
 *
 * returns:
 *     transformed value
 */
uint32_t
crc32_discontinue (uint32_t org_rst, uint8_t *data, uint32_t len);
#endif
/****************EOF****************/

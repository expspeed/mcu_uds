/***************************************************************************//**
    \file          uds-util.c
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0.00
    \date          2016-09-27
    \description   uds network code, base on ISO 14229
*******************************************************************************/

/*******************************************************************************
    Include Files
*******************************************************************************/
#include <stdlib.h>
#include "ucos_ii.h"
#include "uds_type.h"
#include "uds_util.h"
#include "timx.h" 
/*******************************************************************************
    Type Definition
*******************************************************************************/

/*******************************************************************************
    Function  Definition
*******************************************************************************/
/**
 * rand_u8 - get a random uint8_t
 *
 * @void  :
 *
 * returns:
 *     a uint8
 */
uint8_t
rand_u8 (uint8_t id)
{
	uint32_t ticks;
	ticks = get_time_ms() + id;
	srand(ticks);

	return (rand() % 255);
}

/**
 * host_to_canl - transmit a long or short int to can-net endian
 *
 * @buf: ther buffer to storage the result
 * @val: the value to be transformed
 *
 * returns:
 *     0 - ok, -1 - err
 */
int
host_to_canl (uint8_t buf[], uint32_t val)
{
	if (buf == NULL) return -1;

	buf[0] = (val>>24) & 0xff;
	buf[1] = (val>>16) & 0xff;
	buf[2] = (val>>8) & 0xff;
	buf[3] = (val>>0) & 0xff;

	return 0;
}

int
host_to_cans (uint8_t buf[], uint16_t val)
{
	if (buf == NULL) return -1;

	buf[0] = (val>>8) & 0xff;
	buf[1] = (val>>0) & 0xff;

	return 0;
}


/**
 * can_to_hostl - transmit  can-net endian buffer to long or short int
 *
 * @buf: ther buffer to be transformed
 *
 * returns:
 *     transformed value
 */
uint32_t
can_to_hostl (const uint8_t buf[])
{
	uint32_t val;
	if (buf == NULL) return 0;
#if 0
    val = 0;
	val |= ((uint32_t)buf[0]) << 24;
	val |= ((uint32_t)buf[1]) << 16;
	val |= ((uint32_t)buf[2]) << 8;
	val |= ((uint32_t)buf[3]) << 0;
#else
	val  = ((uint32_t)buf[3]) << 0;
	val += ((uint32_t)buf[2]) << 8;
	val += ((uint32_t)buf[1]) << 16;
	val += ((uint32_t)buf[0]) << 24;
#endif
	return val;
}


/**
 * can_to_littl - transmit  can-net endian buffer to a little endian
 *
 * @buf: ther buffer to be transformed
 *
 * returns:
 *     transformed value
 */
uint32_t
can_to_littl (const uint8_t buf[])
{
	uint32_t val;
	if (buf == NULL) return 0;

	val  = ((uint32_t)buf[0]) << 0;
	val += ((uint32_t)buf[1]) << 8;
	val += ((uint32_t)buf[2]) << 16;
	val += ((uint32_t)buf[3]) << 24;

	return val;
}

/**
 * crc32_continue - calculate crc32 once time
 *
 * @data: start position to be transformed
 * @len :
 * returns:
 *     transformed value
 */
uint32_t
crc32_continue (uint8_t *data, uint32_t len)
{
    uint32_t result;
    uint32_t i,j;
    uint8_t  octet;

    result = ~CRC32_INIT;
    
    for (i=0; i<len; i++)
    {
        octet = *(data++);
        for (j=0; j<8; j++)
        {
            if ((octet >> 7) ^ (result >> 31))
            {
                result = (result << 1) ^ CRC32_POLY;
            }
            else
            {
                result = (result << 1);
            }
            octet <<= 1;
        }
    }
    
    return ~result;             /* The complement of the remainder */
}

/**
 * crc32_discontinue - calculate crc32 discontinue
 *
 * @data: start position to be transformed
 *
 * returns:
 *     transformed value
 */
uint32_t
crc32_discontinue (uint32_t org_rst, uint8_t *data, uint32_t len)
{
    uint32_t result;
    uint32_t i,j;
    uint8_t  octet;

    result = ~org_rst;

    for (i=0; i<len; i++)
    {
        octet = *(data++);
        for (j=0; j<8; j++)
        {
            if ((octet >> 7) ^ (result >> 31))
            {
                result = (result << 1) ^ CRC32_POLY;
            }
            else
            {
                result = (result << 1);
            }
            octet <<= 1;
        }
    }

    return ~result;             /* The complement of the remainder */
}



/****************EOF****************/

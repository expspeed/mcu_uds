/***************************************************************************//**
    \file          uds-status.c
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0.00
    \date          2016-10-10
    \description   uds status code, include session and Security access
*******************************************************************************/

/*******************************************************************************
    Include Files
*******************************************************************************/
#include <string.h>
#include "uds_status.h"
#include "uds_util.h"
#include "uds_service.h"
#include "uds_program.h"
/*******************************************************************************
    Type Definition
*******************************************************************************/

/*******************************************************************************
    Global Varaibles
*******************************************************************************/

/*******************************************************************************
    Uds externed data
*******************************************************************************/

/* uds externed data */
uds_data_t uds_data = \
{NON_UPDATE_CODE, UDS_RESET_NONE, 0x00, 0xff, 0x00};

#define UDS_DATA_SIZE  sizeof (uds_data_t)

bool_t stay_in_boot = FALSE;

/*******************************************************************************
    Function  Definition
*******************************************************************************/


/**
 * seedTOKey - the algorithms to compute access key
 *
 * @seed: the seed
 *
 * returns:
 *     the key
 */
static uint32_t
seedTOKey( uint32_t seed )
{
	uint8_t i;
	uint32_t key;
	key = UNLOCKKEY;
	if(!(( seed == UNLOCKSEED )
	||( seed == UNDEFINESEED )))
	{
		for( i = 0; i < 35; i++ )
		{
			if( seed & SEEDMASK )
			{
				seed = seed << SHIFTBIT;
				seed = seed ^ ALGORITHMASK;
			}
			else
			{
				seed = seed << SHIFTBIT;
			}
		}
		key = seed;
	}
	return key;
}

/**
 * uds_security_access - check the key of Security Access
 *
 * @key_buf:  recieved key buff
 * @seed   :  original seed
 *
 * returns:
 *     0 - success， -1 - fail
 */
int
uds_security_access (uint8_t key_buf[], uint8_t seed_buf[])
{
	uint32_t key;
	uint32_t seed;
    key =  can_to_hostl (key_buf);
	seed = can_to_hostl (seed_buf);

	if (key == seedTOKey(seed))
	    return 0;
	else
	    return -1;
}


/**
 * uds_session_check - check the session request
 *
 * @seed: the seed
 *
 * returns:
 *     the key
 */
uint8_t
uds_session_check (uint8_t curr_session, uint8_t sub_function)
{
	uint8_t ret = NRC_NONE;

    switch (sub_function)
	{
		case UDS_SESSION_STD:
		    break;
		case UDS_SESSION_PROG:
		if (curr_session == UDS_SESSION_STD)
		    ret = NRC_CONDITIONS_NOT_CORRECT;
		    break;
		case UDS_SESSION_EOL:
		if (curr_session == UDS_SESSION_PROG)
		    ret = NRC_CONDITIONS_NOT_CORRECT;
		    break;
		default:
		    ret = NRC_SUBFUNCTION_NOT_SUPPORTED;
		    break;
	}

	return ret;
	/**
	 If the server runs the programmingSession in the boot software,
	 the programmingSession shall only be left via an ECUReset (11 hex) service initiated by the client,
	 a DiagnosticSessionControl (10 hex) service with sessionType equal to defaultSession,
	 or a session layer timeout in the server.
	 */
}

/**
 * load_all_uds_data - load all extern data from flash to ram
 *
 * @void:
 *
 * returns:
 *     void
 */
extern void 
load_all_uds_data (void)
{

  memcpy (&uds_data, (void*)UDS_DATA_ADDR, UDS_DATA_SIZE);

  if (uds_data.update_request == UDS_UPDATE_CODE)
    uds_session = UDS_SESSION_PROG;
  if (uds_data.update_request == USB_UPDATE_CODE)
	stay_in_boot = TRUE;
  if (uds_data.app_valid_flag == 0xFF)
	uds_data.app_valid_flag = 0x01;

  uds_data.update_request = NON_UPDATE_CODE;
}

/**
 * save_all_uds_data - save all extern data to flash
 *
 * @void:
 *
 * returns:
 *     0 save ok. 1 save err
 */
extern uint8_t 
save_all_uds_data (void)
{
  if (!memcmp (&uds_data, (void*)UDS_DATA_ADDR, UDS_DATA_SIZE))
    return 0;

  flash_erase (UDS_DATA_ADDR, P_FLASH_PAGE_SIZE);
  flash_program (UDS_DATA_ADDR, (uint8_t *)&uds_data, UDS_DATA_SIZE);

  if (!memcmp (&uds_data, (void*)UDS_DATA_ADDR, UDS_DATA_SIZE))
    return 0;
  else
	return 1;
}
/****************EOF****************/

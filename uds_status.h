/***************************************************************************//**
    \file          uds-status.h
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0
    \date          2016-10-10
    \description   uds status code, include session and Security access
*******************************************************************************/
#ifndef	__UDS_STATUS_H_
#define	__UDS_STATUS_H_
/*******************************************************************************
    Include Files
*******************************************************************************/
#include <stdint.h>
#include "uds_type.h"
/*******************************************************************************
    Type Definition
*******************************************************************************/
/* SECURITYACCESS */
#define UNLOCKKEY					0x00000000
#define UNLOCKSEED					0x00000000
#define UNDEFINESEED				0xFFFFFFFF
#define SEEDMASK					0x80000000
#define SHIFTBIT					1
#define ALGORITHMASK				0x4D313232


typedef struct __UDS_DATA_T__
{
    volatile uint32_t update_request;
	volatile uint8_t  uds_reset_type;
	volatile uint8_t  uds_sadelay;
	volatile uint8_t  uds_safai_rcnt;
	volatile uint8_t  app_valid_flag;
}uds_data_t;
/*******************************************************************************
    Extern  Varaibles
*******************************************************************************/
extern uds_data_t uds_data;
extern bool_t stay_in_boot;
/*******************************************************************************
    Function  Definition
*******************************************************************************/

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
uds_security_access (uint8_t key_buf[], uint8_t seed_buf[]);


uint8_t
uds_session_check (uint8_t session, uint8_t sub_function);

extern void 
load_all_uds_data (void);

extern uint8_t 
save_all_uds_data (void);
#endif
/****************EOF****************/

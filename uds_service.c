/***************************************************************************//**
    \file          uds-service.c
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0.03 - CANoe Passed
    \date          2016-09-27
    \description   uds network code, base on ISO 14229
*******************************************************************************/

/*******************************************************************************
    Include Files
*******************************************************************************/
#include <string.h>
#include "network_layer.h"
#include "uds_service.h"
#include "uds_util.h"
#include "uds_status.h"
#include "uds_support.h"
#include "obd_dtc.h"
#include "uds_program.h"
#include "iap_util.h"
/*******************************************************************************
    Type declaration
*******************************************************************************/
typedef struct __UDS_SERVICE_T__
{
    uint8_t uds_sid;
    uint8_t uds_min_len;
    void (* uds_service)  (uint8_t *, uint16_t);
    bool_t std_spt;   /* default session support */
	bool_t pro_spt;   /* programming session support */
    bool_t ext_spt;   /* extended session support */
    bool_t fun_spt;   /* function address support */
	bool_t ssp_spt;   /* subfunction suppress PosRsp bit support */
    uds_sa_lv uds_sa; /* security access */
}uds_service_t;

/*******************************************************************************
    Function  declaration
*******************************************************************************/
static void
uds_service_10 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_11 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_27 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_28 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_3E (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_85 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_22 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_2E (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_31 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_34 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_36 (uint8_t msg_buf[], uint16_t msg_dlc);
static void
uds_service_37 (uint8_t msg_buf[], uint16_t msg_dlc);

/*******************************************************************************
    Private Varaibles
*******************************************************************************/
uint8_t uds_session = UDS_SESSION_NONE;
static uint8_t org_seed_buf[UDS_SEED_LENGTH];
static uint8_t req_seed = 0;

/* current security access status */
static uds_sa_lv curr_sa = UDS_SA_NON;

/* uds user layer timer */
static uint32_t uds_timer[UDS_TIMER_CNT] = {0};

static uint8_t uds_fsa_cnt = 0;

static bool_t ssp_flg;

static const uds_service_t uds_service_list[] =
{
    {SID_10, SID_10_MIN_LEN, uds_service_10, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_11, SID_11_MIN_LEN, uds_service_11, FALSE, TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_LV1},
    {SID_27, SID_27_MIN_LEN, uds_service_27, FALSE, TRUE,  FALSE, FALSE, FALSE, UDS_SA_NON},
    {SID_28, SID_28_MIN_LEN, uds_service_28, FALSE, FALSE, TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_3E, SID_3E_MIN_LEN, uds_service_3E, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_85, SID_85_MIN_LEN, uds_service_85, FALSE, FALSE, TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_22, SID_22_MIN_LEN, uds_service_22, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_NON},
    {SID_2E, SID_2E_MIN_LEN, uds_service_2E, FALSE, TRUE,  TRUE,  FALSE, FALSE, UDS_SA_LV1},
    {SID_31, SID_31_MIN_LEN, uds_service_31, FALSE, TRUE,  FALSE, FALSE, FALSE, UDS_SA_LV2},
    {SID_34, SID_31_MIN_LEN, uds_service_34, FALSE, TRUE,  FALSE, FALSE, FALSE, UDS_SA_LV2},
    {SID_36, SID_31_MIN_LEN, uds_service_36, FALSE, TRUE,  FALSE, FALSE, FALSE, UDS_SA_LV2},
    {SID_37, SID_31_MIN_LEN, uds_service_37, FALSE, TRUE,  FALSE, FALSE, FALSE, UDS_SA_LV2},
};

#define UDS_SERVICE_NUM (sizeof (uds_service_list) / sizeof (uds_service_t))

/* for data download */

static uint32_t start_mem_addr = 0;
static uint32_t total_mem_size = 0;
static uint32_t total_recv_len = 0;

static uint8_t  local_sn = 1;

/*******************************************************************************
    Global Varaibles
*******************************************************************************/
bool_t dis_normal_xmit;
bool_t dis_normal_recv;

uds_prog_t uds_prog_st = UDS_PROG_NONE;
uint32_t crc32_calc = 0;

#ifdef UDS_INTEG_DATA
/* reserved data */
uint8_t software_integrity_status = 0x01;
uint8_t software_compatibility_status = 0x01;
#endif

/*******************************************************************************
    Function  Definition
*******************************************************************************/
/**
 * uds_timer_start - start uds timer
 *
 * void :
 *
 * returns:
 *     void
 */
static void
uds_timer_start (uint8_t num)
{
	if (num >= UDS_TIMER_CNT) return;

	if (num == UDS_TIMER_FSA)
        uds_timer[UDS_TIMER_FSA] = TIMEOUT_FSA;
	if (num == UDS_TIMER_S3server)
	    uds_timer[UDS_TIMER_S3server] = TIMEOUT_S3server;
}

static void
uds_timer_start_se (uint8_t num)
{
	if (num >= UDS_TIMER_CNT) return;

	if (num == UDS_TIMER_FSA)
        uds_timer[UDS_TIMER_FSA] = TIMEOUT_FSA_SE;
	if (num == UDS_TIMER_S3server)
	    uds_timer[UDS_TIMER_S3server] = TIMEOUT_S3server;

}


static void
uds_timer_stop (uint8_t num)
{
	if (num >= UDS_TIMER_CNT) return;

    uds_timer[num] = 0;
}

/**
 * uds_timer_run - run a uds timer, should be invoked per 1ms
 *
 * void :
 *
 * returns:
 *     0 - timer is not running, 1 - timer is running, -1 - a timeout occur
 */
static int
uds_timer_run (uint8_t num)
{
	if (num >= UDS_TIMER_CNT) return 0;

    if (uds_timer[num] == 0)
	{
        return 0;
	}
	else if (uds_timer[num] == 1)
	{
		uds_timer[num] = 0;
	    return -1;
	}
	else
	{
	    /* if (uds_timer[num] > 1) */
		uds_timer[num]--;
	    return 1;
	}

}

/**
 * uds_timer_chk - check a uds timer and stop it
 *
 * num :
 *
 * returns:
 *     0 - timer is not running, 1 - timer is running,
 */
static int
uds_timer_chk (uint8_t num)
{
	if (num >= UDS_TIMER_CNT) return 0;

	if (uds_timer[num] > 0)
	    return 1;
	else
		return 0;
}

/**
 * uds_no_response - uds response nothing
 *
 * @void :
 *
 * returns:
 *
 */
static void
uds_no_response (void)
{
	return;
}
/**
 * uds_negative_rsp - uds negative response
 *
 * @sid :
 * @rsp_nrc :
 *
 * returns:
 *     0 - ok, other - wrong
 */
void
uds_negative_rsp (uint8_t sid, uint8_t rsp_nrc)
{
	uint8_t temp_buf[8] = {0};

    if (rsp_nrc != NRC_SERVICE_BUSY)
        uds_timer_start (UDS_TIMER_S3server);

    if (g_tatype == N_TATYPE_FUNCTIONAL)
	{
		if (rsp_nrc == NRC_SERVICE_NOT_SUPPORTED ||
		    rsp_nrc == NRC_SUBFUNCTION_NOT_SUPPORTED ||
			rsp_nrc == NRC_REQUEST_OUT_OF_RANGE)
		    return;
	}
	temp_buf[0] = NEGATIVE_RSP;
	temp_buf[1] = sid;
	temp_buf[2] = rsp_nrc;
	network_send_udsmsg (temp_buf, 3);
	return;
}

/**
 * uds_negative_rsp - uds positive response
 *
 * @data :
 * @len  :
 *
 * returns:
 *     0 - ok, other - wrong
 */
static void
uds_positive_rsp (uint8_t data[], uint16_t len)
{
	uds_timer_start (UDS_TIMER_S3server);
	/* SuppressPosRspMsg is true */
	if (ssp_flg == TRUE) return;
	network_send_udsmsg (data, len);
    return;
}



static int16_t
eeprom_write (void)
{
	return 0;
}

/**
 * uds_check_len - check uds request message length
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     bool
 */
static bool_t
uds_check_len (uint8_t msg_buf[], uint16_t msg_dlc)
{
	bool_t result;
	uint8_t subfunction;
	uint8_t ioctrl_param;
	uint8_t sid;
	sid = msg_buf[0];

	if (msg_dlc < 1)
	    return FALSE;

	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);

	switch (sid)
	{
		case SID_10:
		{
		    if (msg_dlc == SID_10_MIN_LEN)
		        result = TRUE;
		    else
			    result = FALSE;
		}
		break;
		case SID_11:
		{
			if (msg_dlc == SID_11_MIN_LEN)
		        result = TRUE;
		    else
			    result = FALSE;
		}
		break;
		case SID_14:
		{
			if (msg_dlc == SID_14_MIN_LEN)
		        result = TRUE;
		    else
			    result = FALSE;
		}
		break;
		case SID_19:
		{
		    if ((subfunction == REPORT_DTC_NUMBER_BY_STATUS_MASK && msg_dlc != (SID_19_MIN_LEN+1))
			 || (subfunction == REPORT_DTC_BY_STATUS_MASK && msg_dlc != (SID_19_MIN_LEN+1))
			 || (subfunction == REPORT_SUPPORTED_DTC && msg_dlc != SID_19_MIN_LEN))
			{
				result = FALSE;
			}
			else
			{
				result = TRUE;
			}
		}
		break;
		case SID_22:
		{
			if (msg_dlc == SID_22_MIN_LEN)
		        result = TRUE;
		    else
			    result = FALSE;
		}
		break;
		case SID_27:
		{
			if ((subfunction == UDS_REQUEST_SEED && msg_dlc != SID_27_MIN_LEN)
			 || (subfunction == UDS_SEND_KEY && msg_dlc != (SID_27_MIN_LEN+4)))

			{
				result = FALSE;
			}
			else
			{
				result = TRUE;
			}
		}
		break;
		case SID_2E:
		{
		    if (msg_dlc > SID_2E_MIN_LEN)
			    result = TRUE;
			else
			    result = FALSE;
		}
		break;
		case SID_2F:
		{
			ioctrl_param = msg_buf[3];
			if ((ioctrl_param == UDS_IOCTRL_RETURN_TO_ECU && msg_dlc != SID_2F_MIN_LEN) ||
				(ioctrl_param == UDS_IOCTRL_SHORT_ADJUSTMENT && msg_dlc != (SID_2F_MIN_LEN + 2) && msg_dlc != (SID_2F_MIN_LEN + 6)))
			{
				result = FALSE;
			}
			else
			{
				result = TRUE;
			}

		}
		break;
		case SID_28:
		{
			if (msg_dlc == SID_28_MIN_LEN)
		        result = TRUE;
		    else
			    result = FALSE;
		}
		break;
		case SID_31:
		{
			if (msg_dlc >= SID_31_MIN_LEN)
			    result = TRUE;
			else
			    result = FALSE;
		}
		break;
		case SID_34:
		{
			if (msg_dlc == SID_34_MIN_LEN)
			    result = TRUE;
			else
			    result = FALSE;
		}
		break;
		case SID_36:
		{
			/**
			 msg dlc of $36 must be UDS_MAX_NUM_BLOCK_LEN,
			 it should be filled with 0 on the end of the last block
			 */
			if ((total_mem_size - total_recv_len) >= UDS_MAX_NUM_BLOCK_LEN)
			{
			    if (msg_dlc == (UDS_MAX_NUM_BLOCK_LEN + 2))
			        result = TRUE;
			    else
			        result = FALSE;
		  }
			else
			{
			    if (msg_dlc == (total_mem_size - total_recv_len + 2))
			        result = TRUE;
			    else
			        result = FALSE;
			}
		}
		break;
		case SID_37:
		{
			if (msg_dlc == SID_37_MIN_LEN)
			    result = TRUE;
			else
			    result = FALSE;
		}
		break;
		case SID_3E:
		{
			if (msg_dlc == SID_3E_MIN_LEN)
		        result = TRUE;
		    else
			    result = FALSE;
		}
		break;
		case SID_85:
		{
			if (msg_dlc == SID_85_MIN_LEN)
		        result = TRUE;
		    else
			    result = FALSE;
		}
		break;
		default:
		    result = FALSE;
		break;
	}

    return result;
}
/**
 * uds_service_10 - uds service 0x10, DiagnosticSessionControl
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_10 (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	uds_nrc_em  ret_nrc;

	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);

	rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_10);
	rsp_buf[1] = subfunction;
	rsp_buf[2] = (uint8_t)(P2_SERVER >> 8);
	rsp_buf[3] = (uint8_t)(P2_SERVER & 0x00ff);
	rsp_buf[4] = (uint8_t)(P2X_SERVER >> 8);
	rsp_buf[5] = (uint8_t)(P2X_SERVER & 0x00ff);

	ret_nrc = uds_session_check (uds_session, subfunction);
	if (ret_nrc == NRC_NONE)
	{
		uds_prog_st = UDS_PROG_NONE;
		curr_sa = UDS_SA_NON;
		uds_session = (uds_session_t)subfunction;
		uds_positive_rsp (rsp_buf, 6);
		if (subfunction == UDS_SESSION_PROG || subfunction == UDS_SESSION_EOL)
		    uds_timer_start (UDS_TIMER_S3server);
	}
	else
	{
		uds_negative_rsp (SID_10,ret_nrc);
	}
    //uds_negative_rsp (SID_10,NRC_CONDITIONS_NOT_CORRECT);
}


/**
 * uds_service_11 - uds service 0x11, ECUReset
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_11 (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];

	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);
	if (subfunction == UDS_RESET_HARD)
	{
		rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_11);
		rsp_buf[1] = UDS_RESET_HARD;
		uds_positive_rsp (rsp_buf,2);
		//set reset flag
		mcu_reset_start (MCU_RESET_FAST_TIMOUT);
	}
	else
	{
		uds_negative_rsp (SID_11,NRC_SUBFUNCTION_NOT_SUPPORTED);
	}
	//uds_negative_rsp (SID_11,NRC_CONDITIONS_NOT_CORRECT);
}

/**
 * uds_service_27 - uds service 0x27, SecurityAccess
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_27 (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	uint16_t i;

	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);

	switch (subfunction)
	{
		case UDS_REQUEST_SEED:
		case UDS_SEND_KEY:
		{
			uds_negative_rsp (SID_27,NRC_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION);
			break;
		}

		case UDS_REQUEST_SEED_LV2:
		{
			if (uds_timer_chk (UDS_TIMER_FSA) > 0)
			{
				uds_negative_rsp (SID_27,NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED);
				break;
			}
			req_seed = ((curr_sa == UDS_SA_LV2)?0:1);
			rsp_buf[0] = USD_GET_POSITIVE_RSP (SID_27);
			rsp_buf[1] = subfunction;
			for (i = 0; i < UDS_SEED_LENGTH; i++) {
				if (curr_sa == UDS_SA_LV2)
					org_seed_buf[i] = 0;
				else
				    org_seed_buf[i] = rand_u8(i);
				rsp_buf[2+i] = org_seed_buf[i];
			}
			uds_positive_rsp (rsp_buf,UDS_SEED_LENGTH+2);

			//printf_os("SecurityAccess seed:%d %d %d %d\r\n",org_seed_buf[0], org_seed_buf[1], org_seed_buf[2], org_seed_buf[3],);
			break;
		}
		case UDS_SEND_KEY_LV2:
		{
			if (req_seed == 0)
			{
				uds_negative_rsp (SID_27,NRC_REQUEST_SEQUENCE_ERROR);
				break;
			}
			req_seed = 0;
#if 0
            if (1)
#else
			if (!uds_security_access (&msg_buf[2], org_seed_buf))
#endif
			{
				rsp_buf[0] = USD_GET_POSITIVE_RSP (SID_27);
				rsp_buf[1] = subfunction;
				uds_positive_rsp (rsp_buf,2);
				curr_sa = UDS_SA_LV2;
				uds_fsa_cnt = 0;
				//printf_os("SA_ PASS\r\n");
			}
			else
			{
				uds_fsa_cnt++;
				if (uds_fsa_cnt >= UDS_FAS_MAX_TIMES) {
					uds_timer_start (UDS_TIMER_FSA);
					uds_negative_rsp (SID_27,NRC_EXCEEDED_NUMBER_OF_ATTEMPTS);
				} else {
					uds_negative_rsp (SID_27,NRC_INVALID_KEY);
				}
			}
			break;
		}
		default:
			uds_negative_rsp (SID_27,NRC_SUBFUNCTION_NOT_SUPPORTED);
			break;
	}
	//uds_negative_rsp (SID_27,NRC_CONDITIONS_NOT_CORRECT);
	/**
	 0.如果已经处于解锁状态，再次请求seed，此时应返回4个0
	   并且req_seed应该设置为0，这样再次发送密钥时返回NRC 24，而不是35
	 1.如果UDS支持02会话，则在APP请求05，06子功能时应返回NRC 7E，在APP请求
	   03，04，07，08等应返回NRC 12
	 */
}

/**
 * uds_service_28 - uds service 0x28, CommunicationControl
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_28 (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	uint8_t cc_type;

	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);
	cc_type = msg_buf[2];


    switch (subfunction)
	{
		case UDS_CC_MODE_RX_TX:
		    if (cc_type == UDS_CC_TYPE_NORMAL || cc_type == UDS_CC_TYPE_NM || cc_type == UDS_CC_TYPE_NM_NOR)
			{
			    dis_normal_xmit = FALSE;
				dis_normal_recv = FALSE;
				rsp_buf[0] = USD_GET_POSITIVE_RSP (SID_28);
				rsp_buf[1] = subfunction;
				uds_positive_rsp (rsp_buf,2);
			}
			else
			{
			    uds_negative_rsp (SID_28,NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		case UDS_CC_MODE_NO_NO:
		    if (cc_type == UDS_CC_TYPE_NORMAL || cc_type == UDS_CC_TYPE_NM || cc_type == UDS_CC_TYPE_NM_NOR)
			{
			    dis_normal_xmit = TRUE;
				dis_normal_recv = TRUE;
				rsp_buf[0] = USD_GET_POSITIVE_RSP (SID_28);
				rsp_buf[1] = subfunction;
				uds_positive_rsp (rsp_buf,2);
			}
			else
			{
			    uds_negative_rsp (SID_28,NRC_REQUEST_OUT_OF_RANGE);
			}
			break;
		default:
		    uds_negative_rsp (SID_28,NRC_SUBFUNCTION_NOT_SUPPORTED);
		    break;
	}

   //uds_negative_rsp (SID_28,NRC_CONDITIONS_NOT_CORRECT);
}

/**
 * uds_service_3E - uds service 0x3E, TesterPresent
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_3E (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];

   /**
	* Any TesterPresent (3E hex) request message that is received 
	* during processing of another request message can be 
    * ignored by the server
	* Note,Cant passed canoe test
    */
	//if (uds_timer_chk (UDS_TIMER_S3server) == 0 && uds_session != UDS_SESSION_STD)
	 //  return;
	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);
	if (subfunction == ZERO_SUBFUNCTION)
	{
		rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_3E);
		rsp_buf[1] = subfunction;
		uds_positive_rsp (rsp_buf,2);
	}
	else
	{
		uds_negative_rsp (SID_3E,NRC_SUBFUNCTION_NOT_SUPPORTED);
	}
}

/**
 * uds_service_85 - uds service 0x85, ControlDTCSetting
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_85 (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);

	switch (subfunction)
	{
		case UDS_DTC_SETTING_ON:
		    obd_dtc_ctrl(DTC_ON);
		    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_85);
		    rsp_buf[1] = subfunction;
		    uds_positive_rsp (rsp_buf,2);
		    break;
		case UDS_DTC_SETTING_OFF:
		    obd_dtc_ctrl(DTC_OFF);
		    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_85);
		    rsp_buf[1] = subfunction;
		    uds_positive_rsp (rsp_buf,2);
		    break;
		default:
		    uds_negative_rsp (SID_85,NRC_SUBFUNCTION_NOT_SUPPORTED);
		    break;
	}
    //uds_negative_rsp (SID_85,NRC_CONDITIONS_NOT_CORRECT);
}


/**
 * uds_service_22 - uds service 0x22, ReadDataByIdentifier
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_22 (uint8_t msg_buf[], uint16_t msg_dlc)
{
	uint8_t rsp_buf[UDS_RSP_LEN_MAX];
	uint16_t did;
	uint16_t rsp_len;
    uint16_t msg_pos, did_n;
	bool_t find_did;

    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_22);
	rsp_len = 1;
    for (msg_pos = 1; msg_pos < msg_dlc; msg_pos+=2)
	{
	    did = ((uint16_t)msg_buf[msg_pos]) << 8;
	    did |= msg_buf[msg_pos+1];
        
		find_did = FALSE;
		for (did_n = 0; did_n < RWDATA_NUM; did_n++)
		{
			if (rwdata_list[did_n].did == did)
			{
				find_did = TRUE;
                rsp_buf[rsp_len++] = msg_buf[msg_pos];
				rsp_buf[rsp_len++] = msg_buf[msg_pos+1];
				memcpy (&rsp_buf[rsp_len], rwdata_list[did_n].p_data, rwdata_list[did_n].dlc);
				rsp_len += rwdata_list[did_n].dlc;
			    break;
			}
		}
		if (find_did == FALSE)
			break;
	}

	if (find_did == TRUE)
        uds_positive_rsp (rsp_buf,rsp_len);
	else
	    uds_negative_rsp (SID_22,NRC_REQUEST_OUT_OF_RANGE);
    
    //uds_negative_rsp (SID_22,NRC_CONDITIONS_NOT_CORRECT);
}

/**
 * uds_service_2E - uds service 0x2E, WriteDataByIdentifier
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_2E (uint8_t msg_buf[], uint16_t msg_dlc)
{
	uint8_t rsp_buf[8];
	uint16_t did;
    uint16_t did_n;
	int16_t write_len;
	bool_t find_did;
	bool_t vali_dlc;

	did = ((uint16_t)msg_buf[1]) << 8;
	did |= msg_buf[2];
	
	find_did = FALSE;
	vali_dlc = FALSE;
	for (did_n = 0; did_n < RWDATA_NUM; did_n++)
	{
		if (rwdata_list[did_n].did == did)
		{
			if (rwdata_list[did_n].rw_mode == UDS_RWDATA_RDONLY)
			    break;
			find_did = TRUE;
		    if (msg_dlc != (rwdata_list[did_n].dlc + 3))
			    break;
			vali_dlc = TRUE;
			rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_2E);
			rsp_buf[1] = msg_buf[1];
			rsp_buf[2] = msg_buf[2];
			memcpy (rwdata_list[did_n].p_data, &msg_buf[3], rwdata_list[did_n].dlc);
			write_len = uds_save_rwdata ();
			break;
		}
	}

	if (find_did == TRUE)
	{
		if (vali_dlc == TRUE)
		{
			if (write_len >= 0)
				uds_positive_rsp (rsp_buf,3);
			else
				uds_negative_rsp (SID_2E,NRC_GENERAL_PROGRAMMING_FAILURE);
		}
		else
		{
			uds_negative_rsp (SID_2E,NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT);
		}
	}
	else
	{
	    uds_negative_rsp (SID_2E,NRC_REQUEST_OUT_OF_RANGE);
	}
    
    //uds_negative_rsp (SID_2E,NRC_CONDITIONS_NOT_CORRECT);
}


/**
 * uds_service_31 - uds service 0x31, RoutineControl
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_31 (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t subfunction;
	uint8_t rsp_buf[8];
	uint8_t ret;
	uint16_t rid;
	uint16_t rid_n;
	bool_t find_rid;

    subfunction = UDS_GET_SUB_FUNCTION (msg_buf[1]);
	rid = ((uint16_t)msg_buf[2]) << 8;
	rid |= msg_buf[3];

	find_rid = FALSE;
	for (rid_n = 0; rid_n < RTCTRL_NUM; rid_n++)
	{
		if (rtctrl_list[rid_n].rid == rid)
		{
			find_rid = TRUE;
			memcpy (rtctrl_list[rid_n].p_data, &msg_buf[4], rtctrl_list[rid_n].dlc);
			break;
		}
	}

	rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_31);
	rsp_buf[1] = msg_buf[1];
	rsp_buf[2] = msg_buf[2];
	rsp_buf[3] = msg_buf[3];
    switch (subfunction)
	{
		case UDS_ROUTINE_CTRL_START:
		    if (find_rid == TRUE)
			{
                if (rtst_list[rid_n] == UDS_RT_ST_RUNNING)
				{
                    uds_negative_rsp (SID_31,NRC_REQUEST_SEQUENCE_ERROR);
				}
				else
				{
					ret = rtctrl_list[rid_n].init_routine ();
					rsp_buf[4] = ((rtst_list[rid_n] == UDS_RT_ST_FAILED)?0x01:0x00);
					if (ret == 0)
					    uds_positive_rsp (rsp_buf,5);
					else
					    uds_negative_rsp (SID_31,ret);
				}
			}
			else
			{
				uds_negative_rsp (SID_31,NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		case UDS_ROUTINE_CTRL_STOP:
		    if (find_rid == TRUE)
			{
                if (rtst_list[rid_n] == UDS_RT_ST_IDLE)
				{
                    uds_negative_rsp (SID_31,NRC_REQUEST_SEQUENCE_ERROR);
				}
				else
				{
					rtctrl_list[rid_n].stop_routine ();
					uds_positive_rsp (rsp_buf,4);
				}
			}
			else
			{
				uds_negative_rsp (SID_31,NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		case UDS_ROUTINE_CTRL_REQUEST_RESULT:
		    if (find_rid == TRUE)
			{
                if (rtst_list[rid_n] == UDS_RT_ST_IDLE)
				{
                    uds_negative_rsp (SID_31,NRC_REQUEST_SEQUENCE_ERROR);
				}
				else
				{
					rsp_buf[4] = (uint8_t)rtst_list[rid_n];
				}
			}
			else
			{
				uds_negative_rsp (SID_31,NRC_REQUEST_OUT_OF_RANGE);
			}
		    break;
		default:
		    uds_negative_rsp (SID_31,NRC_SUBFUNCTION_NOT_SUPPORTED);
		    break;
	}
    //uds_negative_rsp (SID_2F,NRC_CONDITIONS_NOT_CORRECT);
}


/**
 * download_request_valid - check the download request
 *
 * @void : 
 *
 * returns:
 *     void
 */
static bool_t
download_request_valid (uint32_t start_addr, uint32_t end_addr)
{
    bool_t ret;

    if (uds_prog_st == UDS_PROG_NONE &&
         start_addr == P_DRIVER_START_ADDR &&
           end_addr == P_DRIVER_ENDxx_ADDR)
    {
        uds_prog_st =  UDS_PROG_FLASH_DRIVER_DOWNLOADING;
        ret = TRUE;
    } else
    if (uds_prog_st == UDS_PROG_ERASE_MEMORY_COMPLETE && 
         start_addr == P_APP_START_ADDR &&
           end_addr <= P_APP_ENDxx_ADDR)
    {
        uds_prog_st =  UDS_PROG_APP_DOWNLOADING;
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }
    return ret;
}

/**
 * uds_service_34 - uds service 0x34, RequestDownload
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_34 (uint8_t msg_buf[], uint16_t msg_dlc)
{
  uint8_t rsp_buf[8];
  uint8_t data_format;
  uint8_t addr_len_format;
  uint32_t end_mem_addr;

  data_format = msg_buf[1];
  addr_len_format = msg_buf[2];

  start_mem_addr = can_to_hostl(&msg_buf[3]);
	total_mem_size = can_to_hostl(&msg_buf[7]);
	end_mem_addr = start_mem_addr + total_mem_size - 1;

  if (uds_prog_st != UDS_PROG_NONE &&
	    uds_prog_st != UDS_PROG_ERASE_MEMORY_COMPLETE)
	{
      uds_prog_st = UDS_PROG_NONE;
      uds_negative_rsp (SID_34,NRC_CONDITIONS_NOT_CORRECT);
      return;
	}

	if (data_format != UDS_VALID_DATA_FORMAT || addr_len_format != UDS_VALID_ADDR_LEN_FORMAT ||
	    download_request_valid (start_mem_addr, end_mem_addr) == FALSE)
	{
      uds_prog_st = UDS_PROG_NONE;
	    uds_negative_rsp (SID_34,NRC_REQUEST_OUT_OF_RANGE);
      return;
  }

  total_recv_len = 0;
  local_sn = 1;
  crc32_calc = CRC32_INIT;

  rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_34);
  rsp_buf[1] = UDS_LENGTH_FORMAT_ID;
  host_to_canl (&rsp_buf[2], UDS_MAX_NUM_BLOCK_LEN);
  uds_positive_rsp (rsp_buf,6);
  //uds_negative_rsp (SID_2F,NRC_CONDITIONS_NOT_CORRECT);
}

/**
 * uds_service_36 - uds service 0x36, TransferData 
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_36 (uint8_t msg_buf[], uint16_t msg_dlc)
{
  uint8_t rsp_buf[8];
  uint8_t block_sn;
  uint32_t write_addr;
  uint16_t write_len;
  uint16_t ret;

  block_sn = msg_buf[1];
  write_len = msg_dlc - 2;

    if ((uds_prog_st != UDS_PROG_APP_DOWNLOADING && uds_prog_st != UDS_PROG_FLASH_DRIVER_DOWNLOADING) ||
	     total_recv_len == total_mem_size)
	{
		uds_negative_rsp (SID_36,NRC_REQUEST_SEQUENCE_ERROR);
		return;
	}
	if (block_sn != local_sn && block_sn != (local_sn - 1))
	{
	    uds_negative_rsp (SID_36,NRC_WRONG_BLOCK_SEQUENCE_COUNTER);
		return;
	}
    if ((total_recv_len + write_len) > total_mem_size)
	{
		uds_negative_rsp (SID_36,NRC_TRANSFER_DATA_SUSPENDED);
		return;
	}

	write_addr = start_mem_addr + total_recv_len;

  if (uds_prog_st == UDS_PROG_FLASH_DRIVER_DOWNLOADING)
    ret = sdram_program (write_addr, &msg_buf[2], write_len);
  else
    ret = flash_program (write_addr, &msg_buf[2], write_len);
  if (ret == write_len)
  {
    if (block_sn == local_sn)
      total_recv_len += write_len;
    local_sn = block_sn+1;
    crc32_calc = crc32_discontinue (crc32_calc, &msg_buf[2], write_len);

    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_36);
    rsp_buf[1] = block_sn;
    uds_positive_rsp (rsp_buf,2);
	}
	else
	{
    uds_negative_rsp (SID_36,NRC_GENERAL_PROGRAMMING_FAILURE);
	}
}

/**
 * uds_service_37 - uds service 0x37, RequestTransferExit 
 *
 * @msg_buf :
 * @msg_dlc :
 *
 * returns:
 *     void
 */
static void
uds_service_37 (uint8_t msg_buf[], uint16_t msg_dlc)
{
    uint8_t rsp_buf[8];
    if ((uds_prog_st != UDS_PROG_APP_DOWNLOADING && uds_prog_st != UDS_PROG_FLASH_DRIVER_DOWNLOADING) || 
	     total_recv_len != total_mem_size)
    {
        uds_prog_st = UDS_PROG_NONE;
        uds_negative_rsp (SID_36,NRC_REQUEST_SEQUENCE_ERROR);
        return;
    }

    uds_prog_st++;
    total_recv_len = 0;
    local_sn = 1;

    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_37);
    uds_positive_rsp (rsp_buf,1);
}

/**
 * uds_dataff_indication - uds first frame indication callbacl
 *
 * @msg_dlc : first frame dlc
 *
 * returns:
 *     void
 */
static void
uds_dataff_indication (uint16_t msg_dlc)
{
	uds_timer_stop (UDS_TIMER_S3server);
}
/**
 * uds_data_indication - uds data request indication callback, 
 *
 * @msg_buf  :
 * @msg_dlc  :
 * @n_result :
 *
 * returns:
 *     void
 */

static void
uds_data_indication (uint8_t msg_buf[], uint16_t msg_dlc, n_result_t n_result)
{
	uint8_t i;
	uint8_t sid;
	uint8_t ssp;
	bool_t find_sid;
	bool_t session_spt;
	uds_timer_stop (UDS_TIMER_S3server);

    if (n_result != N_OK)
	{
		uds_timer_start (UDS_TIMER_S3server);
	    return;
	}

    sid = msg_buf[0];
	ssp = UDS_GET_SUB_FUNCTION_SUPPRESS_POSRSP (msg_buf[1]);
	find_sid = FALSE;
    for (i = 0; i < UDS_SERVICE_NUM; i++)
	{
        if (sid == uds_service_list[i].uds_sid)
		{
			session_spt = FALSE;
            if (uds_session == UDS_SESSION_STD)
			    session_spt = uds_service_list[i].std_spt;
			if (uds_session == UDS_SESSION_PROG)
				session_spt = uds_service_list[i].pro_spt;
			if (uds_session == UDS_SESSION_EOL)
			    session_spt = uds_service_list[i].ext_spt;
			if (session_spt == TRUE)
			{
                if (g_tatype == N_TATYPE_FUNCTIONAL && uds_service_list[i].fun_spt == FALSE)
				{
					uds_no_response ();
				}
				else
				{
					if (uds_check_len(msg_buf, msg_dlc))
					{
                        if (curr_sa >= uds_service_list[i].uds_sa)
						{
							if (uds_service_list[i].ssp_spt == TRUE && ssp == 0x01)
							    ssp_flg = TRUE;
							else
							    ssp_flg = FALSE;
                            uds_service_list[i].uds_service (msg_buf, msg_dlc);
						}
						else
						{
							uds_negative_rsp (sid, NRC_SECURITY_ACCESS_DENIED);
						}
					}
					else
					{
						uds_negative_rsp (sid, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT);
					}
				}
				    
			}
			else
			{
                uds_negative_rsp (sid, NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
			}
			find_sid = TRUE;
			break;
		}
	}

	if (find_sid == FALSE)
	{
		uds_negative_rsp (sid, NRC_SERVICE_NOT_SUPPORTED);
	}
}

/**
 * uds_data_confirm - uds response confirm
 *
 * @n_result :
 *
 * returns:
 *     void
 */
static void
uds_data_confirm (n_result_t n_result)
{

	uds_timer_start (UDS_TIMER_S3server);
}

/*******************************************************************************
    Function  Definition - external API
*******************************************************************************/ 
/**
 * uds_chk_session - check the uds session, if in program session, 
 *                   then send response
 *
 * @void :
 *
 * returns:
 *     void
 */
extern void
uds_chk_session (void)
{
  uint8_t rsp_buf[8];

  if (uds_session == UDS_SESSION_PROG)
  {
    rsp_buf[0] = USD_GET_POSITIVE_RSP (SID_10);
	rsp_buf[1] = UDS_SESSION_PROG;
	rsp_buf[2] = (uint8_t)(P2_SERVER >> 8);
	rsp_buf[3] = (uint8_t)(P2_SERVER & 0x00ff);
	rsp_buf[4] = (uint8_t)(P2X_SERVER >> 8);
	rsp_buf[5] = (uint8_t)(P2X_SERVER & 0x00ff);
	uds_positive_rsp (rsp_buf, 6);
  }
}

/**
 * uds_get_frame - uds get a can frame, then transmit to network
 *
 * @func_addr : 0 - physical addr, 1 - functional addr
 * @frame_buf : uds can frame data buffer
 * @frame_dlc : uds can frame length
 *
 * returns:
 *     void
 */
extern void
uds_get_frame (uint8_t func_addr, uint8_t frame_buf[], uint8_t frame_dlc)
{
	network_recv_frame (func_addr, frame_buf, frame_dlc);
}


/**
 * uds_main - uds main loop, should be schedule every 1 ms
 *
 * @void  :
 *
 * returns:
 *     void
 */
extern void
uds_main (void)
{
	network_main ();

	if (uds_timer_run (UDS_TIMER_S3server) < 0)
	{
		uds_session = UDS_SESSION_STD;
		curr_sa = UDS_SA_NON;
		dis_normal_xmit = FALSE;
		dis_normal_recv = FALSE;
		obd_dtc_ctrl(DTC_ON);
		uds_ioctrl_allstop ();
	}


	uds_timer_run (UDS_TIMER_FSA);

}


/**
 * uds_init - uds user layer init
 *
 * @void  :
 *
 * returns:
 *     0 - ok
 * descrip: uds init must be front of can_init
 */
extern int
uds_init (void)
{
    nt_usdata_t usdata;

    uds_session = UDS_SESSION_STD;
    uds_ioctrl_allstop();
    uds_load_rwdata ();
	load_all_uds_data ();
    uds_timer_start_se (UDS_TIMER_FSA);	
    usdata.ffindication = uds_dataff_indication;
    usdata.indication = uds_data_indication;
    usdata.confirm = uds_data_confirm;

    return network_reg_usdata (usdata);
}

/****************EOF****************/

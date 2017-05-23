/***************************************************************************//**
    \file          uds-support.c
    \author        huanghai
    \mail          huanghai@auto-link.com
    \version       0.01
    \date          2016-10-8
    \description   uds service spport code, include read/write data,
                   io control and routine control
*******************************************************************************/

/*******************************************************************************
    Include Files
*******************************************************************************/
#include <string.h>
#include <absacc.h>
#include "uds_util.h"
#include "uds_support.h"
#include "uds_program.h"
#include "uds_service.h"
#include "timx.h"
#include "uds_status.h"
/*******************************************************************************
    Function  declaration
*******************************************************************************/
static uint8_t
rtctrl_nothing (void);
static uint8_t
rtctrl_check_routine (void);
static uint8_t
rtctrl_erase_memory (void);
static uint8_t
rtctrl_check_dependence (void);
static uint8_t
ioctrl_init_backlight (void);
static uint8_t
ioctrl_stop_backlight (void);
static uint8_t
ioctrl_init_buzzer (void);
static uint8_t
ioctrl_stop_buzzer (void);
static uint8_t
ioctrl_init_gages (void);
static uint8_t
ioctrl_stop_gages (void);
static uint8_t
ioctrl_init_display (void);
static uint8_t
ioctrl_stop_display (void);
static uint8_t
ioctrl_init_indicator (void);
static uint8_t
ioctrl_stop_indicator (void);
/*******************************************************************************
    Global Varaibles
*******************************************************************************/
uint8_t ASC_boot_ver[10]=
{'v', 'e', 'r', '-', '0', '.', '0', '1', 0, 0};
extern uint8_t uds_session;
#ifdef M12
uint8_t ASC_ecu_part_num[15]=
{'3','8','2','0','0','1','0','0','0','2','-','M','1','2',0};
uint8_t ASC_sys_name[10]=
{'m', '1', '2', '-', 'I', 'C', 'U', 0, 0, 0};
#endif
#ifdef M12E
uint8_t ASC_ecu_part_num[15]=
{'3','8','2','0','0','1','0','0','0','1','-','M','1','2',0};
uint8_t ASC_sys_name[10]=
{'m', '1', '2', 'e', '-', 'I', 'C', 'U', 0, 0};
#endif
uint8_t ASC_sys_supplier_id[5]=
{'1', '2', '3', '4', '5'};
uint8_t ASC_hard_ver[10]=
{'v', 'e', 'r', '-', '0', '.', '0', '1', 0, 0};
uint8_t ASC_soft_ver[10]=
{'v', 'e', 'r', '-', '0', '.', '0', '1', 0, 0};



uint8_t BCD_manufacture_date[3] __at(0x20000400);
uint8_t HEX_ecu_sn[10] __at(0x20000403);
uint8_t ASC_VIN[17] __at(0x2000040D);
uint8_t HEX_tester_sn[10] __at(0x2000041E);
uint8_t BCD_program_date[3] __at(0x20000428);
uint8_t HEX_ICU_config[4] __at(0x2000042B);
uint8_t HEX_clear_maintain[1] __at(0x2000042F);

const uds_rwdata_t rwdata_list[RWDATA_CNT] =
{
    {0xF183, ASC_boot_ver,         10, UDS_RWDATA_RDONLY,      UDS_RWDATA_DFLASH},
    {0xF186, &uds_session,         1,  UDS_RWDATA_RDONLY,      UDS_RWDATA_RAM},
    {0xF187, ASC_ecu_part_num,     15, UDS_RWDATA_RDONLY,      UDS_RWDATA_DFLASH},
    {0xF18A, ASC_sys_supplier_id,  5,  UDS_RWDATA_RDONLY,      UDS_RWDATA_DFLASH},
    {0xF18B, BCD_manufacture_date, 3,  UDS_RWDATA_RDWR,        UDS_RWDATA_EEPROM}, /* be writen after manufacture */
    {0xF18C, HEX_ecu_sn,           10, UDS_RWDATA_RDWR,        UDS_RWDATA_EEPROM}, /* be writen after manufacture */
    {0xF190, ASC_VIN,              17, UDS_RWDATA_RDWR_WRONCE, UDS_RWDATA_EEPROM}, /* be writen after installment */
    {0xF193, ASC_hard_ver,         10, UDS_RWDATA_RDONLY,      UDS_RWDATA_DFLASH},
    {0xF195, ASC_soft_ver,         10, UDS_RWDATA_RDONLY,      UDS_RWDATA_DFLASH},
    {0xF197, ASC_sys_name,         10, UDS_RWDATA_RDONLY,      UDS_RWDATA_DFLASH},
    {0xF198, HEX_tester_sn,        10, UDS_RWDATA_RDWR_INBOOT, UDS_RWDATA_EEPROM}, /* update by tester after program */
    {0xF199, BCD_program_date,     3,  UDS_RWDATA_RDWR_INBOOT, UDS_RWDATA_EEPROM}, /* update by tester after program */
    {0x0100, HEX_ICU_config,       4,  UDS_RWDATA_RDWR, 	   UDS_RWDATA_EEPROM},
    {0x0101, HEX_clear_maintain,   1,  UDS_RWDATA_RDWR, 	   UDS_RWDATA_EEPROM}
};



uint8_t backlight_level[2];
uint8_t buzzer[2];
uint8_t gages[2];
uint8_t segment_disp[2];
uint8_t indicator[6];

uds_ioctrl_t ioctrl_list[IOCTRL_CNT] = 
{
    {0xF092, backlight_level, 2, 0, 0, 0, &ioctrl_init_backlight, &ioctrl_stop_backlight},
    {0xF020, buzzer,          2, 0, 0, 0, &ioctrl_init_buzzer,    &ioctrl_stop_buzzer},
    {0xF021, gages,           2, 0, 0, 0, &ioctrl_init_gages,     &ioctrl_stop_gages},
    {0xF022, segment_disp,    2, 0, 0, 0, &ioctrl_init_display,   &ioctrl_stop_display},
    {0xF024, indicator,       6, 0, 0, 0, &ioctrl_init_indicator, &ioctrl_stop_indicator}
};

uint8_t crc_param[4];
uint8_t mem_aands[9];
uint8_t dpn_param[4];
const uds_rtctrl_t rtctrl_list[RTCTRL_CNT] =
{
    {0xF001, crc_param, 4, &rtctrl_check_routine, &rtctrl_nothing, &rtctrl_nothing},
    {0xFF00, mem_aands, 9, &rtctrl_erase_memory,  &rtctrl_nothing, &rtctrl_nothing},
    {0xFF01, dpn_param, 0, &rtctrl_check_dependence, &rtctrl_nothing, &rtctrl_nothing}
};

uds_rtst_t rtst_list[RTCTRL_CNT] = {UDS_RT_ST_IDLE, UDS_RT_ST_IDLE, UDS_RT_ST_IDLE};


/*******************************************************************************
    Function  Definition -- for routine control
*******************************************************************************/

/**
 * rtctrl_nothing - a temp Function for routine control
 *
 * @void :
 *
 * returns:
 *     0 - ok
 */
static uint8_t
rtctrl_nothing (void)
{
    return 0;
}

/**
 * rtctrl_check_routine - a temp Function for routine control
 *
 * @void :
 *
 * returns:
 *     0 - ok
 */
static uint8_t
rtctrl_check_routine (void)
{
	uint8_t  io_err;
    uint32_t crc_value;

    crc_value = can_to_hostl(crc_param);

    if (uds_prog_st != UDS_PROG_FLASH_DRIVER_DOWNLOAD_COMPLETE &&
        uds_prog_st != UDS_PROG_APP_DOWNLOAD_COMPLETE)
    {
        uds_prog_st = UDS_PROG_NONE;
        return NRC_CONDITIONS_NOT_CORRECT;
    }

    if (uds_prog_st == UDS_PROG_FLASH_DRIVER_DOWNLOAD_COMPLETE)
    {
        crc32_calc = crc32_continue ((uint8_t *)P_DRIVER_START_ADDR, FLASH_DRIVER_LENGTH);
    }
    if (crc_value == crc32_calc)
    {
        if (uds_prog_st == UDS_PROG_APP_DOWNLOAD_COMPLETE)
        {
#ifdef UDS_INTEG_DATA
            app_integrity_status = 0x00;
            spp_compatibility_status = 0x01;
#endif
        }

        uds_prog_st++;
        rtst_list[UDS_RT_CHECK] = UDS_RT_ST_SUCCESS;
    }
    else
    {
        if (uds_prog_st == UDS_PROG_APP_DOWNLOAD_COMPLETE)
        {
#ifdef UDS_INTEG_DATA
            app_integrity_status = 0x01;
            spp_compatibility_status = 0x01;
#endif
            uds_data.app_valid_flag = 0x00;
        }
        uds_prog_st = UDS_PROG_NONE;
        rtst_list[UDS_RT_CHECK] = UDS_RT_ST_FAILED;
    }

    io_err = save_all_uds_data();
	if (io_err == 0)
      return NRC_NONE;
	else
	  return NRC_GENERAL_PROGRAMMING_FAILURE;
}


/**
 * rtctrl_erase_memory - erase memory routine
 *
 * @void :
 *
 * returns:
 *     0 - ok
 */
static uint8_t
rtctrl_erase_memory (void)
{
    uint8_t addr_len;
    uint8_t size_len;
    uint32_t mem_addr;
    uint32_t mem_size;
	uint32_t end_addr;
    uint16_t erase_len;
    uint16_t i;
	uint32_t last_time = 0;
	uint32_t curr_time;

    if (uds_prog_st != UDS_PROG_FLASH_DRIVER_CRC32_VALID)
    {
        uds_prog_st = UDS_PROG_NONE;
        return NRC_CONDITIONS_NOT_CORRECT;
    }

    addr_len = UDS_GET_MEM_ADDR_LEN(mem_aands[0]);
    size_len = UDS_GET_MEM_SIZE_LEN(mem_aands[0]);
    mem_addr = 0;
    mem_size = 0;
    for (i = 0; i < addr_len; i++)
        mem_addr = (mem_addr << 8) + mem_aands[i+1];
    for (i = 0; i < size_len; i++)
        mem_size = (mem_size << 8) + mem_aands[i+addr_len+1];

	end_addr = mem_addr + mem_size - 1;
    if (P_APP_START_ADDR != mem_addr ||
        P_APP_ENDxx_ADDR <  end_addr)
    {
        uds_prog_st = UDS_PROG_NONE;
        return NRC_REQUEST_OUT_OF_RANGE;
    }

	FLASH_UNLOCK;
    erase_len = 0;
    for (; mem_addr <= end_addr; )
    {
        curr_time = get_time_ms();
		if ((curr_time - last_time) > TIMEOUT_P2server_x)
		{
			last_time = curr_time;
            uds_negative_rsp(SID_31, NRC_SERVICE_BUSY);
		}
        //SW_WATCHDOG_SERVICE();
        //HD_WATCHDOG_SERVICE();
        erase_len = flash_erase (mem_addr, P_FLASH_PAGE_SIZE);
        if (erase_len != 0)
            mem_addr += erase_len;
        else
            break;
    }

    if(erase_len == 0)
    {
        rtst_list[UDS_RT_ERASE] = UDS_RT_ST_FAILED;
        uds_prog_st = UDS_PROG_NONE;
        return NRC_GENERAL_PROGRAMMING_FAILURE;
    }
    else
    {
#ifdef UDS_INTEG_DATA
        app_integrity_status = 0x01;
        spp_compatibility_status = 0x01;
#endif
        uds_data.app_valid_flag = 0x00;

        rtst_list[UDS_RT_ERASE] = UDS_RT_ST_SUCCESS;
        uds_prog_st = UDS_PROG_ERASE_MEMORY_COMPLETE;
        return NRC_NONE;
    }
}
/**
 * rtctrl_erase_memory - erase memory routine
 *
 * @void :
 *
 * returns:
 *     0 - ok
 */
static uint8_t
rtctrl_check_dependence (void)
{
	uint8_t io_err;

    if (uds_prog_st != UDS_PROG_APP_CRC32_VALID)
    {
        uds_prog_st = UDS_PROG_NONE;
        return NRC_CONDITIONS_NOT_CORRECT;
    }
    uds_prog_st = UDS_PROG_NONE;
    if (GET_APP_MARK_ID() == APP_MARK_ID)
    {
#ifdef UDS_INTEG_DATA
        app_integrity_status = 0x00;
        spp_compatibility_status = 0x00;
#endif
        uds_data.app_valid_flag = 0x01;
        //prog_failed_times_slr = 0;
        //uds_response_pending(0x31);
        FLASH_LOCK;
        rtst_list[UDS_RT_DEPEND] = UDS_RT_ST_SUCCESS;
		io_err = save_all_uds_data();
	    if (io_err == 0)
          return NRC_NONE;
	    else
	      return NRC_GENERAL_PROGRAMMING_FAILURE;
    }
    else
    {
        rtst_list[UDS_RT_DEPEND] = UDS_RT_ST_FAILED;
        return NRC_NONE;
    }

}

/*******************************************************************************
    Function  Definition -- for i/o control
*******************************************************************************/

/**
 * ioctrl_init_backlight - init the backlight control
 *
 * @void : 
 *
 * returns:
 *     void
 */
static uint8_t
ioctrl_init_backlight (void)
{
    //mcuctrl_set_backlight (backlight_level[0], backlight_level[1]);
    return 0;
}
static uint8_t
ioctrl_stop_backlight (void)
{
    //mcuctrl_end_backlight ();
    return 0;
}

/**
 * ioctrl_init_buzzer - init the buzzer control
 *
 * @void : 
 *
 * returns:
 *     void
 */
static uint8_t
ioctrl_init_buzzer (void)
{
    //mcuctrl_set_buzzer (buzzer[0], buzzer[1]);
    return 0;
}

static vouint8_tid
ioctrl_stop_buzzer (void)
{
    //mcuctrl_end_buzzer (buzzer[0], buzzer[1]);
    return 0;
}

/**
 * ioctrl_init_gages - init the gages control
 *
 * @void : 
 *
 * returns:
 *     void
 */
static uint8_t
ioctrl_init_gages (void)
{
    //mcuctrl_set_gages (gages[0], gages[1]);
    return 0;
}

static uint8_t
ioctrl_stop_gages (void)
{
    //mcuctrl_end_gages (gages[0], gages[1]);
    return 0;
}

/**
 * ioctrl_init_display - init the segment display control
 *
 * @void : 
 *
 * returns:
 *     void
 */
static uint8_t
ioctrl_init_display (void)
{
    //mcuctrl_set_display (segment_disp[0], segment_disp[1]);
    return 0;
}

static uint8_t
ioctrl_stop_display (void)
{
    //mcuctrl_end_display (segment_disp[0], segment_disp[1]);
    return 0;
}

/**
 * ioctrl_init_indicator- init the indicators control
 *
 * @void : 
 *
 * returns:
 *     void
 */
static uint8_t
ioctrl_init_indicator (void)
{
    //mcuctrl_set_indicator (indicator);
    return 0;
}

static uint8_t
ioctrl_stop_indicator (void)
{
    //mcuctrl_end_indicator (indicator);
    return 0;
}

/*******************************************************************************
    Function  Definition - extern to uds
*******************************************************************************/

/**
 * uds_ioctrl_allstop - main handle of io control
 *
 * @void : 
 *
 * returns:
 *     void
 */
void
uds_ioctrl_allstop (void)
{
    uint16_t did_n;

    for (did_n = 0; did_n < IOCTRL_NUM; did_n++)
    {
        if (ioctrl_list[did_n].enable == TRUE)
        {
            /* need to mutex with 2F service UDS_IOCTRL_RETURN_TO_ECU */
            ioctrl_list[did_n].enable = FALSE;
            if (ioctrl_list[did_n].stop_ioctrl != NULL)
                ioctrl_list[did_n].stop_ioctrl ();
        }
    }
}


/**
 * uds_load_rwdata - load read / write data from eeprom to ram
 *
 * @void : 
 *
 * returns:
 *     void
 */
void
uds_load_rwdata (void)
{
    memset (ASC_ecu_part_num, 0x55, (15+3+10+17+10+3));
}

/**
 * uds_save_rwdata - save read / write data from eeprom to ram
 *
 * @void : 
 *
 * returns:
 *     void
 */
void
uds_save_rwdata (void)
{
    memset (ASC_ecu_part_num, 0x55, (15+3+10+17+10+3));
}
/****************EOF****************/

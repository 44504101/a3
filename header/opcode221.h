// ----------------------------------------------------------------------------
/**
 * @file    	acqmtc_dsp_b/src/opcode221.h
 * @author		Benoit LeGuevel
 * @date		11 October 2013
 * @brief		Header file for opcode221.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef OPCODE221_H_
#define OPCODE221_H_

#include "loader_state.h"
#include "timer.h"
#include "comm.h"

/// Enumerated flash status.
typedef enum
{
    FLASH_POLL_NOT_BUSY,                        ///< Flash free.
    FLASH_POLL_BUSY,                            ///< Flash busy.
    FLASH_POLL_ERASE_SUSPENDED,                 ///< Flash erase suspended.
    FLASH_POLL_ERASE_FAIL,                      ///< Flash erase failure.
    FLASH_POLL_PROGRAM_FAIL,                    ///< Flash program failure.
    FLASH_POLL_PROGRAM_ABORTED,                 ///< Flash program aborted.
    FLASH_POLL_PROGRAM_SUSPENDED,               ///< Flash program suspended.
    FLASH_POLL_SECTOR_LOCKED                    ///< Flash sector locked.
} main_flash_status_t;

void opcode221_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
                       Timer_t* timer) ;

#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode221.h;3 */
/*       1*[1792972] 17-FEB-2014 14:55:26 (GMT) BLeguevel */
/*         "Configuration and recording system implemented on the DSP_B downhole code" */
/*       2*[1842524] 27-JUN-2014 16:00:50 (GMT) BLeguevel */
/*         "DSP_B code modifications to match with the new RS485COM module implemented in the common code" */
/*       3*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode221.h;3 */

// ----------------------------------------------------------------------------
/**
 * @file    	acqmtc_dsp_b/src/opcode204.h
 * @author		Benoit LeGuevel
 * @date		04 Dec. 2013
 * @brief		Header file for opcode204.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef OPCODE204_H_
#define OPCODE204_H_

#include "loader_state.h"
#include "timer.h"
#include "comm.h"

void opcode204_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
                       Timer_t* timer) ;

#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode204.h;3 */
/*       1*[1792972] 17-FEB-2014 14:55:26 (GMT) BLeguevel */
/*         "Configuration and recording system implemented on the DSP_B downhole code" */
/*       2*[1842524] 27-JUN-2014 16:00:50 (GMT) BLeguevel */
/*         "DSP_B code modifications to match with the new RS485COM module implemented in the common code" */
/*       3*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode204.h;3 */

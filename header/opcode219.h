// ----------------------------------------------------------------------------
/**
 * @file    	acqmtc_dsp_b/src/opcode219.h
 * @author		Benoit LeGuevel
 * @date		04 December 2014
 * @brief		Header file for opcode219.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef OPCODE219_H_
#define OPCODE219_H_

#include "loader_state.h"
#include "timer.h"
#include "Comm.h"

void opcode219_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,Timer_t* timer);

#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode219.h;2 */
/*       1*[1873747] 09-DEC-2014 13:45:12 (GMT) BLeguevel */
/*         "Implement the opcode219" */
/*       2*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode219.h;2 */

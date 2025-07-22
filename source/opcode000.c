/**
 * \file
 * Implementation of opcode 0 (Active Loader )
 * 
 * @author Scott DiPasquale
 * @date 1 September 2004
 * 
 * (c) Copyright Xi'an Shiyou Univ. Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Xi'an Shiyou Univ.
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */

#include "common_data_types.h"
#include "loader_state.h"
#include "timer.h"
#include "opcode000.h"
#include "tool_specific_config.h"
#include "comm.h"

void opcode0_execute(ELoaderState_t* loaderState, Timer_t* timer)
{
    // If the Loader is in the initial wait period, this opcode is valid.
    // Steps to take:
    //   1) Put the Loader into loading mode
    //   2) Load the timer with the timeout period that corresponds to the loading
    //      mode
    //   3) Send an SSB OK message
    //   4) Reset the timer
    // If the loader is in any other state, then this is an invalid opcode

	//lint -e{788} enum constant not used within switch.
    switch( *loaderState )
    {
		case LOADER_WAITING:
			*loaderState = LOADER_ACTIVATED;
			Timer_TimerSet(timer, LOADERMODE_TIMEOUT);
			//lint -fallthrough  Intentional fallthrough.

		case LOADER_ACTIVATED :
			loader_MessageSend( LOADER_OK, 0, "" );
			Timer_TimerReset( timer );
			break;

		default:
			loader_MessageSend( LOADER_INVALID_OPCODE, 0, "" );
			break;
    }
}

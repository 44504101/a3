/**
 * \file
 * Definition of opcode 1 ( jump to application )
 * 
 * @author Scott DiPasquale
 * @date 7 September 2004
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
#include "comm.h"
#include "opcode001.h"
#include "tool_specific_config.h"
#include "tool_specific_hardware.h"

//lint -e{715} Symbol not referenced - common function prototype to bootloader and promloader.
void opcode1_execute(ELoaderState_t* loaderState, LoaderMessage_t* message)
{
	Uint32	ApplicationExecuteAddress;

    // check that the data length is exactly 4
    if( ( (gBusCOM == BUS_SSB) || (gBusCOM == BUS_ISB))
       && (message->dataLengthInBytes != 4) )
    {
        loader_MessageSend(LOADER_PARAMETER_OUT_OF_RANGE, 0, "");
    }
    else
    {
    	ApplicationExecuteAddress = utils_toUint32(message->dataPtr, TARGET_ENDIAN_TYPE);
    	ToolSpecificHardware_ApplicationExecute((void*)ApplicationExecuteAddress);
    }
}


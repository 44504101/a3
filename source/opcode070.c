/**
 * \file
 * Implementation of opcode 70 (Reset subsystem - bootloader)
 * 
 * @author Scott DiPasquale
 * @date 2 September 2004
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
#include "opcode070.h"
#include "tool_specific_hardware.h"

//lint -e{715} Symbol not referenced - common function prototype to bootloader and promloader.
void opcode70_execute(ELoaderState_t* loaderState, LoaderMessage_t* message)
{
    // There is no point in the bootloader in which reseting can leave
    // the ROM in a bad state upon reboot, so allow a reset at any point in time

    loader_MessageSend(LOADER_OK, 0, "");
	Timer_Wait((Uint32)500u);        // Wait for 0.5 second, allow the last character to be sent
    ToolSpecificHardware_CPUReset();
}

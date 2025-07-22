/**
 * \file
 * Definition of opcode 191 (compute program CRC)
 * 
 * @author Scott DiPasquale
 * @date 14 September 2004
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
#include "opcode191.h"

//lint -e{715} Symbol not referenced - common function prototype to bootloader and promloader.
void opcode191_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
                        Timer_t* timer)
{
    // ALWAYS invalid here
    loader_MessageSend( LOADER_INVALID_OPCODE, 0, "" );
}

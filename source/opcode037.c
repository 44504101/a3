/**
 * \file
 * Implementation of opcode37( download ) for Flashloader
 * 
 * @author Scott DiPasquale
 * @date 14 September 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */

#include "common_data_types.h"
#include "loader_state.h"
#include "timer.h"
#include "comm.h"
#include "opcode037.h"
#include "tool_specific_config.h"
#include "utils.h"
#include "tool_specific_programming.h"
#include "prom_hardware.h"

//***************************************
// Static function declarations
//***************************************

/**
 * Processes the downloaded data.
 * 
 * @param message Pointer to the received message
 * @param timer Current time being used for the system timeout
 */
static void doDownload( LoaderMessage_t * message, Timer_t * timer ) ;


//***************************************
// functions declared in included files
//***************************************

void opcode37_execute( ELoaderState_t * loaderState, LoaderMessage_t * message,
                       Timer_t * timer )
{
	//lint -e{788} enum constant not used within switch.
    switch( *loaderState )
    {
    case LOADER_SCRATCH_PREPARED :
        *loaderState = LOADER_DOWNLOADING;
        
        //lint -fallthrough intentional fall-through
        
    // We're currently downloading an application, so process the download
    case LOADER_DOWNLOADING :
        doDownload( message, timer );
        break;
        
    default:
        loader_MessageSend( LOADER_INVALID_OPCODE, 0, "" );
    }
}

//***************************************
// Static functions
//***************************************

static void doDownload( LoaderMessage_t * message, Timer_t * timer )
{
    Uint32 numBytes;
    Uint32 address;
    
    // check the data length
    // should be at least 5 ( 4 address bytes and 1 size byte )
    if((gBusCOM==BUS_SSB)&& (message->dataLengthInBytes < 5) )
    {
        // send error code and return
        loader_MessageSend( LOADER_WRONG_NUM_PARAMETERS, 0, "" );
        return;
    }

    address = utils_toUint32(message->dataPtr, TARGET_ENDIAN_TYPE);
    numBytes = message->dataPtr[4];
    
    // do the application code write
    if (PromHardware_ProgramMemoryWrite( &message->dataPtr[5], numBytes, address ) )
    {
        loader_MessageSend( LOADER_OK, 0, "" );
        
        Timer_TimerReset( timer );
    }
    else
    {
        // something about one of the parameters was invalid,
        // or we couldn't write to the flash itself, so send a message to that effect.
        loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
    }
}

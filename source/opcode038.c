/**
 * \file
 * Definition of opcode 38 ( upload program ) for Flashloader
 * 
 * @author Scott DiPasquale
 * @date 7 September 2004
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
#include "opcode038.h"
#include "tool_specific_config.h"
#include "utils.h"
#include "tool_specific_programming.h"
#include "prom_hardware.h"

//***************************************
// Static declarations
//***************************************

/**
 * Processes the message and sends the appropriate response.
 * 
 * @param message Pointer to the received message
 * @param timer Timer to be reset upon a successful upload
 */
static void doUpload( LoaderMessage_t * message, Timer_t * timer ) ;

static Uint8 buffer[256] ;   // length field is 1 byte, so this is the maximum length


//***************************************
// Implementations from included files
//***************************************

void opcode38_execute( ELoaderState_t * loaderState, LoaderMessage_t * message,
                       Timer_t * timer )
{
	//lint -e{788} enum constant not used within switch.
    switch( *loaderState )
    {
    
    case LOADER_ACTIVATED:
        // for verify-only
        //lint -fallthrough intentional fall through
    
    case LOADER_DOWNLOADING:
    
        *loaderState = LOADER_UPLOADING;
        //lint -fallthrough intentional fall through
        
    case LOADER_UPLOADING:
    
        doUpload( message, timer );
        break;
        
    default:
        // command doesn't make sense here.
        // send 'invalid' response
        loader_MessageSend( LOADER_INVALID_OPCODE, 0, "" );
    }
}

//***************************************
// Definitions of static functions
//***************************************

static void doUpload( LoaderMessage_t * message, Timer_t * timer )
{
    Uint32 address;
    Uint16 length;
 
    // check that the data length is 5.
    // Need:
    //       4 bytes of address
    //       1 bytes of length
    if((gBusCOM==BUS_SSB)&& (message->dataLengthInBytes != 5) )
    {
        loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
        return;
    }

    address = utils_toUint32( message->dataPtr, TARGET_ENDIAN_TYPE );
    length = message->dataPtr[4];
    
    // maybe check here that we're not moving into verboten memory spaces??
   
    // send the message 
    if (PromHardware_ProgramMemoryRead( buffer, (Uint32)length, address ) )
    {
        // send message
        loader_MessageSend( LOADER_OK, length, (char *)buffer );
        // reset the timer
        Timer_TimerReset( timer );
    }
    else
    {
        loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
    }
}

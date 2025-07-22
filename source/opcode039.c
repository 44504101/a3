/**
 * \file
 * Implementation of opcode 39 (unprotect/protect EEPROM) for Flashloader
 * 
 * @author Scott DiPasquale
 * @date 13 September 2004
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
#include "opcode039.h"
#include "utils.h"
#include "tool_specific_config.h"
#include "tool_specific_programming.h"
#include "prom_hardware.h"


//***************************************
// Static variables
//***************************************
static Uint16 				mMessageType;
static Uint16 				mPartition;
static EProgrammingStatus_t	mCurrentProgrammingState = PROGRAMMING_NOT_BEGUN;
static Uint16 				mFlashErrorCode = 0u;

void opcode39_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
                       Timer_t* timer)
{
	// Reset timeout timer before doing anything else - if this function
	// has been called then a valid opcode has been received, so we need
	// to process it without worrying about timing out and rebooting.
    Timer_TimerReset(timer);

	// check the message payload for size.  needs to be 3 bytes,
    // else send wrong num of parameters error
    if((gBusCOM==BUS_SSB)&& (message->dataLengthInBytes != 3 ))
    {
        loader_MessageSend( LOADER_WRONG_NUM_PARAMETERS, 0, "" );
        return;
    }
    
    // get message type and make sure it's valid
    mMessageType = message->dataPtr[0];
    if( ( mMessageType != OPCODE39_UNPROTECT ) &&
        ( mMessageType != OPCODE39_PROTECT ) &&
        ( mMessageType != OPCODE39_CHECKSUM ) )
    {
        loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
        return;
    }

	//lint -e{788} Enum constant not used within defaulted switch - default is OK.
    switch( *loaderState )
    {

    case LOADER_ACTIVATED :
        // Here we should find a message type of OPCODE39_UNPROTECT.
        // If so, set up the mPartition, else throw an error.
    	// The partition in the data buffer will be in little endian format.
        if( OPCODE39_UNPROTECT == mMessageType )
        {
            mPartition = utils_toUint16(&message->dataPtr[1], TARGET_ENDIAN_TYPE);

            if( PromHardware_isValidPartition(mPartition) == FALSE )
            {
                // Got an invalid mPartition.
                loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
            }
            else
            {
            	// Advance the state to LOADER_PREPARING_SCRATCH and send
            	// back the message BEFORE preparing the partition.
            	// This is because some devices, when erasing the partition,
            	// will just sit in the erase function and only return on
            	// completion, so you want an SSB-reply for Toolscope first.
				*loaderState = LOADER_PREPARING_SCRATCH;
				loader_MessageSend( LOADER_OK, 0, "" );

            	// Prepare the partition (either blank an area in RAM
            	// or erase the flash itself, depending on how the system works).
				// Note than a flash erase could take a long time, and the code
				// might sit in this function (application specific, but possible).
            	mFlashErrorCode = PromHardware_PartitionPrepare(mPartition);
            }
        }
        else
        {
            loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
        }
        break;

    case LOADER_PREPARING_SCRATCH :

        if( OPCODE39_UNPROTECT == mMessageType )
        {
         //we are here if toolscope didn't receive response when loaderState was LOADER_ACTIVATED
         //and OPCODE39_UNPROTECT == mMessageType
         // PromHardware_PartitionPrepare() for Application take around 2 sec
            loader_MessageSend( LOADER_OK, 0, "" );
        }
        // Should find message type of OPCODE39_PROTECT: polling for completion.
        else if( OPCODE39_PROTECT == mMessageType )
        {
        	if (PromHardware_isPartitionPrepared() == TRUE)
        	{
        		loader_MessageSend( LOADER_OK, 0, "" );
        		*loaderState = LOADER_SCRATCH_PREPARED;
        	}
        	else
        	{
        		loader_MessageSend( LOADER_CANNOT_FORMAT, 1, (char*)&mFlashErrorCode );
        	}
        }
        else
        {
            loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
        }
        break;

    case LOADER_SCRATCH_PREPARED :
        if( OPCODE39_PROTECT == mMessageType )
        {
            loader_MessageSend( LOADER_OK, 0, "" );
        }
        else
        {
            loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
        }
        break;

    // This loader state is entered when an opcode 37 is received for the first time
    // (assuming that the system is currently in the LOADER_SCRATCH_PREPARED state,
    // otherwise an invalid opcode error is reported).
    case LOADER_DOWNLOADING:
        // the option exists in the downloader to skip verification, which
        // results in going directly from downloading to programming
        //lint -fallthrough intentional fallthrough

    // This loader state is entered when an opcode 38 is received and we're not already in this state.
    case LOADER_UPLOADING:
        // Should find message of type OPCODE39_CHECKSUM, which will have a CRC attached to it.
        if( OPCODE39_CHECKSUM == mMessageType )
        {
        	// Send message BEFORE we start programming, as hardware may wait in the programming function,
        	// so otherwise there would be no response for a long time.
            mCurrentProgrammingState = PROGRAMMING_NOT_BEGUN;
            loader_MessageSend( LOADER_OK, 0, "" );
            *loaderState = LOADER_PROGRAMMING;

            if ( PromHardware_PartitionCRCValidate(utils_toUint16(&message->dataPtr[1], TARGET_ENDIAN_TYPE) ) )
            {
                // start programming
            	mCurrentProgrammingState = PROGRAMMING_IN_PROGRESS;
                mFlashErrorCode = PromHardware_PartitionProgram();
            }
            else
            {
                mCurrentProgrammingState = PROGRAMMING_INVALID_CRC;
            }
        }
        else
        {
            loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
        }
        break;

    case LOADER_PROGRAMMING:
        // Should find message type OPCODE39_PROTECT: polling for completion
        if( OPCODE39_PROTECT == mMessageType )
        {
            if( PromHardware_isPartitionProgrammed() )
            {
                loader_MessageSend( LOADER_OK, 0, "" );
                *loaderState = LOADER_DONE_PROGRAMMING;
                mCurrentProgrammingState = PROGRAMMING_SUCCEEDED;
            }
            else
            {
                if( PROGRAMMING_IN_PROGRESS == mCurrentProgrammingState )//doesn't happen if hardware waits in program function.
                {
                    loader_MessageSend( LOADER_FORMAT_IN_PROGRESS, 0, "" );
                }
                else if( PROGRAMMING_INVALID_CRC == mCurrentProgrammingState )
                {
                    loader_MessageSend( LOADER_VERIFY_FAILED, 0, "" );
                }
                else
                {
                    //partition not programmed, something went really wrong
					loader_MessageSend(LOADER_CANNOT_FORMAT,1, (char*)&mFlashErrorCode);
                }
            }
        }
        else
        {
            loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
        }
        break;

    case LOADER_DONE_PROGRAMMING:
    	loader_MessageSend( LOADER_OK, 0, "" );
    	break;

    // Any other state just sends back status 99 and the loader state, for debug.
    // There shouldn't be any other states which aren't handled by the above,
    // but just in case...
    default:
        loader_MessageSend( 99, 1,(char*)loaderState );
        break;
    }

    Timer_TimerReset(timer);
}

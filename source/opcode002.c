/**
 * \file
 * Implementation of opcode 2 / 201
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
#include "opcode002.h"
#include "tool_specific_config.h"
#include "comm.h"
#include "self_test.h"

#define BAD_BL_IMAGE_STR    "BL  corrupt"
#define BAD_APP_IMAGE_STR   "App corrupt"
#define BAD_BOTH_IMAGE_STR  "All corrupt"

static char bootString[] = BAD_BL_IMAGE_STR;
static char appString[] = BAD_APP_IMAGE_STR;
static char bothString[] = BAD_BOTH_IMAGE_STR;


void opcode2_execute(ELoaderState_t* loaderState, Timer_t* timer)
{   
    char goodIDString[] = BOOTLOADER_BOARD_ID;
    char badIDString[] = BOOTLOADER_BOARD_ID_ERR;
    char * idString = goodIDString;
    char * errString = NULL;

    // Return the identity of the Loader in all cases, and reset the timer
    // if the loader has been activated
    
    // first, though, determine if whether we need to substitute some of the 
    // characters in the ID string to tell the Toolscope user that there is
    // an error with the CRC checks
    if(!SelfTest_isBootloaderImageValid() )
    {
        if(SelfTest_isApplicationImageValid() )
        {
            errString = bootString;
        }
        else
        {
            errString = bothString;
        }
    }
    else if(!SelfTest_isApplicationImageValid() )
    {
        errString = appString;
    }
    else
    {
        errString = NULL;
    }
    
    // now substitute the err message into the idString if there is an
    // error message
    if( errString != NULL )
    {
        idString = badIDString;
		/* //Don't do substitution, screws up Toolscope
        for( idIndex = BOARD_ID_STATUS_INDEX, errIndex=0;
             (idIndex < BOARD_ID_LENGTH) && (errIndex < strlen(errString));
             ++idIndex, ++errIndex )
        {
            idString[idIndex] = errString[errIndex];
        }
		*/
    }
    
    // Send the message
    loader_MessageSend(LOADER_OK, BOARD_ID_LENGTH, idString);
    Timer_TimerSet(timer,60000);
    if(*loaderState != LOADER_WAITING)
    {
        Timer_TimerSet(timer,60000);
    }
}


// ----------------------------------------------------------------------------
/**
 * @file    	comm.c
 * @author
 * @date
 * @brief		Functions for determining which communication bus to use
 * 				(and then using it) for	SDRM bootloader and flashloader.

 * @note
 * In order to keep this code as similar as possible to the previous version,
 * and to make it (hopefully) more portable, we've used conditional compilation
 * to only pull in the features needed for the communications channels which
 * are actually used. So, if the CAN or debug port isn't used, the code won't
 * be included.  This should also have the added benefit of keeping the code
 * size down, which might be important if you're running on a platform which
 * has limited resources.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "common_data_types.h"
#include "timer.h"
#include "comm.h"
#include "serial_comm.h"
#include "tool_specific_config.h"
#include "tool_specific_hardware.h"


// ----------------------------------------------------------------------------
// Setup bus type and include any bus specific files - not all tools will need
// the CAN or the debug port, so we don't want to have to pull in a whole load
// of stuff we don't need.  If a particular port is not being used, we need
// some function prototypes for dummy functions to mimic the behaviour of the
// real functions.

#ifdef COMM_CAN
#include "can_task.h"
#else
static void 			proccessMessagesReceived(void);
static void             proccessMessagesToTransmit(void);
static int 				hasReceivedSDO(void);
static EMessageStatus_t cop_update_mess(void);
static LoaderMessage_t* cop_GetMessage(void);
static void 			cop_MessageSend(char status, int length, char* data);
#endif

#ifdef COMM_DEBUG
#include "debug.h"
#else
static bool_t			Debug_HaltMessageCheck(void);
static LoaderMessage_t* Debug_LoaderMessagePointerGet(void);
static void 			Debug_MessageSend(Uint8 status, Uint16 length, char_t* pData);
static EMessageStatus_t	Debug_MessageCheck(void);
#endif

#if !defined (COMM_CAN) && !defined (COMM_SSB) && !defined (COMM_DEBUG)
#error Undefined Communications Port!
#endif


// ----------------------------------------------------------------------------
// Setup initial bus type as undefined - note that this has global scope.
// The conditional compilation above means we won't be including ports we don't
// want (there will always be SSB) so we won't get spurious characters \ the
// wrong port selected, so it's safe to do this and let the code choose the
// correct port.

EBusType_t gBusCOM = BUS_UNDEFINED;


// ----------------------------------------------------------------------------
// Buffer for received message(s) - note that this has global scope.

unsigned char gRxBuffer[COMM_MAX_LENGTH];


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * loader_waitForMessage waits for a message from the appropriate communications
 * port, based on the variable gBusCOM.  If this is set to BUS_UNDEFINED, this
 * function takes in messages from all ports, and whichever gets a message first
 * is set to be the bus of choice for future messages.
 * If a message is received successfully, the function returns a pointer to
 * the loader message structure, otherwise it returns NULL.
 * Note the suppression of the Lint warning 'Lacks side effects' - the test
 * code is setup so it doesn't pull in the can, so the dummy function appears
 * to lack side effects, but it's not a problem.
 *
 * @param	pTimer				Pointer to timer structure, for timeouts.
 * @retval	LoaderMessage_t*	Pointer to the loader message structure.
 *
 */
// ----------------------------------------------------------------------------
LoaderMessage_t* loader_waitForMessage(Timer_t* pTimer)
{ 
    EMessageStatus_t 	status = MESSAGE_ERROR;
    bool_t 				bSSBSOFdone = FALSE;
    bool_t              bIsbSOFdone = FALSE;
    bool_t				bGotDebugMessage = FALSE;

    // Enable reception
    ToolSpecificHardware_SSBTransmitDisable();
    //ToolSpecificHardware_ISBTransmitDisable();
    // Wait in this loop for a good message or the timer to time out.
	while( (status != MESSAGE_OK) && (Timer_TimerExpiredCheck(pTimer) == FALSE) )
	{
		// If bus type undefined then check all possible options
		// on a first past the post basis (i.e. whoever talks first is the bus).
		if (gBusCOM == BUS_UNDEFINED)
		{
		    while( (Timer_TimerExpiredCheck(pTimer) == FALSE)
		    		&& (gBusCOM == BUS_UNDEFINED)
		    		&& (bSSBSOFdone == FALSE)
		    		&& (bIsbSOFdone == FALSE)
		    		&& (hasReceivedSDO() == 0)
		    		&& (bGotDebugMessage == FALSE))
		    {

		        bSSBSOFdone = serial_StartCharacterReceivedCheck(BUS_SSB);
		        //bIsbSOFdone = serial_StartCharacterReceivedCheck(BUS_ISB);
//		        bGotDebugMessage = Debug_HaltMessageCheck();
			    //proccessMessagesReceived();						//lint !e522 Lacks side effects.
			    // check if there is a Msg to transmit and process it
			    //proccessMessagesToTransmit();
		    }

		    if (bSSBSOFdone == TRUE)
		    {
		        gBusCOM = BUS_SSB;

		        // Disable CAN interrupt, SSB communications only from now on...
		        ToolSpecificHardware_CANInterruptDisable();

		        // Wait for message, not including start character (as we've found this already).
		        status = serial_MessageWait( pTimer, TRUE , BUS_SSB) ;
		    }
		    else if (bIsbSOFdone == TRUE)
            {
                gBusCOM = BUS_ISB;

                // Disable CAN interrupt, SSB communications only from now on...
                ToolSpecificHardware_CANInterruptDisable();

                // Wait for message, not including start character (as we've found this already).
                status = serial_MessageWait( pTimer, TRUE , BUS_ISB) ;
            }
		    else if (bGotDebugMessage == TRUE)
		    {
		    	gBusCOM = BUS_DEBUG;

		    	// Disable CAN interrupt, debug port communications only from now on...
		        ToolSpecificHardware_CANInterruptDisable();
		        // Force while loop to accept that we've had a message.
		        status = MESSAGE_OK;
		    }
		    else
		    {
		        gBusCOM = BUS_CAN;
		    }	 	
		}
		else if ( (gBusCOM == BUS_SSB) || (gBusCOM == BUS_ISB) )
		{
		    // Wait for message, including start character.
		    status = serial_MessageWait( pTimer, FALSE , gBusCOM);
		}
		else if (gBusCOM == BUS_CAN)
		{
		    while( (Timer_TimerExpiredCheck(pTimer) == FALSE) && (hasReceivedSDO() == 0) )
		    {
                // Wait for an SDO message.
			    proccessMessagesReceived();						//lint !e522 Lacks side effects
			    // check if there is a Msg to transmit and process it
			    proccessMessagesToTransmit();
		    }

		    if (Timer_TimerExpiredCheck(pTimer) == FALSE)
		    {
		        status = cop_update_mess();
		    }
		    else
		    {
		  	    status = MESSAGE_TIMEOUT;	  
		    }
		}
		else if (gBusCOM == BUS_DEBUG)
		{
			// Check for a debug message - note that this doesn't use a timeout
			// as we're going round in a loop anyway, so it's not really needed.
			status = Debug_MessageCheck();
		}
	}

	// Once we get to here we've out of the while loop,
	// so we've either timed out or got a good message.
	if (Timer_TimerExpiredCheck(pTimer) == TRUE)
    {
	    return NULL;
    }

	// Return a pointer to the correct message structure, depending on the port.
	if ((status == MESSAGE_OK) && (gBusCOM == BUS_CAN))
	{
	    return cop_GetMessage();
	}
	else if ((status == MESSAGE_OK) && ( (gBusCOM == BUS_SSB) || (gBusCOM == BUS_ISB) ) )
	{
        return serial_LoaderMessagePointerGet();
    }
	else if ((status == MESSAGE_OK) && (gBusCOM == BUS_DEBUG))
	{
		return Debug_LoaderMessagePointerGet();
	}
	else
    {
		return NULL;
    }
}


// ----------------------------------------------------------------------------
/**
 * @note
 * loader_MessageSend sends a message over the selected communications port.
 * Note the conditional compilation to forward SSB replies to the debug port -
 * if we don't conditionally declare *pMessage, we get Lint warnings.
 *
 * @param	Status					Status value to send.
 * @param	LengthOfDataInBytes		Number of bytes of data to send.
 * @param	pData					Pointer to data to send.
 *
 */
// ----------------------------------------------------------------------------
void loader_MessageSend(Uint8 Status, Uint16 LengthOfDataInBytes, char* pData)
{
#if defined (COMM_DEBUG) && defined (COMM_DEBUG_FORWARD_SSB)
	LoaderMessage_t* pMessage;
#endif

    if (gBusCOM == BUS_SSB)
    {
#if defined (COMM_DEBUG) && defined (COMM_DEBUG_FORWARD_SSB)
    	pMessage = serial_LoaderMessagePointerGet();
    	Debug_LoaderMessageSend(pMessage->opcode, Status, LengthOfDataInBytes, pData);
#endif
    	serial_MessageSend(Status, LengthOfDataInBytes, pData, BUS_SSB);
    }
    else if (gBusCOM == BUS_ISB)
    {
        serial_MessageSend(Status, LengthOfDataInBytes, pData, BUS_ISB);
    }
    else if (gBusCOM == BUS_CAN)
    {
        cop_MessageSend((char)Status, (int)LengthOfDataInBytes, pData);		// TODO Fix compatibility
    }
    else if (gBusCOM == BUS_DEBUG)
    {
   		Debug_MessageSend(Status, LengthOfDataInBytes, pData);
    }
    else
    {
    	;		// Do nothing if we don't know which bus to use.
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - DUMMY FUNCTIONS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Conditional compilation includes these dummy function prototypes if we're
// not using CAN or the debug port, so we don't have to include files we don't
// need.
#ifndef COMM_CAN
static void proccessMessagesReceived(void)
{

}

static void proccessMessagesToTransmit(void)
{

}

static int hasReceivedSDO(void)
{
	return 0;
}

static EMessageStatus_t cop_update_mess(void)
{
	return MESSAGE_OK;
}

static LoaderMessage_t* cop_GetMessage(void)
{
	return NULL;
}

//lint -e{715} Symbols not referenced - function is for SSB-only operation.
static void cop_MessageSend(char status, int length, char* data)
{

}
#endif /* COMM_CAN */

#ifndef COMM_DEBUG
static bool_t Debug_HaltMessageCheck(void)
{
	return FALSE;
}

static LoaderMessage_t* Debug_LoaderMessagePointerGet(void)
{
	return NULL;
}

//lint -e{715} Symbols not referenced - function is for debug port-less operation.
static void Debug_MessageSend(Uint8 status, Uint16 length, char_t* pData)
{

}

EMessageStatus_t Debug_MessageCheck(void)
{
	return MESSAGE_ERROR;
}
#endif /* COMM_DEBUG */


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


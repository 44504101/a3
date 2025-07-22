/*******************************************************************
 *
 *    DESCRIPTION: Handles transmission and buffering of opcode
 *					commands.
 *
 *    AUTHOR: Sterling Wei (based on Scott DiPasque's portable bootloader)
 *
 *    HISTORY:
 *
 *    NOTES: Some debug functionality is built into this code, using
 *    the function ToolSpecificHardware_DebugMessageSend() - this is
 *    called throughout this module, and the user can decide whether
 *    to do anything in the function or not (this removes the previous
 *    conditional compilation which was found throughout - the code
 *    overhead to call a function which does nothing is probably not
 *    worth worrying about, or can be optimised away if it's a problem).
 *
 *******************************************************************/

#include "common_data_types.h"
#include "timer.h"
#include "comm.h"
#include "serial_comm.h"
#include "tool_specific_hardware.h"
#include "tool_specific_config.h"
#include "utils.h"

#define SLAVE_ADDRESS_NOT_SET           (0U)

static void     TransmitEnable(EBusType_t busType);
static void     TransmitDisable(EBusType_t busType);
static void     CommPortByteSend(unsigned char data, EBusType_t busType);
static bool_t 	CheckForNextSerialCharacter(unsigned char* pCharacter, EBusType_t busType);
static bool_t 	CheckForStartCharacter(Timer_t* pTimer, EBusType_t busType);
static bool_t   CheckForSlaveAddress(EBusType_t busType);

static LoaderMessage_t 	mLoaderMessage;
static Timer_t 			mInterCharacterTimer;
static uint8_t			mSSBSlaveAddress = SSB_SLAVE_ADDRESS;
static uint8_t          mAltSSBSlaveAddress = SLAVE_ADDRESS_NOT_SET;
static uint8_t          mISBSlaveAddress = ISB_SLAVE_ADDRESS;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * serial_StartCharacterReceivedCheck reads the BUS port to see if a start of
 * frame (SOF) character has been received.  Note that we return FALSE for
 * either no character received or a character received which wasn't a SOF,
 * and also note that we only read once - we don't wait in here.
 *
 * @param   busType     Serial port bus type (ISB or SSB).
 * @retval	bool_t		TRUE if SOF detected, otherwise FALSE.
 *
 */
// ----------------------------------------------------------------------------
bool_t serial_StartCharacterReceivedCheck(EBusType_t busType)
{
    unsigned char   character;
    bool_t			status = FALSE;

    // If a character is received then the return value is TRUE.
    if ( BUS_SSB == busType )
    {
        status = ToolSpecificHardware_SSBPortCharacterReceiveReadOnce( &character );
    }
    else if ( BUS_ISB == busType )
    {
        status = ToolSpecificHardware_ISBPortCharacterReceiveReadOnce( &character );
    }

    // If a character has been received then check it to make sure it's a start.
    if (status == TRUE)
    {
        status = FALSE;
        if (character == SERIAL_STARTCHAR)
        {
            status = TRUE;
        }
    }
    return status;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * serial_LoaderMessagePointerGet returns a pointer to the loader message structure.
 * This will contain any message which has been received over the BUS port.
 *
 * @retval	LoaderMessage_t*	Pointer to loader message structure.
 *
 */
// ----------------------------------------------------------------------------
LoaderMessage_t* serial_LoaderMessagePointerGet(void)
{
    return &mLoaderMessage;
}



// ----------------------------------------------------------------------------
/**
 * @note
 * serial_MessageWait waits for a timeout or a message received on the SSB port.
 * Note that this function uses two timers - the timer which is passed into the
 * function (which is used for the overall timeout and timeout waiting for the
 * start) and a local timer which is used for the inter-character timeout.
 *
 * @param	pExternalTimer					Pointer to external timer structure.
 * @param	bFoundStartCharacterAlready		Look for start character?
 * @param   busType                         Serial port bus type (ISB or SSB)
 * @retval	LoaderMessage_t*				Pointer to loader message structure.
 *
 */
// ----------------------------------------------------------------------------
EMessageStatus_t serial_MessageWait(Timer_t* pExternalTimer,
                                    bool_t bFoundStartCharacterAlready,
                                    EBusType_t busType)
{
    unsigned char 		character;
    unsigned char 		msgLength[2];
    unsigned char 		checksumBytes[2];
    Uint16 				i;
    Uint16 				dataLength = 0;
    Uint16 				calculatedChecksum;
    bool_t 				done = FALSE;
    EMessageStatus_t 	replyStatus = MESSAGE_OK;
    Uint16				stateCounter = 0u;
    bool_t				bStateResetRequired;

    Timer_TimerSet(&mInterCharacterTimer, (Uint32)COMM_TIMEOUT);

	while (done == FALSE)
	{
		bStateResetRequired = FALSE;

		// Note - the state engine executes one state each time is is called
		// (the while(!done) above ensures we keep looping round).
		// The stateCounter is incremented at the end of the state engine,
		// or reset if the bStateResetRequired flag is set.
		switch (stateCounter)
		{
			// Check for the main timer expired - exit if it has.
			case 0u:
				if (Timer_TimerExpiredCheck(pExternalTimer) == TRUE)
				{
					replyStatus = MESSAGE_TIMEOUT;
					done = TRUE;
				}
				break;

			// Look for the start character (if we need to).
			// If the main timer has expired then exit.
			case 1u:
				if (bFoundStartCharacterAlready == FALSE)
				{
					if (CheckForStartCharacter(pExternalTimer, busType) == FALSE)
					{
						replyStatus = MESSAGE_TIMEOUT;
						done = TRUE;
					}
				}
				break;

			// Look for the address.
			case 2u:
				if (CheckForNextSerialCharacter(&character, busType) == FALSE)
				{
					bStateResetRequired = TRUE;
				}
				break;

			// Copy the address which was stored in the previous state, then
			// look for the length - 2 bytes, dealt with by cases 3 and 4.
			// Note that the state cannot be incremented unless a character has#
			// been received
			case 3u:
				mLoaderMessage.address = character;									//lint !e644 May not have been initialised
				if (CheckForNextSerialCharacter(&msgLength[0], busType) == FALSE)
				{
					bStateResetRequired = TRUE;
				}
				break;

    		// Case 4 also checks that the length is within the proper range.
			case 4u:
				if (CheckForNextSerialCharacter(&msgLength[1], busType) == TRUE)
				{
					mLoaderMessage.length = utils_toUint16(msgLength, TARGET_ENDIAN_TYPE);
					if (mLoaderMessage.length > SERIAL_MAX_LENGTH)
					{
						ToolSpecificHardware_DebugMessageSend("SERIAL PORT: Invalid Length.\r");
						replyStatus = MESSAGE_ERROR;
						done = TRUE;
					}
					else
					{
					    mLoaderMessage.dataLengthInBytes = mLoaderMessage.length - SERIAL_HEADER_LENGTH;
					}
				}
				else
				{
					bStateResetRequired = TRUE;
				}
				break;

			// Look for the command.
			case 5u:
				if (CheckForNextSerialCharacter(&character, busType) == FALSE)
				{
					bStateResetRequired = TRUE;
				}
				break;

			// Copy the opcode which was received in the previous state, then
			// get the data.
			// warning, long delay here is possible if data length
			// somehow becomes REALLY long (bad length read).
			case 6u:
				mLoaderMessage.opcode = character;
				dataLength = mLoaderMessage.length - SERIAL_HEADER_LENGTH;
				for (i = 0; i < dataLength; ++i)
				{
					if (CheckForNextSerialCharacter(&gRxBuffer[i], busType) == FALSE)
					{
						break;	// Out of for loop.
					}
				}

				if (i != dataLength)
				{
					ToolSpecificHardware_DebugMessageSend("SERIAL PORT: Timeout waiting for next data character.\r");
					replyStatus = MESSAGE_TIMEOUT;
					done = TRUE;
				}
				// If we got here, we have all the data.
				else
				{
					mLoaderMessage.dataPtr = gRxBuffer;
				}
				break;

			// Read the checksum - 2 bytes, dealt with by cases 7 and 8.
			case 7u:
				if (CheckForNextSerialCharacter(&checksumBytes[0], busType) == FALSE)
				{
					bStateResetRequired = TRUE;
				}
				break;

			case 8u:
				if (CheckForNextSerialCharacter(&checksumBytes[1], busType) == FALSE)
				{
					bStateResetRequired = TRUE;
				}
				break;

			// Look for the end character.
			case 9u:
				mLoaderMessage.checksum = utils_toUint16(checksumBytes, TARGET_ENDIAN_TYPE);
				if (CheckForNextSerialCharacter(&character, busType) == TRUE)
				{
					if (character != SERIAL_ENDCHAR)
					{
						ToolSpecificHardware_DebugMessageSend("SERIAL PORT: No terminating Character.\r");
						replyStatus = MESSAGE_ERROR;
						done = TRUE;
					}
				}
				else
				{
					bStateResetRequired = TRUE;
				}
				break;

			// Check the checksum.
			case 10u:
				calculatedChecksum = mLoaderMessage.address + msgLength[0] + msgLength[1] + mLoaderMessage.opcode;	//lint !e644 May not have been initialised
				for (i = 0; i < mLoaderMessage.dataLengthInBytes; ++i)
				{
					calculatedChecksum += mLoaderMessage.dataPtr[i];
				}

				if (calculatedChecksum != mLoaderMessage.checksum)
				{
					ToolSpecificHardware_DebugMessageSend("SERIAL PORT: Checksum Error.\r");
					replyStatus = MESSAGE_ERROR;
					done = TRUE;
				}
				break;

			// Check the slave address.
			// Note - according to the opcodes specification, opcode zero is to be treated
			// as a broadcast address.  This doesn't work with multiple slaves, so is not
			// implemented here unless the ALLOW_BROADCAST macro is defined in the tool configuration.
			case 11u:

                if ( !CheckForSlaveAddress( busType ) )
                {
                    ToolSpecificHardware_DebugMessageSend( "SERIAL PORT: Slave Address Error.\r" );
                    replyStatus = MESSAGE_ERROR;
                }

                done = TRUE;
                break;

			default:
				bStateResetRequired = TRUE;
				break;
        }

		// Reset or increment the state counter
		// (depending on what happened above in the current state).
		// Generally the state is reset if we've timed out waiting for a character.
		if (bStateResetRequired == TRUE)
		{
			stateCounter = 0u;
		}
		else
		{
			stateCounter++;
		}
	}

	return replyStatus;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * serial_MessageSend is used to send a message back via the SSB or ISB port.
 *
 * @param   status      Status of returned message.
 * @param   length      Number of bytes of data pointed to by pData
 * @param   pData       Pointer to data to append to end of message.
 * @param   busType     Enumerated type for the bus type
 *
 */
// ----------------------------------------------------------------------------
void serial_MessageSend(Uint8 status, Uint16 length, char * data, EBusType_t busType)
{
    Uint16          msgLength;
    unsigned char   msgLenBytes[2];
    Uint16          checksum;
    unsigned char   checksumBytes[2];
    Uint16          i;

    // Enable transmission (includes delay).
    TransmitEnable(busType);

    // Calculate message length.
    msgLength = length + SERIAL_HEADER_LENGTH;
    utils_to2Bytes(msgLenBytes, (Uint16)msgLength, TARGET_ENDIAN_TYPE);

    // Calculate checksum.
    checksum = mLoaderMessage.address + msgLenBytes[0] + msgLenBytes[1] + status;
    for (i = 0; i < length; ++i)
    {
        checksum += (Uint16)(Int16)data[i];
    }
    utils_to2Bytes(checksumBytes, checksum, TARGET_ENDIAN_TYPE);

    // Send header.
    CommPortByteSend(SERIAL_STARTCHAR, busType);
    CommPortByteSend(mLoaderMessage.address, busType);
    CommPortByteSend(msgLenBytes[0], busType);
    CommPortByteSend(msgLenBytes[1], busType);
    CommPortByteSend(status, busType);

    // Send the data.
    for (i = 0u; i < length; ++i)
    {
        CommPortByteSend((Uint8)data[i], busType);
    }

    // Send the checksum.
    CommPortByteSend(checksumBytes[0], busType);
    CommPortByteSend(checksumBytes[1], busType);

    // Send the end character.
    CommPortByteSend(SERIAL_ENDCHAR, busType);

    // Disable transmission (this waits for transmit done before disabling).
    TransmitDisable(busType);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * serial_CommTimerPointerGet returns a pointer to the inter-character timer - this
 * is normally only used during unit tests.
 *
 * @retval	Timer_t*	Pointer to timer structure.
 *
 */
// ----------------------------------------------------------------------------
const Timer_t* serial_CommTimerPointerGet(void)
{
	return &mInterCharacterTimer;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * serial_SlaveAddressSet allows the SSB address to be set to something other
 * than the default value given by SSB_SLAVE_ADDRESS/ISB_SLAVE_ADDRESS.
 * Note that the new address is not checked, as this code cannot know what is
 * a valid address for the target.
 *
 * @param	NewAddress		New SSB/ISB Slave address.
 *          busType         Bus type
 *
 */
// ----------------------------------------------------------------------------
void serial_SlaveAddressSet(uint8_t NewAddress, EBusType_t busType)
{
    if (BUS_SSB == busType)
    {
        mSSBSlaveAddress = NewAddress;
    }
    else if (BUS_ISB == busType)
    {
        mISBSlaveAddress = NewAddress;
    }
    else
    {
        // Do nothing for other bus types.
    }
}

// ----------------------------------------------------------------------------
/**
 * @note
 * serial_AltSlaveAddressSet sets the alternative slave address for the given
 * bus type.  Currently only the SSB bus type supports an alternative slave
 * address.  The alternative slave address facilitates a common loader for Xceed
 * and Xcel products which use the XPB.
 *
 * @param   NewAddress      New SSB Slave address.
 *          busType         Bus type
 *
 */
// ----------------------------------------------------------------------------
void serial_AltSlaveAddressSet(const uint8_t NewAddress, const EBusType_t busType)
{
    if (BUS_SSB == busType)
    {
        mAltSSBSlaveAddress = NewAddress;
    }
    else
    {
        // Do nothing for other bus types.
    }
}

// ----------------------------------------------------------------------------
/**
 * @note
 * ssb_SlaveAddressGet returns the SSB/ISB slave address.
 *
 * @param   busType     Bus Type
 * @retval	uint8_t		SSB/ISB slave address.
 *
 */
// ----------------------------------------------------------------------------
uint8_t serial_SlaveAddressGet(EBusType_t busType)
{
    if (BUS_SSB == busType)
    {
        return mSSBSlaveAddress;
    }
    else if (BUS_ISB == busType)
    {
        return mISBSlaveAddress;
    }
    else
    {
        return 0xFFu;
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * TransmitEnable enables the SSB or ISB transmit (and disables the receiver).
 *
 * @param   busType     Bus Type
 *
 */
// ----------------------------------------------------------------------------
static void TransmitEnable(EBusType_t busType)
{
    if (BUS_SSB == busType)
    {
        ToolSpecificHardware_SSBTransmitEnable();
    }
    else if (BUS_ISB == busType)
    {
        ToolSpecificHardware_ISBTransmitEnable();
    }
    else
    {
        // Do nothing for other bus types.
    }

    // Allow the driver enough time to switch to transmit.
    Timer_Wait((Uint32)RS485_ENPIN_TOGGLE_TO_RX_DELAY);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * TransmitDisable disables the SSB or ISB transmit (i.e. switches to receive).
 *
 * @param   busType     Bus Type
 *
 */
// ----------------------------------------------------------------------------
static void TransmitDisable(EBusType_t busType)
{
    // Don't issue transmit disable until the buffers are cleared.
    if (BUS_SSB == busType)
    {
        ToolSpecificHardware_SSBPortWaitForSendComplete();
        ToolSpecificHardware_SSBTransmitDisable();
    }
    else if (BUS_ISB == busType)
    {
        ToolSpecificHardware_ISBPortWaitForSendComplete();
        ToolSpecificHardware_ISBTransmitDisable();
    }
    else
    {
        // Do nothing if we're not SSB or ISB.
    }
}


// ----------------------------------------------------------------------------
/**
 * @note
 * CommPortByteSend sends a single byte on the SSB or ISB port.
 *
 * @param   busType     Bus Type
 *
 */
// ----------------------------------------------------------------------------
static void CommPortByteSend(unsigned char data, EBusType_t busType)
{
    if (BUS_SSB == busType)
    {
        ToolSpecificHardware_SSBPortByteSend(data);
    }
    else if (BUS_ISB == busType)
    {
        ToolSpecificHardware_ISBPortByteSend(data);
    }
    else
    {
        // Do nothing if we're not SSB or ISB.
    }
}


// ----------------------------------------------------------------------------
/**
 * @note
 * CheckForNextSSBCharacter polls the SSB/ISB port, looking for the next character,
 * until either we've received a character or the inter-character timer has
 * expired.
 *
 * @param	pCharacter		Pointer to write received character into.
 *          busType         Bus Type
 * @retval	bool_t			TRUE if character received, FALSE if timed out.
 *
 */
// ----------------------------------------------------------------------------
static bool_t CheckForNextSerialCharacter(unsigned char* pCharacter, EBusType_t busType)
{
    Timer_TimerReset(&mInterCharacterTimer);

    if ( BUS_SSB == busType )
    {
        return ToolSpecificHardware_SSBPortCharacterReceiveByPolling( pCharacter , &mInterCharacterTimer);
    }
    else if ( BUS_ISB == busType )
    {
        return ToolSpecificHardware_ISBPortCharacterReceiveByPolling( pCharacter , &mInterCharacterTimer);
    }
    else
    {
        // Do nothing if we're not SSB or ISB.
    }

    return FALSE;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * CheckForStartCharacter polls the SSB port, looking for the start of frame
 * character, until we've either received SOF or the timer has expired.
 *
 * @param	pTimer			Pointer to timer structure to check for expiry.
 *          busType         Bus Type
 * @retval	bool_t			TRUE if SOF received, FALSE if timed out.
 *
 */
// ----------------------------------------------------------------------------
static bool_t CheckForStartCharacter(Timer_t* pTimer, EBusType_t busType)
{
	bool_t			bGotStartCharacter = FALSE;
	unsigned char	character;
	bool_t          status;

    // Look for start character.
    while (bGotStartCharacter == FALSE)
    {
    	// If a new character received then check to see if it's SOF.
    	// Note that ToolSpecificHardware_SSBPortCharacterReceiveByPolling
    	// returns TRUE for any character received, not just SOF!!
        if ( BUS_SSB == busType )
        {
            status = ToolSpecificHardware_SSBPortCharacterReceiveByPolling( &character , pTimer);
        }
        else if ( BUS_ISB == busType )
        {
            status = ToolSpecificHardware_ISBPortCharacterReceiveByPolling( &character , pTimer);
        }

        if ( status != FALSE)
        {
            if (character == SERIAL_STARTCHAR)
            {
                bGotStartCharacter = TRUE;
            }
        }
        // If here then we've timed out, so jump out of while loop.
        else
        {
            break;
        }
    }

    return bGotStartCharacter;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * CheckForSlaveAddress verifies the slave address received.
 *
 * We use conditional compilation here, because not all systems may want to use
 * the broadcast address functionality.
 *
 * An alternative slave address is allowed for the SSB bus type; this facilitates
 * a common loader for Xceed and Xcel products which use the XPB.
 *
 * @param   busType         Bus Type
 * @retval  bool_t          TRUE if expected Slave address, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t CheckForSlaveAddress(EBusType_t busType)
{
    bool_t bExpectedAddress = FALSE;

    if ( (BUS_SSB == busType) || (BUS_ISB == busType) )
    {
        if (   ((BUS_SSB == busType) && (mLoaderMessage.address == mSSBSlaveAddress))
            || ((BUS_SSB == busType) && (SLAVE_ADDRESS_NOT_SET  != mAltSSBSlaveAddress) && (mLoaderMessage.address == mAltSSBSlaveAddress))
            || ((BUS_ISB == busType) && (mLoaderMessage.address == mISBSlaveAddress)) )
        {
            bExpectedAddress = TRUE;
        }

#ifdef ALLOW_BROADCAST_ADDRESS
        if (BROADCAST_ADDRESS == mLoaderMessage.address)
        {
            bExpectedAddress = TRUE;
        }
#endif
    }

    return bExpectedAddress;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

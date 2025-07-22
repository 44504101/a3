// ----------------------------------------------------------------------------
/**
 * @file    	debug.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		5 Mar 2014
 * @brief		Debug port code for SDRM bootloader \ promloader.
 * @details
 * This code handles all messages which are received via the debug port, and
 * also deals with transmission of any replies.  This code allows the system
 * to be tested without using SSB, as we can download code into the device
 * via the debug port.
 *
 * @note
 * Downloaded code must be in Motorola S3 record format, with a romwidth of
 * 16 bits.  This record format is ALWAYS big endian.
 *
 * @warning
 * Parts of this code assume a target with 16 bit memory width.
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

#include <string.h>
#include "common_data_types.h"
#include "timer.h"
#include "comm.h"
#include "debug.h"
#include "buffer_utils.h"
#include "tool_specific_hardware.h"
#include "s_record.h"
#include "tool_specific_config.h"			// Need this for initial debug string.
#include "genericIO.h"
#include "loader_state.h"
#include "opcode039.h"
#include "serial_comm.h"
#include "dsp_crc.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static EMessageStatus_t 	DecodeReceivedMessage(void);
static void 				DownloadModeDo(void);
static void 				UploadModeDo(void);
static void 				UploadNextSetOfData(void);
static void					UnprotectCommandDo(void);
static void					ProtectCommandDo(void);
static void					ChecksumCommandEqualsDo(void);
static void					ChecksumCommandQueryDo(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module

static LoaderMessage_t		mDebugLoaderMessage;
static DebugParameters_t	mDebugParameters;
static uint8_t				mOpcodeDataBuffer[SRECORD_MAX_BYTE_PAIRS];


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * Debug_Initialise sets up all members of the mDebugParameters structure to
 * their starting values, and sets up an initial message to be transmitted
 * from the serial port.
 *
 */
// ----------------------------------------------------------------------------
void Debug_Initialise(void)
{
	mDebugParameters.HaltMessageState = 0u;
	mDebugParameters.ReceiveOffset = 0u;
	mDebugParameters.TransmitBuffer[0] = '\0';
	mDebugParameters.ReceiveBuffer[0] = '\0';
	mDebugParameters.bMotorolaDownloadModeEnabled = FALSE;
	mDebugParameters.bUploadModeEnabled = FALSE;

	strcpy(mDebugParameters.TransmitBuffer, "SDRM BOOTLOADER & PROMLOADER DEBUG PORT, BASELINE: "BASELINE_NAME"\r");
	ToolSpecificHardware_DebugMessageSend(mDebugParameters.TransmitBuffer);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * Debug_HaltMessageCheck is called from the main comm 'task', and checks for
 * a message received via the debug port, and that the message is *HALT!<cr>.
 *
 * @retval	bool_t		TRUE if *HALT!<cr> received, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
bool_t Debug_HaltMessageCheck(void)
{
    bool_t			status;
    unsigned char 	character;
    uint16_t		OldMessageState;

    // If a character is received then the return value is TRUE.
    status = ToolSpecificHardware_DebugPortCharacterReceiveReadOnce(&character);

    if (status == TRUE)
    {
    	status = FALSE;
    	OldMessageState = mDebugParameters.HaltMessageState;

    	// Check for each character in turn, to make sure it's the right one.
    	// Any error resets the state engine, so you have to get the whole
    	// thing right in one go - this means we won't abort the boot unless
    	// we're absolutely sure.
    	switch (mDebugParameters.HaltMessageState)
    	{
			case 0u:
				if (character == '*')
				{
					mDebugParameters.HaltMessageState++;
				}
				break;

			case 1u:
				if (character == 'H')
				{
					mDebugParameters.HaltMessageState++;
				}
				break;

			case 2u:
				if (character == 'A')
				{
					mDebugParameters.HaltMessageState++;
				}
				break;

			case 3u:
				if (character == 'L')
				{
					mDebugParameters.HaltMessageState++;
				}
				break;

			case 4u:
				if (character == 'T')
				{
					mDebugParameters.HaltMessageState++;
				}
				break;

			case 5u:
				if (character == '!')
				{
					mDebugParameters.HaltMessageState++;
				}
				break;

			case 6u:
				// If here and we've received a CR then we've got *HALT!<cr> so we want to
				// setup the message opcode as if we received opcode zero (activate loader).
				if (character == '\r')
				{
					mDebugParameters.HaltMessageState++;
					mDebugLoaderMessage.opcode = 0u;
					status = TRUE;
				}
				break;

			default:
				break;
    	}

    	// If the message state has not been incremented, an invalid character
    	// has been received, so reset the message state.
    	if (OldMessageState == mDebugParameters.HaltMessageState)
    	{
    		mDebugParameters.HaltMessageState = 0u;
    	}
    }

    return status;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * Debug_LoaderMessagePointerGet returns a pointer to the debug loader message.
 * When a command has been decoded, this structure is setup as if the command
 * had come from the SSB port.
 *
 * @retval	LoaderMessage_t*	Pointer to the debug loader message structure.
 *
 */
// ----------------------------------------------------------------------------
LoaderMessage_t* Debug_LoaderMessagePointerGet(void)
{
	return &mDebugLoaderMessage;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * Debug_MessageSend is used to send a message back to the PC if we're running
 * in debug mode.
 *
 * @param	Status				Status of returned message.
 * @param	LengthInBytes		Number of bytes of data pointed to by pData.
 * @param	pData				Pointer to data to append to end of message.
 *
 */
// ----------------------------------------------------------------------------
void Debug_MessageSend(uint8_t Status, uint16_t LengthInBytes, char_t* pData)
{
	uint16_t	Offset;

	// Start of message is OPCODE:xx:yy
	// where xx is the opcode number and yy is the status.
	// This is deliberately generic so we don't keep having to change it if
	// the number of possible status messages changes.
	strcpy(mDebugParameters.TransmitBuffer, "OPCODE:");
	(void)BUFFER_UTILS_8BitsToHex(&mDebugParameters.TransmitBuffer[7], mDebugLoaderMessage.opcode);
	mDebugParameters.TransmitBuffer[9] = ':';
	(void)BUFFER_UTILS_8BitsToHex(&mDebugParameters.TransmitBuffer[10], Status);

	// Offset points to the next free location in the buffer.
	Offset = 12u;

	// If non-zero length then convert all of the data into ASCII and put it
	// in the buffer, with each pair of digits separated by a space.
	if (LengthInBytes != 0u)
	{
		mDebugParameters.TransmitBuffer[Offset] = ':';
		Offset++;

		while (LengthInBytes != 0u)
		{
			(void)BUFFER_UTILS_8BitsToHex(&mDebugParameters.TransmitBuffer[Offset], (uint16_t)(int16_t)*pData);
			pData++;
			Offset += 2;
			mDebugParameters.TransmitBuffer[Offset] = ' ';
			Offset++;
			LengthInBytes--;
		}
		Offset--;
	}

	// Add CR and NULL to end of the buffer
	mDebugParameters.TransmitBuffer[Offset] = '\r';
	Offset++;
	mDebugParameters.TransmitBuffer[Offset] = '\0';

	ToolSpecificHardware_DebugMessageSend(mDebugParameters.TransmitBuffer);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * Debug_LoaderMessageSend is used to send a message back to the PC if we're
 * running in SSB mode and want to echo the SSB message back on the debug port.
 *
 * @param	Opcode				Opcode which is generating message.
 * @param	Status				Status of returned message.
 * @param	LengthInBytes		Number of bytes of data pointed to by pData.
 * @param	pData				Pointer to data to append to end of message.
 *
 */
// ----------------------------------------------------------------------------
void Debug_LoaderMessageSend(uint8_t Opcode, uint8_t Status, uint16_t LengthInBytes, char_t* pData)
{
	mDebugLoaderMessage.opcode = Opcode;				// Set opcode.
	Debug_MessageSend(Status, LengthInBytes, pData);
	mDebugLoaderMessage.opcode = 255u;					// Reset to avoid confusion.
}


// ----------------------------------------------------------------------------
/*
 * @note
 * Debug_MessageCheck is called from the main comm 'task', and checks for
 * a message received via the debug port, and then sets up the loader
 * structure to pretend that the message came from the SSB port.
 *
 * @retval	EMessageStatus_t	Message status.
 *
 */
// ----------------------------------------------------------------------------
EMessageStatus_t Debug_MessageCheck(void)
{
    bool_t				status;
    unsigned char 		character;
    EMessageStatus_t	MessageStatus = MESSAGE_INCOMPLETE;

    // Deliberately set the opcode to an invalid value, so we don't try and
    // process it if we've received a command which doesn't 'become' an opcode,
    // and set the transmit buffer to empty.
    mDebugLoaderMessage.opcode = 255u;
    mDebugParameters.TransmitBuffer[0] = '\0';

    // If a character is received then the return value is TRUE.
    status = ToolSpecificHardware_DebugPortCharacterReceiveReadOnce(&character);

    if (status == TRUE)
    {
	  	mDebugParameters.ReceiveBuffer[mDebugParameters.ReceiveOffset] = (char_t)character;
    	mDebugParameters.ReceiveOffset++;

    	// If message is too long then reset offset and return an error.
    	if (mDebugParameters.ReceiveOffset == MAX_DEBUG_BUFFER_RX_SIZE)
    	{
    		mDebugParameters.ReceiveOffset = 0u;
    		MessageStatus = MESSAGE_ERROR;
    	}
    	// If the last character was a carriage return then we've got
		// a whole message, so add a NULL and then process it.
    	else if (character == '\r')
    	{
    	  	mDebugParameters.ReceiveBuffer[mDebugParameters.ReceiveOffset] = '\0';
    		MessageStatus = DecodeReceivedMessage();
    	}
    	// Otherwise the message isn't all here yet.
    	else
    	{
    		MessageStatus = MESSAGE_INCOMPLETE;
    	}
    }

    // If a message has been received OK then send any reply and reset the
    // receiver offset - this buffer has finished being useful now, as any
    // data will have been copied into the LoaderMessage structure.
    if (MessageStatus == MESSAGE_OK)
    {
    	mDebugParameters.ReceiveOffset = 0u;

    	// Send reply if there's something to be sent.
    	if (mDebugParameters.TransmitBuffer[0] != '\0')
    	{
    	    ToolSpecificHardware_DebugMessageSend(mDebugParameters.TransmitBuffer);
    	}
    }

    return MessageStatus;
}


#ifdef UNIT_TEST_BUILD
// ----------------------------------------------------------------------------
/*
 * @note
 * Debug_ParameterPointerGet_TDD returns a constant pointer to the parameter
 * structure, to allow the contents to be inspected. This is for TDD only,
 * and should not be used to provide direct access under normal usage!
 *
 * @retval	DebugParameters_t*		Pointer to parameter structure.
 *
 */
// ----------------------------------------------------------------------------
const DebugParameters_t* Debug_ParameterPointerGet_TDD(void)
{
	return &mDebugParameters;
}
#endif


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * DecodeReceivedMessage decodes any received message and does something.
 * We know that the message must be terminated with a CR for this to be called.
 * Note that we always return MESSAGE_OK, even if the message is an invalid
 * command, so the reply will always be transmitted over the debug port.
 *
 * @retval	EMessageStatus_t	Always returns MESSAGE_OK.
 *
 */
// ----------------------------------------------------------------------------
static EMessageStatus_t DecodeReceivedMessage(void)
{
	int16_t	Length;

	// Remove any backspaces, and then convert the command to upper case.
	(void)BUFFER_UTILS_BackspaceRemoval(mDebugParameters.ReceiveBuffer);
	(void)BUFFER_UTILS_BufferToUpperCase(mDebugParameters.ReceiveBuffer);

	// If the message was just a CR then either do next upload or send back READY.
	if (mDebugParameters.ReceiveOffset == 1u)
	{
		if (mDebugParameters.bUploadModeEnabled == TRUE)
		{
			UploadNextSetOfData();
		}
		else
		{
			strcpy(mDebugParameters.TransmitBuffer, "READY\r");
		}
	}
	// Otherwise decode the message and do something with it...
	else
	{
		if (mDebugParameters.bMotorolaDownloadModeEnabled == TRUE)
		{
			DownloadModeDo();
		}
		else if (mDebugParameters.bUploadModeEnabled == TRUE)
		{
			UploadModeDo();
		}
		else
		{
			if (strcmp("*DOWNLOAD!\r", mDebugParameters.ReceiveBuffer) == 0)
			{
				mDebugParameters.bMotorolaDownloadModeEnabled = TRUE;
				strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Download Mode Ready\r");
			}
			else if (strcmp("*UPLOAD!\r", mDebugParameters.ReceiveBuffer) == 0)
			{
				mDebugParameters.bUploadModeEnabled = TRUE;
				mDebugParameters.UploadAddress = 0u;
				strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Upload Mode Ready\r");
			}
			else if (strncmp("*UNPROTECT", mDebugParameters.ReceiveBuffer, (int32_t)10) == 0)
			{
				UnprotectCommandDo();
			}
			else if (strcmp("*PROTECT!\r", mDebugParameters.ReceiveBuffer) == 0)
			{
				ProtectCommandDo();
			}
			else if (strncmp("*CHECKSUM=", mDebugParameters.ReceiveBuffer, (int32_t)10) == 0)
			{
				ChecksumCommandEqualsDo();
			}
			else if (strncmp("*CHECKSUM?", mDebugParameters.ReceiveBuffer, (int32_t)10) == 0)
			{
				ChecksumCommandQueryDo();
			}
			else if (strcmp("*WHOAMI?\r", mDebugParameters.ReceiveBuffer) == 0)
			{
				strcpy(mDebugParameters.TransmitBuffer, "#WHOAMI?"BASELINE_NAME", SSB slave address = 0x");
				Length = (int16_t)strlen(mDebugParameters.TransmitBuffer);
				(void)BUFFER_UTILS_8BitsToHex(&mDebugParameters.TransmitBuffer[Length], serial_SlaveAddressGet(BUS_SSB) );
				strcpy(&mDebugParameters.TransmitBuffer[Length + 2], ", ISB slave address = 0x");
				(void)BUFFER_UTILS_8BitsToHex(&mDebugParameters.TransmitBuffer[Length + 26], serial_SlaveAddressGet(BUS_ISB) );
				strcpy(&mDebugParameters.TransmitBuffer[Length + 28], ", build date = "BASELINE_DATE"\r");
			}
			else if (strcmp("*RESET!\r", mDebugParameters.ReceiveBuffer) == 0)
			{
				strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Reset received - passing to opcode 70\r");
				mDebugLoaderMessage.opcode = 70u;
			}
			else
			{
				strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Invalid Command\r");
			}
		}
	}

	return MESSAGE_OK;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * HandleDownloadMode is called when we're in download mode, and the system is
 * expecting either a line of Motorola S-Record data, or a Z to exit download
 * mode.
 *
 */
// ----------------------------------------------------------------------------
static void DownloadModeDo(void)
{
	ESRecordDecodeMessages_t 	Message;
	SRecordDecodeResults_t		DecodedData;
	uint16_t					Offset;
	uint16_t					DecodeCounter;

	if (strcmp("Z\r", mDebugParameters.ReceiveBuffer) == 0)
	{
		mDebugParameters.bMotorolaDownloadModeEnabled = FALSE;
		strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Exit Download Mode\r");
	}
	else
	{
		Message = SRecord_LineDecode(mDebugParameters.ReceiveBuffer, &DecodedData);

		switch (Message)
		{
			case SRECORD_CORRUPTED_LINE_INVALID_START_CODE:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode failed with invalid start code\r");
				break;

			case SRECORD_CORRUPTED_LINE_INVALID_BYTE_COUNT:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode failed with invalid byte count\r");
				break;

			case SRECORD_CORRUPTED_LINE_INVALID_LINE_LENGTH:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode failed with invalid line length\r");
				break;

			case SRECORD_CORRUPTED_LINE_INVALID_BYTE_CHARACTER:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode failed with invalid byte character\r");
				break;

			case SRECORD_CORRUPTED_LINE_INVALID_CHECKSUM:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode failed with invalid checksum\r");
				break;

			case SRECORD_DATA_LINE_DECODE_OK_WAS_BLOCK_HEADER:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode OK - block header ignored\r");
				break;

			case SRECORD_DATA_LINE_DECODE_OK_RECORD_NOT_SUPPORTED:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode OK - record type not supported\r");
				break;

			// If end of block then pass address into opcode 1, to boot new code.
			case SRECORD_DATA_LINE_DECODE_OK_WAS_END_OF_BLOCK:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode OK - passing to opcode 1 to boot new code\r");
				mDebugLoaderMessage.opcode = 1u;

				// Convert address back into little endian format and put in the opcode data buffer.
				utils_to4Bytes(mOpcodeDataBuffer, DecodedData.Address, LITTLE_ENDIAN);

				// Setup pointer to data buffer and set number of bytes which are in the buffer.
				mDebugLoaderMessage.dataPtr = mOpcodeDataBuffer;
				mDebugLoaderMessage.dataLengthInBytes = 4u;
				break;

			// If good data then put in opcode buffer and then pass to opcode 37, for downloading.
			// Note that the data in the opcode buffer is in different formats - the address is
			// little endian, but the data is big endian!
			case SRECORD_DATA_LINE_DECODED_OK:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode OK - passing to opcode 37 for download\r");
				mDebugLoaderMessage.opcode = 37u;
				utils_to4Bytes(mOpcodeDataBuffer, DecodedData.Address, LITTLE_ENDIAN);
				mOpcodeDataBuffer[4] = (uint8_t)(DecodedData.NumberOfDecodedDataWords * 2u);

				// Put all 16 bit data words in the opcode data buffer, in big endian format,
				// starting at offset [5].
				Offset = 5u;
				for (DecodeCounter = 0u; DecodeCounter < DecodedData.NumberOfDecodedDataWords; DecodeCounter++)
				{
					utils_to2Bytes(&mOpcodeDataBuffer[Offset], DecodedData.Data[DecodeCounter], BIG_ENDIAN);
					Offset += 2u;
				}

				// Setup pointer to data buffer, and number of data bytes which are in the buffer.
				mDebugLoaderMessage.dataPtr = mOpcodeDataBuffer;
				mDebugLoaderMessage.dataLengthInBytes = DecodedData.NumberOfDecodedDataWords * 2u;
				break;

			default:
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: SRecord decode response not recognised\r");
				break;
		}
	}
}


// ----------------------------------------------------------------------------
/**
 * @note
 * HandleUploadMode is called when we're in upload mode, and the system is
 * expecting either a ?, a new address, or a Z to exit upload mode.
 *
 */
// ----------------------------------------------------------------------------
static void UploadModeDo(void)
{
	char_t		TempBuffer[5];
	bool_t		bConvertedOK;
	uint16_t	MSBAddress;
	uint16_t	LSBAddress;

	if (strcmp("Z\r", mDebugParameters.ReceiveBuffer) == 0)
	{
		mDebugParameters.bUploadModeEnabled = FALSE;
		strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Exit Upload Mode\r");
	}
	else
	{
		// If ?\r received then use opcode 38 to upload the next 16 words.
		if (strcmp("?\r", mDebugParameters.ReceiveBuffer) == 0)
		{
			strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Upload ? - passing to opcode 38 for upload\r");
			mDebugLoaderMessage.opcode = 38u;
			utils_to4Bytes(mOpcodeDataBuffer, mDebugParameters.UploadAddress, LITTLE_ENDIAN);
			mOpcodeDataBuffer[4] = (uint8_t)32u;				// 16 words in bytes.
			mDebugLoaderMessage.dataPtr = mOpcodeDataBuffer;
			mDebugLoaderMessage.dataLengthInBytes = 5u;
			mDebugParameters.UploadAddress += 16u;
		}
		// Otherwise assume we've receive a new address, as MSB,LSB, so try and decode
		// the string and update mDebugParameters.UploadAddress.
		else
		{
			(void)strncpy(TempBuffer, &mDebugParameters.ReceiveBuffer[0], (int32_t)4u);
			TempBuffer[4] = '\0';
			bConvertedOK = BUFFER_UTILS_StringToUint16(TempBuffer, &MSBAddress, BUFFER_RADIX_HEX);

			if (bConvertedOK == FALSE)
			{
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Upload failed - invalid MSB of address\r");
			}
			else
			{
				if (mDebugParameters.ReceiveBuffer[4] != ',')
				{
					strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Upload failed - no comma between address words\r");
				}
				else
				{
					(void)strncpy(TempBuffer, &mDebugParameters.ReceiveBuffer[5], (int32_t)4u);
					TempBuffer[4] = '\0';
					bConvertedOK = BUFFER_UTILS_StringToUint16(TempBuffer, &LSBAddress, BUFFER_RADIX_HEX);

					if (bConvertedOK == FALSE)
					{
						strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Upload failed - invalid LSB of address\r");
					}
					// If both words are OK then combine into the new upload address.
					else
					{
						mDebugParameters.UploadAddress = (uint32_t)MSBAddress << 16u;
						mDebugParameters.UploadAddress |= (uint32_t)LSBAddress;
						strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Upload address set to 0x");
						(void)BUFFER_UTILS_32BitsToHex(&mDebugParameters.TransmitBuffer[31], mDebugParameters.UploadAddress);
						mDebugParameters.TransmitBuffer[39] = '\r';
						mDebugParameters.TransmitBuffer[40] = '\0';
					}
				}
			}
		}
	}
}


// ----------------------------------------------------------------------------
/**
 * @note
 * UploadNextSetOfData reads the next 16 words of data from the memory and
 * puts them in the transmit buffer - this doesn't use opcode38 as this
 * is limited to the address range it will read from, whereas we want to be
 * able to read from the entire memory here.
 *
 * @warning
 * This assumes that the data is 16 bits wide.
 *
 */
// ----------------------------------------------------------------------------
static void UploadNextSetOfData(void)
{
	uint16_t	Counter;
	uint16_t	Offset;
	uint16_t	Data;

	// Put start address of block to read in transmit buffer, then a ':'.
	(void)BUFFER_UTILS_32BitsToHex(&mDebugParameters.TransmitBuffer[0],
								mDebugParameters.UploadAddress);
	mDebugParameters.TransmitBuffer[8] = ':';

	Offset = 9u;
	for (Counter = 0u; Counter < 16u; Counter++)
	{
		Data = genericIO_16bitRead(mDebugParameters.UploadAddress);
		(void)BUFFER_UTILS_16BitsToHex(&mDebugParameters.TransmitBuffer[Offset], Data);
		Offset += 4u;
		mDebugParameters.TransmitBuffer[Offset] = ' ';
		Offset++;
		mDebugParameters.UploadAddress++;
	}

	// Add CR amd NULL to end of the buffer.
	Offset--;
	mDebugParameters.TransmitBuffer[Offset] = '\r';
	Offset++;
	mDebugParameters.TransmitBuffer[Offset] = '\0';
}


// ----------------------------------------------------------------------------
/**
 * @note
 * UnprotectCommandDo deals with the *UNPROTECT command - sets up the
 * opcode data for opcode39.  Note that the number of data bytes MUST always
 * be 3, otherwise opcode 39 generates an error, so we pad the data.
 *
 */
// ----------------------------------------------------------------------------
static void	UnprotectCommandDo(void)
{
	int16_t		Length;
	bool_t		bConvertedOK;
	uint16_t	Partition;
	bool_t		bOpcodeUpdateRequired = TRUE;
	char_t*		pTempPointer;

	// If command was just *UNPROTECT! then partition number isn't important
	// so set to something which won't be used - if we ever change this
	// then opcode39 will throw it out, which is probably a good thing.
	if (strncmp("!\r", &mDebugParameters.ReceiveBuffer[10], (int32_t)2) == 0)
	{
		strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Unprotect - no partition - passing to opcode 39\r");
		mOpcodeDataBuffer[1] = 0xFFu;
		mOpcodeDataBuffer[2] = 0xFFu;
	}
	// If command was *UNPROTECT=something then extract partition number.
	else if (mDebugParameters.ReceiveBuffer[10] == '=')
	{
		// Find end of buffer and then overwrite CR with a NUL, so the
		// string function knows where to stop.
		Length = (int16_t)strlen(mDebugParameters.ReceiveBuffer);
		mDebugParameters.ReceiveBuffer[Length - 1] = '\0';
		bConvertedOK = BUFFER_UTILS_StringToUint16(&mDebugParameters.ReceiveBuffer[11], &Partition, BUFFER_RADIX_HEX);

		if (bConvertedOK == FALSE)
		{
			strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Unprotect error - invalid partition\r");
			bOpcodeUpdateRequired = FALSE;
		}
		else
		{
			strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Unprotect partition 0x");
			pTempPointer = BUFFER_UTILS_16BitsToHex(&mDebugParameters.TransmitBuffer[29], Partition);
			strcpy(pTempPointer, " - passing to opcode 39\r");

			// Put partition number in buffer - this is always little endian.
			utils_to2Bytes(&mOpcodeDataBuffer[1], Partition, LITTLE_ENDIAN);
		}
	}
	else
	{
		strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Unprotect error - command not recognised\r");
		bOpcodeUpdateRequired = FALSE;
	}

	// Setup the rest of the loader message structure.
	if (bOpcodeUpdateRequired == TRUE)
	{
		mDebugLoaderMessage.opcode = 39u;
		mOpcodeDataBuffer[0] = OPCODE39_UNPROTECT;
		mDebugLoaderMessage.dataPtr = mOpcodeDataBuffer;
		mDebugLoaderMessage.dataLengthInBytes = 3u;
	}
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ProtectCommandDo deals with the *PROTECT command - sets up the
 * opcode data for opcode39.  Note that the number of data bytes MUST always
 * be 3, otherwise opcode 39 generates an error, so we pad the data.
 *
 */
// ----------------------------------------------------------------------------
static void	ProtectCommandDo(void)
{
	strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Protect - passing to opcode 39\r");
	mDebugLoaderMessage.opcode = 39u;
	mOpcodeDataBuffer[0] = OPCODE39_PROTECT;
	mOpcodeDataBuffer[1] = 0xFFu;
	mOpcodeDataBuffer[2] = 0xFFu;
	mDebugLoaderMessage.dataPtr = mOpcodeDataBuffer;
	mDebugLoaderMessage.dataLengthInBytes = 3u;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ChecksumCommandEqualsDo deals with the *CHECKSUM= command - sets up the
 * opcode data for opcode39.  Note that the number of data bytes MUST always
 * be 3, otherwise opcode 39 generates an error, so we pad the data.
 *
 */
// ----------------------------------------------------------------------------
static void	ChecksumCommandEqualsDo(void)
{
	int16_t		Length;
	bool_t		bConvertedOK;
	uint16_t	Checksum;
	char_t*		pTempPointer;

	if (mDebugParameters.ReceiveBuffer[10] == '\r')
	{
		strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Checksum error - no expected checksum\r");
	}
	else
	{
		// Find end of buffer and then overwrite CR with a NUL, so the
		// string function knows where to stop.
		Length = (int16_t)strlen(mDebugParameters.ReceiveBuffer);
		mDebugParameters.ReceiveBuffer[Length - 1] = '\0';
		bConvertedOK = BUFFER_UTILS_StringToUint16(&mDebugParameters.ReceiveBuffer[10], &Checksum, BUFFER_RADIX_HEX);

		if (bConvertedOK == FALSE)
		{
			strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Checksum error - expected checksum invalid\r");
		}
		else
		{
			strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Checksum 0x");
			pTempPointer = BUFFER_UTILS_16BitsToHex(&mDebugParameters.TransmitBuffer[18], Checksum);
			strcpy(pTempPointer, " - passing to opcode 39\r");

			mDebugLoaderMessage.opcode = 39u;
			mOpcodeDataBuffer[0] = OPCODE39_CHECKSUM;

			// Put checksum in buffer - this is always little endian.
			utils_to2Bytes(&mOpcodeDataBuffer[1], Checksum, LITTLE_ENDIAN);

			mDebugLoaderMessage.dataPtr = mOpcodeDataBuffer;
			mDebugLoaderMessage.dataLengthInBytes = 3u;
		}
	}
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ChecksumCommandQueryDo deals with the *CHECKSUM? command - calculates the
 * checksum of an area of memory.  Note that this doesn't use any functions
 * which aren't visible to both bootloader and promloader.
 *
 */
// ----------------------------------------------------------------------------
static void ChecksumCommandQueryDo(void)
{
	uint16_t	ChecksumAddress[4];
	bool_t		bConvertedOK;
	char_t		TempBuffer[5];
	uint16_t	BufferOffset;
	uint16_t	ChecksumOffset;
	uint32_t	StartAddress;
	uint32_t	EndAddress;
	uint16_t	CalculatedCRC;

	if (mDebugParameters.ReceiveBuffer[10] == '\r')
	{
		strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Checksum query error - no parameters\r");
	}
	else
	{
		BufferOffset = 10u;

		// Convert all four arguments from strings into 16 bit hex values - these
		// end up in in the array ChecksumAddress[].
		for (ChecksumOffset = 0u; ChecksumOffset < 4u; ChecksumOffset++)
		{
			(void)strncpy(TempBuffer, &mDebugParameters.ReceiveBuffer[BufferOffset], (int32_t)4u);
			TempBuffer[4] = '\0';

			bConvertedOK = BUFFER_UTILS_StringToUint16(TempBuffer, &ChecksumAddress[ChecksumOffset], BUFFER_RADIX_HEX);

			if (bConvertedOK == FALSE)
			{
				strcpy(mDebugParameters.TransmitBuffer,	"DEBUG: Checksum query error - invalid parameter\r");
				break;
			}
			else
			{
				BufferOffset += 4u;

				// Check for either commas between the arguments or a CR on the end of the message.
				if (ChecksumOffset < 3u)
				{
					if (mDebugParameters.ReceiveBuffer[BufferOffset] != ',')
					{
						strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Checksum query error - no comma between address words\r");
						bConvertedOK = FALSE;
						break;
					}
				}
				else
				{
					if (mDebugParameters.ReceiveBuffer[BufferOffset] != '\r')
					{
						strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Checksum query error - no CR after parameter list\r");
						bConvertedOK = FALSE;
						break;
					}
				}
				BufferOffset++;
			}
		}

		// If all arguments have been converted successfully then calculate the
		// checksum of the address range specified (assuming start and end
		// addresses are OK).
		if (bConvertedOK == TRUE)
		{
			StartAddress = (uint32_t)ChecksumAddress[0] << 16u;
			StartAddress |= (uint32_t)ChecksumAddress[1];
			EndAddress = (uint32_t)ChecksumAddress[2] << 16u;
			EndAddress |= (uint32_t)ChecksumAddress[3];

			if (StartAddress >= EndAddress)
			{
				strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Checksum query error - start address is after end address\r");
			}
			else
			{
				CalculatedCRC = 0;
				CalculatedCRC = crc_calcRunningCRC(CalculatedCRC,(const Uint16*)StartAddress,
														(EndAddress - StartAddress), WORD_CRC_CALC);
				CalculatedCRC = crc_calcFinalCRC(CalculatedCRC, WORD_CRC_CALC);

				strcpy(mDebugParameters.TransmitBuffer, "DEBUG: Checksum query - checksum for address range 0x");
				(void)BUFFER_UTILS_32BitsToHex(&mDebugParameters.TransmitBuffer[53], StartAddress);
				strcpy(&mDebugParameters.TransmitBuffer[61]," to 0x");
				(void)BUFFER_UTILS_32BitsToHex(&mDebugParameters.TransmitBuffer[67], EndAddress);
				strcpy(&mDebugParameters.TransmitBuffer[75]," is 0x");
				(void)BUFFER_UTILS_16BitsToHex(&mDebugParameters.TransmitBuffer[81], CalculatedCRC);
				mDebugParameters.TransmitBuffer[85] = '\r';
				mDebugParameters.TransmitBuffer[86] = '\0';
			}
		}
	}
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
/**
 * @file    	SRecord.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		7 Mar 2014
 * @brief		Functions for decoding Motorola S-Record files.
 *
 * @note
 * Currently only deals with S3 format, where the data is 16 bits wide and
 * the address is a 32 bit value.
 * All hex numbers in an SREC file are big endian in format - this means that
 * the number 0x0A0B0C0D will be represented as 0A 0B 0C 0D in successive
 * locations in the input string.
 *
 * @warning
 * Only works for a target platform which is little endian - the conversion
 * utilities used assume the target is little endian.  TODO Fix this!!
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
#include "s_record.h"
#include "buffer_utils.h"
#include "utils.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static 	ESRecordDecodeMessages_t LineDecode_Impl
			(const char_t pDataLine[], SRecordDecodeResults_t* pDecodedLine);

uint16_t 	CheckLineIsTheCorrectLength(const char_t pLine[],
											ESRecordDecodeMessages_t* pDecodeMessage);
uint16_t 	ConvertPairsOfDigitsIntoBytes(const char_t pLine[], uint16_t NumberOfBytes,
											uint8_t pConvertedBytes[],
											uint16_t* pRunningChecksum);
void 		ConvertDataSequenceIntoData(uint8_t pDataSequence[],
											SRecordDecodeResults_t* pConvertedData,
											uint16_t NumberOfBytesInDataSequence);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * LineDecode_Impl decodes a single line of SRecord data.
 * This function uses a function pointer so we can mock it out for testing.
 *
 * @param	pDataLine[]					Pointer to line of data to decode.
 * @param   SRecordDecodeResults_t*		Pointer to structure to put results in.
 * @retval	ESRecordDecodeMessages_t		Enumerated type for status of decode.
 *
 */
// ----------------------------------------------------------------------------
static ESRecordDecodeMessages_t LineDecode_Impl
			(const char_t pDataLine[], SRecordDecodeResults_t* pDecodedLine)
{
	ESRecordDecodeMessages_t	DecodeMessage;
	uint16_t				ExpectedNumberOfBytes;
	uint16_t				NumberOfConvertedBytes;
	uint8_t					TempResults[SRECORD_MAX_BYTE_PAIRS];
	uint16_t				DecodedChecksum;
	uint16_t				RunningChecksum;

	if (pDataLine[0] != 'S')
	{
		DecodeMessage = SRECORD_CORRUPTED_LINE_INVALID_START_CODE;
	}
	else
	{
		// Check line length - if any error then DecodeMessage will be updated
		// to hold the relevant error message, and ExpectedNumberOfBytes will
		// be set to zero.
		ExpectedNumberOfBytes = CheckLineIsTheCorrectLength(pDataLine, &DecodeMessage);

		if (ExpectedNumberOfBytes != 0u)
		{
			// Convert pairs of digits into bytes - note the pointer to the
			// data line starts at offset 4 - this is where the first digit is.
			NumberOfConvertedBytes = ConvertPairsOfDigitsIntoBytes
										(&pDataLine[4], ExpectedNumberOfBytes,
											TempResults, &RunningChecksum);

			if (NumberOfConvertedBytes == 0u)
			{
				DecodeMessage = SRECORD_CORRUPTED_LINE_INVALID_BYTE_CHARACTER;
			}
			else
			{
				// Decoded checksum is the last result which was converted.
				DecodedChecksum = TempResults[NumberOfConvertedBytes - 1u];

				// Adjust the running checksum - the checksum should include the
				// expected number of bytes, so add this value. The convert
				// function above sums all bytes into the running checksum,
				// including the checksum byte, so subtract the checksum byte.
				// Then take the ones' complement and convert to 8 bits.
				// This should match the decoded value.
				RunningChecksum += ExpectedNumberOfBytes;
				RunningChecksum -= DecodedChecksum;
				RunningChecksum = ~RunningChecksum;
				RunningChecksum &= 0x00FFu;

				if (RunningChecksum != DecodedChecksum)
				{
					DecodeMessage = SRECORD_CORRUPTED_LINE_INVALID_CHECKSUM;
				}
				else
				{
					// If line is a block header then just ignore it.
					if (pDataLine[1] == '0')
					{
						DecodeMessage = SRECORD_DATA_LINE_DECODE_OK_WAS_BLOCK_HEADER;
					}
					// If line is a data sequence then convert it.
					else if (pDataLine[1] == '3')
					{
						ConvertDataSequenceIntoData(TempResults, pDecodedLine,
														ExpectedNumberOfBytes);
						DecodeMessage = SRECORD_DATA_LINE_DECODED_OK;
					}
					// If line is end of block then just extract address
					// - this is probably the boot address.
					else if (pDataLine[1] == '7')
					{
						pDecodedLine->Address = utils_toUint32(&TempResults[0], BIG_ENDIAN);
						DecodeMessage = SRECORD_DATA_LINE_DECODE_OK_WAS_END_OF_BLOCK;
					}
					// Otherwise line is OK but is something we don't support.
					// (Records S1, S2, S5, S8, S9).
					else
					{
						DecodeMessage = SRECORD_DATA_LINE_DECODE_OK_RECORD_NOT_SUPPORTED;
					}
				}
			}
		}
	}

	return DecodeMessage;
}

/// Defining instance of the global function pointer SRecord_LineDecode.
/// The pointer is initialised to point to LineDecode_Impl.
ESRecordDecodeMessages_t (*SRecord_LineDecode)(const char_t pDataLine[], SRecordDecodeResults_t* pDecodedLine) = &LineDecode_Impl;	//lint !e546


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * CheckLineIsTheCorrectLength ensures the the number of characters in the line
 * matches with what is contained in the byte count field of the SRecord.
 * Note that the byte count indicates the number of bytes in the address, data
 * and checksum fields of the line (so, after the Sxxx, stating at an index of 4).
 *
 * @param	pLine[]						Pointer to line of data.
 * @param	ESRecordDecodeMessages_t*	Pointer to location to write errors into.
 * @retval	uint16_t					Expected number of byts, or zero if error.
 *
 */
// ----------------------------------------------------------------------------
uint16_t CheckLineIsTheCorrectLength(const char_t pLine[],
										ESRecordDecodeMessages_t* pDecodeMessage)
{
	char_t		TempBuffer[3];
	uint16_t	ExpectedNumberOfBytes;
	uint16_t	ActualNumberOfDigits;
	uint16_t	Offset;
	bool_t		bConvertedOK;

	TempBuffer[0] = pLine[2];
	TempBuffer[1] = pLine[3];
	TempBuffer[2] = '\0';
	bConvertedOK = BUFFER_UTILS_StringToUint16(TempBuffer, &ExpectedNumberOfBytes, BUFFER_RADIX_HEX);

	if (bConvertedOK == FALSE)
	{
		ExpectedNumberOfBytes = 0u;
		*pDecodeMessage = SRECORD_CORRUPTED_LINE_INVALID_BYTE_COUNT;
	}
	else
	{
		// Get number of digits after the Sxxx part of the line.
		ActualNumberOfDigits = (uint16_t)strlen(&pLine[4]);

		// Setup offset to point to last character in buffer.
		Offset = ActualNumberOfDigits + 3u;

		// Reduce the number of digits found to cope with CR, LF or CRLF.
		while ( (pLine[Offset] == '\r') || (pLine[Offset] == '\n') )
		{
			ActualNumberOfDigits--;
			Offset--;
		}

		// We're expecting twice as many digits as bytes - each byte consists
		// of a pair of hex digits in ASCII.
		if (ActualNumberOfDigits != (ExpectedNumberOfBytes * 2u) )
		{
			ExpectedNumberOfBytes = 0u;
			*pDecodeMessage = SRECORD_CORRUPTED_LINE_INVALID_LINE_LENGTH;
		}
	}

	return ExpectedNumberOfBytes;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ConvertPairsOfDigitsIntoBytes converts a number of pairs of digits into
 * bytes, and calculates the checksum for all converted bytes in the line.
 * The checksum is simply the arithmetic sum of all the bytes - note that the
 * checksum works on the converted bytes, not the individual hex values.
 *
 * @param	pLine[]				Pointer to line of data containing digits.
 * @param	NumberOfBytes		Number of bytes of data to be converted.
 * @param	pConvertedBytes[]	Pointer to array to put converted bytes in.
 * @param	pRunningChecksum	Pointer to location to put checksum in.
 * @retval	uint16_t			Number of converted bytes, or zero if error.
 *
 */
// ----------------------------------------------------------------------------
uint16_t ConvertPairsOfDigitsIntoBytes(const char_t pLine[], uint16_t NumberOfBytes,
										uint8_t pConvertedBytes[],
										uint16_t* pRunningChecksum)
{
	uint16_t	Offset = 0u;
	uint16_t	ResultCount = 0u;
	uint16_t	RunningChecksum = 0u;
	char_t		TempBuffer[3];
	uint16_t	SingleResult;
	bool_t		bConvertedOK;

	while (NumberOfBytes != 0u)
	{
		// Convert each pair of digits into an 8 bit result
		// (don't worry that we use a 16 bit conversion function - it just returns a 16 bit result
		// but will work with as many digits as you give it - in this case 2).
		TempBuffer[0] = pLine[Offset];
		Offset++;
		TempBuffer[1] = pLine[Offset];
		Offset++;
		TempBuffer[2] = '\0';
		bConvertedOK = BUFFER_UTILS_StringToUint16(TempBuffer, &SingleResult, BUFFER_RADIX_HEX);

		if (bConvertedOK == FALSE)
		{
			ResultCount = 0u;
			break;
		}
		else
		{
			pConvertedBytes[ResultCount] = (uint8_t)SingleResult;
			RunningChecksum += SingleResult;
			ResultCount++;
			NumberOfBytes--;
		}
	}

	// If all digits have been converted OK then return the checksum we've calculated.
	if (ResultCount != 0u)
	{
		*pRunningChecksum = RunningChecksum;
	}

	return ResultCount;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ConvertDataSequenceIntoData converts the bytes of data which have been
 * extracted from an S3 data sequence into a 32 bit address and some 16 bit
 * data words.
 *
 * @param	pDataSequence[]		Pointer to sequence of bytes.
 * @param	pConvertedData		Pointer to structure to put converted data in.
 * @param	NumberOfBytesInDataSequence		Number of bytes in data sequence.
 *
 */
// ----------------------------------------------------------------------------
void ConvertDataSequenceIntoData(uint8_t pDataSequence[],
									SRecordDecodeResults_t* pConvertedData,
									uint16_t NumberOfBytesInDataSequence)
{
	uint16_t 	ExpectedNumberOfDataWords;
	uint16_t	Offset;
	uint16_t	ResultCount;

	// Number of data words is (Bytes - 5) / 2
	// (because the address is 4 bytes and the checksum is 1 byte, and the results are 16 bits).
	ExpectedNumberOfDataWords = (NumberOfBytesInDataSequence - 5u) / 2u;

	// Extract the address from the first 4 bytes of the data sequence,
	// and setup the number of decoded words.
	pConvertedData->Address = utils_toUint32(&pDataSequence[0], BIG_ENDIAN);
	pConvertedData->NumberOfDecodedDataWords = ExpectedNumberOfDataWords;

	// Now extract the data words themselves - the data has been checked already
	// so we know these are valid bytes, so just convert them.
	Offset = 4u;
	ResultCount = 0u;
	while (ExpectedNumberOfDataWords != 0u)
	{
		pConvertedData->Data[ResultCount] = utils_toUint16(&pDataSequence[Offset], BIG_ENDIAN);
		Offset += 2;
		ResultCount++;
		ExpectedNumberOfDataWords--;
	}
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


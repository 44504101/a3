// ----------------------------------------------------------------------------
/**
 * @file    	SRecord.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		7 Mar 2014
 * @brief		Header file for SRecord.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SRECORD_H_
#define SRECORD_H_

#define SRECORD_MAX_DATA_WORDS	15u
#define SRECORD_MAX_BYTE_PAIRS	40u

typedef enum SRecordDecodeMessages
{
	SRECORD_CORRUPTED_LINE_INVALID_START_CODE,
	SRECORD_CORRUPTED_LINE_INVALID_BYTE_COUNT,
	SRECORD_CORRUPTED_LINE_INVALID_LINE_LENGTH,
	SRECORD_CORRUPTED_LINE_INVALID_BYTE_CHARACTER,
	SRECORD_CORRUPTED_LINE_INVALID_CHECKSUM,
	SRECORD_DATA_LINE_DECODED_OK,
	SRECORD_DATA_LINE_DECODE_OK_WAS_BLOCK_HEADER,
	SRECORD_DATA_LINE_DECODE_OK_WAS_END_OF_BLOCK,
	SRECORD_DATA_LINE_DECODE_OK_RECORD_NOT_SUPPORTED
} ESRecordDecodeMessages_t;

typedef struct SRecordDecodeResults
{
	uint32_t	Address;
	uint16_t	Data[SRECORD_MAX_DATA_WORDS];
	uint16_t	NumberOfDecodedDataWords;
} SRecordDecodeResults_t;

extern ESRecordDecodeMessages_t (*SRecord_LineDecode)(const char_t pDataLine[],
											SRecordDecodeResults_t* pDecodedLine);

#endif /* SRECORD_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

/**
 * \file
 * Function prototypes for CRC calculations
 *
 * @author Scott DiPasquale
 * @date 3 June 2005
 *
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2005.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved.
 *
 */

#ifndef DSP_CRC_H
#define DSP_CRC_H

typedef enum CrcCalcMode
{
	BYTE_CRC_CALC,
	WORD_CRC_CALC
} ECrcCalcMode_t;
 
/**
 * Computes a 16-bit CRC on the given data buffer
 */
Uint16 crc_calcRunningCRC(const Uint16 runningCRC,const Uint16* data, Uint32 length, ECrcCalcMode_t crcCalcType);
Uint16 crc_calcFinalCRC(const Uint16 runningCRC, ECrcCalcMode_t crcCalcType);

#endif   // CRC_PROTO_H

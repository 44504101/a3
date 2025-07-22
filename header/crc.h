// ----------------------------------------------------------------------------
/**
 * @file    	common/src/crc.h
 * @author		Fei Li (LIF@xsyu.edu.cn)
 * @date		25 Mar 2013
 * @brief		Header file for crc.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef CRC_H_
#define CRC_H_

bool_t 		CRC_Check(const uint16_t* const pBuffer, const uint32_t LengthInBytes,
				      const uint16_t InitialValue, const uint16_t ExpectedCRC);

uint16_t 	CRC_CCITTCalculate(const uint16_t * const pBuffer,
         	                   uint32_t LengthInBytes,
         	                   const uint16_t InitialValue);

uint16_t 	CRC_CCITTOnByteCalculate(const uint8_t * const pBuffer,
         	                         uint32_t LengthInBytes,
                                     const uint16_t InitialValue);
uint16_t    CheckNum_Calculate(const uint8_t * const pBuffer,
                                     uint32_t LengthInBytes,
                                     const uint16_t InitialValue);

#endif /* CRC_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

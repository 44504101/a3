// ----------------------------------------------------------------------------
/**
 * @file    	prom_memory_access.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		26 Feb 2014
 * @brief		Header file for prom_memory_access.c
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef PROMMEMORYACCESS_H_
#define PROMMEMORYACCESS_H_

bool_t PromMemoryAccess_MemoryWrite(Uint8* pData, Uint16 LengthInBytes, Uint32 StartAddress,
									const TargetDataWidth_t TargetWidth);
bool_t PromMemoryAccess_MemoryRead(Uint8* pData, Uint16 LengthInBytes, Uint32 StartAddress,
									const TargetDataWidth_t TargetWidth);

#endif	/* PROMMEMORYACCESS_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

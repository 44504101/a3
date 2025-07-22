// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/dspflash.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		29 Aug 2013
 * @brief		Header file for dspflash.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef DSPFLASH_H_
#define DSPFLASH_H_

/// Structure for DSP flash wait state initialisation.
typedef struct
{
	uint16_t	FlashPageWait;          ///< Flash page wait.
	uint16_t	FlashRandomWait;        ///< Flash random wait.
	uint16_t	OTPWait;                ///< One time programmable memory wait.
} DSPFlash_t;

void DSPFLASH_Initialise(const DSPFlash_t* const pDSPFlash);
void DSPFLASH_MemCopy(const uint16_t* pSourceAddr, 
                        const uint16_t* const pSourceEndAddr,
						uint16_t* pDestAddr);

#endif /* DSPFLASH_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

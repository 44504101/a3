// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/dspflash.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		29 Aug 2013
 * @brief		Functions to setup internal DSP flash.
 * @details
 * Functions to setup the wait states in the internal DSP flash, and copy data
 * from the flash into ram.
 *
 * @warning
 * The wait state code MUST run from internal RAM, otherwise the behaviour can
 * be undefined.  This function must only be called after the code has been
 * copied into the RAM.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "dspflash.h"
#include "DSP28335_device.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

// If using the GCC compiler then disable warning for unknown pragmas - the GCC
// compiler doesn't recognise CODE_SECTION.  Note that this warning is only
// disabled in this file by this instruction.
#ifdef __WIN32__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * DSPFLASH_Initialise sets up the wait states for the internal flash memory.
 * The formulae for wait state calculations is given in SPRS581, section 6.17.
 *
 * @param	pDSPFlash	Pointer to structure containing required wait states.
 *
 */
// ----------------------------------------------------------------------------
#pragma CODE_SECTION(DSPFLASH_Initialise, "ramfuncs")
void DSPFLASH_Initialise(const DSPFlash_t* const pDSPFlash)
{
	// Flash registers are EALLOW protected, so unprotect before writing.
	EALLOW;

	// Enable Flash Pipeline mode to improve performance of code run from flash.
	FlashRegs.FOPT.bit.ENPIPE = 1u;

	// Set the Paged Wait state for the flash.
	//lint -e{9034} Expression assigned to narrower type - use of bitfields.
	FlashRegs.FBANKWAIT.bit.PAGEWAIT = pDSPFlash->FlashPageWait;

	// Set the Random Wait state for the flash.
    //lint -e{9034} Expression assigned to narrower type - use of bitfields.
	FlashRegs.FBANKWAIT.bit.RANDWAIT = pDSPFlash->FlashRandomWait;

	// Set the Wait state for the OTP.
    //lint -e{9034} Expression assigned to narrower type - use of bitfields.
	FlashRegs.FOTPWAIT.bit.OTPWAIT = pDSPFlash->OTPWait;

	// *** CAUTION ***
	// ONLY THE DEFAULT VALUE FOR THESE 2 REGISTERS SHOULD BE USED,
	// (as per the instructions in SPRUFB0D, section 3).
	FlashRegs.FSTDBYWAIT.bit.STDBYWAIT = 0x01FFu;
	FlashRegs.FACTIVEWAIT.bit.ACTIVEWAIT = 0x01FFu;

	EDIS;

	// Force a pipeline flush to ensure that the write to
	// the last register configured occurs before returning.
#ifndef UNIT_TEST_BUILD
	asm(" RPT #7 || NOP");			//lint !e586 Asm keyword is deprecated
#endif
}


// ----------------------------------------------------------------------------
/**
 * DSPFLASH_MemCopy copies a block of data from one block to another.
 * This is in a slightly different form from the standard memcpy, and is used
 * to copy data from the flash memory into the ram.
 *
 * @warning
 * This code does NOT check for invalid memory locations, or that the
 * destination is large enough for the source data.
 *
 * @param	pSourceAddr		Pointer to start of source data.
 * @param	pSourceEndAddr	Pointer to end of source data.
 * @param	pDestAddr		Pointer to start of destination data.
 *
 */
// ----------------------------------------------------------------------------
void DSPFLASH_MemCopy(const uint16_t* pSourceAddr, 
						const uint16_t* const pSourceEndAddr,
						uint16_t* pDestAddr)
{
    while (pSourceAddr < pSourceEndAddr)	//lint !e946 Pointer arithmetic
    {
       *pDestAddr = *pSourceAddr;
       pDestAddr++;							//lint !e960
       pSourceAddr++;						//lint !e960
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

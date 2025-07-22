// ----------------------------------------------------------------------------
/**
 * @file    	tool_specific_programming.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		18 Mar 2014
 * @brief		Header file for ToolSpecificHardware.c abstraction layer.
 *
 * @note
 * These are the function prototypes for all tool specific programming
 * functions - anything which programs \ erases etc the program memory
 * should be controlled via this hardware abstraction layer.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef TOOLSPECIFICPROGRAMMING_H_
#define TOOLSPECIFICPROGRAMMING_H_

/*
 * Flash API Status Structure
 *
 * This structure is used to pass debug data back to the calling routine.
 * Not all fields are populated by all functions - application dependent.
 * The only 'must' is that the FlashStatusCode must be zero if the operation
 * is successful.
 */
typedef struct FlashAPIStatus
{
    uint32_t  FirstFailAddr;
    uint16_t  ExpectedData;
    uint16_t  ActualData;
    uint16_t  FlashStatusCode;
}FlashStatus_t;


// ----------------------------------------------------------------------------
/**
 * ToolSpecificProgramming_SafeFlashErase is used to erase the particular
 * flash sectors - the 'Safe' refers to the fact that this code should not be
 * able to erase certain sectors, to be determined by the user in their code.
 *
 * @warning
 * For compatibility with the promloader application, any user erase function
 * MUST wait in the erase code until erase is complete (or until a failure is
 * detected).
 *
 * @param	SectorMask		Mask for sectors to erase.
 * @param	pFEraseStatus	Pointer to flash status for any error codes.
 * @retval	bool_t			TRUE if erase succeeded, FALSE if erase failed.
 */
bool_t	ToolSpecificProgramming_SafeFlashErase(uint16_t SectorMask, FlashStatus_t* pFlashEraseStatus);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificProgramming_SafeFlashProgram is used to program the flash memory.
 * The 'Safe' refers to the fact that this code should not be able to
 * program certain sectors, to be determined by the user in their code.
 *
 * @warning
 * For compatibility with the promloader application, any user programming
 * function MUST wait in the programming code until programming is complete
 * (or until a failure is detected).
 *
 * @param	pFlashAddress	Pointer to start location in flash to program.
 * @param	pBufferAddress	Pointer to start location to read data from.
 * @param	Length			Number of memory locations to program.
 * @param	pFProgrammingStatus		Pointer to flash status for any error codes.
 * @retval	bool_t			TRUE if programming succeeded, FALSE if failed.
 */
bool_t	ToolSpecificProgramming_SafeFlashProgram(void* pFlashAddress,
														void* pBufferAddress,
														uint32_t Length,
														FlashStatus_t* pFlashProgrammingStatus);

// ----------------------------------------------------------------------------
/**
 * ToolSpecificProgramming_FlashBlankCheck is used to check whether a particular
 * sector (or number of sectors, depending on the implementation) is\are blank.
 *
 * @retval	uint16_t    Sector mask of sectors which are NOT blank.
 */
uint16_t ToolSpecificProgramming_FlashBlankCheck(uint16_t SectorMask);


#endif /* TOOLSPECIFICPROGRAMMING_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

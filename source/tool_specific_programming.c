// ----------------------------------------------------------------------------
/**
 * @file    	ToolSpecificProgramming.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		24 Mar 2014
 * @brief		Tool specific programming functions for Xceed ACQ \ MTC promloader.
 * @details
 * These are the functions for all tool specific programming functions - anything
 * which programs \ erases etc the program memory should be controlled via this
 * hardware abstraction layer.
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
#include "tool_specific_programming.h"
#include "tool_specific_config.h"
#include "tool_specific_hardware.h"
#include "Flash2833x_API_Library.h"
#include "testpoints.h"
#include "testpointoffsets.h"
#include "genericIO.h"
#include "DSP28335_device.h"

// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

#define MAX_SECTORS 8u


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static void FlashEraseCallBackFunction(void);
static void FlashProgrammingCallBackFunction(void);
static void GenerateDebugMessageForSectorErase(uint16_t SectorMask);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module

typedef struct FlashSector
{
    char_t      SectorIdentifier;
    uint16_t    SectorMask;
    uint32_t    StartAddress;
    uint32_t    EndAddress;
} FlashSector_t;

const FlashSector_t mFlashSectorDetails[MAX_SECTORS] =
{
 { 'A', SECTORA, (uint32_t)0x00338000, (uint32_t)0x00340000},
 { 'B', SECTORB, (uint32_t)0x00330000, (uint32_t)0x00338000},
 { 'C', SECTORC, (uint32_t)0x00328000, (uint32_t)0x00330000},
 { 'D', SECTORD, (uint32_t)0x00320000, (uint32_t)0x00328000},
 { 'E', SECTORE, (uint32_t)0x00318000, (uint32_t)0x00320000},
 { 'F', SECTORF, (uint32_t)0x00310000, (uint32_t)0x00318000},
 { 'G', SECTORG, (uint32_t)0x00308000, (uint32_t)0x00310000},
 { 'H', SECTORH, (uint32_t)0x00300000, (uint32_t)0x00308000}
};

static char_t   mTempBufferForErase[32];


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool_t ToolSpecificProgramming_SafeFlashErase(uint16_t SectorMask,
											  FlashStatus_t* pFlashEraseStatus)
{
	FLASH_ST	FlashStatus;
	uint16_t	EraseAPIStatus;
	uint16_t    ValidatedSectorMask;
	bool_t      bEraseSucceeded = TRUE;

	// Set the testpoint to show that erase has started.
	TESTPOINTS_Set(TP_OFFSET_FLASH_ERASE);

    // Check to make sure sectors need to be erased.
	ToolSpecificHardware_DebugMessageSend("Checking erase status for sector(s) ");
	GenerateDebugMessageForSectorErase(SectorMask);
	ValidatedSectorMask = ToolSpecificProgramming_FlashBlankCheck(SectorMask);

	// Erase appropriate sector(s) using TI flash erase library function.
	// Note that the code will wait in here until erase has completed.
    ToolSpecificHardware_DebugMessageSend("Erasing sector(s) ");
    GenerateDebugMessageForSectorErase(ValidatedSectorMask);

    // Only call the erase function if there's actually something to erase.
    if (ValidatedSectorMask != 0u)
    {
        //Flash_CallbackPtr = FlashEraseCallBackFunction;
        DINT;
        EraseAPIStatus = Flash_Erase(ValidatedSectorMask, &FlashStatus);
        EINT;

        // Copy status from TI structure into Thor flash status structure.
        pFlashEraseStatus->ActualData = FlashStatus.ActualData;
        pFlashEraseStatus->ExpectedData = FlashStatus.ExpectedData;
        pFlashEraseStatus->FirstFailAddr = FlashStatus.FirstFailAddr;

        // Add return value from TI erase function into Thor flash status structure.
        pFlashEraseStatus->FlashStatusCode = EraseAPIStatus;

        // The TI erase function returns zero if erase successful.
        if (EraseAPIStatus == 0)
        {
            ToolSpecificHardware_DebugMessageSend("Erased OK\r");
        }
        else
        {
            ToolSpecificHardware_DebugMessageSend("Erase FAILED\r");
            bEraseSucceeded = FALSE;
        }
    }

    // Clear testpoint just in case it was left in active state by callback
    // (or, if we haven't called the erase function, it will be set).
    TESTPOINTS_Clear(TP_OFFSET_FLASH_ERASE);

	return bEraseSucceeded;
}


bool_t ToolSpecificProgramming_SafeFlashProgram(void* pFlashAddress,
												void* pBufferAddress,
												uint32_t Length,
												FlashStatus_t* pFlashProgrammingStatus)
{
	FLASH_ST	FlashStatus;
	uint16_t	ProgramAPIStatus;

	// Set the testpoint to show that programming has started.
	TESTPOINTS_Set(TP_OFFSET_FLASH_PROGRAM);

	// Program appropriate sector(s) using TI flash program library function.
	// Note that the code will wait in here until programming has completed.
	//Flash_CallbackPtr = FlashProgrammingCallBackFunction;
	DINT;
	ProgramAPIStatus = Flash_Program( (Uint16*)pFlashAddress, (Uint16*)pBufferAddress, Length, &FlashStatus);
    EINT;
	// Copy status from TI structure into Thor flash status structure.
	pFlashProgrammingStatus->ActualData = FlashStatus.ActualData;
	pFlashProgrammingStatus->ExpectedData = FlashStatus.ExpectedData;
	pFlashProgrammingStatus->FirstFailAddr = FlashStatus.FirstFailAddr;

	// Add return value from TI erase function into Thor flash status structure.
	pFlashProgrammingStatus->FlashStatusCode = ProgramAPIStatus;

	// Clear testpoint just in case it was left in active state by callback.
	TESTPOINTS_Clear(TP_OFFSET_FLASH_PROGRAM);

	// The TI erase function returns zero if erase successful.
	if (ProgramAPIStatus == 0)
	{
		return TRUE;
	}
	else
	{
	    return FALSE;
	}
}


uint16_t ToolSpecificProgramming_FlashBlankCheck(uint16_t SectorMask)
{
    uint16_t    CurrentSectorMask;
    uint32_t    AddressCounter;
    bool_t      bSectorIsBlank;
    uint16_t    ReadResult;
    uint16_t    ValidatedSectorMask = SectorMask;
    uint16_t    SectorCounter;

    // Check each sector in turn.
    for (SectorCounter = 0u; SectorCounter < MAX_SECTORS; SectorCounter++)
    {
        // Get a mask for the particular sector we want to examine.
        CurrentSectorMask = SectorMask & mFlashSectorDetails[SectorCounter].SectorMask;

        // If the mask is non-zero then we want to check this sector
        // (so we don't check sectors we're not interested in).
        if (CurrentSectorMask != 0u)
        {
            bSectorIsBlank = TRUE;

            // Read every location in the sector - as soon as a location
            // is found which is not blank, jump out of the loop.
            for (AddressCounter = mFlashSectorDetails[SectorCounter].StartAddress;
                    AddressCounter < mFlashSectorDetails[SectorCounter].EndAddress;
                    AddressCounter++)
            {
                ReadResult = genericIO_16bitRead(AddressCounter);

                if (ReadResult != 0xFFFFu)
                {
                    bSectorIsBlank = FALSE;
                    break;
                }
            }

            // If the sector is blank then clear the relevant bit in the mask,
            // so the DSP won't try and erase this sector (just wastes time).
            if (bSectorIsBlank == TRUE)
            {
                ValidatedSectorMask &= (uint16_t)~CurrentSectorMask;
            }
        }
    }

	return ValidatedSectorMask;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void GenerateDebugMessageForSectorErase(uint16_t SectorMask)
{
    uint16_t    BufferOffset = 0u;
    uint16_t    SectorCounter;

    // Check each sector in turn.
    for (SectorCounter = 0u; SectorCounter < MAX_SECTORS; SectorCounter++)
    {
        // If sector is required then add sector identifier (A-H), a comma
        // and a space to the temporary buffer.
        if ( (SectorMask & mFlashSectorDetails[SectorCounter].SectorMask) != 0u)
        {
            mTempBufferForErase[BufferOffset] = mFlashSectorDetails[SectorCounter].SectorIdentifier;
            BufferOffset++;
            mTempBufferForErase[BufferOffset] = ',';
            BufferOffset++;
            mTempBufferForErase[BufferOffset] = ' ';
            BufferOffset++;
        }
    }

    // If the buffer offset is non-zero it means there's something in there,
    // so overwrite the last comma and space with '...\r'.
    if (BufferOffset != 0)
    {
        BufferOffset -= 2u;
        strcpy(&mTempBufferForErase[BufferOffset], "...\r");
    }
    // Otherwise nothing was required at all so just put 'NONE\r' in the buffer.
    else
    {
        strcpy(mTempBufferForErase, "- NONE\r");
    }

    ToolSpecificHardware_DebugMessageSend(mTempBufferForErase);
}


// ----------------------------------------------------------------------------
// Callback function for flash erase - this is called during the erase function,
// and we can use this to check that the device is still alive.
static void FlashEraseCallBackFunction(void)
{
	TESTPOINTS_Toggle(TP_OFFSET_FLASH_ERASE);
}

// Callback function for flash programming - this is called during the program
// function, and we can use this to check that the device is still alive.
static void FlashProgrammingCallBackFunction(void)
{
	TESTPOINTS_Toggle(TP_OFFSET_FLASH_PROGRAM);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

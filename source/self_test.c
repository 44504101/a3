// ----------------------------------------------------------------------------
/**
 * @file    	SelfTest.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		24 Feb 2014
 * @brief		Self test code for SDRM bootloader.
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "common_data_types.h"
#include "self_test.h"
#include "tool_specific_config.h"
#include "timer.h"
#include "tool_specific_hardware.h"
#include "dsp_crc.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static Uint16 CalculateBootloaderCRC(void);
static Uint16 CalculateApplicationCRC(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module

static SelfTestResult_t mSelfTestResult;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * SelfTest_TestExecute tests the SCI port, bootloader CRC and application CRC,
 * and populates the structure mSelfTest result accordingly.
 *
 */
// ----------------------------------------------------------------------------
void SelfTest_TestExecute(void)
{
    mSelfTestResult.SSBPort_Status = 255; //Untested
    mSelfTestResult.ISBPort_Status = 255; //Untested
#ifdef COMM_SSB
    mSelfTestResult.SSBPort_Status = ToolSpecificHardware_SSBPortSelfTest();
#endif

#ifdef COMM_ISB
    mSelfTestResult.ISBPort_Status = ToolSpecificHardware_ISBPortSelfTest();
#endif

    mSelfTestResult.ActualBootloaderCRC = CalculateBootloaderCRC();
    mSelfTestResult.ActualApplicationCRC = CalculateApplicationCRC();
    mSelfTestResult.ExpectedBootloaderCRC = *((Uint16*)BOOTLOADER_CRC_ADDRESS);
    mSelfTestResult.ExpectedApplicationCRC = *((Uint16*)APPLICATION_CRC_ADDRESS);

    if (mSelfTestResult.ActualBootloaderCRC == mSelfTestResult.ExpectedBootloaderCRC)
    {
    	mSelfTestResult.bBootloaderCRCIsOK = TRUE;
    }
    else
    {
    	mSelfTestResult.bBootloaderCRCIsOK = FALSE;
    }

    if (mSelfTestResult.ActualApplicationCRC == mSelfTestResult.ExpectedApplicationCRC)
    {
    	mSelfTestResult.bApplicationCRCIsOK = TRUE;
    }
    else
    {
    	mSelfTestResult.bApplicationCRCIsOK = FALSE;
    }

#ifdef _WEI_DEBUG
    //Print out crcs

     SCIB_Send_Msg("Boot CRC: ");
     Num2HexStr(mSelfTestResult.ActualBootloaderCRC,Str);
     SCIB_Send_Msg(Str);
     SCIB_Send_Msg("\n\r");
     if(SelfTestResult.BootCRCStatus){
     SCIB_Send_Msg("Passed\n\r");
     }else{
     SCIB_Send_Msg("Failed\n\r");
     }
     SCIB_Send_Msg("App CRC:  ");
     Num2HexStr(SelfTestResult.ActualApplicationCRC,Str);
     SCIB_Send_Msg(Str);
     SCIB_Send_Msg("\n\r");
     if(SelfTestResult.AppCRCStatus){
     SCIB_Send_Msg("Passed\n\r");
     }else{
     SCIB_Send_Msg("Failed\n\r");
     }

#endif
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SelfTest_isBootloaderImageValid returns whether the bootloader CRC is valid
 * or not.
 *
 * @retval	bool_t	TRUE if bootloader image valid, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
bool_t SelfTest_isBootloaderImageValid(void)
{
    return mSelfTestResult.bBootloaderCRCIsOK;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SelfTest_isApplicationImageValid returns whether the application CRC is valid
 * or not.
 *
 * @retval	bool_t	TRUE if application image valid, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
bool_t SelfTest_isApplicationImageValid(void)
{
    return mSelfTestResult.bApplicationCRCIsOK;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SelfTest_ResultPointerGet returns a pointer to the self test result structure,
 * to allow other module to access the results.
 *
 * @retval	SelfTestResult_t*	Pointer to self test result structure.
 *
 */
// ----------------------------------------------------------------------------
const SelfTestResult_t* SelfTest_ResultPointerGet(void)
{
	return &mSelfTestResult;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * CalculateBootloaderCRC calculates the CRC of the bootloader area of memory.
 *
 * @retval	Uint16		Bootloader CRC.
 *
 */
// ----------------------------------------------------------------------------
static Uint16 CalculateBootloaderCRC(void)
{
    Uint16 CRC;

    CRC = crc_calcRunningCRC(0, (Uint16*)BOOTLOADER_START_ADDRESS, BOOTLOADER_LENGTH, WORD_CRC_CALC);
    CRC = crc_calcFinalCRC(CRC, WORD_CRC_CALC);
    return CRC;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * CalculateApplicationCRC calculates the CRC of the application area of memory.
 *
 * @retval	Uint16		Application CRC.
 *
 */
// ----------------------------------------------------------------------------
static Uint16 CalculateApplicationCRC(void)
{
    Uint16 CRC;

    CRC = crc_calcRunningCRC(0, (Uint16*)APPLICATION_START_ADDRESS, APPLICATION_LENGTH, WORD_CRC_CALC);
    CRC = crc_calcFinalCRC(CRC, WORD_CRC_CALC);
    return CRC;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

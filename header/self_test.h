// ----------------------------------------------------------------------------
/**
 * @file    	SelfTest.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		23 Jan 2014
 * @brief		Header file for SelfTest.c
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SELFTEST_H_
#define SELFTEST_H_

typedef struct SelfTestResult
{
    //Result of SCI test
    Uint16 SSBPort_Status;
    Uint16 ISBPort_Status;

    //Result of Boot CRC
    Uint16 	ActualBootloaderCRC;
    Uint16 	ExpectedBootloaderCRC;
    bool_t	bBootloaderCRCIsOK;

    //Result of Application CRC
    Uint16 	ActualApplicationCRC;
    Uint16 	ExpectedApplicationCRC;
    bool_t 	bApplicationCRCIsOK;
} SelfTestResult_t;

void 					SelfTest_TestExecute(void);
bool_t					SelfTest_isBootloaderImageValid(void);
bool_t					SelfTest_isApplicationImageValid(void);
const SelfTestResult_t*	SelfTest_ResultPointerGet(void);

#endif /* SELFTEST_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

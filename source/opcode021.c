/**
 * \file
 * Implementation of opcode 2 / 201
 * 
 * @author Scott DiPasquale
 * @date 1 September 2004
 * 
 * (c) Copyright Xi'an Shiyou Univ. Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Xi'an Shiyou Univ.
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */

#include "common_data_types.h"
#include "loader_state.h"
#include "timer.h"
#include "opcode021.h"
#include "comm.h"
#include "tool_specific_config.h"
#include "self_test.h"

void opcode21_execute(ELoaderState_t* loaderState, Timer_t* timer)
{   
	unsigned char 			msg[SELF_TEST_LENGTH];
	const SelfTestResult_t*	pSelfTestResult;
	unsigned char           iPortStatus;

	pSelfTestResult = SelfTest_ResultPointerGet();

    //Composer the self-test result message
	msg[0]=(unsigned char)pSelfTestResult->bBootloaderCRCIsOK;
	utils_to2Bytes(&(msg[1]),pSelfTestResult->ActualBootloaderCRC,LITTLE_ENDIAN);
	msg[3]=(unsigned char)pSelfTestResult->bApplicationCRCIsOK;
	utils_to2Bytes(&(msg[4]),pSelfTestResult->ActualApplicationCRC,LITTLE_ENDIAN);

	// if one of the serial ports fails, this way we will get it reported back
	iPortStatus = (unsigned char)( pSelfTestResult->SSBPort_Status & pSelfTestResult->ISBPort_Status );

	// if any port is not supported, overwrite
#if ( ( defined( COMM_SSB )) & (!defined(COMM_ISB)) )
	iPortStatus = (unsigned char)pSelfTestResult->SSBPort_Status;

#endif

#if ( ( defined( COMM_ISB )) & (!defined(COMM_SSB)) )
	iPortStatus = (unsigned char)pSelfTestResult->ISBPort_Status;
#endif

	msg[6]= iPortStatus;

    // Send the message
    loader_MessageSend( LOADER_OK, SELF_TEST_LENGTH, (char *) msg );
    
    if( *loaderState != LOADER_WAITING )
    {
        Timer_TimerReset( timer );
    }
}


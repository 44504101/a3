// ----------------------------------------------------------------------------
/**
 * @file    	debug.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		5 Mar 2014
 * @brief		Header file for debug.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef DEBUG_H_
#define DEBUG_H_
#include "comm.h"

#define MAX_DEBUG_BUFFER_TX_SIZE	1024u
#define MAX_DEBUG_BUFFER_RX_SIZE	128u

typedef struct DebugParameters
{
	uint16_t		HaltMessageState;
	char_t    		TransmitBuffer[MAX_DEBUG_BUFFER_TX_SIZE];
	char_t			ReceiveBuffer[MAX_DEBUG_BUFFER_RX_SIZE];
	uint16_t		ReceiveOffset;
	bool_t			bMotorolaDownloadModeEnabled;
	bool_t			bUploadModeEnabled;
	uint32_t		UploadAddress;
} DebugParameters_t;

void 						Debug_Initialise(void);
bool_t						Debug_HaltMessageCheck(void);
LoaderMessage_t* 			Debug_LoaderMessagePointerGet(void);
void 						Debug_MessageSend(Uint8 Status, Uint16 LengthInBytes, char_t* pData);
void 						Debug_LoaderMessageSend(uint8_t Opcode, uint8_t Status, uint16_t LengthInBytes, char_t* pData);
EMessageStatus_t			Debug_MessageCheck(void);

#ifdef UNIT_TEST_BUILD
const DebugParameters_t*	Debug_ParameterPointerGet_TDD(void);
#endif

#endif /* Debug_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


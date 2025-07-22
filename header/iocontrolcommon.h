// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/iocontrolcommon.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		07 May 2013
 * @brief		Header file for iocontrolcommon.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef IOCONTROLCOMMON_H_
#define IOCONTROLCOMMON_H_

/// Enumerated values for DSP ID.
typedef enum
{
	DSPID_DSPA,         ///< DSP A.
	DSPID_DSPB          ///< DSP B.
} EDSPID_t;

// Function prototypes for functions which are used by both DSP's.
void 		IOCONTROLCOMMON_RS485ReceiverEnable(void);
void 		IOCONTROLCOMMON_RS485ReceiverDisable(void);
void 		IOCONTROLCOMMON_RS485TransmitterEnable(void);
void 		IOCONTROLCOMMON_RS485TransmitterDisable(void);
void 		IOCONTROLCOMMON_CANLoopbackEnable(void);
void 		IOCONTROLCOMMON_CANLoopbackDisable(void);
EDSPID_t	IOCONTROLCOMMON_DSPIdentifierGet(void);

#endif /* IOCONTROL_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
/**
 * @file    	testpointoffsets.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Mar 2013
 * @brief		Test point offsets for promloader code for Xceed DSP A and DSP B.
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef TESTPOINTOFFSETS_H_
#define TESTPOINTOFFSETS_H_

// Maximum number of testpoints - there should always be the same number of
// #define entries below as this quantity.
#define MAXIMUM_NUMBER_OF_TESTPOINTS	16u

// Definition for each testpoint offset.
// The naming convention we've adopted is TP_OFFSET_MODULENAME_DESCRIPTION
#define	TP_OFFSET_FLASH_ERASE			0u
#define	TP_OFFSET_FLASH_PROGRAM			1u
#define	TP_OFFSET_2						2u
#define TP_OFFSET_3						3u
#define	TP_OFFSET_4						4u
#define	TP_OFFSET_5						5u
#define	TP_OFFSET_6						6u
#define	TP_OFFSET_7						7u
#define	TP_OFFSET_8						8u
#define	TP_OFFSET_9						9u
#define	TP_OFFSET_10					10u
#define	TP_OFFSET_11                    11u
#define	TP_OFFSET_12     				12u
#define	TP_OFFSET_SCI_RXINTA			13u
#define	TP_OFFSET_SCI_TXINTA			14u
#define	TP_OFFSET_MAIN_LED				15u

#endif /* TESTPOINTOFFSETS_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


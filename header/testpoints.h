// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/testpoints.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Jun 2013
 * @brief		Header file for testpoints.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef TESTPOINTS_H_
#define TESTPOINTS_H_

/// Structure to hold all variables required for testpoint module.
typedef struct
{
	volatile uint32_t*	pSetRegister;       ///< Pointer to GPIO register to set bit.
	volatile uint32_t*	pClearRegister;     ///< Pointer to GPIO register to clear bit.
	volatile uint32_t*	pToggleRegister;    ///< Pointer to GPIO register to toggle bit.
	uint32_t			BitMask;            ///< Bit mask to apply to register.
	uint16_t			GpioBitNumber;      ///< Bit number in GPIO register.
} Testpoints_t;

bool_t 				TESTPOINTS_Initialise(const uint16_t Offset,
											const uint16_t GpioNumber);
bool_t 				TESTPOINTS_ArrayReset(const uint16_t Offset);
uint16_t 			TESTPOINTS_ArrayQuery(const uint16_t Offset);
void 				TESTPOINTS_Set(const uint16_t Offset);
void 				TESTPOINTS_Clear(const uint16_t Offset);
void 				TESTPOINTS_Toggle(const uint16_t Offset);
const Testpoints_t* TESTPOINTS_ArrayPointerGet(const uint16_t Offset);

#endif /* TESTPOINTS_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

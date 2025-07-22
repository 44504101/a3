// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/watchdog.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		5 Jul 2012
 * @brief		Header file for watchdog.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef WATCHDOG_H_
#define WATCHDOG_H_

/// Enumerated values for the available watchdog prescalers.
typedef enum
{
	WATCHDOG_NO_PRESCALE	=	0,      ///< No prescaler.
	WATCHDOG_PRESCALE_1		=	1,      ///< Prescaler of 1.
	WATCHDOG_PRESCALE_2		=	2,      ///< Prescaler of 2.
	WATCHDOG_PRESCALE_4		=	3,      ///< Prescaler of 4.
	WATCHDOG_PRESCALE_8		=	4,      ///< Prescaler of 8.
	WATCHDOG_PRESCALE_16	=	5,      ///< Prescaler of 16.
	WATCHDOG_PRESCALE_32	=	6,      ///< Prescaler of 32.
	WATCHDOG_PRESCALE_64	=	7       ///< Prescaler of 64.
} EWatchdogPrescalers_t;

bool_t 	WATCHDOG_Disable(void);
bool_t 	WATCHDOG_Enable(const EWatchdogPrescalers_t RequiredPrescaler);
void	WATCHDOG_LockWDOverrideBit(void);
void	WATCHDOG_ForceSoftwareReset(void);
void	WATCHDOG_KickDog(void);
bool_t  WATCHDOG_IsEnabledCheck(void);
bool_t  WATCHDOG_LastResetWasWatchdog(void);

#endif /* WATCHDOG_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

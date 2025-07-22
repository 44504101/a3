// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/frame.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		25 Jul 2012
 * @brief		Header file for frame.c.
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef FRAME_H_
#define FRAME_H_

/*
 * If running unit tests then define away the interrupt keyword, so functions
 * are treated as 'straight' functions, rather than special interrupts.
 *
 * This produces a Lint warning for 'macro defined with the same name as a C
 * keyword', which is exactly what we want to do, so we disable the warning.
 */
#ifdef UNIT_TEST_BUILD
#define interrupt               //lint !e9051 Re-use of C keyword is deliberate.
#endif

interrupt void 	FRAME_SynchronisingTick_ISR(void);
bool_t 			FRAME_SynchronisingStateGet(void);
void 			FRAME_SynchronisingStateClear(void);
uint16_t 		FRAME_FrameTimerPrescalerGet(void);
bool_t 			FRAME_FrameTimerPrescalerSet(const uint16_t RequiredPrescaler);
void            FRAME_CoreTimerReset(void);
uint32_t        FRAME_CoreTimerGet(void);
bool_t          FRAME_PwmNumberSet(const uint16_t PwmNumber);
uint16_t        FRAME_CurrentTickPeriodGet(void);
uint16_t        FRAME_CurrentTickTimeGet(void);

#ifdef UNIT_TEST_BUILD
void            FRAME_SynchronisingStateSet_TDD(void);
void            FRAME_CoreTimerSet_TDD(const uint32_t NewValue);
#endif

#endif /* FRAME_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/frame.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		25 Jul 2012
 * @brief		Framing code for DSP's for Xceed ACQ \ MTC code.
 * @details
 * Sets a flag every time we get an EPWMx interrupt (nominally every 1mS) - this
 * provides a synchronising 'tick' off which all other code is run (a poor man's
 * RTOS).
 *
 * It is assumed that the PWM and associated interrupts have been setup in
 * other code.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "DSP28335_device.h"
#include "frame.h"
#include "testpoints.h"
#include "testpointoffsets.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/// Synchronising flag - set by the EPWMx interrupt.
static bool_t volatile 	            mbSynchronisedFlagSet = FALSE;

/// Prescaler for frame timer - set flag after this number of interrupts.
//lint -e{956} Doesn't need to be volatile.
static uint16_t			            mFrameTimerPrescaler = 1u;

/// Free running core timer - just keeps on counting.
static uint32_t volatile            mCoreTimer = 0u;

/// Pointer to PWM registers - defaults to using EPWM1.
//lint -e{956} Doesn't need to be volatile.
static volatile struct EPWM_REGS*   m_p_EPwmRegs = &EPwm1Regs;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * FRAME_SynchronisingTick_ISR is the interrupt service routine for EPWMx.
 * This sets the flag mbSynchronisedFlagSet, so the rest of the code can synch
 * up to it.
 *
 */
// ----------------------------------------------------------------------------
interrupt void FRAME_SynchronisingTick_ISR(void)
{
    //lint -e{956} Doesn't need to be volatile.
	static uint16_t RunningFrameTimer = 0u;

    // Allow higher priority interrupts to be serviced from within this ISR.
    EINT;

    // Acknowledge the interrupt to allow other interrupts from this group to
    // be serviced - all EPWM interrupts are mapped into PIE group 3, so we
    // do this no matter which PWM we are using.
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

    // Increment core timer.
    mCoreTimer++;

	// Increment counter for running timer - if this now matches (or exceeds)
	// the prescaler then we can set the synchronising flag and clear the
	// running timer for next time.
	RunningFrameTimer++;
	if (RunningFrameTimer >= mFrameTimerPrescaler)
	{
		TESTPOINTS_Toggle(TP_OFFSET_MAIN_LED);		// Toggle the main LED.
		mbSynchronisedFlagSet = TRUE;
		RunningFrameTimer = 0u;
	}

	// Set bit in ETCLR register to enable further interrupts to be generated.
	m_p_EPwmRegs->ETCLR.bit.INT = 1u;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_SynchronisingStateGet returns the status of the synchronising flag.
 * This flag is set in the interrupt routine above, triggered by the EPWM1
 * timer overflowing.
 *
 * @retval	bool_t	Boolean value of mbSynchronisedFlagSet
 *
 */
// ----------------------------------------------------------------------------
bool_t FRAME_SynchronisingStateGet(void)
{
	return mbSynchronisedFlagSet;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_SynchronisingStateClear sets the status of the synchronising flag to
 * FALSE.  This is done from the main loop, once all of the processing for a
 * frame has been carried out (so we can then wait and synch up all over again).
 *
 */
// ----------------------------------------------------------------------------
void FRAME_SynchronisingStateClear(void)
{
	mbSynchronisedFlagSet = FALSE;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_FrameTimerPrescalerGet returns the value of the frame timer prescaler.
 *
 * @retval	uint16_t	Value of frame timer prescaler.
 *
 */
// ----------------------------------------------------------------------------
uint16_t FRAME_FrameTimerPrescalerGet(void)
{
	return mFrameTimerPrescaler;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_FrameTimerPrescalerSet sets the value of the frame timer prescaler.
 * Note that the function will always return TRUE - we need to have the same
 * prototype as all the other set functions.
 *
 * @param	RequiredPrescaler	Required value of prescaler.
 * @retval	bool_t				Always returns TRUE.
 *
 */
// ----------------------------------------------------------------------------
bool_t FRAME_FrameTimerPrescalerSet(const uint16_t RequiredPrescaler)
{
	mFrameTimerPrescaler = RequiredPrescaler;
	return TRUE;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_CoreTimerReset resets (zeroes) the core timer.
 *
 */
// ----------------------------------------------------------------------------
void FRAME_CoreTimerReset(void)
{
    mCoreTimer = 0u;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_CoreTimerGet returns the current value of the core timer.
 *
 * @retval  uint32_t    Current value of core timer.
 *
 */
// ----------------------------------------------------------------------------
uint32_t FRAME_CoreTimerGet(void)
{
    return mCoreTimer;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_PwmNumberSet sets up which PWM channel (1-6) is being used to run
 * the frame timer.  An invalid number sets up the default of EPWM1.
 *
 * @param   PwmNumber   PWM channel to use for frame timer (1-6).
 * @retval  bool_t      TRUE if number is valid, FALSE if number is invalid.
 *
 */
// ----------------------------------------------------------------------------
bool_t FRAME_PwmNumberSet(const uint16_t PwmNumber)
{
    bool_t  bPwmNumberSetOK = TRUE;

    switch (PwmNumber)
    {
        case 1u:
            m_p_EPwmRegs = &EPwm1Regs;
            break;

        case 2u:
            m_p_EPwmRegs = &EPwm2Regs;
            break;

        case 3u:
            m_p_EPwmRegs = &EPwm3Regs;
            break;

        case 4u:
            m_p_EPwmRegs = &EPwm4Regs;
            break;

        case 5u:
            m_p_EPwmRegs = &EPwm5Regs;
            break;

        case 6u:
            m_p_EPwmRegs = &EPwm6Regs;
            break;

        default:
            m_p_EPwmRegs = &EPwm1Regs;
            bPwmNumberSetOK = FALSE;
            break;
    }

    return bPwmNumberSetOK;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_CurrentTickPeriodGet returns the value of the period register for
 * whichever PWM channel is being used to run the frame timer.
 *
 * @retval  uint16_t    Timer period.
 *
 */
// ----------------------------------------------------------------------------
uint16_t FRAME_CurrentTickPeriodGet(void)
{
    return m_p_EPwmRegs->TBPRD;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_CurrentTickTimeGet returns the value of the counter register for
 * whichever PWM channel is being used to run the frame timer - this is the
 * current timer value.
 *
 * @retval  uint16_t    Counter register.
 *
 */
// ----------------------------------------------------------------------------
uint16_t FRAME_CurrentTickTimeGet(void)
{
    return m_p_EPwmRegs->TBCTR;
}


#ifdef UNIT_TEST_BUILD
// ----------------------------------------------------------------------------
/**
 * FRAME_SynchronisingStateSet_TDD allows the synchronising flag to be set for
 * unit test purposes.  This function should not be used otherwise!
 *
 */
// ----------------------------------------------------------------------------
void FRAME_SynchronisingStateSet_TDD(void)
{
    mbSynchronisedFlagSet = TRUE;
}


// ----------------------------------------------------------------------------
/**
 * FRAME_CoreTimerSet_TDD allows the core timer value to be adjusted for
 * unit test purposes.  This function should not be used otherwise!
 *
 */
// ----------------------------------------------------------------------------
void FRAME_CoreTimerSet_TDD(const uint32_t NewValue)
{
    mCoreTimer = NewValue;
}
#endif


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

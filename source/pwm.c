// ----------------------------------------------------------------------------
/**
 * @file    	pwm.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		04 Mar 2014
 * @brief		Functions for setting up PWM modules on TI's 28335 DSP.
 * @details
 * Functions for setting up PWM modules on TI's 28335 DSP.
 *
 * @note
 * Sets up the various PWM and eCAP modules on the 28335 - although the PWM
 * pins are used for different things (triggering the ADC, triggering the
 * resolver, driving the motor drive etc), it seems logical to set all the
 * modules up in one place, so we can be confident that all control bits have
 * been set appropriately.
 *
 * @warning
 * The GPIO multiplexers need to be set-up so that the PWM pins are mux'ed
 * through to the correct IO pins - this will need to be taken care of in a
 * separate module, so all of the muxes are setup at the same time.
 *
 * @warning
 * When using a PWM to generate a pulse which can be placed anywhere within the
 * period, note that the functionality of the other PWM channel is somewhat
 * restricted as both compare registers need to be used for the 'sliding'
 * channel.  This means that the other PWM channel can only generate pulses
 * with a period equal to the timer period.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "common_data_types.h"
#include "DSP28335_device.h"
#include "pwm.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

#define SYSCLKOUT		58982400u			///< 58.9824 MHz sysclk

// EPWM1 is the frame timer - everything is synchronised from this timer.
#define EPWM1_CLKDIV	1u					///< clock prescale of 1
#define EPWM1_HSPCLKDIV	2u					///< high speed clock prescale of 2
#define EPWM1_FPS		1000u				///< 1kHz
#define EPWM1_LOCMP		0x1000u				///< value for timer match, pin lo
#define EPWM1_HICMP		0x2300u				///< value for timer match, pin hi

/// Macro to cast bits correctly to ensure MISRA compliance when using <<.
#define IO_CAST_16BIT		uint16_t)((uint16_t


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static void SetupEPWM1(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * PWM_Initialise sets up all of the PWM and eCAP modules.  The proper procedure
 * for setting up the PWM module is as follows (from SPRUG04A, section 4.8):
 * 	1. Disable global interrupts (CPU INTM flag)
 * 	2. Disable ePWM interrupts
 * 	3. Set TBCLKSYNC=0
 * 	4. Initialise peripheral registers
 * 	5. Set TBCLKSYNC=1
 * 	6. Clear any spurious ePWM flags (including PIEIFR)
 * 	7. Enable ePWM interrupts
 * 	8. Enable global interrupts
 *
 * @warning
 * Remember that the SYSCTRL registers are EALLOW protected...
 *
 */
// ----------------------------------------------------------------------------
void PWM_Initialise(void)
{
	// Disable global interrupts
	DINT;

	// Disable ePWM interrupts (PIE group 3)
	PieCtrlRegs.PIEIER3.all = 0u;

	// Set TBCLKSYNC = 0 - note that this actually stops the clock.
	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0u;
	EDIS;

	SetupEPWM1();

	// Clear any pending ePWM interrupts
	PieCtrlRegs.PIEIFR3.all = 0u;
	IFR &= ~M_INT3;

	// Start timers - all PWM based are started simply by setting TBCLKSYNC.
	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1u;
	EDIS;
	
	// Enable the appropriate PWM interrupts - EPWM1 is 3.1, EPWM2 is 3.2
	PieCtrlRegs.PIEIER3.bit.INTx1 = 1u;			// PIE Group 3, interrupt 1

	// Enable the appropriate CPU interrupt for peripheral group 3.
	IER |= M_INT3;

	// Enable global interrupts
	EINT;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * PWM_DisableAll stops the timer into the PWM modules and disables all PWM
 * interrupts - this is probably a bit heavy-handed, but will do for now.
 *
 * @warning
 * Remember that the SYSCTRL registers are EALLOW protected...
 *
 */
// ----------------------------------------------------------------------------
void PWM_DisableAll(void)
{
	// Disable global interrupts
	DINT;

	// Disable ePWM interrupts (PIE group 3)
	PieCtrlRegs.PIEIER3.all = 0u;

	// Set TBCLKSYNC = 0 - note that this actually stops the clock.
	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0u;
	EDIS;

	// Clear any pending ePWM interrupts
	PieCtrlRegs.PIEIFR3.all = 0u;
	IFR &= ~M_INT3;

	// Enable global interrupts
	EINT;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * PWM_FrameEnable enables the output for the frame timer.  Note that the PWM
 * should have already been initialised, but there will be a continuous
 * software force which is holding the pin high (inactive).  This function is
 * not named relating to a specific PWM to allow us to change the underlying
 * details of which PWM does what, without having to change the external call.
 *
 */
// ----------------------------------------------------------------------------
void PWM_FrameEnable(void)
{
	// Disable continuous software force for EPWM1A.
	// This will take effect when the counter = period.
	EPwm1Regs.AQCSFRC.bit.CSFA = 0u;

	// Enable the interrupt.
	EPwm1Regs.ETSEL.bit.INTEN = 1u;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * PWM_FrameDisable disables the output for the frame timer, by enabling a
 * continuous software force for the relevant PWM pin.  This function is
 * not named relating to a specific PWM to allow us to change the underlying
 * details of which PWM does what, without having to change the external call.
 *
 */
// ----------------------------------------------------------------------------
void PWM_FrameDisable(void)
{
	// Enable continuous software force for EPWM1A.
	// This will take effect when the counter = period.
	EPwm1Regs.AQCSFRC.bit.CSFA = 2u;

	// Disable the interrupt.
	EPwm1Regs.ETSEL.bit.INTEN = 0u;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * SetupEPWM1 initialises EPWM1.
 *
 * This module is used as follows:
 * 		EPWM1A	-	generates the frame timer from which all other PWMs are
 * 					synchronised.
 * 		EPWM1B	-	does nothing.
 *
 */
// ----------------------------------------------------------------------------
static void SetupEPWM1(void)
{
	uint16_t RequiredData;


	// ---------- TIME BASE SUBMODULE -----------------------------------------

	// Setup all bits to write into the TBCTL register, and write them.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 14) |	//  00: FREE,SOFT - stop during emulation
		(IO_CAST_16BIT)0u << 13) |	//   0: count down after sync event (up\down only)
		(IO_CAST_16BIT)0u << 10) |	// 000: CLKDIV divide by 1
		(IO_CAST_16BIT)1u << 7)	|  	// 001: HSPCLKDIV divide by 2
		(IO_CAST_16BIT)0u << 6) |	//   0: no SWFSYNC
		(IO_CAST_16BIT)1u << 4) |	//  01: synch out when CTR = zero
		(IO_CAST_16BIT)0u << 3) |	//   0: use shadow register for TBPRD loads
		(IO_CAST_16BIT)0u << 2) |	//   0: don't synchronise using phase register
		(IO_CAST_16BIT)0u << 0);	//  00: up count mode
	EPwm1Regs.TBCTL.all = RequiredData;

	// Not using the phase register but zero for completeness.
	EPwm1Regs.TBPHS.all = 0u;

	// Setup period for EPWM1 - note that the #defines for the xxxDIV need to
	// match those setup in the control register above.
	EPwm1Regs.TBPRD = (uint16_t)(SYSCLKOUT /
						(EPWM1_CLKDIV * EPWM1_HSPCLKDIV * EPWM1_FPS) ) - 1u;

	// Reset timer to zero.  Note that if TBCLKSYNC=0, the timer will be not be
	// running at this point.
	EPwm1Regs.TBCTR = 0u;


	// ---------- COUNTER COMPARE SUBMODULE -----------------------------------

	// Setup compare registers - these show when the pin will go low and high.
	EPwm1Regs.CMPA.half.CMPAHR = 0u;
	EPwm1Regs.CMPA.half.CMPA = EPWM1_LOCMP;
	EPwm1Regs.CMPB = EPWM1_HICMP;

	// Setup counter compare control register - leave at default of zero,
	// which means shadow mode for everything, on CTR=zero.
	EPwm1Regs.CMPCTL.all = 0u;


	// ---------- ACTION QUALIFIER SUBMODULE ----------------------------------

	// Setup all bits to write into the AQCTLA register, and write them.
	// This register controls the EPWM1A output, which we want to toggle at
	// the end of each period.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: reserved
		(IO_CAST_16BIT)0u << 14) |	//   0: reserved
		(IO_CAST_16BIT)0u << 13) |	//   0: reserved
		(IO_CAST_16BIT)0u << 12) |	//   0: reserved
		(IO_CAST_16BIT)0u << 10) |	//  00: CBD do nothing
		(IO_CAST_16BIT)0u << 8) |	//  00: CBU do nothing
		(IO_CAST_16BIT)0u << 6) |	//  00: CAD do nothing
		(IO_CAST_16BIT)0u << 4)	|  	//  00: CAU do nothing
		(IO_CAST_16BIT)0u << 2) |	//  00: PRD do nothing
		(IO_CAST_16BIT)3u << 0);	//  00: ZRO do nothing
	EPwm1Regs.AQCTLA.all = RequiredData;

	// Setup all bits to write into the AQCTLB register, and write them.
	// This register controls the EPWM1B output, which we want to go low when
	// the counter matches compare A, and then go high again when the counter
	// matches control B.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: reserved
		(IO_CAST_16BIT)0u << 14) |	//   0: reserved
		(IO_CAST_16BIT)0u << 13) |	//   0: reserved
		(IO_CAST_16BIT)0u << 12) |	//   0: reserved
		(IO_CAST_16BIT)0u << 10) |	//  00: CBD do nothing
		(IO_CAST_16BIT)0u << 8) |	//  00: CBU do nothing
		(IO_CAST_16BIT)0u << 6) |	//  00: CAD do nothing
		(IO_CAST_16BIT)0u << 4)	|  	//  00: CAU do nothing
		(IO_CAST_16BIT)0u << 2) |	//  00: PRD do nothing
		(IO_CAST_16BIT)0u << 0);	//  00: ZRO do nothing
	EPwm1Regs.AQCTLB.all = RequiredData;

	// Setup all bits to write into the AQSFRC register, and write them.
	// This register controls software forcing, which we use to turn this PWM
	// output on and off.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: reserved
		(IO_CAST_16BIT)0u << 14) |	//   0: reserved
		(IO_CAST_16BIT)0u << 13) |	//   0: reserved
		(IO_CAST_16BIT)0u << 12) |	//   0: reserved
		(IO_CAST_16BIT)0u << 11) |	//   0: reserved
		(IO_CAST_16BIT)0u << 10) |	//   0: reserved
		(IO_CAST_16BIT)0u << 9) |	//   0: reserved
		(IO_CAST_16BIT)0u << 8) |	//   0: reserved
		(IO_CAST_16BIT)2u << 6) |	//  10: load on counter=0 or counter=period
		(IO_CAST_16BIT)0u << 5) |	//   0: output B, no one time forced event
		(IO_CAST_16BIT)0u << 3)	|  	//  00: output B, do nothing on one-time SW force
		(IO_CAST_16BIT)0u << 2) |	//   0: output A, no one time forced event
		(IO_CAST_16BIT)0u << 0);	//  00: output A, do nothing on one-time SW force
	EPwm1Regs.AQSFRC.all = RequiredData;

	// Setup all bits to write into the AQCSFRC register, and write them.
	// This register controls continuous software forcing, which we use to turn
	// this PWM output on and off - forcing a continuous high on EPWM1B will
	// inhibit the #CNVST pin on the ADC.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: reserved
		(IO_CAST_16BIT)0u << 14) |	//   0: reserved
		(IO_CAST_16BIT)0u << 13) |	//   0: reserved
		(IO_CAST_16BIT)0u << 12) |	//   0: reserved
		(IO_CAST_16BIT)0u << 11) |	//   0: reserved
		(IO_CAST_16BIT)0u << 10) |	//   0: reserved
		(IO_CAST_16BIT)0u << 9) |	//   0: reserved
		(IO_CAST_16BIT)0u << 8) |	//   0: reserved
		(IO_CAST_16BIT)0u << 7) |	//   0: reserved
		(IO_CAST_16BIT)0u << 6) |	//   0: reserved
		(IO_CAST_16BIT)0u << 5) |	//   0: reserved
		(IO_CAST_16BIT)0u << 4) |	//   0: reserved
		(IO_CAST_16BIT)2u << 2)	|  	//  10: output B, continuous force high
		(IO_CAST_16BIT)2u << 0);	//  10: output A, continuous force high
	EPwm1Regs.AQCSFRC.all = RequiredData;


	// ---------- DEAD BAND GENERATOR SUBMODULE -------------------------------

	// Dead band generator not required, so zero all associated registers.
	EPwm1Regs.DBCTL.all = 0u;
	EPwm1Regs.DBRED = 0u;
	EPwm1Regs.DBFED = 0u;


	// ---------- PWM CHOPPER SUBMODULE ---------------------------------------

	// PWM chopper not required, so zero it.
	EPwm1Regs.PCCTL.all = 0u;


	// ---------- TRIP ZONE SUBMODULE -----------------------------------------

	// Trip zone not required, so zero control register just in case.
	EPwm1Regs.TZSEL.all = 0u;

	// Setup all bits to write into the TZCTL register, and write them.
	// Even though the trip zone has been disabled, we'll still set this
	// register to the 'do nothing' values, just in case.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: reserved
		(IO_CAST_16BIT)0u << 14) |	//   0: reserved
		(IO_CAST_16BIT)0u << 13) |	//   0: reserved
		(IO_CAST_16BIT)0u << 12) |	//   0: reserved
		(IO_CAST_16BIT)0u << 11) |	//   0: reserved
		(IO_CAST_16BIT)0u << 10) |	//   0: reserved
		(IO_CAST_16BIT)0u << 9) |	//   0: reserved
		(IO_CAST_16BIT)0u << 8) |	//   0: reserved
		(IO_CAST_16BIT)0u << 7) |	//   0: reserved
		(IO_CAST_16BIT)0u << 6) |	//   0: reserved
		(IO_CAST_16BIT)0u << 5) |	//   0: reserved
		(IO_CAST_16BIT)0u << 4) |	//   0: reserved
		(IO_CAST_16BIT)3u << 2) |	//  11: No action taken on EPWM1B
		(IO_CAST_16BIT)3u << 0);	//  11: No action taken on EPWN1A
	EALLOW;
	EPwm1Regs.TZCTL.all = RequiredData;
	EDIS;

	// Setup all bits to write into the TZEINT register, and write them.
	// (Note that bits 15:3 and 0 are reserved, so writing a zero has no effect).
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: reserved
		(IO_CAST_16BIT)0u << 14) |	//   0: reserved
		(IO_CAST_16BIT)0u << 13) |	//   0: reserved
		(IO_CAST_16BIT)0u << 12) |	//   0: reserved
		(IO_CAST_16BIT)0u << 11) |	//   0: reserved
		(IO_CAST_16BIT)0u << 10) |	//   0: reserved
		(IO_CAST_16BIT)0u << 9) |	//   0: reserved
		(IO_CAST_16BIT)0u << 8) |	//   0: reserved
		(IO_CAST_16BIT)0u << 7) |	//   0: reserved
		(IO_CAST_16BIT)0u << 6) |	//   0: reserved
		(IO_CAST_16BIT)0u << 5) |	//   0: reserved
		(IO_CAST_16BIT)0u << 4) |	//   0: reserved
		(IO_CAST_16BIT)0u << 3) |	//   0: reserved
		(IO_CAST_16BIT)0u << 2) |	//   0: disable one shot interrupt generation
		(IO_CAST_16BIT)0u << 1)	|	//	 0: disable cycle by cycle interrupt generation
		(IO_CAST_16BIT)0u << 0);	//   0: reserved
	EALLOW;
	EPwm1Regs.TZEINT.all = RequiredData;
	EDIS;


	// ---------- EVENT TRIGGER SUBMODULE -------------------------------------

	// Setup all bits to write into the ETSEL register, and write them.
	// Note that the SOC ADC events are disabled, so even though the selection
	// options (bits 14:12 and 10:8) are set, nothing will happen.
	// Also note that bits 7:4 are reserved, so are written with zeros.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: Disable EPWM1SOCB
		(IO_CAST_16BIT)1u << 12) |	// 001: EPWM1SOCB generated on TBCTR=0
		(IO_CAST_16BIT)0u << 11) |	//   0: Disable EPWM1SOCA
		(IO_CAST_16BIT)1u << 8)	|  	// 001: EPWM1SOCA generated on TBCTR=0
		(IO_CAST_16BIT)0u << 7) |	//   0: reserved
		(IO_CAST_16BIT)0u << 6) |	//   0: reserved
		(IO_CAST_16BIT)0u << 5) |	//   0: reserved
		(IO_CAST_16BIT)0u << 4) |	//   0: reserved
		(IO_CAST_16BIT)0u << 3) |	//   0: Disable EPWM1_INT generation (for startup - enabled when SW force removed)
		(IO_CAST_16BIT)1u << 0);	// 001: EPWM1_INT generated on TBCTR=0
	EPwm1Regs.ETSEL.all = RequiredData;

	// Setup all bits to write into the ETPS register, and write them.
	//lint --e(835)
	RequiredData =
		(IO_CAST_16BIT)0u << 15) |	//   0: read only bit
		(IO_CAST_16BIT)0u << 14) |	//   0: read only bit
		(IO_CAST_16BIT)0u << 12) |	//  00: Disable SOCB event counter
		(IO_CAST_16BIT)0u << 11) |	//   0: read only bit
		(IO_CAST_16BIT)0u << 10) |	//   0: read only bit
		(IO_CAST_16BIT)0u << 8)	|  	//  00: Disable SOCA event counter
		(IO_CAST_16BIT)0u << 7) |	//   0: reserved
		(IO_CAST_16BIT)0u << 6) |	//   0: reserved
		(IO_CAST_16BIT)0u << 5) |	//   0: reserved
		(IO_CAST_16BIT)0u << 4) |	//   0: reserved
		(IO_CAST_16BIT)0u << 3) |	//   0: read only bit
		(IO_CAST_16BIT)0u << 2) |	//   0: read only bit
		(IO_CAST_16BIT)1u << 0);	//  01: Generate interrupt on the first event
	EPwm1Regs.ETPS.all = RequiredData;

	// Setup all bits to write into the ETCLR register, and write them.
	// This clears any pending interrupts.
	//lint --e(835)
	RequiredData =
			(IO_CAST_16BIT)0u << 15) |	//   0: reserved
			(IO_CAST_16BIT)0u << 14) |	//   0: reserved
			(IO_CAST_16BIT)0u << 13) |	//   0: reserved
			(IO_CAST_16BIT)0u << 12) |	//   0: reserved
			(IO_CAST_16BIT)0u << 11) |	//   0: reserved
			(IO_CAST_16BIT)0u << 10) |	//   0: reserved
			(IO_CAST_16BIT)0u << 9) |	//   0: reserved
			(IO_CAST_16BIT)0u << 8) |	//   0: reserved
			(IO_CAST_16BIT)0u << 7) |	//   0: reserved
			(IO_CAST_16BIT)0u << 6) |	//   0: reserved
			(IO_CAST_16BIT)0u << 5) |	//   0: reserved
			(IO_CAST_16BIT)0u << 4) |	//   0: reserved
			(IO_CAST_16BIT)1u << 3) |	//   1: clear the ETFLG[SOCB] bit
			(IO_CAST_16BIT)1u << 2) |	//   1: clear the ETFLG[SOCA] bit
			(IO_CAST_16BIT)0u << 1) |	//   0: reserved
			(IO_CAST_16BIT)1u << 0);	//   1: clear the ETFLG[INT] flag
	EPwm1Regs.ETCLR.all = RequiredData;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


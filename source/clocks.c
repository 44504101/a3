// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/clocks.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Jul 2012
 * @brief		PLL and peripheral clock driver functions for TI's 28335 DSP.
 * @details
 * Functions are provided to setup the PLL and associated divider, enable or
 * disable the clock to individual peripherals, and to setup the prescalers for
 * the high and low speed peripheral dividers.
 *
 * @warning
 * The PLL code doesn't currently have a timeout for when the PLL won't lock.
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
#include "clocks.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define PCLKCR0_ECANBENCLK_BIT_MASK		0x8000u		///< ECANBENCLK is bit 15
#define PCLKCR0_ECANAENCLK_BIT_MASK		0x4000u		///< ECANAENCLK is bit 14
#define PCLKCR0_MCBSPBENCLK_BIT_MASK	0x2000u		///< MCBSPBENCLK is bit 13
#define PCLKCR0_MCBSPAENCLK_BIT_MASK	0x1000u		///< MCBSPAENCLK is bit 12
#define PCLKCR0_SCIBENCLK_BIT_MASK		0x0800u		///< SCIBENCLK is bit 11
#define PCLKCR0_SCIAENCLK_BIT_MASK		0x0400u		///< SCIAENCLK is bit 10
#define PCLKCR0_SPIAENCLK_BIT_MASK		0x0100u		///< SPIBENCLK is bit 8
#define PCLKCR0_SCICENCLK_BIT_MASK		0x0020u		///< SCICENCLK is bit 5
#define PCLKCR0_I2CAENCLK_BIT_MASK		0x0010u		///< I2CAENCLK is bit 4
#define PCLKCR0_ADCENCLK_BIT_MASK		0x0008u		///< ADCENCLK is bit 3
#define PCLKCR0_TBCLKSYNC_BIT_MASK		0x0004u		///< TBCLKSYNC is bit 2

#define PCLKCR1_EQEP2ENCLK_BIT_MASK		0x8000u		///< EQEP2ENCLK is bit 15
#define PCLKCR1_EQEP1ENCLK_BIT_MASK		0x4000u		///< EQEP1ENCLK is bit 14
#define PCLKCR1_ECAP6ENCLK_BIT_MASK		0x2000u		///< ECAP6ENCLK is bit 13
#define PCLKCR1_ECAP5ENCLK_BIT_MASK		0x1000u		///< ECAP5ENCLK is bit 12
#define PCLKCR1_ECAP4ENCLK_BIT_MASK		0x0800u		///< ECAP4ENCLK is bit 11
#define PCLKCR1_ECAP3ENCLK_BIT_MASK		0x0400u		///< ECAP3ENCLK is bit 10
#define PCLKCR1_ECAP2ENCLK_BIT_MASK		0x0200u		///< ECAP2ENCLK is bit 9
#define PCLKCR1_ECAP1ENCLK_BIT_MASK		0x0100u		///< ECAP1ENCLK is bit 8
#define PCLKCR1_EPWM6ENCLK_BIT_MASK		0x0020u		///< EPWM6ENCLK is bit 5
#define PCLKCR1_EPWM5ENCLK_BIT_MASK		0x0010u		///< EPWM5ENCLK is bit 4
#define PCLKCR1_EPWM4ENCLK_BIT_MASK		0x0008u		///< EPWM4ENCLK is bit 3
#define PCLKCR1_EPWM3ENCLK_BIT_MASK		0x0004u		///< EPWM3ENCLK is bit 2
#define PCLKCR1_EPWM2ENCLK_BIT_MASK		0x0002u		///< EPWM2ENCLK is bit 1
#define PCLKCR1_EPWM1ENCLK_BIT_MASK		0x0001u		///< EPWM1ENCLK is bit 0

#define PCLKCR3_GPIOINENCLK_BIT_MASK	0x2000u		///< GPIOINENCLK is bit 13
#define PCLKCR3_XINTFENCLK_BIT_MASK		0x1000u		///< XINTFENCLK is bit 12
#define PCLKCR3_DMAENCLK_BIT_MASK		0x0800u		///< DMAENCLK is bit 11
#define PCLKCR3_CPUTIMER2ENCLK_BIT_MASK	0x0400u		///< CPUTIMER2ENCLK is bit 10
#define PCLKCR3_CPUTIMER1ENCLK_BIT_MASK	0x0200u		///< CPUTIMER1ENCLK is bit 9
#define PCLKCR3_CPUTIMER0ENCLK_BIT_MASK	0x0100u		///< CPUTIMER0ENCLK is bit 8

#define MAX_VCOCLK_WITH_NO_PLL		(30000000u)     ///< Maximum clock is 30 MHz
#define MAX_VCOCLK_WITH_PLL			(300000000u)    ///< Maximum clock is 300 MHz


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/// Array holding bit masks for each peripheral enable.
static const Uint16 ClockBitMasks[] = {	PCLKCR0_ECANBENCLK_BIT_MASK,
                                        PCLKCR0_ECANAENCLK_BIT_MASK,
                                        PCLKCR0_MCBSPBENCLK_BIT_MASK,
                                        PCLKCR0_MCBSPAENCLK_BIT_MASK,
                                        PCLKCR0_SCIBENCLK_BIT_MASK,
                                        PCLKCR0_SCIAENCLK_BIT_MASK,
                                        PCLKCR0_SPIAENCLK_BIT_MASK,
                                        PCLKCR0_SCICENCLK_BIT_MASK,
                                        PCLKCR0_I2CAENCLK_BIT_MASK,
                                        PCLKCR0_ADCENCLK_BIT_MASK,
                                        PCLKCR0_TBCLKSYNC_BIT_MASK,
                                        PCLKCR1_EQEP2ENCLK_BIT_MASK,
                                        PCLKCR1_EQEP1ENCLK_BIT_MASK,
                                        PCLKCR1_ECAP6ENCLK_BIT_MASK,
                                        PCLKCR1_ECAP5ENCLK_BIT_MASK,
                                        PCLKCR1_ECAP4ENCLK_BIT_MASK,
                                        PCLKCR1_ECAP3ENCLK_BIT_MASK,
                                        PCLKCR1_ECAP2ENCLK_BIT_MASK,
                                        PCLKCR1_ECAP1ENCLK_BIT_MASK,
                                        PCLKCR1_EPWM6ENCLK_BIT_MASK,
                                        PCLKCR1_EPWM5ENCLK_BIT_MASK,
                                        PCLKCR1_EPWM4ENCLK_BIT_MASK,
                                        PCLKCR1_EPWM3ENCLK_BIT_MASK,
                                        PCLKCR1_EPWM2ENCLK_BIT_MASK,
                                        PCLKCR1_EPWM1ENCLK_BIT_MASK,
                                        PCLKCR3_GPIOINENCLK_BIT_MASK,
                                        PCLKCR3_XINTFENCLK_BIT_MASK,
                                        PCLKCR3_DMAENCLK_BIT_MASK,
                                        PCLKCR3_CPUTIMER2ENCLK_BIT_MASK,
                                        PCLKCR3_CPUTIMER1ENCLK_BIT_MASK,
                                        PCLKCR3_CPUTIMER0ENCLK_BIT_MASK		};

/// Array holding register address for each bit mask.
//lint -e{956} This is volatile, but we still get a Lint warning...
static volatile Uint16* ClockRegisters[] = { &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR0.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR1.all,
                                             &SysCtrlRegs.PCLKCR3.all,
                                             &SysCtrlRegs.PCLKCR3.all,
                                             &SysCtrlRegs.PCLKCR3.all,
                                             &SysCtrlRegs.PCLKCR3.all,
                                             &SysCtrlRegs.PCLKCR3.all,
                                             &SysCtrlRegs.PCLKCR3.all		};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * CLOCKS_PLLSetup sets up the PLL on the 28335, following the PLL change
 * procedure detailed in SPRUFB0D, chapter 5, figure 22.  For simplicity, the
 * user must enter the required multiplier and divider, rather than have the
 * firmware calculate all the permutations and combinations if given a required
 * frequency.  Whenever one of the PLL registers is written to, the EALLOW
 * instruction must be issued to unlock the protected register.
 *
 * @param	iExtClk_Hz		External clock frequency, in Hz.
 * @param	PLLMultiplier	Required PLL multiplier.
 * @param	ClockDivider	Required divider after PLL multiplication.
 * @retval	ReturnValue		Enumerated value for possible failures.
 *
 */
// ----------------------------------------------------------------------------
EClocksFailureModes_t CLOCKS_PLLSetup(const uint32_t iExtClk_Hz,
										const EPLLMultipliers_t PLLMultiplier,
										const EPLLClockDividers_t ClockDivider)
{
	EClocksFailureModes_t 	ReturnValue = PLL_SETUP_OK;

	// If the Missing CLock STatuS bit is set then the DSP is in limp mode.
	if (SysCtrlRegs.PLLSTS.bit.MCLKSTS == 1u)
	{
		ReturnValue = DEVICE_IN_LIMP_MODE;
	}
	// Otherwise the DSP clock is running OK, so now check for permitted
	// combinations of input clock, PLL multiplier and clock divider.
	else
	{
		// If the PLL is bypassed and the input clock exceeds the permitted
		// value for this combination, then generate an error.
		if ( (PLLMultiplier == PLL_BYPASS) && (iExtClk_Hz > MAX_VCOCLK_WITH_NO_PLL) )
		{
			ReturnValue = VCOCLK_TOO_HIGH_NO_PLL;
		}
		// Otherwise the PLL is required, so check that the product of the input
		// clock and PLL multiplier doesn't exceed the required value.
		else
		{
			// If the VCOCLK would be too high then generate an error.
		    //lint -e{930, 921} Cast to uint32_t OK - enums are all positive.
			if ( (iExtClk_Hz * (uint32_t)PLLMultiplier) > MAX_VCOCLK_WITH_PLL)
			{
				ReturnValue = VCOCLK_TOO_HIGH_WITH_PLL;
			}
			// Otherwise VCOCLK is OK, so check that the divider is valid (it
			// is not permitted to have no divider with the PLL enabled).
			else
			{
				if (ClockDivider == CLOCK_NO_DIVIDE)
				{
					ReturnValue = INVALID_DIVIDER;
				}
			}
		}
	}

	// if the ReturnValue hasn't been corrupted from the initialised value, we
	// are OK to go ahead and setup the PLL...
	if (ReturnValue == PLL_SETUP_OK)
	{
		// Set divider bits to zero before changing anything else, then
		// disable failed oscillator detection logic while changing PLL
		// then update the PLLCR register with the new PLL multiplier value.
		EALLOW;
		SysCtrlRegs.PLLSTS.bit.DIVSEL = 0u;
		SysCtrlRegs.PLLSTS.bit.MCLKOFF = 0u;
		//lint -e{930, 921} Cast to uint16_t OK - enums are all positive.
		SysCtrlRegs.PLLCR.all = (uint16_t)PLLMultiplier;
		EDIS;

		// Read from the status register and wait for the PLL to lock
		// @TODO Might need to consider adding a timeout in here.
		while (SysCtrlRegs.PLLSTS.bit.PLLLOCKS == 0u)
		{
			;
		}

		// Enable failed oscillator detection logic by clearing MCLKOFF bit
		// and setup the required clock divider.  Note the Lint directive - this
		// disables a warning for implicit conversion to smaller type (because
		// we're using bitfields, MISRA doesn't like it).
		EALLOW;
		SysCtrlRegs.PLLSTS.bit.MCLKOFF = 1u;
		//lint -e{930, 921} Cast to uint32_t OK - enums are all positive.
		SysCtrlRegs.PLLSTS.bit.DIVSEL = (uint16_t)ClockDivider;     //lint !e9034
		EDIS;
	}

	return ReturnValue;
}


// ----------------------------------------------------------------------------
/**
 * CLOCKS_PeripheralClocksAllDisable disables the clock to all of the
 * peripherals.  Note that it doesn't matter if your particular device doesn't
 * contain one of the modules, because it is still permitted to write a zero
 * to the bit in the PCLKCRx register.
 *
*/
// ----------------------------------------------------------------------------
void CLOCKS_PeripheralClocksAllDisable(void)
{
	EALLOW;
	SysCtrlRegs.PCLKCR0.all = 0u;
	SysCtrlRegs.PCLKCR1.all = 0u;
	SysCtrlRegs.PCLKCR3.all = 0u;
	EDIS;
}


// ----------------------------------------------------------------------------
/**
 * CLOCKS_PeripheralClocksEnable enables the clock to a specific peripheral.
 * Note that this currently doesn't check to make sure the peripheral exists
 * on the particular device, because the 28335 has everything!
 *
 * @warning
 * This uses the peripheral typedef Uint16
 *
 * @param	RequiredClock	Enumerated value for required peripheral module clock.
*/
// ----------------------------------------------------------------------------
void CLOCKS_PeripheralClocksEnable(const EPeripheralClocks_t RequiredClock)
{
	Uint16				BitMask;
	volatile Uint16*	Address;

	Address = ClockRegisters[RequiredClock];
	BitMask = ClockBitMasks[RequiredClock];

	EALLOW;
	*Address |= BitMask;
	EDIS;
}


// ----------------------------------------------------------------------------
/**
 * CLOCKS_PeripheralClocksDisable disables the clock to a specific peripheral.
 * This uses a pair of lookup tables, rather than an enormous case statement.
 *
 * @warning
 * This uses the peripheral typedef Uint16
 *
 * @param	RequiredClock	Enumerated value for required peripheral module clock.
*/
// ----------------------------------------------------------------------------
void CLOCKS_PeripheralClocksDisable(const EPeripheralClocks_t RequiredClock)
{
	Uint16				BitMask;
	volatile Uint16*	Address;

	Address = ClockRegisters[RequiredClock];
	BitMask = ClockBitMasks[RequiredClock];

	EALLOW;
	*Address &= (Uint16)~BitMask;   //lint !e921 Cast to uint16_t after negation.
	EDIS;
}


// ----------------------------------------------------------------------------
/**
 * CLOCKS_PeripheralLowSpeedPrescalerSet sets up the required prescaler for any
 * peripherals which are controlled by the low speed clock (currently the SCI,
 * SPI and McBSP ports).  Note that the enumerated values have been initialised
 * to be the same as the value written into the prescaler register.
 *
 * @param	RequiredPrescaler	Enumerated value for required prescaler.
*/
// ----------------------------------------------------------------------------
void CLOCKS_PeripheralLowSpeedPrescalerSet(const EPeripheralClockDividers_t RequiredPrescaler)
{
	EALLOW;
	//lint -e{930, 921} Cast to uint16_t OK - enums are all positive.
	SysCtrlRegs.LOSPCP.all = (uint16_t)RequiredPrescaler;
	EDIS;
}


// ----------------------------------------------------------------------------
/**
 * CLOCKS_PeripheralHighSpeedPrescalerSet sets up the required prescaler for any
 * peripherals which are controlled by the high speed clock (currently only the
 * ADC module).  Note that the enumerated values have been initialised to be the
 * same as the value written into the prescaler register.
 *
 * @param	RequiredPrescaler	Enumerated value for required prescaler.
*/
// ----------------------------------------------------------------------------
void CLOCKS_PeripheralHighSpeedPrescalerSet(const EPeripheralClockDividers_t RequiredPrescaler)
{
	EALLOW;
	//lint -e{930, 921} Cast to uint16_t OK - enums are all positive.
	SysCtrlRegs.HISPCP.all = (uint16_t)RequiredPrescaler;
	EDIS;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

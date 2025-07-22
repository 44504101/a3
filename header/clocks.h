// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/clocks.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Jul 2012
 * @brief		Header file for clocks.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef CLOCKS_H_
#define CLOCKS_H_

/// Enumerated values for clock failure modes.
typedef enum
{
	PLL_SETUP_OK,                   ///< No failure, PLL setup OK.
	DEVICE_IN_LIMP_MODE,            ///< Device has started in limp mode.
	VCOCLK_TOO_HIGH_NO_PLL,         ///< VCO clock is too high with no PLL selected.
	VCOCLK_TOO_HIGH_WITH_PLL,       ///< VCO clock is too high with PLL selected.
	INVALID_DIVIDER                 ///< Invalid divider selected.
} EClocksFailureModes_t;

/// Enumerated values for PLL multipliers.
typedef enum
{
	PLL_BYPASS			=	0,      ///< Bypass the PLL.
	PLL_TIMES_1			=	1,      ///< Multiplier x1.
	PLL_TIMES_2			=	2,      ///< Multiplier x2.
	PLL_TIMES_3			=	3,      ///< Multiplier x3.
	PLL_TIMES_4			=	4,      ///< Multiplier x4.
	PLL_TIMES_5			=	5,      ///< Multiplier x5.
	PLL_TIMES_6			=	6,      ///< Multiplier x6.
	PLL_TIMES_7			=	7,      ///< Multiplier x7.
	PLL_TIMES_8			=	8,      ///< Multiplier x8.
	PLL_TIMES_9			=	9,      ///< Multiplier x9.
	PLL_TIMES_10		=	10      ///< Multiplier x10.
} EPLLMultipliers_t;

/// Enumerated values for PLL clock dividers.
typedef enum
{
	CLOCK_DIVIDE_BY_4	=	0,      ///< Divide by 4.
	CLOCK_DIVIDE_BY_2	= 	2,      ///< Divide by 2.
	CLOCK_NO_DIVIDE		=   3       ///< No clock divider.
} EPLLClockDividers_t;

/// Enumerated values for peripheral clocks.
typedef enum
{
	ECAN_B_CLOCK = 0,               ///< Can module B clock.
	ECAN_A_CLOCK,                   ///< Can module A clock.
	MCBSP_B_CLOCK,                  ///< McBSP module B clock.
	MCBSP_A_CLOCK,                  ///< McBSP module A clock.
	SCI_B_CLOCK,                    ///< SCI port B clock.
	SCI_A_CLOCK,                    ///< SCI port A clock.
	SPI_A_CLOCK,                    ///< SPI port A clock.
	SCI_C_CLOCK,                    ///< SCI port C clock.
	I2C_A_CLOCK,                    ///< I2C port A clock.
	ADC_CLOCK,                      ///< ADC clock.
	TBSYNC_CLOCK,                   ///< TBSYNC clock.

	EQEP2_CLOCK,                    ///< QEP module 2 clock.
	EQEP1_CLOCK,                    ///< QEP module 1 clock.
	ECAP6_CLOCK,                    ///< Capture compare module 6 clock.
	ECAP5_CLOCK,                    ///< Capture compare module 5 clock.
	ECAP4_CLOCK,                    ///< Capture compare module 4 clock.
	ECAP3_CLOCK,                    ///< Capture compare module 3 clock.
	ECAP2_CLOCK,                    ///< Capture compare module 2 clock.
	ECAP1_CLOCK,                    ///< Capture compare module 1 clock.
	EPWM6_CLOCK,                    ///< PWM module 6 clock.
	EPWM5_CLOCK,                    ///< PWM module 5 clock.
	EPWM4_CLOCK,                    ///< PWM module 4 clock.
	EPWM3_CLOCK,                    ///< PWM module 3 clock.
	EPWM2_CLOCK,                    ///< PWM module 2 clock.
	EPWM1_CLOCK,                    ///< PWM module 1 clock.

	GPIO_CLOCK,                     ///< GPIO module clock.
	XINTF_CLOCK,                    ///< XINTF module clock.
	DMA_CLOCK,                      ///< DMA controller clock.
	CPUTIMER2_CLOCK,                ///< CPU timer 2 clock.
	CPUTIMER1_CLOCK,                ///< CPU timer 1 clock.
	CPUTIMER0_CLOCK                 ///< CPU timer 0 clock.
} EPeripheralClocks_t;

/// Enumerated values for peripheral clock dividers.
typedef enum
{
	PERIPHERAL_NO_DIVIDE 	= 0,    ///< No peripheral clock divider.
	PERIPHERAL_DIVIDE_BY_2	= 1,    ///< Divide by 2.
	PERIPHERAL_DIVIDE_BY_4	= 2,    ///< Divide by 4.
	PERIPHERAL_DIVIDE_BY_6	= 3,    ///< Divide by 6.
	PERIPHERAL_DIVIDE_BY_8 	= 4,    ///< Divide by 8.
	PERIPHERAL_DIVIDE_BY_10 = 5,    ///< Divide by 10.
	PERIPHERAL_DIVIDE_BY_12 = 6,    ///< Divide by 12.
	PERIPHERAL_DIVIDE_BY_14 = 7     ///< Divide by 14.
} EPeripheralClockDividers_t;

EClocksFailureModes_t	CLOCKS_PLLSetup(const uint32_t iExtClk_Hz,
										const EPLLMultipliers_t PLLMultiplier,
										const EPLLClockDividers_t ClockDivider);

void CLOCKS_PeripheralClocksAllDisable(void);
void CLOCKS_PeripheralClocksEnable(const EPeripheralClocks_t RequiredClock);
void CLOCKS_PeripheralClocksDisable(const EPeripheralClocks_t RequiredClock);
void CLOCKS_PeripheralLowSpeedPrescalerSet(const EPeripheralClockDividers_t RequiredPrescaler);
void CLOCKS_PeripheralHighSpeedPrescalerSet(const EPeripheralClockDividers_t RequiredPrescaler);

#endif /* CLOCKS_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

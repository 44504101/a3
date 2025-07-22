// ----------------------------------------------------------------------------
/**
 * @file    	acqmtccpu_dsp_b/platform/gpiomux.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		22 Jan 2013
 * @brief		Functions for setting up GPIO MUX for TI's 28335 DSP, DSP B.
 * @details
 * Sets up the pull-up, output value, mux setting and direction for all of the
 * I/O pins on the 28335 DSP. This code uses the TI standard bitfields for the
 * various registers, and is organised is groups for each I/O pin.  Although
 * this is not terribly efficient (in terms of both code space and execution),
 * it only needs to be executed once, and it seems clearer to set all functions
 * for an I/O pin 'together'.
 *
 * @warning
 * This code is for the Xceed ACQ \ MTC board, DSP B.
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "gpiomux.h"
#include "DSP28335_device.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * GPIOMUX_Initialise sets up each GPIO pin in turn - this is not the most
 * efficient way of doing this, but grouping all of the bits together for each
 * I/O pin results in code that's easier to read and modify.
 * If a pin uses the peripheral function, the direction bit is ignored, so can
 * be left as an input.
 *
*/
// ----------------------------------------------------------------------------
void GPIOMUX_Initialise(void)
{
	EALLOW;

	// GPIO0
	GpioDataRegs.GPACLEAR.bit.GPIO1 = 1u;	// bit clear (Clink Power disabled, not required as the output is driven by the ePWM1 module)
	GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1u;		// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1u;	// EPWM1A - Clink Power pulsed signal
	GpioCtrlRegs.GPADIR.bit.GPIO3 = 1u;		// output (Not required as the output is driven by the ePWM1 module)

	// GPIO1
	GpioDataRegs.GPASET.bit.GPIO1 = 1u;		// bit set (low oil switch de-energised)
	GpioCtrlRegs.GPAPUD.bit.GPIO1 = 1u;		// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0u;	// GPIO - LOW_OIL_ENA
	GpioCtrlRegs.GPADIR.bit.GPIO1 = 1u;		// output

	// GPIO2
	GpioDataRegs.GPASET.bit.GPIO2   = 1u;    // bit set (RTC chip disabled)
	GpioCtrlRegs.GPAPUD.bit.GPIO2 	= 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO2 	= 0u;	// GPIO - #RTC_chip_enabled
	GpioCtrlRegs.GPADIR.bit.GPIO2 	= 1u;	// output

	// GPIO3
	GpioDataRegs.GPASET.bit.GPIO3 = 1u;		// bit set (receiver disabled)
	GpioCtrlRegs.GPAPUD.bit.GPIO3 = 1u;		// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 0u;	// GPIO - RS485 #RE (EPWM2B used but is internal signal)
	GpioCtrlRegs.GPADIR.bit.GPIO3 = 1u;		// output

	// GPIO4
	GpioDataRegs.GPACLEAR.bit.GPIO4 = 1u;	// bit clear (transmitter disabled)
	GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1u;		// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 0u;	// GPIO - RS485 DE
	GpioCtrlRegs.GPADIR.bit.GPIO4 = 1u;		// output

	// GPIO5
	GpioCtrlRegs.GPAPUD.bit.GPIO5 = 1u;		// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 3u;	// ECAP1 - down link detection
	GpioCtrlRegs.GPADIR.bit.GPIO5 = 0u;		// input (don't care as pin is peripheral)

	// GPIO6
	GpioCtrlRegs.GPAPUD.bit.GPIO6 = 1u;		// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 2u;	// EPWMSYNCI - sync pulse from DSP A
	GpioCtrlRegs.GPADIR.bit.GPIO6 = 0u;		// input (don't care as pin is peripheral)

	// GPIO7
	GpioDataRegs.GPACLEAR.bit.GPIO7 = 1u;	// set pin low (LED off)
	GpioCtrlRegs.GPAPUD.bit.GPIO7 	= 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO7 	= 0u;	// GPIO - LED
	GpioCtrlRegs.GPADIR.bit.GPIO7 	= 1u;	// output

	// GPIO8
	GpioDataRegs.GPASET.bit.GPIO8 	= 1u;	// set pin high (Reset the HSB100 modem), reset disabled.
	GpioCtrlRegs.GPAPUD.bit.GPIO8 	= 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO8 	= 0u;	// GPIO - nModem reset
	GpioCtrlRegs.GPADIR.bit.GPIO8 	= 1u;	// output

	// GPIO9
	GpioCtrlRegs.GPAPUD.bit.GPIO9  = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 3u;	// eCap3 input - LTB modem interrupt
	GpioCtrlRegs.GPADIR.bit.GPIO9  = 0u;	// input

	// GPIO10
	GpioDataRegs.GPACLEAR.bit.GPIO10 = 1u;	// bit clear Rx mode.
	GpioCtrlRegs.GPAPUD.bit.GPIO10 	 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO10  = 0u;	// GPIO - modem rs485 driver TXnRX line
	GpioCtrlRegs.GPADIR.bit.GPIO10   = 1u;	// output

	// GPIO11
	GpioCtrlRegs.GPAPUD.bit.GPIO11 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 0u;	// GPIO - not used
	GpioCtrlRegs.GPADIR.bit.GPIO11 = 0u;	// input

	// GPIO12
	GpioCtrlRegs.GPAPUD.bit.GPIO12 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 3u;	// MDXB (output)
	GpioCtrlRegs.GPADIR.bit.GPIO12 = 0u;	// input (don't care as pin is peripheral)

	// GPIO13
	GpioCtrlRegs.GPAPUD.bit.GPIO13 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 3u;	// MDRB (input)
	GpioCtrlRegs.GPADIR.bit.GPIO13 = 0u;	// input (don't care as pin is peripheral)

	// GPIO14
	GpioCtrlRegs.GPAPUD.bit.GPIO14 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 3u;	// MCLKXB (I/O)
	GpioCtrlRegs.GPADIR.bit.GPIO14 = 0u;	// input (don't care as pin is peripheral)

	// GPIO15
	GpioCtrlRegs.GPAPUD.bit.GPIO15 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 3u;	// MFSXB (I/O)
	GpioCtrlRegs.GPADIR.bit.GPIO15 = 0u;	// input (don't care as pin is peripheral)

	// GPIO16
	GpioCtrlRegs.GPAPUD.bit.GPIO16 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 2u;	// CANTXB (output)
	GpioCtrlRegs.GPADIR.bit.GPIO16 = 0u;	// input (don't care as pin is peripheral)

	// GPIO17
	GpioCtrlRegs.GPAPUD.bit.GPIO17 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 2u;	// CANRXB (input)
	GpioCtrlRegs.GPADIR.bit.GPIO17 = 0u;	// input (don't care as pin is peripheral)

	// GPIO18
	GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 2u;	// SCITXDB - RS485 DI (output)
	GpioCtrlRegs.GPADIR.bit.GPIO18 = 0u;	// input (don't care as pin is peripheral)

	// GPIO19
	GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 2u;	// SCIRXDB - RS485 RO (input)
	GpioCtrlRegs.GPADIR.bit.GPIO19 = 0u;	// input

	// GPIO20
	GpioDataRegs.GPACLEAR.bit.GPIO20 = 1u;	// bit clear (set address pin low)
	GpioCtrlRegs.GPAPUD.bit.GPIO20 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0u;	// GPIO - flash address bus A20
	GpioCtrlRegs.GPADIR.bit.GPIO20 = 1u;	// output

	// GPIO21
	GpioDataRegs.GPACLEAR.bit.GPIO21 = 1u;	// bit clear (set address pin low)
	GpioCtrlRegs.GPAPUD.bit.GPIO21 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 0u;	// GPIO - flash address bus A21
	GpioCtrlRegs.GPADIR.bit.GPIO21 = 1u;	// output

	// GPIO22
	GpioDataRegs.GPACLEAR.bit.GPIO22 = 1u;	// bit clear (set address pin low)
	GpioCtrlRegs.GPAPUD.bit.GPIO22 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 0u;	// GPIO - flash address bus A22
	GpioCtrlRegs.GPADIR.bit.GPIO22 = 1u;	// output

	// GPIO23
	GpioDataRegs.GPACLEAR.bit.GPIO23 = 1u;	// bit clear (set address pin low)
	GpioCtrlRegs.GPAPUD.bit.GPIO23 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 0u;	// GPIO - flash address bus A23
	GpioCtrlRegs.GPADIR.bit.GPIO23 = 1u;	// output

	// GPIO24
	GpioDataRegs.GPACLEAR.bit.GPIO24 = 1u;	// bit clear (set address pin low)
	GpioCtrlRegs.GPAPUD.bit.GPIO24 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 0u;	// GPIO - flash address bus A24
	GpioCtrlRegs.GPADIR.bit.GPIO24 = 1u;	// output

	// GPIO25
	GpioDataRegs.GPACLEAR.bit.GPIO25 = 1u;	// bit clear (set address pin low)
	GpioCtrlRegs.GPAPUD.bit.GPIO25 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 0u;	// GPIO - flash address bus A25
	GpioCtrlRegs.GPADIR.bit.GPIO25 = 1u;	// output

	// GPIO26
	GpioDataRegs.GPACLEAR.bit.GPIO26 = 1u;	// bit clear (set address pin low)
	GpioCtrlRegs.GPAPUD.bit.GPIO26 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 0u;	// GPIO - flash address bus A26
	GpioCtrlRegs.GPADIR.bit.GPIO26 = 1u;	// output

	// GPIO27
	GpioCtrlRegs.GPAPUD.bit.GPIO27 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 0u;	// GPIO - low oil switch
	GpioCtrlRegs.GPADIR.bit.GPIO27 = 0u;	// input

	// GPIO28
	GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0u;    // pull-up enabled (needed to stop floating input from generating spurious receiver interrupts).
	GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1u;	// SCIRXDA - debug port (input)
	GpioCtrlRegs.GPADIR.bit.GPIO28 = 0u;	// input (don't care as pin is peripheral)

	// GPIO29
	GpioCtrlRegs.GPAPUD.bit.GPIO29 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 3u;	// XA19 - flash address bus A19
	GpioCtrlRegs.GPADIR.bit.GPIO29 = 0u;	// input (don't care as pin is peripheral)

	// GPIO30
	GpioCtrlRegs.GPAPUD.bit.GPIO30 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 3u;	// XA18 - flash address bus A18
	GpioCtrlRegs.GPADIR.bit.GPIO30 = 0u;	// input (don't care as pin is peripheral)

	// GPIO31
	GpioCtrlRegs.GPAPUD.bit.GPIO31 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 3u;	// XA17 - flash address bus A17
	GpioCtrlRegs.GPADIR.bit.GPIO31 = 0u;	// input (don't care as pin is peripheral)


	// GPIO32
	GpioCtrlRegs.GPBPUD.bit.GPIO32 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 1u;	// SDAA - D&I eeprom serial data
	GpioCtrlRegs.GPBDIR.bit.GPIO32 = 0u;	// input (don't care as pin is peripheral)

	// GPIO33
	GpioCtrlRegs.GPBPUD.bit.GPIO33 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 1u;	// SCLA - D&I eeprom serial clock
	GpioCtrlRegs.GPBDIR.bit.GPIO33 = 0u;	// input (don't care as pin is peripheral)

	// GPIO34
	GpioCtrlRegs.GPBPUD.bit.GPIO34 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0u;	// GPIO - DSP B identifier (tied to 3v3)
	GpioCtrlRegs.GPBDIR.bit.GPIO34 = 0u;	// input

	// GPIO35
	GpioCtrlRegs.GPBPUD.bit.GPIO35 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO35 = 1u;	// SCITXDA - debug port (output)
	GpioCtrlRegs.GPBDIR.bit.GPIO35 = 0u;	// input

	// GPIO36
	GpioDataRegs.GPBCLEAR.bit.GPIO36 = 1u;	// bit clear (output is low)
	GpioCtrlRegs.GPBPUD.bit.GPIO36 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO36 = 0u;	// GPIO - test point
	GpioCtrlRegs.GPBDIR.bit.GPIO36 = 1u;	// output

	// GPIO37
	GpioCtrlRegs.GPBPUD.bit.GPIO37 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO37 = 3u;	// #XZCS7 - flash chip select
	GpioCtrlRegs.GPBDIR.bit.GPIO37 = 0u;	// input (don't care as pin is peripheral)

	// GPIO38
	GpioCtrlRegs.GPBPUD.bit.GPIO38 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO38 = 3u;	// XWE0 - flash chip write strobe
	GpioCtrlRegs.GPBDIR.bit.GPIO38 = 0u;	// input (don't care as pin is peripheral)

	// GPIO39
	GpioCtrlRegs.GPBPUD.bit.GPIO39 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 3u;	// XA16 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO39 = 0u;	// input (don't care as pin is peripheral)

	// GPIO40
	GpioCtrlRegs.GPBPUD.bit.GPIO40 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO40 = 3u;	// XA0 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO40 = 0u;	// input (don't care as pin is peripheral)

	// GPIO41
	GpioCtrlRegs.GPBPUD.bit.GPIO41 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 3u;	// XA1 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO41 = 0u;	// input (don't care as pin is peripheral)

	// GPIO42
	GpioCtrlRegs.GPBPUD.bit.GPIO42 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO42 = 3u;	// XA2 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO42 = 0u;	// input (don't care as pin is peripheral)

	// GPIO43
	GpioCtrlRegs.GPBPUD.bit.GPIO43 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO43 = 3u;	// XA3 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO43 = 0u;	// input (don't care as pin is peripheral)

	// GPIO44
	GpioCtrlRegs.GPBPUD.bit.GPIO44 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO44 = 3u;	// XA4 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO44 = 0u;	// input (don't care as pin is peripheral)

	// GPIO45
	GpioCtrlRegs.GPBPUD.bit.GPIO45 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO45 = 3u;	// XA5 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO45 = 0u;	// input (don't care as pin is peripheral)

	// GPIO46
	GpioCtrlRegs.GPBPUD.bit.GPIO46 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO46 = 3u;	// XA6 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO46 = 0u;	// input (don't care as pin is peripheral)

	// GPIO47
	GpioCtrlRegs.GPBPUD.bit.GPIO47 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX1.bit.GPIO47 = 3u;	// XA7 - flash address bus
	GpioCtrlRegs.GPBDIR.bit.GPIO47 = 0u;	// input (don't care as pin is peripheral)

	// GPIO48
	GpioDataRegs.GPBSET.bit.GPIO48 = 1u;	// bit set (enable loopback mode)
	GpioCtrlRegs.GPBPUD.bit.GPIO48 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO48 = 0u;	// GPIO - CAN loopback
	GpioCtrlRegs.GPBDIR.bit.GPIO48 = 1u;	// output

	// GPIO49
	GpioCtrlRegs.GPBPUD.bit.GPIO49 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO49 = 1u;	// ECAP6 - gamma ray
	GpioCtrlRegs.GPBDIR.bit.GPIO49 = 0u;	// input (don't care as pin is peripheral)

	// GPIO50
	GpioDataRegs.GPBCLEAR.bit.GPIO50 = 1u;	// bit clear (output low, device protected)
	GpioCtrlRegs.GPBPUD.bit.GPIO50 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO50 = 0u;	// GPIO - SPI eeprom write protect
	GpioCtrlRegs.GPBDIR.bit.GPIO50 = 1u;	// output

	// GPIO51
	GpioDataRegs.GPBCLEAR.bit.GPIO51 = 1u;	// bit clear (output low, device protected)
	GpioCtrlRegs.GPBPUD.bit.GPIO51 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO51 = 0u;	// GPIO - flash 1 sector write protect
	GpioCtrlRegs.GPBDIR.bit.GPIO51 = 1u;	// output

	// GPIO52
	GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1u;	// bit clear (output low, device protected)
	GpioCtrlRegs.GPBPUD.bit.GPIO52 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO52 = 0u;	// GPIO - flash 2 sector write protect
	GpioCtrlRegs.GPBDIR.bit.GPIO52 = 1u;	// output

	// GPIO53
	GpioCtrlRegs.GPBPUD.bit.GPIO53 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO53 = 0u;	// GPIO - flash chip 1 ready \ busy
	GpioCtrlRegs.GPBDIR.bit.GPIO53 = 0u;	// input

	// GPIO54
	GpioCtrlRegs.GPBPUD.bit.GPIO54 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO54 = 1u;	// SPISIMOA - SPI eeprom SI
	GpioCtrlRegs.GPBDIR.bit.GPIO54 = 0u;	// input (don't care as pin is peripheral)
//	GpioCtrlRegs.GPAQSEL2.bit.GPIO54 = 3u;	// asynchronous qualification TODO Check this

	// GPIO55
	GpioCtrlRegs.GPBPUD.bit.GPIO55 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO55 = 1u;	// SPISOMIA - SPI eeprom SO
	GpioCtrlRegs.GPBDIR.bit.GPIO55 = 0u;	// input (don't care as pin is peripheral)

	// GPIO56
	GpioCtrlRegs.GPBPUD.bit.GPIO56 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO56 = 1u;	// SPICLKA - SPI eeprom SCLK
	GpioCtrlRegs.GPBDIR.bit.GPIO56 = 0u;	// input (don't care as pin is peripheral)

	// GPIO57
	GpioDataRegs.GPBSET.bit.GPIO57 = 1u;	// set pin high (#CS disabled)
	GpioCtrlRegs.GPBPUD.bit.GPIO57 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO57 = 0u;	// GPIO - SPI eeprom #CS (not using peripheral)
	GpioCtrlRegs.GPBDIR.bit.GPIO57 = 1u;	// output

	// GPIO58
	GpioDataRegs.GPBCLEAR.bit.GPIO58 = 1u;	// set pin low (hold flash chips in reset)
	GpioCtrlRegs.GPBPUD.bit.GPIO58 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 0u;	// GPIO - flash chips RESET
	GpioCtrlRegs.GPBDIR.bit.GPIO58 = 1u;	// output

	// GPIO59
	GpioCtrlRegs.GPBPUD.bit.GPIO59 = 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO59 = 0u;	// GPIO - flash chip 2 ready \ busy
	GpioCtrlRegs.GPBDIR.bit.GPIO59 = 0u;	// input

	// GPIO60
	GpioCtrlRegs.GPBPUD.bit.GPIO60 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO60 = 1u;	// MCLKRB (I/O)
	GpioCtrlRegs.GPBDIR.bit.GPIO60 = 0u;	// input (don't care as pin is peripheral)

	// GPIO61
	GpioCtrlRegs.GPBPUD.bit.GPIO61 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO61 = 1u;	// MFSRB (I/O)
	GpioCtrlRegs.GPBDIR.bit.GPIO61 = 0u;	// input (don't care as pin is peripheral)

	// GPIO62
	GpioCtrlRegs.GPBPUD.bit.GPIO62  = 1u;	// pull-up disabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO62 = 1u;	// GPIO - LTB modem RX
	GpioCtrlRegs.GPBDIR.bit.GPIO62  = 0u;	// input

	// GPIO63
	GpioDataRegs.GPBSET.bit.GPIO63  = 1u;	// set pin high
	GpioCtrlRegs.GPBPUD.bit.GPIO63 	= 0u;	// pull-up enabled
	GpioCtrlRegs.GPBMUX2.bit.GPIO63 = 1u;	// GPIO - LTB modem TX
	GpioCtrlRegs.GPBDIR.bit.GPIO63 	= 1u;	// output


	// GPIO64
	GpioCtrlRegs.GPCPUD.bit.GPIO64 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO64 = 3u;	// XD15 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO64 = 0u;	// input (don't care as pin is peripheral)

	// GPIO65
	GpioCtrlRegs.GPCPUD.bit.GPIO65 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO65 = 3u;	// XD14 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO65 = 0u;	// input (don't care as pin is peripheral)

	// GPIO66
	GpioCtrlRegs.GPCPUD.bit.GPIO66 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO66 = 3u;	// XD13 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO66 = 0u;	// input (don't care as pin is peripheral)

	// GPIO67
	GpioCtrlRegs.GPCPUD.bit.GPIO67 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO67 = 3u;	// XD12 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO67 = 0u;	// input (don't care as pin is peripheral)

	// GPIO68
	GpioCtrlRegs.GPCPUD.bit.GPIO68 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO68 = 3u;	// XD11 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO68 = 0u;	// input (don't care as pin is peripheral)

	// GPIO69
	GpioCtrlRegs.GPCPUD.bit.GPIO69 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO69 = 3u;	// XD10 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO69 = 0u;	// input (don't care as pin is peripheral)

	// GPIO70
	GpioCtrlRegs.GPCPUD.bit.GPIO70 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO70 = 3u;	// XD9 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO70 = 0u;	// input (don't care as pin is peripheral)

	// GPIO71
	GpioCtrlRegs.GPCPUD.bit.GPIO71 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO71 = 3u;	// XD8 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO71 = 0u;	// input (don't care as pin is peripheral)

	// GPIO72
	GpioCtrlRegs.GPCPUD.bit.GPIO72 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO72 = 3u;	// XD7 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO72 = 0u;	// input (don't care as pin is peripheral)

	// GPIO73
	GpioCtrlRegs.GPCPUD.bit.GPIO73 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO73 = 3u;	// XD6 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO73 = 0u;	// input (don't care as pin is peripheral)

	// GPIO74
	GpioCtrlRegs.GPCPUD.bit.GPIO74 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO74 = 3u;	// XD5 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO74 = 0u;	// input (don't care as pin is peripheral)

	// GPIO75
	GpioCtrlRegs.GPCPUD.bit.GPIO75 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO75 = 3u;	// XD4 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO75 = 0u;	// input (don't care as pin is peripheral)

	// GPIO76
	GpioCtrlRegs.GPCPUD.bit.GPIO76 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO76 = 3u;	// XD3 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO76 = 0u;	// input (don't care as pin is peripheral)

	// GPIO77
	GpioCtrlRegs.GPCPUD.bit.GPIO77 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO77 = 3u;	// XD2 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO77 = 0u;	// input (don't care as pin is peripheral)

	// GPIO78
	GpioCtrlRegs.GPCPUD.bit.GPIO78 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO78 = 3u;	// XD1 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO78 = 0u;	// input (don't care as pin is peripheral)

	// GPIO79
	GpioCtrlRegs.GPCPUD.bit.GPIO79 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX1.bit.GPIO79 = 3u;	// XD0 - flash data bus
	GpioCtrlRegs.GPCDIR.bit.GPIO79 = 0u;	// input (don't care as pin is peripheral)

	// GPIO80
	GpioCtrlRegs.GPCPUD.bit.GPIO80 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO80 = 3u;	// XA8 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO80 = 0u;	// input (don't care as pin is peripheral)

	// GPIO81
	GpioCtrlRegs.GPCPUD.bit.GPIO81 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO81 = 3u;	// XA9 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO81 = 0u;	// input (don't care as pin is peripheral)

	// GPIO82
	GpioCtrlRegs.GPCPUD.bit.GPIO82 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO82 = 3u;	// XA10 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO82 = 0u;	// input (don't care as pin is peripheral)

	// GPIO83
	GpioCtrlRegs.GPCPUD.bit.GPIO83 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO83 = 3u;	// XA11 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO83 = 0u;	// input (don't care as pin is peripheral)

	// GPIO84
	GpioCtrlRegs.GPCPUD.bit.GPIO84 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO84 = 3u;	// XA12 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO84 = 0u;	// input (don't care as pin is peripheral)

	// GPIO85
	GpioCtrlRegs.GPCPUD.bit.GPIO85 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO85 = 3u;	// XA13 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO85 = 0u;	// input (don't care as pin is peripheral)

	// GPIO86
	GpioCtrlRegs.GPCPUD.bit.GPIO86 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO86 = 3u;	// XA14 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO86 = 0u;	// input (don't care as pin is peripheral)

	// GPIO87
	GpioCtrlRegs.GPCPUD.bit.GPIO87 = 1u;	// pull-up disabled
	GpioCtrlRegs.GPCMUX2.bit.GPIO87 = 3u;	// XA15 - flash address bus
	GpioCtrlRegs.GPCDIR.bit.GPIO87 = 0u;	// input (don't care as pin is peripheral)

	EDIS;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

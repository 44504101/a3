// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/iocontrolcommon.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		23 Jan 2013
 * @brief		Common functions to control DSP I/O pins on ACQ \ MTC board.
 * @details
 * Functions to control the various I/O pins for the DSP's on the ACQ \ MTC
 * board, for example to reset the ADC or flash an LED.  These functions are
 * common to both DSP, because the I/O for these pins is the same for both.
 *
 * Some of these functions could be placed within a particular driver, but on
 * the whole they don't really 'fit' - for example, the RS485 RE and DE control
 * could go in the SCI driver, but there are often situations where the SCI
 * driver will be used for ports other than RS485.
 * All of the functions in this file use defines for the I/O bits - all the
 * defines are at the top of the code, so it the I/O is changed, it should
 * just be a case of changing the relevant defines.  This makes the I/O
 * functions themselves slightly less readable, unless you're using Eclipse or
 * another editor which automatically expands macros on mouse over, but this
 * is the compromise I've decided on.
 *
 * @warning
 * The GPIO multiplexers must be set up before attempting to call any of these
 * functions, otherwise you may get unexpected results!
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "iocontrolcommon.h"
#include "DSP28335_device.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

//#define RS485_RE_LO		(GpioDataRegs.GPACLEAR.bit.GPIO3 = 1u)      ///< GPIO to set RE pin low.
//#define RS485_RE_HI		(GpioDataRegs.GPASET.bit.GPIO3 = 1u)        ///< GPIO to set RE pin high.
//#define RS485_DE_LO		(GpioDataRegs.GPACLEAR.bit.GPIO4 = 1u)      ///< GPIO to set DE pin low.
//#define RS485_DE_HI		(GpioDataRegs.GPASET.bit.GPIO4 = 1u)        ///< GPIO to set DE pin high.

#define RS485_RE_LO     (GpioDataRegs.GPBCLEAR.bit.GPIO49 = 1u)      ///< GPIO to set RE pin low.
#define RS485_RE_HI     (GpioDataRegs.GPBSET.bit.GPIO49 = 0u)        ///< GPIO to set RE pin high.
#define RS485_DE_LO     (GpioDataRegs.GPBCLEAR.bit.GPIO49 = 0u)      ///< GPIO to set DE pin low.
#define RS485_DE_HI     (GpioDataRegs.GPBSET.bit.GPIO49 = 1u)        ///< GPIO to set DE pin high.

#define CAN_LBK_LO		(GpioDataRegs.GPBCLEAR.bit.GPIO48 = 1u)     ///< GPIO to set loopback pin low.
#define CAN_LBK_HI		(GpioDataRegs.GPBSET.bit.GPIO48 = 1u)       ///< GPIO to set loopback pin high.
#define DSPID_IO		(GpioDataRegs.GPBDAT.bit.GPIO34)            ///< GPIO to read for DSP ID.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * IOCONTROL_RS485ReceiverEnable enables the RS485 receiver, by driving the
 * RE line low.  Note that we don't touch the DE line in here, because there
 * might be a situation where you want to loopback so drive the transmitter
 * and receiver at the same time.
 *
 */
// ----------------------------------------------------------------------------
void IOCONTROLCOMMON_RS485ReceiverEnable(void)
{
	RS485_RE_LO;
}


// ----------------------------------------------------------------------------
/**
 * IOCONTROL_RS485ReceiverDisable disables the RS485 receiver, by driving the
 * RE line high.  Note that we don't touch the DE line in here, because there
 * might be a situation where you want to loopback so drive the transmitter
 * and receiver at the same time.
 *
 */
// ----------------------------------------------------------------------------
void IOCONTROLCOMMON_RS485ReceiverDisable(void)
{
	RS485_RE_HI;
}


// ----------------------------------------------------------------------------
/**
 * IOCONTROL_RS485TransmitterEnable enables the RS485 transmitter, by driving
 * the DE line high.  Note that we don't touch the RE line in here, because
 * there might be a situation where you want to loopback so drive the
 * transmitter and receiver at the same time.
 *
 */
// ----------------------------------------------------------------------------
void IOCONTROLCOMMON_RS485TransmitterEnable(void)
{
	RS485_DE_HI;
}


// ----------------------------------------------------------------------------
/**
 * IOCONTROL_RS485TransmitterDisable disables the RS485 transmitter, by driving
 * the DE line low.  Note that we don't touch the RE line in here, because
 * there might be a situation where you want to loopback so drive the
 * transmitter and receiver at the same time.
 *
 */
// ----------------------------------------------------------------------------
void IOCONTROLCOMMON_RS485TransmitterDisable(void)
{
	RS485_DE_LO;
}


// ----------------------------------------------------------------------------
/**
 * IOCONTROL_CANLoopbackEnable enables the loopback function on the CAN
 * transceiver, by driving the LBK pin high.
 *
 */
// ----------------------------------------------------------------------------
void IOCONTROLCOMMON_CANLoopbackEnable(void)
{
	CAN_LBK_HI;
}


// ----------------------------------------------------------------------------
/**
 * IOCONTROL_CANLoopbackDisable disables the loopback function on the CAN
 * transceiver, by driving the LBK pin low.
 *
 */
// ----------------------------------------------------------------------------
void IOCONTROLCOMMON_CANLoopbackDisable(void)
{
	CAN_LBK_LO;
}


// ----------------------------------------------------------------------------
/**
 * IOCONTROL_DSPIdentifierGet reads the GPIO pin to determine which DSP we are.
 *
 * @retval	EDSPID_t	Enumerated value for DSP A or DSP B.
 *
 */
// ----------------------------------------------------------------------------
EDSPID_t IOCONTROLCOMMON_DSPIdentifierGet(void)
{
	EDSPID_t dspID = DSPID_DSPA;

	if (DSPID_IO == 1u)
	{
		dspID = DSPID_DSPB;
	}

	return dspID;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/DSP28335_device.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Jul 2012
 * @brief		Header file for TI DSP28335 device specific definitions.
 * @note		This header file makes use of UNIT_TEST_BUILD, to allow
 * 				device specific functions (e.g. intrinsics) to be 'overloaded'
 * 				such that the GCC compiler can deal with them and we can test
 * 				the code on a PC.  This file is based on the TI supplied file
 * 				DSP2833x_Device.h, but modified accordingly - this is so we can
 * 				update all of TI's DSP2833x_ files without having to worry about
 * 				'our' changes within the files.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef DSP28335_DEVICE_H_
#define DSP28335_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// User To Select Target Device:

#define   DSP28_28335   1		///< Selects '28335/'28235


//---------------------------------------------------------------------------
// Common CPU Definitions:
//
// If running unit tests then use the following #defines:
#ifdef UNIT_TEST_BUILD
#define cregister				///< Define away cregister keyword.
#define interrupt				///< Define away interrupt keyword.
#define	EINT                    ///< Define away EINT macro (removes assembler code).
#define DINT                    ///< Define away DINT macro (removes assembler code).
#define ERTM                    ///< Define away ERTM macro (removes assembler code).
#define DRTM                    ///< Define away DRTM macro (removes assembler code).
#define	EALLOW                  ///< Define away EALLOW macro (removes assembler code).
#define	EDIS                    ///< Define away EDIS macro (removes assembler code).
#define ESTOP0                  ///< Define away ESTOP0 macro (removes assembler code).
#define NOP                     ///< Define away NOP macro (removes assembler code).
#define asm(x)                  ///< Define away all assembly language.

// otherwise assume we're on the 'proper' target platform,
// so use the following #defines:
#else
#define  EINT   asm(" clrc INTM")
#define  DINT   asm(" setc INTM")
#define  ERTM   asm(" clrc DBGM")
#define  DRTM   asm(" setc DBGM")
#define  EALLOW asm(" EALLOW")
#define  EDIS   asm(" EDIS")
#define  ESTOP0 asm(" ESTOP0")
#define  NOP    asm(" NOP")
#endif

extern cregister volatile Uint16 IFR;       ///< Interrupt flag register.
extern cregister volatile Uint16 IER;       ///< Interrupt enable register.

#define M_INT1  0x0001u         ///< Bit mask for INT1 interrupt.
#define M_INT2  0x0002u         ///< Bit mask for INT2 interrupt.
#define M_INT3  0x0004u         ///< Bit mask for INT3 interrupt.
#define M_INT4  0x0008u         ///< Bit mask for INT4 interrupt.
#define M_INT5  0x0010u         ///< Bit mask for INT5 interrupt.
#define M_INT6  0x0020u         ///< Bit mask for INT6 interrupt.
#define M_INT7  0x0040u         ///< Bit mask for INT7 interrupt.
#define M_INT8  0x0080u         ///< Bit mask for INT8 interrupt.
#define M_INT9  0x0100u         ///< Bit mask for INT9 interrupt.
#define M_INT10 0x0200u         ///< Bit mask for INT10 interrupt.
#define M_INT11 0x0400u         ///< Bit mask for INT11 interrupt.
#define M_INT12 0x0800u         ///< Bit mask for INT12 interrupt.
#define M_INT13 0x1000u         ///< Bit mask for INT13 interrupt.
#define M_INT14 0x2000u         ///< Bit mask for INT14 interrupt.
#define M_DLOG  0x4000u         ///< Bit mask for DLOG interrupt.
#define M_RTOS  0x8000u         ///< Bit mask for RTOS interrupt.

#define BIT0    0x0001u         ///< Bit mask for bit position 0.
#define BIT1    0x0002u         ///< Bit mask for bit position 1.
#define BIT2    0x0004u         ///< Bit mask for bit position 2.
#define BIT3    0x0008u         ///< Bit mask for bit position 3.
#define BIT4    0x0010u         ///< Bit mask for bit position 4.
#define BIT5    0x0020u         ///< Bit mask for bit position 5.
#define BIT6    0x0040u         ///< Bit mask for bit position 6.
#define BIT7    0x0080u         ///< Bit mask for bit position 7.
#define BIT8    0x0100u         ///< Bit mask for bit position 8.
#define BIT9    0x0200u         ///< Bit mask for bit position 9.
#define BIT10   0x0400u         ///< Bit mask for bit position 10.
#define BIT11   0x0800u         ///< Bit mask for bit position 11.
#define BIT12   0x1000u         ///< Bit mask for bit position 12.
#define BIT13   0x2000u         ///< Bit mask for bit position 13.
#define BIT14   0x4000u         ///< Bit mask for bit position 14.
#define BIT15   0x8000u         ///< Bit mask for bit position 15.


//---------------------------------------------------------------------------
// For Portability, User Is Recommended To Use Following Data Type Size
// Definitions For 16-bit and 32-Bit Signed/Unsigned Integers:
//
// User data types can be found in common_data_types.h - this comment just
// left so files can be diff'ed to see what we've changed!


//---------------------------------------------------------------------------
// Include All Peripheral Header Files:
//

#include "DSP2833x_Adc.h"                // ADC Registers
#include "DSP2833x_DevEmu.h"             // Device Emulation Registers
#include "DSP2833x_CpuTimers.h"          // 32-bit CPU Timers
#include "DSP2833x_ECan.h"               // Enhanced eCAN Registers
#include "DSP2833x_ECap.h"               // Enhanced Capture
#include "DSP2833x_DMA.h"                // DMA Registers
#include "DSP2833x_EPwm.h"               // Enhanced PWM
#include "DSP2833x_EQep.h"               // Enhanced QEP
#include "DSP2833x_Gpio.h"               // General Purpose I/O Registers
#include "DSP2833x_I2c.h"                // I2C Registers
#include "DSP2833x_McBSP.h"              // McBSP
#include "DSP2833x_PieCtrl.h"            // PIE Control Registers
#include "DSP2833x_PieVect.h"            // PIE Vector Table
#include "DSP2833x_Spi.h"                // SPI Registers
#include "DSP2833x_Sci.h"                // SCI Registers
#include "DSP2833x_SysCtrl.h"            // System Control/Power Modes
#include "DSP2833x_XIntrupt.h"           // External Interrupts
#include "DSP2833x_Xintf.h"              // XINTF External Interface

#if DSP28_28335
#define DSP28_EPWM1  1                  ///< EPWM1 module is present.
#define DSP28_EPWM2  1                  ///< EPWM2 module is present.
#define DSP28_EPWM3  1                  ///< EPWM3 module is present.
#define DSP28_EPWM4  1                  ///< EPWM4 module is present.
#define DSP28_EPWM5  1                  ///< EPWM5 module is present.
#define DSP28_EPWM6  1                  ///< EPWM6 module is present.
#define DSP28_ECAP1  1                  ///< ECAP1 module is present.
#define DSP28_ECAP2  1                  ///< ECAP2 module is present.
#define DSP28_ECAP3  1                  ///< ECAP3 module is present.
#define DSP28_ECAP4  1                  ///< ECAP4 module is present.
#define DSP28_ECAP5  1                  ///< ECAP5 module is present.
#define DSP28_ECAP6  1                  ///< ECAP6 module is present.
#define DSP28_EQEP1  1                  ///< EQEP1 module is present.
#define DSP28_EQEP2  1                  ///< EQEP2 module is present.
#define DSP28_ECANA  1                  ///< ECANA module is present.
#define DSP28_ECANB  1                  ///< ECANB module is present.
#define DSP28_MCBSPA 1                  ///< MCBSPA module is present.
#define DSP28_MCBSPB 1                  ///< MCBSPB module is present.
#define DSP28_SPIA   1                  ///< SPIA module is present.
#define DSP28_SCIA   1                  ///< SCIA module is present.
#define DSP28_SCIB   1                  ///< SCIB module is present.
#define DSP28_SCIC   1                  ///< SCIC module is present.
#define DSP28_I2CA   1                  ///< I2CA module is present.
#endif  // end DSP28_28335

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* DSP28335_DEVICE_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
/**
 * @file    	interrupts.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		04 Mar 2014
 * @brief		Interrupt code for Xceed loaders, for TI's 28335 DSP.
 * @details
 * Sets up the vector table for the interrupt controller, and enables the
 * various interrupts which we want to use.  Note that the actual ISR functions
 * are contained in the appropriate source files, not in this file, as follows:
 *
 * ISR name							Filename		Mapped To
 * ----------------------------------------------------------------------------
 * FRAME_SynchronisingTick_ISR		frame.c			Group3, interrupt1 (EPWM1)
 * SCI_RxInterruptA_ISR				sci.c			Group9, interrupt1 (SCI-A)
 * SCI_TxInterruptA_ISR				sci.c			Group9, interrupt2 (SCI-A)
 * SCI_RxInterruptB_ISR				sci.c			Group9, interrupt3 (SCI-B)
 * SCI_TxInterruptB_ISR				sci.c			Group9, interrupt4 (SCI-B)
 *
 *
 * @note
 * This code is based on a couple of the TI supplied library files, namely:
 * DSP2833x_DefaultIsr.c
 * DSP2833x_PieVect.c
 *
 * @warning
 * Note that this implementation differs slightly from the TI examples - in the
 * TI examples, the vector table is initialised and then various entries are
 * overwritten with the user required vectors.  In this implementation, the
 * vector table is simply initialised once with the required user vectors.
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
#include "interrupts.h"
#include "frame.h"					// include this for EPWM1 ISR
#include "sci.h"					// include this for SCI ISR's
#include "DSP2833x_DefaultIsr.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

void ResetAllPIEControlRegisters(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module

/// Structure containing the vector table which we want to use.
// Note that we inhibit lint option 960 here - this removes the MISRA warning
// for rule 16.9 - function identifier used without '&' (if we add &'s to each
// of the function names below, we get a different warning for suspicious '&'.
// Any function name without () is automatically converted into a pointer to
// the function, which is what we want to do here.  Note that the lint option
// uses curly brackets, so we only disable it for the next expression.
//lint -e{960}
const struct PIE_VECT_TABLE PieVectTableInit1 = {
      PIE_RESERVED,  // 0  Reserved space
      PIE_RESERVED,  // 1  Reserved space
      PIE_RESERVED,  // 2  Reserved space
      PIE_RESERVED,  // 3  Reserved space
      PIE_RESERVED,  // 4  Reserved space
      PIE_RESERVED,  // 5  Reserved space
      PIE_RESERVED,  // 6  Reserved space
      PIE_RESERVED,  // 7  Reserved space
      PIE_RESERVED,  // 8  Reserved space
      PIE_RESERVED,  // 9  Reserved space
      PIE_RESERVED,  // 10 Reserved space
      PIE_RESERVED,  // 11 Reserved space
      PIE_RESERVED,  // 12 Reserved space

// Non-Peripheral Interrupts
      INT13_ISR,     // XINT13 or CPU-Timer 1
      INT14_ISR,     // CPU-Timer2
      DATALOG_ISR,   // Data logging interrupt
      RTOSINT_ISR,   // RTOS interrupt
      EMUINT_ISR,    // Emulation interrupt
      NMI_ISR,       // Non-maskable interrupt
      ILLEGAL_ISR,   // Illegal operation TRAP
      USER1_ISR,     // User Defined trap 1
      USER2_ISR,     // User Defined trap 2
      USER3_ISR,     // User Defined trap 3
      USER4_ISR,     // User Defined trap 4
      USER5_ISR,     // User Defined trap 5
      USER6_ISR,     // User Defined trap 6
      USER7_ISR,     // User Defined trap 7
      USER8_ISR,     // User Defined trap 8
      USER9_ISR,     // User Defined trap 9
      USER10_ISR,    // User Defined trap 10
      USER11_ISR,    // User Defined trap 11
      USER12_ISR,    // User Defined trap 12

// Group 1 PIE Vectors
      SEQ1INT_ISR,     // 1.1 ADC
      SEQ2INT_ISR,     // 1.2 ADC
      rsvd_ISR,        // 1.3
      XINT1_ISR,       // 1.4
      XINT2_ISR,       // 1.5
      ADCINT_ISR,      // 1.6 ADC
      TINT0_ISR,       // 1.7 Timer 0
      WAKEINT_ISR,     // 1.8 WD, Low Power

// Group 2 PIE Vectors
      EPWM1_TZINT_ISR, // 2.1 EPWM-1 Trip Zone
      EPWM2_TZINT_ISR, // 2.2 EPWM-2 Trip Zone
      EPWM3_TZINT_ISR, // 2.3 EPWM-3 Trip Zone
      EPWM4_TZINT_ISR, // 2.4 EPWM-4 Trip Zone
      EPWM5_TZINT_ISR, // 2.5 EPWM-5 Trip Zone
      EPWM6_TZINT_ISR, // 2.6 EPWM-6 Trip Zone
      rsvd_ISR,        // 2.7
      rsvd_ISR,        // 2.8

// Group 3 PIE Vectors
      FRAME_SynchronisingTick_ISR,  // 3.1 EPWM-1 Interrupt
      EPWM2_INT_ISR,			   	// 3.2 EPWM-2 Interrupt
      EPWM3_INT_ISR,   				// 3.3 EPWM-3 Interrupt
      EPWM4_INT_ISR,   				// 3.4 EPWM-4 Interrupt
      EPWM5_INT_ISR,   				// 3.5 EPWM-5 Interrupt
      EPWM6_INT_ISR,   				// 3.6 EPWM-6 Interrupt
      rsvd_ISR,        				// 3.7
      rsvd_ISR,        				// 3.8

// Group 4 PIE Vectors
      ECAP1_INT_ISR,   // 4.1 ECAP-1
      ECAP2_INT_ISR,   // 4.2 ECAP-2
      ECAP3_INT_ISR,   // 4.3 ECAP-3
      ECAP4_INT_ISR,   // 4.4 ECAP-4
      ECAP5_INT_ISR,   // 4.5 ECAP-5
      ECAP6_INT_ISR,   // 4.6 ECAP-6
      rsvd_ISR,        // 4.7
      rsvd_ISR,        // 4.8

// Group 5 PIE Vectors
      EQEP1_INT_ISR,   // 5.1 EQEP-1
      EQEP2_INT_ISR,   // 5.2 EQEP-2
      rsvd_ISR,        // 5.3
      rsvd_ISR,        // 5.4
      rsvd_ISR,        // 5.5
      rsvd_ISR,        // 5.6
      rsvd_ISR,        // 5.7
      rsvd_ISR,        // 5.8

// Group 6 PIE Vectors
      SPIRXINTA_ISR,   // 6.1 SPI-A
      SPITXINTA_ISR,   // 6.2 SPI-A
      MRINTA_ISR,      // 6.3 McBSP-A
      MXINTA_ISR,      // 6.4 McBSP-A
      MRINTB_ISR,      // 6.5 McBSP-B
      MXINTB_ISR,      // 6.6 McBSP-B
      rsvd_ISR,        // 6.7
      rsvd_ISR,        // 6.8

// Group 7 PIE Vectors
      DINTCH1_ISR,			     	// 7.1  DMA channel 1
      DINTCH2_ISR,					// 7.2  DMA channel 2
      DINTCH3_ISR,     				// 7.3  DMA channel 3
      DINTCH4_ISR,					// 7.4  DMA channel 4
      DINTCH5_ISR,     				// 7.5  DMA channel 5
      DINTCH6_ISR,     				// 7.6  DMA channel 6
      rsvd_ISR,        				// 7.7
      rsvd_ISR,        				// 7.8

// Group 8 PIE Vectors
      I2CINT1A_ISR,    // 8.1  I2C
      I2CINT2A_ISR,    // 8.2  I2C
      rsvd_ISR,        // 8.3
      rsvd_ISR,        // 8.4
      SCIRXINTC_ISR,   // 8.5  SCI-C
      SCITXINTC_ISR,   // 8.6  SCI-C
      rsvd_ISR,        // 8.7
      rsvd_ISR,        // 8.8

// Group 9 PIE Vectors
      SCI_RxInterruptA_ISR,   		// 9.1 SCI-A
      SCI_TxInterruptA_ISR,   		// 9.2 SCI-A
      SCI_RxInterruptB_ISR,			// 9.3 SCI-B
      SCI_TxInterruptB_ISR,			// 9.4 SCI-B
      ECAN0INTA_ISR,   				// 9.5 eCAN-A
      ECAN1INTA_ISR,   				// 9.6 eCAN-A
      ECAN0INTB_ISR,   				// 9.7 eCAN-B
      ECAN1INTB_ISR,   				// 9.8 eCAN-B

// Group 10 PIE Vectors
      rsvd_ISR,        // 10.1
      rsvd_ISR,        // 10.2
      rsvd_ISR,        // 10.3
      rsvd_ISR,        // 10.4
      rsvd_ISR,        // 10.5
      rsvd_ISR,        // 10.6
      rsvd_ISR,        // 10.7
      rsvd_ISR,        // 10.8

// Group 11 PIE Vectors
      rsvd_ISR,        // 11.1
      rsvd_ISR,        // 11.2
      rsvd_ISR,        // 11.3
      rsvd_ISR,        // 11.4
      rsvd_ISR,        // 11.5
      rsvd_ISR,        // 11.6
      rsvd_ISR,        // 11.7
      rsvd_ISR,        // 11.8

// Group 12 PIE Vectors
      XINT3_ISR,       // 12.1
      XINT4_ISR,       // 12.2
      XINT5_ISR,       // 12.3
      XINT6_ISR,       // 12.4
      XINT7_ISR,       // 12.5
      rsvd_ISR,        // 12.6
      LVF_ISR,         // 12.7
      LUF_ISR,         // 12.8
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * INTERRUPTS_PieVectorTableInitialise initialises the vector table, by copying
 * all the entries from the PieVectTableInit structure into the vector table
 * itself.  Note that we inhibit all lint warnings for MISRA compliance in this
 * function  - we are casting away a const and using pointer arithmetic, which
 * MISRA doesn't like, but it works OK so we'll leave it alone.
 *
 * @warning
 * The PIE vector table is not actually enabled here - this is done during the
 * setup of all the other interrupt registers.
 *
*/
// ----------------------------------------------------------------------------
void INTERRUPTS_PieVectorTableInitialise(void)
{
	uint16_t	i;
	uint32_t	*Source = (void *) &PieVectTableInit1;	//lint !e960
	uint32_t	*Dest = (void *) &PieVectTable;

	EALLOW;

	for(i = 0u; i < 128u; i++)
	{
		*Dest++ = *Source++;		//lint !e960 !e961
	}

	EDIS;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * INTERRUPTS_Initialise initialises all required interrupts which are not
 * enabled in other modules - this should be called first to start from a good
 * known state.
 *
 * @warning
 * The PIE vector table MUST be initialised before this function is called.
 *
*/
// ----------------------------------------------------------------------------
void INTERRUPTS_Initialise(void)
{
	// Disable interrupts at the CPU level.
	DINT;

	// Disable the PIE Vector Table.
	PieCtrlRegs.PIECTRL.bit.ENPIE = 0u;

	// Set all PIE control registers to a known state (zero).
	ResetAllPIEControlRegisters();

	// Disable CPU interrupts and clear all CPU interrupt flags.
	// (note that these registers can only be cleared using the AND instruction,
	// so we need to disable the lint warnings for zero given as right hand
	// argument to operator '&')
	IER &= 0x0000u;		//lint !e835
	IFR &= 0x0000u;		//lint !e835

	// 9.1 (SCI-A RX), 9.2 (SCI-A TX), 9.3(SCI-B RX), 9.4(SCI-B TX).
	PieCtrlRegs.PIEIER9.bit.INTx1 = 1u;			// PIE Group 9, interrupt 1
	PieCtrlRegs.PIEIER9.bit.INTx2 = 1u;			// PIE Group 9, interrupt 2
	PieCtrlRegs.PIEIER9.bit.INTx3 = 1u;			// PIE Group 9, interrupt 3
	PieCtrlRegs.PIEIER9.bit.INTx4 = 1u;			// PIE Group 9, interrupt 4

	// Enable the appropriate CPU interrupts for the required peripheral interrupts.
	IER |= M_INT9;

	// Enable the PIE Vector Table
	PieCtrlRegs.PIECTRL.bit.ENPIE = 1u;

	// Enable interrupts at the CPU level
	EINT;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// SHELL ISR ROUTINES FOR DEBUG BELOW HERE - TO BE REMOVED AS CODE PROGRESSES
//
// These are copied from the TI file DSP2833x_DefaultIsr.c, and modified to
// replace all the asm("ESTOP") commands with the macro ESTOP0 (so we can
// #define it away when doing TDD with the GCC compiler).  Also modified to
// #define our for(;;) loops when running under GCC \ TI compiler for TDD
// (otherwise the code just hangs).
//
// Typically these shell ISR routines can be used to populate the entire PIE
// vector table during device debug.  In this manner if an interrupt is taken
// during firmware development, there will always be an ISR to catch it.
//
// As development progresses, these ISR routines can be eliminated and replaced
// with the user's own ISR routines for each interrupt.  Since these shell ISRs
// include infinite loops they will typically not be included as-is in the final
// production firmware.
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Connected to INT13 of CPU (use MINT13 mask):
// Note CPU-Timer1 is reserved for TI use, however XINT13
// ISR can be used by the user.
interrupt void INT13_ISR(void)     // INT13 or CPU-Timer1
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// Note CPU-Timer2 is reserved for TI use.
interrupt void INT14_ISR(void)     // CPU-Timer2
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void DATALOG_ISR(void)   // Datalogging interrupt
{
   // Insert ISR Code here

   // Next two lines for debug only to halt the processor here
   // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void RTOSINT_ISR(void)   // RTOS interrupt
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void EMUINT_ISR(void)    // Emulation interrupt
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void NMI_ISR(void)       // Non-maskable interrupt
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void ILLEGAL_ISR(void)   // Illegal operation TRAP
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER1_ISR(void)     // User Defined trap 1
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER2_ISR(void)     // User Defined trap 2
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER3_ISR(void)     // User Defined trap 3
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER4_ISR(void)     // User Defined trap 4
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER5_ISR(void)     // User Defined trap 5
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER6_ISR(void)     // User Defined trap 6
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER7_ISR(void)     // User Defined trap 7
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER8_ISR(void)     // User Defined trap 8
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER9_ISR(void)     // User Defined trap 9
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER10_ISR(void)    // User Defined trap 10
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER11_ISR(void)    // User Defined trap 11
{
  // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void USER12_ISR(void)     // User Defined trap 12
{
 // Insert ISR Code here

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// -----------------------------------------------------------
// PIE Group 1 - MUXed into CPU INT1
// -----------------------------------------------------------

// INT1.1
interrupt void SEQ1INT_ISR(void)   //SEQ1 ADC
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT1.2
interrupt void SEQ2INT_ISR(void)  //SEQ2 ADC
{

  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}
// INT1.3 - Reserved

// INT1.4
interrupt void  XINT1_ISR(void)
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT1.5
interrupt void  XINT2_ISR(void)
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT1.6
interrupt void  ADCINT_ISR(void)     // ADC
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT1.7
interrupt void  TINT0_ISR(void)      // CPU-Timer 0
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}


// INT1.8
interrupt void  WAKEINT_ISR(void)    // WD, LOW Power
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}


// -----------------------------------------------------------
// PIE Group 2 - MUXed into CPU INT2
// -----------------------------------------------------------

// INT2.1
interrupt void EPWM1_TZINT_ISR(void)    // EPWM-1
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT2.2
interrupt void EPWM2_TZINT_ISR(void)    // EPWM-2
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT2.3
interrupt void EPWM3_TZINT_ISR(void)    // EPWM-3
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}


// INT2.4
interrupt void EPWM4_TZINT_ISR(void)    // EPWM-4
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}


// INT2.5
interrupt void EPWM5_TZINT_ISR(void)    // EPWM-5
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT2.6
interrupt void EPWM6_TZINT_ISR(void)   // EPWM-6
{
  // Insert ISR Code here


  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT2.7 - Reserved
// INT2.8 - Reserved

// -----------------------------------------------------------
// PIE Group 3 - MUXed into CPU INT3
// -----------------------------------------------------------
// INT3.2
interrupt void EPWM2_INT_ISR(void)    // EPWM-3
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT3.3
interrupt void EPWM3_INT_ISR(void)    // EPWM-3
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT3.4
interrupt void EPWM4_INT_ISR(void)    // EPWM-4
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT3.5
interrupt void EPWM5_INT_ISR(void)    // EPWM-5
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT3.6
interrupt void EPWM6_INT_ISR(void)    // EPWM-6
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT3.7 - Reserved
// INT3.8 - Reserved


// -----------------------------------------------------------
// PIE Group 4 - MUXed into CPU INT4
// -----------------------------------------------------------

// INT 4.1
interrupt void ECAP1_INT_ISR(void)    // ECAP-1
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT4.2
interrupt void ECAP2_INT_ISR(void)    // ECAP-2
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT4.3
interrupt void ECAP3_INT_ISR(void)    // ECAP-3
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT4.4
interrupt void ECAP4_INT_ISR(void)     // ECAP-4
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT4.5
interrupt void ECAP5_INT_ISR(void)     // ECAP-5
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}
// INT4.6
interrupt void ECAP6_INT_ISR(void)     // ECAP-6
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}
// INT4.7 - Reserved
// INT4.8 - Reserved

// -----------------------------------------------------------
// PIE Group 5 - MUXed into CPU INT5
// -----------------------------------------------------------

// INT 5.1
interrupt void EQEP1_INT_ISR(void)    // EQEP-1
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP5;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT5.2
interrupt void EQEP2_INT_ISR(void)    // EQEP-2
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP5;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT5.3 - Reserved
// INT5.4 - Reserved
// INT5.5 - Reserved
// INT5.6 - Reserved
// INT5.7 - Reserved
// INT5.8 - Reserved

// -----------------------------------------------------------
// PIE Group 6 - MUXed into CPU INT6
// -----------------------------------------------------------

// INT6.1
interrupt void SPIRXINTA_ISR(void)    // SPI-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT6.2
interrupt void SPITXINTA_ISR(void)     // SPI-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT6.3
interrupt void MRINTB_ISR(void)     // McBSP-B
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT6.4
interrupt void MXINTB_ISR(void)     // McBSP-B
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT6.5
interrupt void MRINTA_ISR(void)     // McBSP-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT6.6
interrupt void MXINTA_ISR(void)     // McBSP-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP6;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT6.7 - Reserved
// INT6.8 - Reserved




// -----------------------------------------------------------
// PIE Group 7 - MUXed into CPU INT7
// -----------------------------------------------------------

// INT7.1
interrupt void DINTCH1_ISR(void)     // DMA
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP7;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT7.2
interrupt void DINTCH2_ISR(void)     // DMA
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP7;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT7.3
interrupt void DINTCH3_ISR(void)     // DMA
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP7;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT7.4
interrupt void DINTCH4_ISR(void)     // DMA
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP7;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT7.5
interrupt void DINTCH5_ISR(void)     // DMA
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP7;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT7.6
interrupt void DINTCH6_ISR(void)     // DMA
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP7;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT7.7 - Reserved
// INT7.8 - Reserved

// -----------------------------------------------------------
// PIE Group 8 - MUXed into CPU INT8
// -----------------------------------------------------------

// INT8.1
interrupt void I2CINT1A_ISR(void)     // I2C-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT8.2
interrupt void I2CINT2A_ISR(void)     // I2C-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT8.3 - Reserved
// INT8.4 - Reserved

// INT8.5
interrupt void SCIRXINTC_ISR(void)     // SCI-C
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT8.6
interrupt void SCITXINTC_ISR(void)     // SCI-C
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT8.7 - Reserved
// INT8.8 - Reserved


// -----------------------------------------------------------
// PIE Group 9 - MUXed into CPU INT9
// -----------------------------------------------------------

// INT9.5
interrupt void ECAN0INTA_ISR(void)  // eCAN-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT9.6
interrupt void ECAN1INTA_ISR(void)  // eCAN-A
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT9.7
interrupt void ECAN0INTB_ISR(void)  // eCAN-B
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT9.8
interrupt void ECAN1INTB_ISR(void)  // eCAN-B
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// -----------------------------------------------------------
// PIE Group 10 - MUXed into CPU INT10
// -----------------------------------------------------------

// INT10.1 - Reserved
// INT10.2 - Reserved
// INT10.3 - Reserved
// INT10.4 - Reserved
// INT10.5 - Reserved
// INT10.6 - Reserved
// INT10.7 - Reserved
// INT10.8 - Reserved


// -----------------------------------------------------------
// PIE Group 11 - MUXed into CPU INT11
// -----------------------------------------------------------

// INT11.1 - Reserved
// INT11.2 - Reserved
// INT11.3 - Reserved
// INT11.4 - Reserved
// INT11.5 - Reserved
// INT11.6 - Reserved
// INT11.7 - Reserved
// INT11.8 - Reserved

// -----------------------------------------------------------
// PIE Group 12 - MUXed into CPU INT12
// -----------------------------------------------------------

// INT12.1
interrupt void XINT3_ISR(void)  // External Interrupt
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT12.2
interrupt void XINT4_ISR(void)  // External Interrupt
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT12.3
interrupt void XINT5_ISR(void)  // External Interrupt
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT12.4
interrupt void XINT6_ISR(void)  // External Interrupt
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT12.5
interrupt void XINT7_ISR(void)  // External Interrupt
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT12.6 - Reserved
// INT12.7
interrupt void LVF_ISR(void)  // Latched overflow
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

// INT12.8
interrupt void LUF_ISR(void)  // Latched underflow
{
  // Insert ISR Code here

  // To receive more interrupts from this PIE group, acknowledge this interrupt
  // PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;

  // Next two lines for debug only to halt the processor here
  // Remove after inserting ISR Code
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

//---------------------------------------------------------------------------
// Catch All Default ISRs:
//

interrupt void PIE_RESERVED(void)  // Reserved space.  For test.
{
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}

interrupt void rsvd_ISR(void)      // For test
{
#if !(defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER))
  ESTOP0;
  for(;;)
  {
	  ;
  }
#endif
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * ResetAllPIEControlRegisters zeroes the PIEIERx and PIEIFRx registers.
 *
 * @warning
 * Interrupts must have been disabled when this function is called, as we're
 * directly modifying the PIEIFRx registers - this is not recommended by TI,
 * but during initialisation we want to start from a known state.
 */
// ----------------------------------------------------------------------------
void ResetAllPIEControlRegisters(void)
{
	// Clear all PIEIER registers:
	PieCtrlRegs.PIEIER1.all = 0u;
	PieCtrlRegs.PIEIER2.all = 0u;
	PieCtrlRegs.PIEIER3.all = 0u;
	PieCtrlRegs.PIEIER4.all = 0u;
	PieCtrlRegs.PIEIER5.all = 0u;
	PieCtrlRegs.PIEIER6.all = 0u;
	PieCtrlRegs.PIEIER7.all = 0u;
	PieCtrlRegs.PIEIER8.all = 0u;
	PieCtrlRegs.PIEIER9.all = 0u;
	PieCtrlRegs.PIEIER10.all = 0u;
	PieCtrlRegs.PIEIER11.all = 0u;
	PieCtrlRegs.PIEIER12.all = 0u;

	// Clear all PIEIFR registers:
	PieCtrlRegs.PIEIFR1.all = 0u;
	PieCtrlRegs.PIEIFR2.all = 0u;
	PieCtrlRegs.PIEIFR3.all = 0u;
	PieCtrlRegs.PIEIFR4.all = 0u;
	PieCtrlRegs.PIEIFR5.all = 0u;
	PieCtrlRegs.PIEIFR6.all = 0u;
	PieCtrlRegs.PIEIFR7.all = 0u;
	PieCtrlRegs.PIEIFR8.all = 0u;
	PieCtrlRegs.PIEIFR9.all = 0u;
	PieCtrlRegs.PIEIFR10.all = 0u;
	PieCtrlRegs.PIEIFR11.all = 0u;
	PieCtrlRegs.PIEIFR12.all = 0u;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

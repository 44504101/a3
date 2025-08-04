// ----------------------------------------------------------------------------
/**
 * @file    	tool_specific_hardware.c
 * @author		leig
 * @brief		hardware functions for 28335.
 *
 */
// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include <string.h>
#include "common_data_types.h"
#include "DSP28335_device.h"            // need this for DINT macro.
#include "loader_state.h"
#include "timer.h"
#include "comm.h"
#include "serial_comm.h"
#include "tool_specific_hardware.h"
#include "tool_specific_config.h"
#include "clocks.h"
#include "dspflash.h"
#include "frame.h"
#include "gpiomux.h"
#include "interrupts.h"
#include "iocontrolcommon.h"
#include "pwm.h"
#include "sci.h"
#include "testpoints.h"
#include "testpointoffsets.h"
#include "watchdog.h"
#include "spi.h"
#include "i2c.h"
#include "xintfconfig.h"
#include "m95.h"
#include "iocontrol.h"

#define FLASH
#define I_AM_THE_BOOTLOADER
#ifdef I_AM_THE_BOOTLOADER
#include "prom_memory_access.h"
#include "Flash2833x_API_Library.h"
#elif defined I_AM_THE_PROMLOADER
#include "Flash2833x_API_Library.h"
#else
#error "Code doesn't know whether it's the bootloader or promloader"
#endif	


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

#define	HALT_FOR_TEST	for(;;){;}	///< infinite loop for halting under test
#define MAX_DEBUG_BUFFER_RX_SIZE	128u	// For receive interrupt to use.
#define MAX_SSB_BUFFER_RX_SIZE		513u	// For receive interrupt to use (max SSB length plus 1 for NULL).

#ifdef FLASH
extern uint16_t	RamfuncsLoadStart;
extern uint16_t	RamfuncsLoadEnd;
extern uint16_t	RamfuncsRunStart;
#endif


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static void SSBRxBufferInitialise(void);
static void DebugRxBufferInitialise(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module.
//
// The receive buffers are temporary buffers which the RX interrupts use
// and which are then read from by the main code - these buffers need to be
// big enough to store an entire message, just in case the main code can't
// read any characters before the whole message is received.
// These buffers are declared as having module scope to avoid putting them on
// the stack, to make the memory usage more predictable (easier to work out).

static char_t  mDebugReceiveInterruptBuffer[MAX_DEBUG_BUFFER_RX_SIZE];
static char_t  mSSBReceiveInterruptBuffer[MAX_SSB_BUFFER_RX_SIZE];
static char_t  mSSBTransmitBuffer[2];

void InitScibGpio_test();
void InitScibGpio_test()
{
   EALLOW;

/* Enable internal pull-up for the selected pins */
// Pull-ups can be enabled or disabled disabled by the user.
// This will enable the pullups for the specified pins.
// Comment out other unwanted lines.

//  GpioCtrlRegs.GPAPUD.bit.GPIO9 = 0;     // Enable pull-up for GPIO9  (SCITXDB)
//   GpioCtrlRegs.GPAPUD.bit.GPIO14 = 0;    // Enable pull-up for GPIO14 (SCITXDB)
//  GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;    // Enable pull-up for GPIO18 (SCITXDB)
  GpioCtrlRegs.GPAPUD.bit.GPIO22 = 0;    // Enable pull-up for GPIO22 (SCITXDB)


//  GpioCtrlRegs.GPAPUD.bit.GPIO11 = 0;    // Enable pull-up for GPIO11 (SCIRXDB)
//    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 0;    // Enable pull-up for GPIO15 (SCIRXDB)
//    GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0;      // Enable pull-up for GPIO19 (SCIRXDB)
  GpioCtrlRegs.GPAPUD.bit.GPIO23 = 0;    // Enable pull-up for GPIO23 (SCIRXDB)

/* Set qualification for selected pins to asynch only */
// This will select asynch (no qualification) for the selected pins.
// Comment out other unwanted lines.

//  GpioCtrlRegs.GPAQSEL1.bit.GPIO11 = 3;  // Asynch input GPIO11 (SCIRXDB)
//    GpioCtrlRegs.GPAQSEL1.bit.GPIO15 = 3;  // Asynch input GPIO15 (SCIRXDB)
//  GpioCtrlRegs.GPAQSEL2.bit.GPIO19 = 3;  // Asynch input GPIO19 (SCIRXDB)
  GpioCtrlRegs.GPAQSEL2.bit.GPIO23 = 3;  // Asynch input GPIO23 (SCIRXDB)

/* Configure SCI-B pins using GPIO regs*/
// This specifies which of the possible GPIO pins will be SCI functional pins.
// Comment out other unwanted lines.

//  GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 2;    // Configure GPIO9 for SCITXDB operation
//    GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 2;   // Configure GPIO14 for SCITXDB operation
//  GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 2;   // Configure GPIO18 for SCITXDB operation
  GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 3;   // Configure GPIO22 for SCITXDB operation

//  GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 2;   // Configure GPIO11 for SCIRXDB operation
//    GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 2;   // Configure GPIO15 for SCIRXDB operation
//    GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 2;   // Configure GPIO19 for SCIRXDB operation
  GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 3;   // Configure GPIO23 for SCIRXDB operation

    EDIS;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_Initialise sets up the processor itself.
 * All hardware initialisation should go here - this function is the first
 * thing which main.c calls.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_Initialise(void)
{
	EClocksFailureModes_t		ClockStatus;
	bool_t						WatchdogDisabledOk;
	bool_t						BaudRateSetupOk;

#ifdef FLASH
	DSPFlash_t					DSPFlashStructure;
#endif

	// Disable the watchdog TODO Need to use the watchdog, just not yet
	WatchdogDisabledOk = WATCHDOG_Disable();

	// If watchdog cannot be disabled then just halt here - put a breakpoint here.
	if (WatchdogDisabledOk == FALSE)
	{
		HALT_FOR_TEST;	//lint !e527 !e960
	}

	// Only used if running from FLASH.
#ifdef FLASH
	// Copy functions which have to run from ram from the flash to the ram.
//	DSPFLASH_MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
//	DSPFLASH_MemCopy(&Flash28_API_LoadStart, &Flash28_API_LoadEnd,&Flash28_API_RunStart);
//	DSPFlashStructure.FlashPageWait = 5u;
//	DSPFlashStructure.FlashRandomWait = 5u;
//	DSPFlashStructure.OTPWait = 8u;
//
//	// Setup wait states for internal DSP flash.
//	DSPFLASH_Initialise(&DSPFlashStructure);
#endif	// Flash

	// Setup PLL - 14.7456 MHz oscillator, and we want 58.9824 MHz SYSCLOCK
//	ClockStatus = CLOCKS_PLLSetup((uint32_t)14745600u, PLL_TIMES_8, CLOCK_DIVIDE_BY_2);
	ClockStatus = CLOCKS_PLLSetup((uint32_t)30000000u, PLL_TIMES_10, CLOCK_DIVIDE_BY_2);

	// If PLL cannot be setup then just halt here - put a breakpoint here.
	if (ClockStatus != PLL_SETUP_OK)
	{
		HALT_FOR_TEST;	//lint !e527 !e960
	}

	// Setup peripheral clocks and prescalers
	CLOCKS_PeripheralClocksAllDisable();
	CLOCKS_PeripheralLowSpeedPrescalerSet(0x02);
	CLOCKS_PeripheralHighSpeedPrescalerSet(0x01);
	CLOCKS_PeripheralClocksEnable(EPWM1_CLOCK);		// used for timer
	CLOCKS_PeripheralClocksEnable(SCI_A_CLOCK);		// debug port
	CLOCKS_PeripheralClocksEnable(SCI_B_CLOCK);		// rs485 port
    CLOCKS_PeripheralClocksEnable(GPIO_CLOCK);      // need this to be able to read back GPIO pins.
//	CLOCKS_PeripheralClocksEnable(SPI_A_CLOCK);     // SPI port  雷戈 2022/6/13 添加
//    CLOCKS_PeripheralClocksEnable(XINTF_CLOCK);     // XINTF  雷戈 2022/6/14 添加
//    CLOCKS_PeripheralClocksEnable(I2C_A_CLOCK);     // I2C  雷戈 2022/6/14 添加

	// Open debug port (serial port A), set the baud rate to 921600
	// and setup the receive buffer which is used by the receive interrupt.
//	SCI_Open(SCI_A);
////	BaudRateSetupOk = SCI_BaudRateSet(SCI_A, (uint32_t)58982400u, 921600u);
//	BaudRateSetupOk = SCI_BaudRateSet(SCI_A, (uint32_t)37500000u, 57600u);
//	DebugRxBufferInitialise();

	// If baud rate cannot be setup then just halt here - put a breakpoint here.
//	if (BaudRateSetupOk == FALSE)
//	{
//		HALT_FOR_TEST;	//lint !e527 !e960
//	}

    // leig add
    InitScibGpio_test();
    // leig add init 485 io
    EALLOW;
    GpioCtrlRegs.GPBPUD.bit.GPIO49 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO49 = 1;
    GpioDataRegs.GPBCLEAR.bit.GPIO49 = 1;
    EDIS;

	// Open rs485 port (serial port B), set the baud rate to 57600
	// and setup the receive buffer which is used by the receive interrupt.
	SCI_Open(SCI_B);
//	BaudRateSetupOk = SCI_BaudRateSet(SCI_B, (uint32_t)58982400u, (uint32_t)57600u);
	BaudRateSetupOk = SCI_BaudRateSet(SCI_B, (uint32_t)37500000u, (uint32_t)57600u);
	SSBRxBufferInitialise();

	// If baud rate cannot be setup then just halt here - put a breakpoint here.
	if (BaudRateSetupOk == FALSE)
	{
		HALT_FOR_TEST;	//lint !e527 !e960
	}

//	// 添加SPI总线打开函数  2022/6/13
//    // Setup the SPI port for 8 bit operation and 2MHz clock.
//    SPI_Open(8u);
//    (void)SPI_BaudRateSet((uint32_t)58982400u, 2000000u);
//
//    // Setup the I2C port for 10kHz operation.
//    (void)I2C_Open((uint32_t)58982400u, (uint32_t)10000u);   //雷戈添加
//
//    XINTFCONFIG_Initialise();                        //雷戈添加   2022/06/14

    // Initialise the GPIO now we've setup all of the peripherals.
//    GPIOMUX_Initialise();

    // Release flash chips from reset now we've started up.
    IOCONTROL_FlashReleaseFromReset();
    IOCONTROL_Flash1WriteProtectDisable();
    IOCONTROL_Flash2WriteProtectDisable();

    // Setup SPI flash size as 128 byte pages, 64k in total.
//    M95_DeviceSizeInitialise((uint32_t)128u, 65536u);

    // Enable writes to the SPI flash.
//    IOCONTROL_SPIWriteProtectDisable();

	// Initialise the PIE vector table and some of the required interrupts.
	// Remember to do this before setting up any other interrupts.
	INTERRUPTS_PieVectorTableInitialise();
	INTERRUPTS_Initialise();

	// Set the prescaler (to control the LED flash rate)
	// and reset the core timer (used for various communications timeouts).
#ifdef I_AM_THE_BOOTLOADER	
	(void)FRAME_FrameTimerPrescalerSet(100u);
#elif defined I_AM_THE_PROMLOADER
	(void)FRAME_FrameTimerPrescalerSet(500u);
#endif

	FRAME_CoreTimerReset();

	// Setup the various PWMs and associated interrupts.
	// Remember that the PWMs are running but the outputs are disabled.
	PWM_Initialise();

	// Start the frame timer - do this after setting up the PWM.
	PWM_FrameEnable();

	// Setup SSB slave address and test point array depending on which DSP we are.
//	if (IOCONTROLCOMMON_DSPIdentifierGet() == DSPID_DSPA)
//	{
//		serial_SlaveAddressSet(ISB_SLAVE_ADDRESS, BUS_SSB);
//		(void)TESTPOINTS_Initialise(TP_OFFSET_MAIN_LED, 82u);		// GPIO82 - main LED for DSP A.
//#ifdef I_AM_THE_PROMLOADER
//		(void)TESTPOINTS_Initialise(TP_OFFSET_FLASH_ERASE, 85u);	// GPIO85 - TP69
//		(void)TESTPOINTS_Initialise(TP_OFFSET_FLASH_PROGRAM, 86u);	// GPIO31 - TP68
//#endif
//	}
//	else
//	{
	    /* Two slave addresses are configured in order for the bootloader/
	     * promloader to support both the Xceed (on XPB) and Xcel products.
	     * Note that the Xceed product normally operates at 9600 baud, but
	     * for the common loader the bootloader and promloader operate at
	     * 57600 baud (the Xceed firmware continues to operate at 9600 baud).
	     */
		serial_SlaveAddressSet(SSB_SLAVE_ADDRESS, BUS_SSB);
		serial_AltSlaveAddressSet(SSB_SLAVE_ADDRESS, BUS_SSB);
//		(void)TESTPOINTS_Initialise(TP_OFFSET_MAIN_LED, 07u);		// GPIO07 - main LED for DSP B.
//	}

#ifdef FLASH
	EALLOW;
    // Setup scale factor for flash programming and erase operations.
    // Set callback pointer to NULL for the moment.
    Flash_CPUScaleFactor = SCALE_FACTOR;
    Flash_CallbackPtr = NULL;
    EDIS;
#endif
	// Turn main LED on once the initialisation has finished.
	// So, if LED doesn't come on, it's indicative of some kind of serious
	// hardware error - no watchdog, PLL can't start etc.
//	TESTPOINTS_Set(TP_OFFSET_MAIN_LED);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_TimerDisableAndReset is called from the main code
 * when the bootloader wants to try and boot the main application.  This
 * function needs to disable any timers and associated interrupts, so there
 * is no chance of one occurring when the new application has just booted, or
 * is just about to boot.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_TimerDisableAndReset(void)
{
	PWM_FrameDisable();
	PWM_DisableAll();
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_TimerRawTimeGet reads the hardware specific timer and
 * returns the result.
 *
 * @retval  Uint32		Current value of hardware specific timer.
 *
 */
// ----------------------------------------------------------------------------
Uint32 ToolSpecificHardware_TimerRawTimeGet(void)
{
    return FRAME_CoreTimerGet();
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_SSBTransmitDisable disables the SSB transmitter, and
 * enables the receiver.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_SSBTransmitDisable(void)
{
	IOCONTROLCOMMON_RS485TransmitterDisable();
	IOCONTROLCOMMON_RS485ReceiverEnable();
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ISBTransmitDisable does nothing, as there is no ISB
 * port on the Xceed board (SSB only).
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_ISBTransmitDisable(void)
{
    ;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_SSBTransmitEnable disables the SSB receiver, and
 * enables the transmitter.  We also reset the receive interrupt buffer in
 * here - as the SSB is single duplex, it is impossible to receive and transmit
 * at the same time, so the buffer can be reset here.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_SSBTransmitEnable(void)
{
	IOCONTROLCOMMON_RS485ReceiverDisable();
	IOCONTROLCOMMON_RS485TransmitterEnable();
	SSBRxBufferInitialise();
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ISBTransmitTransmit does nothing, as there is no ISB
 * port on the Xceed board (SSB only).
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_ISBTransmitEnable(void)
{
    ;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_CANInterruptDisable does nothing - the Xceed board
 * doesn't use the CAN.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_CANInterruptDisable(void)
{
	;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_CPUReset performs a CPU reset, using the watchdog.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_CPUReset(void)
{
	WATCHDOG_ForceSoftwareReset();
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ApplicationExecute boots the application, by jumping
 * to the execution address which is passed into the function.
 *
 * @param   ExecutionAddress	Address to jump to.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{586} -e{715} Asm keyword is deprecated, symbol not referenced (Lint doesn't know about asm).
void ToolSpecificHardware_ApplicationExecute(void* ExecutionAddress)
{
    // Need to disable ALL interrupts
    DINT;

    // Disable CPU interrupts and clear all CPU interrupt flags.
	// (note that these registers can only be cleared using the AND instruction,
	// so we need to disable the lint warnings for zero given as right hand
	// argument to operator '&')
	IER &= 0x0000u;		//lint !e835
	IFR &= 0x0000u;		//lint !e835

    // Execution address should be in XAR4.
    // Move XAR4 into XAR7 using the stack, before executing long branch
    // instruction (indirect long branch can only use *XAR7).
    asm(" PUSH XAR4");
    asm(" POP XAR7");
    asm(" LB *XAR7");
    asm(" RPT #3 || NOP");
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_SSBPortCharacterReceiveReadOnce reads from the SSB
 * serial port (SCI B) once (doesn't poll waiting for a character).
 *
 * @param   pData		Pointer to location to put any read character into.
 * @retval  bool_t		TRUE if character read, FALSE if not read or error.
 *
 */
// ----------------------------------------------------------------------------
bool_t ToolSpecificHardware_SSBPortCharacterReceiveReadOnce(unsigned char* pData)
{
	static uint16_t		PreviousNumberOfCharacters = 0u;
	uint16_t			CurrentNumberOfCharacters;
	bool_t				bFoundACharacter = FALSE;

	CurrentNumberOfCharacters = SCI_RxBufferNumberOfCharsGet(SCI_B);

	// If there are no characters in the receive buffer then reset the
	// previous counter - we're waiting for something to happen.
	if (CurrentNumberOfCharacters == 0u)
	{
		PreviousNumberOfCharacters = 0u;
	}
	else
	{
		// If we've received one (or more) characters then read one from
		// the receive buffer.  Any other characters will simply be buffered
		// in the interrupt buffer until they're read.
		if (CurrentNumberOfCharacters > PreviousNumberOfCharacters)
		{
			bFoundACharacter = TRUE;
			*pData = mSSBReceiveInterruptBuffer[PreviousNumberOfCharacters];
			PreviousNumberOfCharacters++;
		}
	}

	return bFoundACharacter;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ISBPortCharacterReceiveReadOnce does nothing, as there
 * is no ISB port on the Xceed board, so we always just return FALSE.
 *
 * @param   pData       Pointer to location to put any read character into.
 * @retval  bool_t      TRUE if character read, FALSE if not read or error.
 *
 */
// ----------------------------------------------------------------------------
bool_t ToolSpecificHardware_ISBPortCharacterReceiveReadOnce(unsigned char* pData)
{
    return FALSE;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_SSBPortWaitForSendComplete waits until the transmit
 * interrupt has finished transmitting whatever it was dealing with.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_SSBPortWaitForSendComplete(void)
{
	while (SCI_TxDoneCheck(SCI_B) == FALSE)
	{
		;
	}
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ISBPortWaitForSendComplete does nothing, as there is no
 * ISB port on the Xceed board.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_ISBPortWaitForSendComplete(void)
{
    ;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_SSBPortByteSend sends a byte of data, using the transmit
 * interrupt, sending 1 character at a time!  This is not very efficient, but
 * is the simplest way of combining the SDRM code with the Xceed common code.
 *
 * @param   data	Data byte to send.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_SSBPortByteSend(unsigned char data)
{
	// Wait for transmit buffer to be ready for another character.
	ToolSpecificHardware_SSBPortWaitForSendComplete();

	// Put single character into the transmit buffer and then send it.
	mSSBTransmitBuffer[0] = data;
	SCI_TxStart(SCI_B, (uint8_t*)mSSBTransmitBuffer, 1u);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ISBPortByteSend does nothing, as there is no ISB port
 * on the Xceed board.
 *
 * @param   data    Data byte to send.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_ISBPortByteSend(unsigned char data)
{
    ;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_SSBPortCharacterReceiveByPolling polls the SSB port
 * looking for a character (until the timer times out).
 *
 * @param   pData	Pointer to buffer to put received character in.
 * @param	pTimer	Pointer to timer structure to check against for timeout.
 * @retval  bool_t	TRUE if character received, FALSE if error or timeout.
 *
 */
// ----------------------------------------------------------------------------
bool_t ToolSpecificHardware_SSBPortCharacterReceiveByPolling(unsigned char *pData, Timer_t* pTimer)
{
	bool_t	bTimerHasTimedOut = FALSE;
	bool_t	bCharacterReceived = FALSE;

	// While nothing has been received, and no error and no timeout just wait.
	while ( (bTimerHasTimedOut == FALSE) && (bCharacterReceived == FALSE) )
	{
		bTimerHasTimedOut = Timer_TimerExpiredCheck(pTimer);
		bCharacterReceived = ToolSpecificHardware_SSBPortCharacterReceiveReadOnce(pData);
	}

	return bCharacterReceived;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ISBPortCharacterReceiveByPolling does nothing, as there
 * is no ISB port on the Xceed board, so we just return FALSE.
 *
 * @param   pData   Pointer to buffer to put received character in.
 * @param   pTimer  Pointer to timer structure to check against for timeout.
 * @retval  bool_t  TRUE if character received, FALSE if error or timeout.
 *
 */
// ----------------------------------------------------------------------------
bool_t ToolSpecificHardware_ISBPortCharacterReceiveByPolling(unsigned char *pData, Timer_t* pTimer)
{
    return FALSE;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_SSBPortSelfTest does nothing - in the previous version
 * it did a loopback test, but if it's broken, it's broken.
 *
 * @retval	Uint16		1 if test passed.
 */
// ----------------------------------------------------------------------------
Uint16 ToolSpecificHardware_SSBPortSelfTest(void)
{
	return 1u;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_ISBPortSelfTest does nothing - in the previous version
 * it did a loopback test, but if it's broken, it's broken.
 *
 * @retval  Uint16      1 if test passed.
 */
// ----------------------------------------------------------------------------
Uint16 ToolSpecificHardware_ISBPortSelfTest(void)
{
    return 1u;
}


#ifdef 0
// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_PromMemoryWrite is used to copy the promloader into
 * an area of RAM, from which is it run.
 *
 * @param   pData			Pointer to data to copy.
 * @param	Length			Number of BYTES to copy.
 * @param	StartAddress	Start address in RAM to copy data into.
 * @retval  bool_t			TRUE if copy OK, FALSE if any address out of range.
 *
 */
// ----------------------------------------------------------------------------
bool_t ToolSpecificHardware_PromMemoryWrite(Uint8* pData, Uint16 Length,
		Uint32 StartAddress)
{
	return PromMemoryAccess_MemoryWrite(pData, Length, StartAddress, WIDTH_16BITS);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_PromMemoryRead is used to read the promloader application
 * from an area of RAM where it has been loaded into (to check it's loaded
 * before running it).
 *
 * @param   pData			Pointer to memory to read promloader into.
 * @param	Length			Number of BYTES to read.
 * @param	StartAddress	Start address in RAM to read promloader data from.
 * @retval  bool_t			TRUE if read OK, FALSE if any address out of range.
 *
 */
// ----------------------------------------------------------------------------
bool_t ToolSpecificHardware_PromMemoryRead(Uint8* pData, Uint16 Length,
		Uint32 StartAddress)
{
	return PromMemoryAccess_MemoryRead(pData, Length, StartAddress, WIDTH_16BITS);
}
#endif


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_DebugMessageSend transmits a message using the
 * debug port.
 *
 * @param   pDebugMessage	Pointer to message to transmit.
 *
 */
// ----------------------------------------------------------------------------
void ToolSpecificHardware_DebugMessageSend(char* pDebugMessage)
{
	// Wait for any previous message to be transmitted.
	while (SCI_TxDoneCheck(SCI_A) == FALSE)
	{
		;
	}

	//lint -e(926) Cast from pointer to pointer.
	SCI_TxStart(SCI_A, (uint8_t*)pDebugMessage, (uint16_t)strlen(pDebugMessage));

	// Wait for the message to be transmitted.
	while (SCI_TxDoneCheck(SCI_A) == FALSE)
	{
		;
	}
}


// ----------------------------------------------------------------------------
/**
 * @note
 * ToolSpecificHardware_DebugPortCharacterReceiveReadOnce reads a single
 * character from the local mDebugReceiveInterruptBuffer if there is one or
 * more new characters in the buffer.  The buffer is populated by the receive
 * interrupt.  Note that, as per the instructions in the header file, this
 * function only reads a single character at a time, even if there are more
 * new characters in the local buffer.
 *
 * @param   pData		Pointer to buffer to write received character into.
 * @retval	bool_t		TRUE if a character received, FALSE if no character.
 *
 */
// ----------------------------------------------------------------------------
bool_t ToolSpecificHardware_DebugPortCharacterReceiveReadOnce(unsigned char* pData)
{
	static uint16_t		PreviousNumberOfCharacters = 0u;
	uint16_t			CurrentNumberOfCharacters;
	bool_t				bFoundACharacter = FALSE;

	CurrentNumberOfCharacters = SCI_RxBufferNumberOfCharsGet(SCI_A);

	// If there are no characters in the receive buffer then reset the
	// previous counter - we're waiting for something to happen.
	if (CurrentNumberOfCharacters == 0u)
	{
		PreviousNumberOfCharacters = 0u;
	}
	else
	{
		// If we've received one (or more) characters then read one from
		// the receive buffer.  Any other characters will simply be buffered
		// in the interrupt buffer until they're read.
		if (CurrentNumberOfCharacters > PreviousNumberOfCharacters)
		{
			bFoundACharacter = TRUE;
			*pData = mDebugReceiveInterruptBuffer[PreviousNumberOfCharacters];
			PreviousNumberOfCharacters++;

			// If character received is CR then reset the buffer.
			// This is safe to do because we only receive the characters one by one,
			// so we don't need to worry about overwriting data in the buffer - by
			// this point the characters are 'old' and are no longer useful.
			if (*pData == '\r')
			{
				DebugRxBufferInitialise();
			}
		}
	}

	return bFoundACharacter;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void SSBRxBufferInitialise(void)
{
	// Note the cast from pointer to pointer - the ssb buffer is a buffer of
	// type char_t, but the serial port driver itself uses type uint8_t.
	// We therefore inhibit the Lint warning for cast from pointer to pointer.
	SCI_RxBufferInitialise(SCI_B, (uint8_t*)mSSBReceiveInterruptBuffer, MAX_SSB_BUFFER_RX_SIZE);	//lint !e926
}

static void DebugRxBufferInitialise(void)
{
	// Note the cast from pointer to pointer - the debug buffer is a buffer of
	// type char_t, but the serial port driver itself uses type uint8_t.
	// We therefore inhibit the Lint warning for cast from pointer to pointer.
	SCI_RxBufferInitialise(SCI_A, (uint8_t*)mDebugReceiveInterruptBuffer, MAX_DEBUG_BUFFER_RX_SIZE);	//lint !e926
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


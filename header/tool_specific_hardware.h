// ----------------------------------------------------------------------------
/**
 * @file    	ToolSpecificHardware.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		3 Feb 2014
 * @brief		Header file for ToolSpecificHardware.c abstraction layer.
 *
 * @note
 * These are the function prototypes for all tool specific hardware functions
 * - anything which 'touches' the registers, peripherals, interrupts, GPIO etc
 * should be controlled via this hardware abstraction layer.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef TOOLSPECIFICHARDWARE_H_
#define TOOLSPECIFICHARDWARE_H_


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_Initialise is used to initialise the hardware - this
 * should contain most (if not all) of the hardware specific stuff (register
 * setup, ports, peripherals, interrupts etc).
 */
void 	ToolSpecificHardware_Initialise(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_TimerDisableAndReset disables and resets the timer.
 *
 */
void 	ToolSpecificHardware_TimerDisableAndReset(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_TimerRawTimeGet returns the raw time from the system
 * timer, in 1mS chunks.
 *
 * @retval	Uint32		Raw time, with 1mS resolution.
 */
Uint32 	ToolSpecificHardware_TimerRawTimeGet(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_SSBTransmitDisable disables the SSB transmitter (and
 * enables the SSB receiver - it is assumed that the SSB is a single duplex
 * bus so if you're not doing TX, you're doing RX and vice versa).
 */
void 	ToolSpecificHardware_SSBTransmitDisable(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ISBTransmitDisable disables the ISB transmitter (and
 * enables the ISB receiver - it is assumed that the ISB is a single duplex
 * bus so if you're not doing TX, you're doing RX and vice versa).
 */
void    ToolSpecificHardware_ISBTransmitDisable(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_SSBTransmitEnable enables the SSB transmitter (and
 * disables the SSB receiver - it is assumed that the SSB is a single duplex
 * bus so if you're not doing TX, you're doing RX and vice versa).
 */
void 	ToolSpecificHardware_SSBTransmitEnable(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ISBTransmitEnable enables the ISB transmitter (and
 * disables the ISB receiver - it is assumed that the ISB is a single duplex
 * bus so if you're not doing TX, you're doing RX and vice versa).
 */
void    ToolSpecificHardware_ISBTransmitEnable(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_CANInterruptDisable disables the CAN interrupt.
 */
void 	ToolSpecificHardware_CANInterruptDisable(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_CPUReset reset the CPU - it is assumed that if anything
 * 'special' needs to be done before resetting, it will be done in this function.
 */
void 	ToolSpecificHardware_CPUReset(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ApplicationExecute forces the CPU to jump to the
 * address given - this is normally used for booting the new code.
 *
 * @param	ExecutionAddress	Void pointer to address to jump to.
 */
void 	ToolSpecificHardware_ApplicationExecute(void* ExecutionAddress);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_SSBPortCharacterReceiveReadOnce reads from the SSB
 * serial port once (doesn't poll waiting for a character).
 *
 * @param	pData	Pointer to location to put any character which was read in.
 * @retval	bool_t	TRUE if a character was read, FALSE if no character read.
 */
bool_t 	ToolSpecificHardware_SSBPortCharacterReceiveReadOnce(unsigned char* pData);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ISBPortCharacterReceiveReadOnce reads from the ISB
 * serial port once (doesn't poll waiting for a character).
 *
 * @param   pData   Pointer to location to put any character which was read in.
 * @retval  bool_t  TRUE if a character was read, FALSE if no character read.
 */

bool_t  ToolSpecificHardware_ISBPortCharacterReceiveReadOnce(unsigned char* pData);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_SSBPortWaitForSendComplete waits for any SSB message
 * which is being transmitted to have finished sending.
 */
void 	ToolSpecificHardware_SSBPortWaitForSendComplete(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ISBPortWaitForSendComplete waits for any ISB message
 * which is being transmitted to have finished sending.
 */
void    ToolSpecificHardware_ISBPortWaitForSendComplete(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_SSBPortByteSend transmits a single byte of data.
 * This code must be able to buffer characters or wait until a previous
 * character has been transmitted, as this function could be called in rapid
 * succession.
 *
 * @param	data		Single character to transmit.
 */
void 	ToolSpecificHardware_SSBPortByteSend(unsigned char data);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ISBPortByteSend transmits a single byte of data.
 * This code must be able to buffer characters or wait until a previous
 * character has been transmitted, as this function could be called in rapid
 * succession.
 *
 * @param   data        Single character to transmit.
 */
void    ToolSpecificHardware_ISBPortByteSend(unsigned char data);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_SSBPortCharacterReceiveByPolling polls the SSB port
 * looking for a character (until the timer times out).
 *
 * @param	pData	Pointer to location to put any character which was read in.
 * @param	pTimer	Pointer to timer structure for timeout checking.
 * @retval	bool_t	TRUE if character received, FALSE if timeout \ no char \ error.
 */
bool_t 	ToolSpecificHardware_SSBPortCharacterReceiveByPolling(unsigned char *pData, Timer_t* pTimer);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ISBPortCharacterReceiveByPolling polls the ISB port
 * looking for a character (until the timer times out).
 *
 * @param   pData   Pointer to location to put any character which was read in.
 * @param   pTimer  Pointer to timer structure for timeout checking.
 * @retval  bool_t  TRUE if character received, FALSE if timeout \ no char \ error.
 */
bool_t ToolSpecificHardware_ISBPortCharacterReceiveByPolling(unsigned char *pData, Timer_t* pTimer);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_SSBPortSelfTest performs a self test on the SSB port.
 *
 * @retval Uint16	1 if test passed.
 */
Uint16	ToolSpecificHardware_SSBPortSelfTest(void);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_ISBPortSelfTest performs a self test on the ISB port.
 *
 * @retval Uint16   1 if test passed.
 */
Uint16  ToolSpecificHardware_ISBPortSelfTest(void);


#ifdef I_AM_THE_BOOTLOADER
// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_PromMemoryWrite attempts to copy data into an area of
 * memory designated for the promloader.  The usual failure mechanism is an
 * address violation, because the promloader can only be copied to a certain
 * range of addresses, specified in ToolSpecificConfig.h by the #defines
 * PROMLOADER_RAM_START_ADDRESS and PROMLOADER_RAM_SIZE.
 *
 * @param	pData			Pointer to contiguous bytes of data to be copied.
 * @param	Length			Number of bytes of data.
 * @param	StartAddress	Start address for copy.
 * @retval	bool_t			TRUE if copy OK, FALSE if failed (address violation).
 */
bool_t	ToolSpecificHardware_PromMemoryWrite(Uint8* pData, Uint16 Length, Uint32 StartAddress);


// ----------------------------------------------------------------------------
/**
 * ToolSpecificHardware_PromMemoryRead attempts to read data from an area of
 * memory designated for the promloader.  The usual failure mechanism is an
 * address violation, because the promloader can only be read from a certain
 * range of addresses, specified in ToolSpecificConfig.h by the #defines
 * PROMLOADER_RAM_START_ADDRESS and PROMLOADER_RAM_SIZE.
 *
 * @param	pData			Pointer to array to but read data in.
 * @param	Length			Number of bytes of data to read.
 * @param	StartAddress	Start address for read.
 * @retval	bool_t			TRUE if read OK, FALSE if failed (address violation).
 */
bool_t	ToolSpecificHardware_PromMemoryRead(Uint8* pData, Uint16 Length, Uint32 StartAddress);
#endif


// ----------------------------------------------------------------------------
/*
 * ToolSpecificHardware_DebugMessageSend sends a message via the debug port.
 * This message will always be in ASCII, and be null terminated.
 *
 * @param	pDebugMessage	Pointer to null terminated message.
 */
void	ToolSpecificHardware_DebugMessageSend(char* pDebugMessage);


// ----------------------------------------------------------------------------
/*
 * ToolSpecificHardware_DebugPortCharacterReceiveReadOnce reads from the debug
 * serial port once (doesn't poll waiting for a character).  Note that this
 * function must only read a single character at a time, as the destination
 * may not be an array.
 *
 * @param	pData	Pointer to location to put any character which was read in.
 * @retval	bool_t	TRUE if character read, FALSE if no character at this time.
 */
bool_t 	ToolSpecificHardware_DebugPortCharacterReceiveReadOnce(unsigned char* pData);


// ----------------------------------------------------------------------------

#endif /* TOOLSPECIFICHARDWARE_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

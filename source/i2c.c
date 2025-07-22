// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/i2c.c
 * @author		Fei Li (LIF@xsyu.edu.cn)
 * @date		17 Dec 2012
 * @brief		I2C driver for TI's 28335 DSP.
 * @details
 * Functions are provided to open and close the I2C port, and to read and write
 * data using the I2C bus.  Some of the device registers are accessed as 16 bit
 * reads \ writes, using the genericIO functions - this allows the code to be
 * tested using TDD, as the IO functions can be mocked out (whereas direct
 * access using a pointer to a volatile cannot).
 *
 * The convention for writing the individual bits comes from a series of
 * articles on embeddedgurus.com by Nigel Jones, in which he makes a reasoned
 * case for not using bit fields or defines for the individual bits, to reduce
 * the likelihood of errors (please refer to Configuring Hardware parts 1-3).
 * As part of this implementation, we disable lint warnings for zero being given
 * as an argument to ?? (835) and also for the right side of the | operator
 * being zero (845).  We also perform some fairly heavy duty casting, for
 * compliance with MISRA rule 10.5, to ensure that the data is always 16 bits
 * following a shift, i.e. it is implementation independent.
 *
 * @warning
 * The GPIO multiplexers need to be set-up so that the I2C pins are mux'ed
 * through to the correct IO pins - this will need to be taken care of in a
 * separate module, so all of the muxes are setup at the same time (which also
 * allows for different pin mappings for different DSP's, while still using
 * this common code).
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "i2c.h"
#include "DSP28335_device.h"
#include "genericIO.h"
#include "testpoints.h"
#include "testpointoffsets.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define I2C_DESIRED_MODULE_CLOCK	(8000000u)	///< desired module clock is 8MHz
#define I2C_MINIMUM_DATA_RATE		10000u		///< minimum data rate is 10kbps
#define I2C_MAXIMUM_DATA_RATE		(400000u)	///< maximum data rate is 400kbps

/* Note - disable Lint warnings for the casts here - need 32 bit arguments. */

#define I2CSTR_ADDRESS	(/*lint -e(921)*/(uint32_t)0x00007902u)	///< I2CSTR address is 0x7902
#define I2CCNT_ADDRESS	(/*lint -e(921)*/(uint32_t)0x00007905u)	///< I2CCNT address is 0x7905
#define I2CDRR_ADDRESS	(/*lint -e(921)*/(uint32_t)0x00007906u)	///< I2CDRR address is 0x7906
#define I2CSAR_ADDRESS	(/*lint -e(921)*/(uint32_t)0x00007907u)	///< I2CSAR address is 0x7907
#define I2CDXR_ADDRESS	(/*lint -e(921)*/(uint32_t)0x00007908u)	///< I2CDXR address is 0x7908
#define I2CMDR_ADDRESS	(/*lint -e(921)*/(uint32_t)0x00007909u) ///< I2CMDR address is 0x7909

#define I2CSTR_XRDY_BIT_MASK	0x0010u			///< XRDY is bit 4
#define I2CSTR_RRDY_BIT_MASK	0x0008u			///< RRDY is bit 3
#define I2CSTR_ARDY_BIT_MASK	0x0004u			///< ARDY is bit 2
#define I2CSTR_NACK_BIT_MASK	0x0002u			///< NACK is bit 1


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static EI2CStatus_t I2C_Read_Impl(const uint16_t SlaveAddress,
                                  const uint16_t DeviceAddress,
                                  uint16_t DataCount,
                                  uint8_t * const pData);

static EI2CStatus_t I2C_Write_Impl(const uint16_t SlaveAddress,
                                   const uint16_t DeviceAddress,
                                   uint16_t DataCount,
                                   const uint8_t * const pData);

static EI2CStatus_t I2C_AckPoll_Impl(const uint16_t SlaveAddress,
                                     const uint16_t MaxTimeout);

static bool_t   TransmitSlaveAndDeviceAddresses(const uint16_t SlaveAddress,
                                                const uint16_t DeviceAddress);

static bool_t   PollForTransmitRegisterReady(void);
static void     PollForReceivedDataReady(void);
static void     ResetCountAndSendStopBit(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/// Volatile flag to force a timeout during the polling function.
static volatile bool_t m_b_force_timeout = FALSE;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * I2C_Open sets up the I2C module - this doesn't actually do very much because
 * the I2C_Read and I2C_Write functions do most of the work.
 * The code attempts to set the prescaler to give a module clock frequency of
 * I2C_DESIRED_MODULE_CLOCK - if the system clock is lower than the desired,
 * the setup will abort and the I2C will be held in reset.  The bit rate
 * dividers are setup to give a 50% duty cycle for the SCLK pin.
 *
 * @warning
 * The bit rate dividers are not checked to ensure the value written into them
 * is non-zero.  This is because there is enough other checking to prevent this
 * happening.  However, if the I2C module changes and ever allows a 1Mbps data
 * rate, this check will need to be added (so, it's not enough to just change
 * the define for maximum data rate).
 *
 * @param	iSysClk_Hz		SYSCLKOUT frequency, in Hz.
 * @param	iDataRate		Desired data rate, in bits per second.
 * @retval	SetupOK			TRUE if I2C is setup OK, FALSE if setup failed.
 *
 */
// ----------------------------------------------------------------------------
bool_t I2C_Open(const uint32_t iSysClk_Hz, const uint32_t iDataRate)
{
    uint16_t Prescaler;
    bool_t SetupOK = TRUE;
    uint32_t ActualModuleClock;
    uint16_t ModuleClockDivider;

    // Disable module before changing prescaler bits
    I2caRegs.I2CMDR.bit.IRS = 0u;

    // If system clock is too low then abort setup.
    if (( iSysClk_Hz < I2C_DESIRED_MODULE_CLOCK )
            || ( iDataRate < I2C_MINIMUM_DATA_RATE )
            || ( iDataRate > I2C_MAXIMUM_DATA_RATE ))
    {
        SetupOK = FALSE;
    }
    // Otherwise setup prescaler and enable module - please refer to section
    // 5.6 of SPRUG03B for details of the I2CPSC register and equation, and
    // section 5.7 for details of the I2CCLKx registers.
    else
    {
        //lint -e{921} Cast to uint16_t OK, prescaler is only 16 bits.
        Prescaler = (uint16_t)( ( iSysClk_Hz / I2C_DESIRED_MODULE_CLOCK ) - 1u );
        I2caRegs.I2CPSC.all = Prescaler;

        // Calculate actual module clock to account for any rounding above.
        //lint -e{921} Cast to uint32_t because working values are 32 bit here.
        ActualModuleClock = ( iSysClk_Hz / ( (uint32_t)Prescaler + 1u ) );

        // For a 50% duty cycle, the divider is calculated like this:
        //lint -e{921} Cast to uint16_t OK, divider is only 16 bits.
        ModuleClockDivider = (uint16_t)( ( ActualModuleClock / iDataRate ) / 2u );

        // Adjust divider based on value of extra delay, d - the delay is
        // given in table 15 of SPRUG03B.
        if (Prescaler > 1u)
        {
            ModuleClockDivider -= 5u;
        }
        else if (Prescaler == 1u)
        {
            ModuleClockDivider -= 6u;
        }
        else
        {
            ModuleClockDivider -= 7u;
        }

        // Write divider into clock divide registers.
        I2caRegs.I2CCLKL = ModuleClockDivider;
        I2caRegs.I2CCLKH = ModuleClockDivider;

        // Enable module
        I2caRegs.I2CMDR.bit.IRS = 1u;
    }

    return SetupOK;
}


// ----------------------------------------------------------------------------
/**
 * I2C_Close disables the I2C module by clearing the I2CMDR,IRS bit.
 *
 *
 */
// ----------------------------------------------------------------------------
void I2C_Close(void)
{
    I2caRegs.I2CMDR.bit.IRS = 0u;
}


// ----------------------------------------------------------------------------
/**
 * I2C_Read_Impl reads from an I2C device.
 * Note that this currently only deals with devices which require a single
 * address word, and this code doesn't deal with buffer overrun.
 * We use the form _Impl and declare the function as static because we're using
 * a function pointer to access this function (to allow for redirection under TDD).
 *
 * @param	SlaveAddress	Slave address of device on the I2C bus.
 * @param	DeviceAddress	Start address within the device to read from.
 * @param	DataCount		Number of reads to perform.
 * @param	pData			Pointer to buffer to put data in.
 * @retval	EI2CStatus_t	Enumerated status.
 *
 */
// ----------------------------------------------------------------------------
static EI2CStatus_t I2C_Read_Impl(const uint16_t SlaveAddress,
                                  const uint16_t DeviceAddress,
                                  uint16_t DataCount,
                                  uint8_t * const pData)
{
    EI2CStatus_t    I2CReturnStatus = I2C_COMPLETED_OK;
    uint16_t        RequiredData;
    uint32_t        ReadCounter = 0u;
    bool_t          b_AckReceivedFromSlave;

    // If the STP bit is still set then the I2C hasn't transmitted the stop
    // bit yet, so jump out and return the appropriate status.
    if (I2caRegs.I2CMDR.bit.STP == 1u)
    {
        I2CReturnStatus = I2C_STP_NOT_READY;
    }
    // If the BB bit is set then the bus has received or transmitted a start
    // bit, so is busy, so jump out and return the appropriate status.
    else if (I2caRegs.I2CSTR.bit.BB == 1u)
    {
        I2CReturnStatus = I2C_BUS_BUSY;
    }
    else
    {
        // Transmit control byte and address, and wait for data to be transmitted
        // and the slave device to respond with an 'ACK'.
        b_AckReceivedFromSlave = TransmitSlaveAndDeviceAddresses(SlaveAddress, DeviceAddress);

        // If the slave has acknowledged, then carry on and read the data.
        if (b_AckReceivedFromSlave)
        {
            // Write number of bytes to receive.
            genericIO_16bitWrite(I2CCNT_ADDRESS, DataCount);

            // Setup mode register to send command - sends a repeated start bit and then
            // reads the required number of bytes (with an ack for each byte) and then
            // a stop bit once the reads have finished.
            //lint -e{835, 845, 921}
            RequiredData =
                ( (uint16_t)0u << 15 ) |    // 0: I2C module sends ACK during each acknowledge cycle
                ( (uint16_t)0u << 14 ) |    // 0: module stops when a breakpoint occurs
                ( (uint16_t)1u << 13 ) |    // 1: generate a start condition on the bus
                ( (uint16_t)0u << 12 ) |    // 0: reserved bit, writes have no effect
                ( (uint16_t)1u << 11 ) |    // 1: generate a stop bit
                ( (uint16_t)1u << 10 ) |    // 1: I2C module is a master
                ( (uint16_t)0u << 9 ) |	    // 0: I2C module is a receiver
                ( (uint16_t)0u << 8 ) |	    // 0: 7 bit addressing mode
                ( (uint16_t)0u << 7 ) |	    // 0: non-repeat mode
                ( (uint16_t)0u << 6 ) |	    // 0: digital loopback disabled
                ( (uint16_t)1u << 5 ) | 	// 1: I2C module is enabled
                ( (uint16_t)0u << 4 ) | 	// 0: not in START byte mode
                ( (uint16_t)0u << 3 ) | 	// 0: free data format mode is disabled
                ( (uint16_t)0u << 0 );	    // 000: 8 bits per data byte
            genericIO_16bitWrite(I2CMDR_ADDRESS, RequiredData);

            // Read the required number of data bytes from the device.
            while (DataCount != 0u)
            {
                uint8_t i;
                // Wait for next byte of data to be received.
                PollForReceivedDataReady();

                // Read data and put in buffer, increment read counter
                // and decrement the byte counter.
                //lint -e{921} Cast to 8 bits is OK as data is actually only 8 bits.
                pData[ReadCounter] = (uint8_t)genericIO_16bitRead(I2CDRR_ADDRESS);
                ReadCounter++;
                DataCount--;
                for(i = 0;i< 1000;i++){}
            }
        }
        else
        {
            I2CReturnStatus = I2C_NO_ACK_RECEIVED_FROM_SLAVE;
        }
    }

    return I2CReturnStatus;
}

/// This is the defining instance of the global function pointer I2C_Read.
/// The pointer is initialised to point to I2C_Read_Impl.
//lint -e{956} Function pointer doesn't need to be volatile.
EI2CStatus_t (*I2C_Read)(const uint16_t SlaveAddress,
							const uint16_t DeviceAddress,
							uint16_t DataCount,
							uint8_t * const pData) = I2C_Read_Impl;


// ----------------------------------------------------------------------------
/**
 * I2C_Write_Impl writes to an I2C device.
 * Note that this currently only deals with devices which require a single
 * address word, and doesn't do any acknowledgement polling (most I2C devices
 * use the same mechanism for ack poll, but it doesn't seem like a good idea
 * to put it in here in case we come across a device with a different mechanism).
 * We use the form _Impl and declare the function as static because we're using
 * a function pointer to access this function (to allow for redirection under TDD).
 *
 * @param	SlaveAddress	Slave address of device on the I2C bus.
 * @param	DeviceAddress	Start address within the device to write to.
 * @param	DataCount		Number of writes to perform.
 * @param	pData			Pointer to buffer to get data for be written from.
 * @retval	EI2CStatus_t	Enumerated status.
 *
 */
// ----------------------------------------------------------------------------
static EI2CStatus_t I2C_Write_Impl(const uint16_t SlaveAddress,
                                   const uint16_t DeviceAddress,
                                   uint16_t DataCount,
                                   const uint8_t * const pData)
{
    EI2CStatus_t    I2CReturnStatus = I2C_COMPLETED_OK;
    bool_t          bAckReceivedFromSlave;
    uint16_t        RequiredData;
    uint32_t        WriteCounter = 0u;

    // If the STP bit is still set then the I2C hasn't transmitted the stop
    // bit yet, so jump out and return the appropriate status.
    if (I2caRegs.I2CMDR.bit.STP == 1u)
    {
        I2CReturnStatus = I2C_STP_NOT_READY;
    }
    // If the BB bit is set then the bus has received or transmitted a start
    // bit, so is busy, so jump out and return the appropriate status.
    else if (I2caRegs.I2CSTR.bit.BB == 1u)
    {
        I2CReturnStatus = I2C_BUS_BUSY;
    }
    else
    {
        // Write slave address and number of bytes to transmit
        // (one more than data bytes because of address),
        // and setup transmit register with address.
        genericIO_16bitWrite(I2CSAR_ADDRESS, SlaveAddress);
        genericIO_16bitWrite(I2CCNT_ADDRESS, ( DataCount + 2u ));
        genericIO_16bitWrite(I2CDXR_ADDRESS, (DeviceAddress >> 8) & 0x000F);
        /*genericIO_16bitWrite(I2CCNT_ADDRESS, ( DataCount + 1u ));
        genericIO_16bitWrite(I2CDXR_ADDRESS, DeviceAddress);*/

        // Setup mode register to send command - writes the slave address and
        // device address which were setup above out on the I2C bus - we set
        // the bit to generate a stop bit because we want a stop bit after all
        // the data has been transmitted - this starts the internal write cycle
        // of the device (if it does one).
        //lint -e{835, 845, 921}
        RequiredData =
            ( (uint16_t)0u << 15 ) |    // 0: NACK mode (not used as we're transmitting)
            ( (uint16_t)0u << 14 ) |    // 0: module stops when a breakpoint occurs
            ( (uint16_t)1u << 13 ) |    // 1: generate a start condition on the bus
            ( (uint16_t)0u << 12 ) |    // 0: reserved bit, writes have no effect
            ( (uint16_t)1u << 11 ) |    // 1: generate a stop bit
            ( (uint16_t)1u << 10 ) |    // 1: I2C module is a master
            ( (uint16_t)1u << 9 ) |	    // 1: I2C module is a transmitter
            ( (uint16_t)0u << 8 ) |	    // 0: 7 bit addressing mode
            ( (uint16_t)0u << 7 ) |	    // 0: non-repeat mode
            ( (uint16_t)0u << 6 ) |	    // 0: digital loopback disabled
            ( (uint16_t)1u << 5 ) |     // 1: I2C module is enabled
            ( (uint16_t)0u << 4 ) |     // 0: not in START byte mode
            ( (uint16_t)0u << 3 ) |     // 0: free data format mode is disabled
            ( (uint16_t)0u << 0 );	    // 000: 8 bits per data byte
        genericIO_16bitWrite(I2CMDR_ADDRESS, RequiredData);
        PollForTransmitRegisterReady();
        genericIO_16bitWrite(I2CDXR_ADDRESS, DeviceAddress & 0x00FF);
        // Wait for data to be transmitted and return whether if we got an ACK.
        bAckReceivedFromSlave = PollForTransmitRegisterReady();

        // If slave doesn't ACK then send a stop bit, just to be on the safe side.
        // Also reset the number of bytes to transmit to zero.
        if (!bAckReceivedFromSlave)
        {
            ResetCountAndSendStopBit();
            I2CReturnStatus = I2C_NO_ACK_RECEIVED_FROM_SLAVE;
        }
        else
        {
            // Write the required number of data bytes into the slave device.
            while (DataCount != 0u)
            {
                uint8_t i;
                // Get next byte from buffer and transmit it.
                //lint -e{921} Cast to uint16_t OK - write function needs 16 bits.
                genericIO_16bitWrite(I2CDXR_ADDRESS, (uint16_t)pData[WriteCounter]);
                WriteCounter++;
                for(i = 0;i< 1000;i++){}
                // Wait for next byte of data to be transmitted.
                bAckReceivedFromSlave = PollForTransmitRegisterReady();

                if (!bAckReceivedFromSlave)
                {
                    ResetCountAndSendStopBit();
                    I2CReturnStatus = I2C_NO_ACK_RECEIVED_FROM_SLAVE;
                    break;
                }

                // Decrement counter and go round again.
                DataCount--;
            }
        }

        // Wait for the stop bit to finish before doing anything else.
        while (I2caRegs.I2CMDR.bit.STP != 0u)
        {
            ;
        }
    }

    return I2CReturnStatus;
}

/// This is the defining instance of the global function pointer I2C_Write.
/// The pointer is initialised to point to I2C_Write_Impl.
//lint -e{956} Function pointer doesn't need to be volatile.
EI2CStatus_t (*I2C_Write)(const uint16_t SlaveAddress,
							const uint16_t DeviceAddress,
							uint16_t DataCount,
							const uint8_t * const pData) = I2C_Write_Impl;


// ----------------------------------------------------------------------------
/**
 * I2C_AckPoll_Impl uses the 'standard' form of acknowledgement polling - sending
 * the slave address, with R/Wbar set to zero, and checking for an ACK.  Once
 * an ACK has been received (or the system has timed out), a stop bit is sent.
 * We use the form _Impl and declare the function as static because we're using
 * a function pointer to access this function (to allow for redirection under TDD).
 *
 * @note
 * We exclude the reset of the force timeout flag at the start of this function
 * if we're running unit tests, otherwise we can never check for a timeout.
 *
 * @param   SlaveAddress    Slave address of device on the I2C bus.
 * @param	MaxTimeout		Maximum number of times we can poll.
 * @retval	EI2CStatus_t	Either operation complete or timeout exceeded.
 *
 */
// ----------------------------------------------------------------------------
static EI2CStatus_t I2C_AckPoll_Impl(const uint16_t SlaveAddress,
                                     const uint16_t MaxTimeout)
{
    uint16_t        RequiredData;
    bool_t          bGotAck = FALSE;
    EI2CStatus_t    I2CReturnStatus = I2C_COMPLETED_OK;
    uint16_t        RunningTimeout;
    uint16_t        Status = 0u;

#ifndef UNIT_TEST_BUILD
    m_b_force_timeout = FALSE;
#endif

    // Setup running value of timeout
    RunningTimeout = MaxTimeout;

    while (!bGotAck)
    {
        // Write slave address and number of bytes to transmit (zero).
        genericIO_16bitWrite(I2CSAR_ADDRESS, SlaveAddress);
        genericIO_16bitWrite(I2CCNT_ADDRESS, 0u);

        // Setup mode register to send command - writes the slave address out on
        // the I2C bus.  Note that we don't \ can't generate a stop bit in this
        // mode, so we add the stop bit by hand once we've finished transmitting.
        // This seems a bit convoluted, but the I2C module doesn't seem to have a way
        // of just transmitting a start bit + address + stop bit automatically.
        //lint -e{835, 845, 921}
        RequiredData =
            ( (uint16_t)0u << 15 ) |    // 0: NACK mode (not used as we're transmitting)
            ( (uint16_t)0u << 14 ) |    // 0: module stops when a breakpoint occurs
            ( (uint16_t)1u << 13 ) |    // 1: generate a start condition on the bus
            ( (uint16_t)0u << 12 ) |    // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 11 ) |    // 0: don't generate a stop bit
            ( (uint16_t)1u << 10 ) |    // 1: I2C module is a master
            ( (uint16_t)1u << 9 ) |	    // 1: I2C module is a transmitter
            ( (uint16_t)0u << 8 ) |	    // 0: 7 bit addressing mode
            ( (uint16_t)1u << 7 ) |	    // 1: repeating mode
            ( (uint16_t)0u << 6 ) |	    // 0: digital loopback disabled
            ( (uint16_t)1u << 5 ) |     // 1: I2C module is enabled
            ( (uint16_t)0u << 4 ) |     // 0: not in START byte mode
            ( (uint16_t)0u << 3 ) |     // 0: free data format mode is disabled
            ( (uint16_t)0u << 0 );	    // 000: 8 bits per data byte
        genericIO_16bitWrite(I2CMDR_ADDRESS, RequiredData);

        // Wait for data to be transmitted - note that we just check the ARDY
        // bit here (the data sheet is not very clear on this, but testing has
        // shown that this is set AFTER the XRDY bit, and you need to wait for
        // ARDY before doing anything else, otherwise the I2C locks up).
        // @warning
        // Have to set Status = 0u before attempting read, otherwise it locks up
        // (even though Status is initialised with a value of zero, and when you
        // debug, it shows up as zero).
        // This makes no sense at the moment. @TODO Investigate this some more.
        Status = 0u;
        while (( Status & I2CSTR_ARDY_BIT_MASK ) == 0u)
        {
            Status = genericIO_16bitRead(I2CSTR_ADDRESS);
        }

        // Check for ACK \ NACK - if we've received a NACK (the slave doesn't
        // acknowledge and the pull-up resistor pulls the data pin high) then
        // we MUST clear the NACK bit (by writing to it), otherwise the I2C
        // locks up - the data sheet doesn't mention this, and suggests that
        // the bit will be cleared by the first ACK which comes along, but this
        // is not the case.
        bGotAck = TRUE;
        if (( Status & I2CSTR_NACK_BIT_MASK ) != 0u)
        {
            bGotAck = FALSE;
            I2caRegs.I2CSTR.bit.NACK = 1u;
        }

        // Generate stop bit and wait for stop bit to be sent.
        ResetCountAndSendStopBit();

        // If the MaxTimeout is non-zero then we want to check the timeout,
        // so we decrement it, and if it's zero then set the return value
        // accordingly and jump out of the while loop.
        if (MaxTimeout != 0u)
        {
            RunningTimeout--;

            // Only set return status to timeout exceeded if the timeout has
            // expired AND the last acknowledge was FALSE (otherwise you can time
            // out even if the last poll returned an ack).
            if ( ( RunningTimeout == 0u ) && (!bGotAck) )
            {
                I2CReturnStatus = I2C_ACKPOLL_TIMEOUT_EXCEEDED;
            }
        }
        // Otherwise if MaxTimeout is zero then we want to check the force
        // timeout flag instead.
        else
        {
            // Only set the return status to timeout exceed if the force timeout
            // flag is set AND the last acknowledge was FALSE (otherwise you
            // might time out even if the last poll returned an ack).
            if ( (m_b_force_timeout) && (!bGotAck) )
            {
                I2CReturnStatus = I2C_ACKPOLL_TIMEOUT_EXCEEDED;
            }
        }

        // Jump out of the polling loop if timeout exceeded.
        if (I2CReturnStatus == I2C_ACKPOLL_TIMEOUT_EXCEEDED)
        {
            break;
        }
    }

    return I2CReturnStatus;
}

/// This is the defining instance of the global function pointer I2C_AckPoll.
/// The pointer is initialised to point to I2C_AckPoll_Impl.
//lint -e{956} Function pointer doesn't need to be volatile.
EI2CStatus_t (*I2C_AckPoll)(const uint16_t SlaveAddress,
								const uint16_t MaxTimeout) = I2C_AckPoll_Impl;


// ----------------------------------------------------------------------------
/**
 * I2C_AckPollTimeoutFlagSet set the force timeout flag, which will force
 * the acknowledgement polling function to exit.
 *
 * @note
 * This assumes that a task that is higher priority than the one running the
 * polling function is calling this function.
 *
 */
// ----------------------------------------------------------------------------
void I2C_AckPollTimeoutFlagSet(void)
{
    m_b_force_timeout = TRUE;
}


#ifdef UNIT_TEST_BUILD
// ----------------------------------------------------------------------------
/**
 * I2C_AckPollTimeoutFlagReset_TDD resets the force timeout flag.
 *
 * This is only required when unit testing, hence the conditional compilation.
 *
 */
// ----------------------------------------------------------------------------
void I2C_AckPollTimeoutFlagReset_TDD(void)
{
    m_b_force_timeout = FALSE;
}
#endif


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * TransmitSlaveAndDeviceAddresses transmits the slave and device addresses
 * and checks whether the slave has sent back an 'ack'.  This function doesn't
 * generate a stop bit because we're always going to want to do something else
 * after this (unless the slave device doesn't ACK back again, in which case
 * a stop bit is sent to terminate the operation).
 *
 * @param	SlaveAddress	Slave address of device.
 * @param	DeviceAddress	Start address within the device.
 * @retval	bool_t	TRUE if ack has been received, FALSE if NACK received.
 *
 */
// ----------------------------------------------------------------------------
static bool_t TransmitSlaveAndDeviceAddresses(const uint16_t SlaveAddress,
                                              const uint16_t DeviceAddress)
{
    uint16_t    RequiredData;
    bool_t      bGotAck;

    // Write slave address and number of bytes to transmit,
    // and setup transmit register with address.
    genericIO_16bitWrite(I2CSAR_ADDRESS, SlaveAddress);
    genericIO_16bitWrite(I2CCNT_ADDRESS, 2u);
    genericIO_16bitWrite(I2CDXR_ADDRESS, (DeviceAddress >> 8) & 0xFF);
    /*genericIO_16bitWrite(I2CCNT_ADDRESS, 1u);
    genericIO_16bitWrite(I2CDXR_ADDRESS, DeviceAddress);*/

    // Setup mode register to send command - writes the slave address and device
    // address which were setup above out on the I2C bus, with no stop bit
    // (because we want to send a repeated start next, and read some data).
    //lint -e{835, 845, 921}
    RequiredData =
        ( (uint16_t)0u << 15 ) |    // 0: NACK mode (not used as we're transmitting)
        ( (uint16_t)0u << 14 ) |    // 0: module stops when a breakpoint occurs
        ( (uint16_t)1u << 13 ) |    // 1: generate a start condition on the bus
        ( (uint16_t)0u << 12 ) |    // 0: reserved bit, writes have no effect
        ( (uint16_t)0u << 11 ) |    // 0: don't generate a stop bit
        ( (uint16_t)1u << 10 ) |    // 1: I2C module is a master
        ( (uint16_t)1u << 9 ) |	    // 1: I2C module is a transmitter
        ( (uint16_t)0u << 8 ) |	    // 0: 7 bit addressing mode
        ( (uint16_t)0u << 7 ) |	    // 0: non-repeat mode
        ( (uint16_t)0u << 6 ) |	    // 0: digital loopback disabled
        ( (uint16_t)1u << 5 ) |     // 1: I2C module is enabled
        ( (uint16_t)0u << 4 ) |     // 0: not in START byte mode
        ( (uint16_t)0u << 3 ) |     // 0: free data format mode is disabled
        ( (uint16_t)0u << 0 );	    // 000: 8 bits per data byte
    genericIO_16bitWrite(I2CMDR_ADDRESS, RequiredData);
    PollForTransmitRegisterReady();
    genericIO_16bitWrite(I2CDXR_ADDRESS, DeviceAddress & 0xFF);
    // Wait for data to be transmitted and return whether if we got an ACK.
    bGotAck = PollForTransmitRegisterReady();

    // If slave doesn't ACK then send a stop bit, just to be on the safe side.
    // Also reset the number of bytes to transmit to zero.
    if (!bGotAck)
    {
        ResetCountAndSendStopBit();
    }

    return bGotAck;
}


// ----------------------------------------------------------------------------
/**
 * PollForTransmitRegisterReady polls the I2C status register, waiting for the
 * XRDY bit to be set.  As this is a hardware function, it's unlikely that the
 * code will get stuck in here unless the hardware itself has failed.  Once
 * the XRDY bit is set, this function also checks to see if an ACK has been
 * received or not - it might be that not all calling functions need this, but
 * generally a byte is being transmitted and we want to see if a slave has
 * responded with an ACK.
 *
 * @retval	bool_t	TRUE if ack has been received, FALSE if NACK received.
 *
 */
// ----------------------------------------------------------------------------
static bool_t PollForTransmitRegisterReady(void)
{
    uint16_t    Status;
    bool_t      AckReceived = TRUE;

    // Wait for data to be transmitted
    Status = 0u;
    while (( Status & ( I2CSTR_XRDY_BIT_MASK | I2CSTR_ARDY_BIT_MASK ) ) == 0u)
    {
        Status = genericIO_16bitRead(I2CSTR_ADDRESS);
    }

    // If the NACK bit is set then set the AckReceived flag to false, and reset
    // the NACK bit by writing a 1 to it.  Failure to reset the NACK results
    // in the I2C locking up, although the data sheet doesn't mention this!
    if (( Status & I2CSTR_NACK_BIT_MASK ) != 0u)
    {
        AckReceived = FALSE;
        I2caRegs.I2CSTR.bit.NACK = 1u;
    }

    return AckReceived;
}


// ----------------------------------------------------------------------------
/**
 * PollForReceivedDataReady polls the I2C status register, waiting for the
 * RRDY bit to be set.  As this is a hardware function, it's unlikely that the
 * code will get stuck in here unless the hardware itself has failed.
 *
 */
// ----------------------------------------------------------------------------
static void PollForReceivedDataReady(void)
{
    uint16_t    Status;

    // Wait for data to be received
    Status = 0u;
    while (Status == 0u)
    {
        Status = genericIO_16bitRead(I2CSTR_ADDRESS);
        Status &= ( I2CSTR_RRDY_BIT_MASK | I2CSTR_ARDY_BIT_MASK );
    }
}


// ----------------------------------------------------------------------------
/**
 * ResetCountAndSendStopBit resets the count register and transmits a stop bit.
 *
 */
// ----------------------------------------------------------------------------
static void ResetCountAndSendStopBit(void)
{
    uint16_t    RequiredData;

    genericIO_16bitWrite(I2CCNT_ADDRESS, 0u);

    // Setup mode register to send command - just sends a stop bit.
    //lint -e{835, 845, 921}
    RequiredData =
        ( (uint16_t)0u << 15 ) |    // 0: NACK mode (not used as we're transmitting)
        ( (uint16_t)0u << 14 ) |    // 0: module stops when a breakpoint occurs
        ( (uint16_t)0u << 13 ) |    // 0: don't generate a start bit
        ( (uint16_t)0u << 12 ) |    // 0: reserved bit, writes have no effect
        ( (uint16_t)1u << 11 ) |    // 1: generate a stop bit
        ( (uint16_t)1u << 10 ) |    // 1: I2C module is a master
        ( (uint16_t)1u << 9 ) |	    // 1: I2C module is a transmitter
        ( (uint16_t)0u << 8 ) |	    // 0: 7 bit addressing mode
        ( (uint16_t)1u << 7 ) |	    // 1: repeat mode - should just send a stop bit
        ( (uint16_t)0u << 6 ) |	    // 0: digital loopback disabled
        ( (uint16_t)1u << 5 ) |     // 1: I2C module is enabled
        ( (uint16_t)0u << 4 ) |     // 0: not in START byte mode
        ( (uint16_t)0u << 3 ) |     // 0: free data format mode is disabled
        ( (uint16_t)0u << 0 );	    // 000: 8 bits per data byte
    genericIO_16bitWrite(I2CMDR_ADDRESS, RequiredData);

    // Wait for stop bit to be generated - MST bit is cleared when finished.
    while (I2caRegs.I2CMDR.bit.MST != 0u)
    {
        ;
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

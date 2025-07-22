/*
 * spi.c
 *
 *  Created on: 2022Äê5ÔÂ30ÈÕ
 *      Author: l
 */

#include "common_data_types.h"
#include "spi.h"
#include "DSP28335_device.h"
#include "genericIO.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

/**
 * Base address for SPI.
 *
 * Note the cast - force the address to be calculated as 32 bits (disable the
 * Lint warning for cast to uint32_t as this is what we want to do).
 */
#define SPI_A_BASE_ADDRESS	/*lint -e(921)*/(uint32_t)0x00007040u

#define	SPICCR_OFFSET		0x0000u			///< Offset from base for SPICCR
#define SPICTL_OFFSET		0x0001u			///< Offset from base for SPICTL
#define SPISTS_OFFSET		0x0002u			///< Offset from base for SPISTS
#define SPIBRR_OFFSET		0x0004u			///< Offset from base for SPIBRR
#define SPIRXBUF_OFFSET		0x0007u			///< Offset from base for SPIRXBUF
#define SPITXBUF_OFFSET		0x0008u			///< Offset from base for SPITXBUF
#define SPIFFTX_OFFSET		0x000Au			///< Offset from base for SPIFFTX
#define SPIFFRX_OFFSET		0x000Bu			///< Offset from base for SPIFFRX
#define SPIFFCT_OFFSET		0x000Cu			///< Offset from base for SPIFFCT
#define SPIPRI_OFFSET		0x000Fu			///< Offset from base for SPIPRI

#define SPISTS_SPIINT_BIT_MASK		0x0040u	///< SPIINT is bit 6

#define EEPROM_ACTIVE_STATE_SET 	GpioDataRegs.GPBCLEAR.bit.GPIO57 ///< Sets the EEPROM SPISTE pin into the active state.
#define EEPROM_INACTIVE_STATE_SET 	GpioDataRegs.GPBSET.bit.GPIO57	 ///< Sets the EEPROM SPISTE pin into the inactive state.

#define RTC_ACTIVE_STATE_SET 	GpioDataRegs.GPACLEAR.bit.GPIO2 ///< Sets the RTC SPISTE pin into the active state.
#define RTC_INACTIVE_STATE_SET 	GpioDataRegs.GPASET.bit.GPIO2	 ///< Sets the RTC SPISTE pin into the inactive state.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static void ResetAllSPIRegisters(void);
static void WaitForSPIReady(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module

/// Number of bit to transmit in a SPI transaction.
//lint -e{956} Doesn't need to be volatile.
static uint16_t	mNumberOfSPIDataBits = 0u;

/// Array holding bit masks for received data - valid data is right justified
/// in the receive buffer, so the upper bits need to be masked off.
static const uint16_t	mReceiveBitMask[16] = {	0x0001u, 0x0003u, 0x0007u, 0x000Fu,
                     	                       	0x001Fu, 0x003Fu, 0x007Fu, 0x00FFu,
                     	                       	0x01FFu, 0x03FFu, 0x07FFu, 0x0FFFu,
                     	                       	0x1FFFu, 0x3FFFu, 0x7FFFu, 0xFFFFu	};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_Open opens the SPI serial port (SPI-A) on the 28335.
 * Interrupts and FIFOs are not currently used.
 *
*/
// ----------------------------------------------------------------------------
//lint -e{9044} Function parameter modified, but it's deliberate, so it's OK.
void SPI_Open(uint16_t RequiredNumberOfBits)
{
	uint16_t	RequiredData;
	uint16_t	Bits;

	ResetAllSPIRegisters();

	// If RequiredNumberOfBits is outside the allowable range then default
	// to 16 bits.
	if ( (RequiredNumberOfBits > 16u) || (RequiredNumberOfBits == 0u) )
	{
		RequiredNumberOfBits = 16u;
	}

	// Setup value to write into SPICCR register - one less than required.
	Bits = RequiredNumberOfBits - 1u;

	// Hold SPI module in reset while making changes.
	SpiaRegs.SPICCR.bit.SPISWRESET = 0u;

	// Setup all bits to write into the SPICCR register, and write them.
	//lint -e{835, 845, 921}
	RequiredData =
		( (uint16_t)0u << 7) |	// 0: SPI software reset - hold in reset
		( (uint16_t)1u << 6) |	// 1: data output on falling edge, input on rising edge
		( (uint16_t)0u << 5) | 	// 0: reserved
		( (uint16_t)0u << 4) | 	// 0: loop back mode disabled
		( (uint16_t)0u << 3) | 	// 0: idle-line mode protocol selected
		( (uint16_t)Bits << 0);	// ????: required number of bits - 1 (bits 3:0)
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPICCR_OFFSET), RequiredData); //lint !e835

	// Setup variable to hold number of data bits which system will use.
	mNumberOfSPIDataBits = RequiredNumberOfBits;

	// Setup all bits to write into the SPICTL register, and write them.
	//lint -e{835, 845, 921}
	RequiredData =
		( (uint16_t)0u << 7) |	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 6) |	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 5) | 	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 4) | 	// 0: disable receiver overrun interrupts
		( (uint16_t)0u << 3) | 	// 0: clock phase normal (not delayed)
		( (uint16_t)1u << 2) | 	// 1: SPI is master
		( (uint16_t)1u << 1) | 	// 1: transmitter enabled (SPISIMO not Hi-Z)
		( (uint16_t)0u << 0);	// 0: no SPI interrupt (will poll for data ready)
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPICTL_OFFSET), RequiredData);

	// Setup all bits to write into the SPIFFTX register, and write them
	//lint -e{835, 845, 921}
	RequiredData =
		( (uint16_t)1u << 15) |	// 1: SPI FIFO can resume RX or TX
		( (uint16_t)0u << 14) |	// 0: SPI FIFO is disabled
		( (uint16_t)1u << 13) |	// 1: SPI FIFO transmit pointer reset
		( (uint16_t)0u << 12) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 11) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 10) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 9) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 8) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 7) |	// 0: read only bit, writes have no effect
		( (uint16_t)1u << 6) |	// 1: clear any outstanding TXFFINT
		( (uint16_t)0u << 5) | 	// 0: TX FIFO interrupt based on match disabled
		( (uint16_t)0u << 0);	// 00000: TX FIFO interrupt depth = 0 (bits 4:0)
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIFFTX_OFFSET), RequiredData);

	// Setup all bits to write into the SPIFFRX register, and write them
	//lint -e{835, 845, 921}
	RequiredData =
		( (uint16_t)0u << 15) |	// 0: read only bit, writes have no effect
		( (uint16_t)1u << 14) |	// 1: clear any outstanding RXFFOVF
		( (uint16_t)1u << 13) |	// 1: SPI FIFO receive pointer enabled
		( (uint16_t)0u << 12) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 11) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 10) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 9) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 8) |	// 0: read only bit, writes have no effect
		( (uint16_t)0u << 7) |	// 0: read only bit, writes have no effect
		( (uint16_t)1u << 6) |	// 1: clear any outstanding RXFFINT
		( (uint16_t)0u << 5) | 	// 0: RX FIFO interrupt based on match disabled
		( (uint16_t)1u << 0);	// 00001: RX FIFO interrupt depth = 1 (bits 4:0)
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIFFRX_OFFSET), RequiredData);

	// Setup all bits to write into the SPIFFCT register, and write them
	//lint -e{835, 845, 921}
	RequiredData =
		( (uint16_t)0u << 15) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 14) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 13) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 12) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 11) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 10) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 9) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 8) |	// 0: reserved, writes have no effect
		( (uint16_t)0u << 0);	// 00000000: FIFO transmit delay = 0 (bits 7:0)
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIFFCT_OFFSET), RequiredData);

	// Setup all bits to write into the SPIPRI register, and write them
	//lint -e{835, 845, 921}
	RequiredData =
		( (uint16_t)0u << 7) |	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 6) |	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 4) |	// 00: immediate stop on suspend (bits 5:4)
		( (uint16_t)0u << 3) |	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 2) |	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 1) |	// 0: reserved bit, writes have no effect
		( (uint16_t)0u << 0);	// 0: reserved bit, writes have no effect
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIPRI_OFFSET), RequiredData);

	// Release SPI module from reset
	SpiaRegs.SPICCR.bit.SPISWRESET = 1u;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_Close closes (disables) the SPI port on the 28335.
 *
*/
// ----------------------------------------------------------------------------
void SPI_Close(void)
{
	ResetAllSPIRegisters();
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_BaudRateSet sets up the baud rate generator for the SPI port.
 *
 * @warning
 * This does not check for non-exact baud rate values, it is the responsibIlity
 * of the programmer to ensure that your LSPCLK will give you the baud rate you
 * require (or near	enough to work).
 *
 * @param	iLspClk_Hz	LSPCLK which is fed into the serial port, in Hz.
 * @param	iBaudRate	Required baud rate, in bits per second.
 * @retval	bool_t		TRUE if baud rate OK, FALSE if value out of range.
 *
 */
// ----------------------------------------------------------------------------
bool_t SPI_BaudRateSet(const uint32_t iLspClk_Hz, const uint32_t iBaudRate)
{
	uint32_t	iResult;
	bool_t		bValidBaudRate = FALSE;
	uint16_t	iData;

	// If required baud rate <= LSPCLK/4 then this is a valid baud rate.
	if (iBaudRate <= (iLspClk_Hz / 4u) )
	{
		// Baud rate divider = (LSPCLK / BAUD RATE) - 1
		// (taken from example 1.2 in SPRUEU3A - SPI reference manual).
		iResult = (iLspClk_Hz / iBaudRate) - 1u;

		// If result is less than 128 then this is a valid baud rate.
		if (iResult < 128u)
		{
			bValidBaudRate = TRUE;

			// Mask off everything except bottom 8 bits and write to baud register.
			//lint -e{921} Cast to uint16_t as write function uses 16 bits.
			iData = (uint16_t)(iResult & 0x000000FFu);
			genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIBRR_OFFSET), iData);
		}
	}

	return bValidBaudRate;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_Read reads a word of data from the SPI port.
 *
 * This is achieved by writing a dummy word and reading back once the write
 * has finished. The function allows you to specify the dummy word, in case the
 * device requires the dummy word to be a certain value to perform the read.
 *
 * @warning
 * This function does not drive the SPISTE pin - it is the responsibility of
 * the next software layer up to do this.  This is to allow commands which
 * are a mix of read and write, but which want to keep the SPISTE pin low
 * throughout.  The SPISTE pin needs to be set up as GPIO for this to work.
 *
 * @param	DummyWord		Dummy word to transmit.
 * @retval	uint16_t		Value read from SPI port.
 *
 */
// ----------------------------------------------------------------------------
uint16_t SPI_Read(const uint16_t DummyWord)
{
	uint16_t	MaskedData;

	// Write dummy word - received data is clocked in as this word is sent.
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPITXBUF_OFFSET), DummyWord);

	// Wait until word has been read.
	WaitForSPIReady();

	// Read from receive buffer, mask off the bits which are not required,
	// and return the value.
	MaskedData = genericIO_16bitRead(SPI_A_BASE_ADDRESS + SPIRXBUF_OFFSET);
	MaskedData &= mReceiveBitMask[mNumberOfSPIDataBits - 1u];

	return MaskedData;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_Write writes a word of data to the SPI port.
 *
 * @warning
 * This function does not drive the SPISTE pin - it is the responsibility of
 * the next software layer up to do this.  This is to allow commands which
 * are a mix of read and write, but which want to keep the SPISTE pin low
 * throughout.  The SPISTE pin needs to be set up as GPIO for this to work.
 *
 * @param	DataToWrite		Data word to write to SPI port.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{9044} Function parameter modified, but it's deliberate, so it's OK.
void SPI_Write(uint16_t DataToWrite)
{
	uint16_t	ShiftQty;

	// Data to transmit must be left justified in the transmit buffer,
	// so shift upwards by (16 - number of bits).
	ShiftQty    = 16u - mNumberOfSPIDataBits;
	DataToWrite = DataToWrite << ShiftQty;

	// Write shifted data to transmit buffer.
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPITXBUF_OFFSET), DataToWrite);

	// Wait until transmit has finished before doing anything else
	// (This allows the next software layer up to drive the SPISTE pin
	// without having to worry about chopping the end of the word off).
	WaitForSPIReady();

	// Perform dummy read to stop overrun bit being set.
	//lint -e{920} Cast to void, discard return value as it's a dummy read.
	(void)genericIO_16bitRead(SPI_A_BASE_ADDRESS + SPIRXBUF_OFFSET);
}


// ----------------------------------------------------------------------------
/**
 * SPI_EEPROMActiveSet sets the EEPROM SPISTE pin into the active (low) state, by
 * writing a 1 into the appropriate GPIO 'CLEAR' register.  This is done using
 * a define so the I/O can be changed easily.  Note that the pin must have
 * already been setup as GPIO for this to work.
 *
 */
// ----------------------------------------------------------------------------
void SPI_EEPROMActiveSet(void)
{
	EEPROM_ACTIVE_STATE_SET = 1u;
}


// ----------------------------------------------------------------------------
/**
 * SPI_RTCActiveSet sets the RTC SPISTE pin into the active (low) state, by
 * writing a 1 into the appropriate GPIO 'CLEAR' register.  This is done using
 * a define so the I/O can be changed easily.  Note that the pin must have
 * already been setup as GPIO for this to work.
 *
 */
// ----------------------------------------------------------------------------
void SPI_RTCActiveSet(void)
{
	RTC_ACTIVE_STATE_SET = 1u;
}

// ----------------------------------------------------------------------------
/**
 * SPI_EEPROMInactiveSet sets the EEPROM SPISTE pin into the inactive (high) state, by
 * writing a 1 into the appropriate GPIO 'SET' register.  This is done using
 * a define so the I/O can be changed easily.  Note that the pin must have
 * already been setup as GPIO for this to work.
 *
 */
// ----------------------------------------------------------------------------
void SPI_EEPROMInactiveSet(void)
{
	EEPROM_INACTIVE_STATE_SET = 1u;
}

// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_RTCInactiveSet sets the SPISTE pin into the inactive (high) state, by
 * writing a 1 into the appropriate GPIO 'SET' register.  This is done using
 * a define so the I/O can be changed easily.  Note that the pin must have
 * already been setup as GPIO for this to work.
 *
 */
// ----------------------------------------------------------------------------
void SPI_RTCInactiveSet(void)
{
	RTC_INACTIVE_STATE_SET = 1u;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_NumberOfDataBitsGet returns the number of data bits which was set up
 * during SPI_Open.
 *
 */
// ----------------------------------------------------------------------------
uint16_t SPI_NumberOfDataBitsGet(void)
{
	return mNumberOfSPIDataBits;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * SPI_ReceivedBitMaskGet returns the bit mask for the number of data bits
 * which was set up during SPI_Open.
 *
 */
// ----------------------------------------------------------------------------
uint16_t SPI_ReceivedBitMaskGet(void)
{
	return mReceiveBitMask[mNumberOfSPIDataBits - 1u];
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * ResetAllSPIRegisters sets all the SPI control registers to zero.
 *
 */
// ----------------------------------------------------------------------------
static void ResetAllSPIRegisters(void)
{
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPICCR_OFFSET), 0u);	//lint !e835 zero given as right hand argument
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPICTL_OFFSET), 0u);
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIBRR_OFFSET), 0u);
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIFFTX_OFFSET), 0u);
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIFFRX_OFFSET), 0u);
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIFFCT_OFFSET), 0u);
	genericIO_16bitWrite( (SPI_A_BASE_ADDRESS + SPIPRI_OFFSET), 0u);
}


// ----------------------------------------------------------------------------
/**
 * @note
 * WaitForSPIReady reads the status register, and waits for the SPIINT bit to
 * be set - this tells us that any transmission or reception has finished.
 *
 */
// ----------------------------------------------------------------------------
static void WaitForSPIReady(void)
{
	uint16_t	status = 0u;

	while (status == 0u)
	{
		// Read status register and mask off everything except SPIINT - this
		// is set when the SPI has finished transmitting the current word
		// (and has therefore received the data from the slave device).
		status = genericIO_16bitRead( (SPI_A_BASE_ADDRESS + SPISTS_OFFSET) );
		status &= SPISTS_SPIINT_BIT_MASK;
	}

}


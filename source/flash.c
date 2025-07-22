// ----------------------------------------------------------------------------
/**
 * @file    	acqmtc_dsp_b/src/flash.c
 * @author		Fei Li
 * @date		4 Dec 2012
 * @brief		Read and write functions for the various flash devices.
 * @details
 * The read and write functions are mapped to whichever chipset drivers are
 * required - this means that if the flash chips are changed it should only be
 * necessary to insert the alternate chipset drivers and modify the read and
 * write functions in here.
 *
 * @note
 * It could be argued that it would be better to have separate modules for
 * the parallel flash, serial flash and serial eeprom, but as there are only
 * 2 x functions for each device (read and write) it makes sense to have them
 * all grouped together in here for the moment...
 * Note the use of Lint directive 952 - for some reason we get a message which
 * says that *pData could be declared const, but if we do that the compiler
 * complains about stripping away the const qualifier in the functions which
 * are called.
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "flash.h"
#include "lld.h"			// chipset drivers for Spansion flash
#include "M95.h"			// chipset drivers for M95M01 serial flash
#include "x24lc32a.h"			// chipset drivers for X24LC32A serial EEPROM


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

#define LOWER_DEVICE_MAX	0x04000000u		///< maximum address in device zero


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static void Flash_ParallelFlashRead
				(uint32_t address, uint32_t WordCount, uint16_t *pData);

static EFLASHProgramStatus_t Flash_ParallelFlashWrite
		(const uint32_t address, const uint32_t WordCount, uint16_t* pData);

static EFLASHProgramStatus_t Flash_SerialFlashWrite
		(const uint32_t address, const uint32_t WordCount, uint8_t* pData);

static EFLASHProgramStatus_t Flash_SerialEEPROMWrite
			(const uint32_t address, const uint32_t WordCount, uint8_t* pData);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * FLASH_Read reads 16 bit data from a 32 bit address, using the
 * appropriate local function.
 * Note that X24LC32A_BlockRead can only accept a 16 bit quantity for the number of
 * words, but WordCount is 32 bit because some of the other devices might accept
 * 32 bit quantities, hence the cast.
 *
 * @param	device					Enumerated value for device to read from.
 * @param	address					Start address to read from.
 * @param	WordCount				Number of words to read from flash.
 * @param	pData					Pointer to first location for read data.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{952} pData can't be made const.
void FLASH_Read(const EFLASHDeviceNumber_t device, const uint32_t address,
					const uint32_t WordCount, void* pData)
{
	switch (device)
	{
		case FLASH_DEVICE_PARALLEL:
			Flash_ParallelFlashRead(address, WordCount, pData);
			break;

		case FLASH_DEVICE_SERIAL:
			M95_BlockRead(address, WordCount, pData);
			break;

		// Discard return value from read - if it doesn't work we'll
		// deal with this in the level above (when the checksum doesn't match).
		case FLASH_DEVICE_EEPROM:
			(void)X24LC32A_BlockRead(address, (uint16_t)WordCount, pData);
			break;

		default:
			break;
	}
}


// ----------------------------------------------------------------------------
/**
 * FLASH_Write writes 16 bit data into the appropriate flash, using
 * the appropriate local function.
 *
 * @param	device					Enumerated value for device to write to.
 * @param	address					Start address to write to.
 * @param	WordCount				Number of words to write into flash.
 * @param	pData					Pointer to first word of source data.
 * @retval	EFLASHProgramStatus_t	Status code for write operation.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{952} pData can't be made const.
EFLASHProgramStatus_t FLASH_Write(const EFLASHDeviceNumber_t device,
										const uint32_t address,
										const uint32_t WordCount,
										void* pData)
{
	EFLASHProgramStatus_t	ReturnValue;

	switch (device)
	{
		case FLASH_DEVICE_PARALLEL:
			ReturnValue = Flash_ParallelFlashWrite(address, WordCount, pData);
			break;

		case FLASH_DEVICE_SERIAL:
			ReturnValue = Flash_SerialFlashWrite(address, WordCount, pData);
			break;

		case FLASH_DEVICE_EEPROM:
			ReturnValue = Flash_SerialEEPROMWrite(address, WordCount, pData);
			break;

		default:
			ReturnValue = FLASH_PROGRAM_UNKNOWN_DEVICE;
			break;
	}

	return ReturnValue;
}


// ----------------------------------------------------------------------------
/**
 * FLASH_Erase erases the appropriate flash - the external flash is erased
 * using the erase command, but the serial flash and eeprom are erased by
 * writing multiple pages of 0xFF into the device.
 *
 * @param	DeviceType				Enumerated value for device to erase.
 * @param	DeviceNumber			Device Number to erase (or zero if not used).
 * @retval	EFLASHPollStatus_t		Status code for erase operation.
 *
 */
// ----------------------------------------------------------------------------
EFLASHPollStatus_t FLASH_Erase(const EFLASHDeviceNumber_t DeviceType,
								const uint16_t DeviceNumber)
{
	EFLASHPollStatus_t		ReturnValue = FLASH_POLL_NOT_BUSY;
	EM95PollStatus_t		SerialPollStatus;
	EI2CStatus_t			I2CPollStatus;

	switch (DeviceType)
	{
		case FLASH_DEVICE_PARALLEL:
			ReturnValue = FLASH_ExternalFlashPoll(DeviceNumber);
			if (ReturnValue == FLASH_POLL_NOT_BUSY)
			{
				if (DeviceNumber == 0u)
				{
					lld_ChipEraseCmd(DEVICE_ZERO_BASE);
					ReturnValue = FLASH_POLL_BUSY;
				}
				else if (DeviceNumber == 1u)
				{
					lld_ChipEraseCmd(DEVICE_ONE_BASE);		//lint !e923 cast from int to pointer
					ReturnValue = FLASH_POLL_BUSY;
				}
				else
				{
					ReturnValue = FLASH_POLL_ERASE_FAIL;
				}
			}
			break;

		case FLASH_DEVICE_SERIAL:
			SerialPollStatus = M95_DeviceErase();
			if (SerialPollStatus != M95_POLL_NO_WRITE_IN_PROGRESS)
			{
				ReturnValue = FLASH_POLL_ERASE_FAIL;
			}
			break;

		case FLASH_DEVICE_EEPROM:
			I2CPollStatus = X24LC32A_DeviceErase();
			if (I2CPollStatus != I2C_COMPLETED_OK)
			{
				ReturnValue = FLASH_POLL_ERASE_FAIL;
			}
			break;

		default:
			ReturnValue = FLASH_POLL_NOT_BUSY;
			break;
	}

	return ReturnValue;
}


// ----------------------------------------------------------------------------
/**
 * FLASH_ExternalFlashPoll polls the external flash, reading from the status
 * register.
 * Note - MISRA strength Lint warnings are disabled for this function.  The
 * header file lld.h is not very strict, so we get a number of warnings about
 * implicit conversion changes sign and bitwise operator applied to signed type.
 * As the header file is standard, it's best not to modify it too much.
 *
 * @param	DeviceNumber			Chip number - 0 or 1.
 * @retval	EFLASHPollStatus_t		Enumerated value for status.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{960}
EFLASHPollStatus_t FLASH_ExternalFlashPoll(const uint16_t DeviceNumber)
{
	EFLASHPollStatus_t	ReturnValue;
	FLASHDATA			StatusRegister;

	if (DeviceNumber == 0u)
	{
		// Send the status register read command.
		lld_StatusRegReadCmd(DEVICE_ZERO_BASE);

		// Read from the status register.
		StatusRegister = lld_ReadOp(DEVICE_ZERO_BASE, (ADDRESS)0u);
	}
	else if (DeviceNumber == 1u)
	{
		// Send the status register read command.
		lld_StatusRegReadCmd(DEVICE_ONE_BASE);				//lint !e923 cast from int to pointer

		// Read from the status register.
		StatusRegister = lld_ReadOp(DEVICE_ONE_BASE, (ADDRESS)0u);	//lint !e923 cast from int to pointer
	}
	else
	{
		StatusRegister = 0u;
	}

	// If the flash is BUSY then all other bits in the status register are
	// invalid, so just return that the device is busy.
	if ( (StatusRegister & DEV_RDY_MASK) != DEV_RDY_MASK)
	{
		ReturnValue = FLASH_POLL_BUSY;
	}
	// Otherwise flash is not busy, but there might be other things in the
	// status register which are important, so check all the different options.
	else
	{
		// If erase suspend bit is set then return erase suspended code.
		if ( (StatusRegister & DEV_ERASE_SUSP_MASK) == DEV_ERASE_SUSP_MASK)
		{
			ReturnValue = FLASH_POLL_ERASE_SUSPENDED;
		}
		// If erase fail bit is set then check for sector locked, and either
		// return erase fail or sector locked code.
		else if ( (StatusRegister & DEV_ERASE_MASK) == DEV_ERASE_MASK)
		{
			ReturnValue = FLASH_POLL_ERASE_FAIL;

			if ( (StatusRegister & DEV_SEC_LOCK_MASK) == DEV_SEC_LOCK_MASK)
			{
				ReturnValue = FLASH_POLL_SECTOR_LOCKED;
			}
		}
		// If program fail bit is set then check for sector locked, and either
		// return program fail or sector locked code.
		else if ( (StatusRegister & DEV_PROGRAM_MASK) == DEV_PROGRAM_MASK)
		{
			ReturnValue = FLASH_POLL_PROGRAM_FAIL;

			if ( (StatusRegister & DEV_SEC_LOCK_MASK) == DEV_SEC_LOCK_MASK)
			{
				ReturnValue = FLASH_POLL_SECTOR_LOCKED;
			}
		}
		// If program aborted bit is set then return program aborted code.
		// Note that #define in lld.h is for a reserved bit,but it is used!
		else if ( (StatusRegister & DEV_RFU_MASK) == DEV_RFU_MASK)
		{
			ReturnValue = FLASH_POLL_PROGRAM_ABORTED;
		}
		// If program suspended bit is set then return program suspended code.
		else if ( (StatusRegister & DEV_PROGRAM_SUSP_MASK) == DEV_PROGRAM_SUSP_MASK)
		{
			ReturnValue = FLASH_POLL_PROGRAM_SUSPENDED;
		}
		// If sector lock bit is set (on its own) then return sector locked code.
		// Note that this bit probably can't be set on its own - it should go
		// with either program or erase fail, which are dealt with above.
		else if ( (StatusRegister & DEV_SEC_LOCK_MASK) == DEV_SEC_LOCK_MASK)
		{
			ReturnValue = FLASH_POLL_SECTOR_LOCKED;
		}
		else
		{
			ReturnValue = FLASH_POLL_NOT_BUSY;
		}
	}

	return ReturnValue;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * Flash_ParallelFlashRead reads 16 bit data from the parallel flash and stores
 * it in a buffer, using the appropriate chipset driver code.  Note that this
 * code doesn't check for buffer overrun.
 *
 * @param	address					Start address to read from.
 * @param	WordCount				Number of words to read from flash.
 * @param	pData					Pointer to first location for read data.
 *
 */
// ----------------------------------------------------------------------------
static void Flash_ParallelFlashRead
				(uint32_t address, uint32_t WordCount, uint16_t* pData)
{
	// Read the required number of words from the parallel flash.
	while (WordCount != 0u)
	{
		if (address < LOWER_DEVICE_MAX)
		{
			*pData = lld_ReadOp(DEVICE_ZERO_BASE, address);
		}
		else
		{
			// lld_ReadOp's second argument is the offset into the device,
			// so we need to subtract the maximum address of the lower device
			// to get the desired offset.  Note that we have to disable the
			// Lint warning for cast from int to pointer (in DEVICE_ONE_BASE) -
			// this contravenes MISRA rule 11.1, but is a function of the way
			// the Spansion library code works, so is difficult to change.
			*pData = lld_ReadOp(DEVICE_ONE_BASE, (address - LOWER_DEVICE_MAX) );	//lint !e923
		}

		// Adjust address, destination pointer and wordcount for next read.
		// Note that we're violating MISRA rule 17.4 by using pointer arithmetic,
		// but we can't check the index anyway, so we'll have to live with it.
		address++;
		pData++;		//lint !e960
		WordCount--;
	}
}


// ----------------------------------------------------------------------------
/**
 * Flash_ParallelFlashWrite writes 16 bit data into the parallel flash, using the
 * appropriate chipset driver code.  Note that lld_memcpy can only accept a
 * 16 bit quantity for the number of words, but WordCount is 32 bit because
 * some of the other devices might accept 32 bit quantities.  Hence the cast.
 *
 * @param	address					Start address to write to.
 * @param	WordCount				Number of words to write into flash.
 * @param	pData					Pointer to first word of source data.
 * @retval	EFLASHProgramStatus_t	Status code for write operation.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{952} *pData can't be declared const.
static EFLASHProgramStatus_t Flash_ParallelFlashWrite
			(const uint32_t address, const uint32_t WordCount, uint16_t *pData)
{
	DEVSTATUS				WriteStatus;
	EFLASHProgramStatus_t	ReturnValue;

	if (address < LOWER_DEVICE_MAX)
	{
		WriteStatus = lld_memcpy(DEVICE_ZERO_BASE, address, (uint16_t)WordCount, pData);
	}
	else
	{
		// lld_memcpy's second argument is the offset into the device,
		// so we need to subtract the maximum address of the lower device
		// to get the desired offset.  Note that we have to disable the
		// Lint warning for cast from int to pointer (in DEVICE_ONE_BASE) -
		// this contravenes MISRA rule 11.1, but is a function of the way
		// the Spansion library code works, so is difficult to change.
		WriteStatus = lld_memcpy(DEVICE_ONE_BASE, (address - LOWER_DEVICE_MAX), (uint16_t)WordCount, pData );	//lint !e923
	}

	// Generate correct reply code based on returned value from lld_memcpy.
	// We do this so that if the underlying driver (and messages) changes, we're
	// not relying on them in other modules, so we just need to change this
	// section of the code to cater for the new \ different messages.
	// Note that we're disabling the lint warning for enum constant not used
	// within defaulted switch - DEVSTATUS contains a number of other status
	// options, but we just have a catch all case for these because there are
	// only three options which the lld_memcpy function can return.
	//lint -e{788}
	switch (WriteStatus)
	{
		case DEV_NOT_BUSY:
			ReturnValue = FLASH_PROGRAM_OK;
			break;

		case DEV_PROGRAM_ERROR:
			ReturnValue = FLASH_PROGRAM_ERROR;
			break;

		case DEV_SECTOR_LOCK:
			ReturnValue = FLASH_PROGRAM_SECTOR_LOCKED_ERROR;
			break;

		default:
			ReturnValue = FLASH_PROGRAM_UNKNOWN_ERROR;
			break;
	}

	return ReturnValue;
}


// ----------------------------------------------------------------------------
/**
 * Flash_SerialFlashWrite writes 8 bit data into the serial flash, using the
 * appropriate chipset driver code.
 *
 * @param	address					Start address to write to.
 * @param	WordCount				Number of words to write into flash.
 * @param	pData					Pointer to first word of source data.
 * @retval	EFLASHProgramStatus_t	Status code for write operation.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{952, 818} *pData can't be declared const.
static EFLASHProgramStatus_t Flash_SerialFlashWrite
			(const uint32_t address, const uint32_t WordCount, uint8_t* pData)
{
	EM95PollStatus_t		PollStatus;
	EFLASHProgramStatus_t	ReturnValue;

	PollStatus = M95_memcpy(address, WordCount, pData);

	// Note that we're disabling the lint warning for enum constant not used
	// within defaulted switch - PollStatus contains a number of other status
	// options, but we just have a catch all case for these.
	//lint -e{788}
	switch (PollStatus)
	{
		case M95_POLL_NO_WRITE_IN_PROGRESS:
			ReturnValue = FLASH_PROGRAM_OK;
			break;

		default:
			ReturnValue = FLASH_PROGRAM_ERROR;
			break;
	}

	return ReturnValue;
}


// ----------------------------------------------------------------------------
/**
 * Flash_SerialEEPROMWrite writes 8 bit data into the serial EEPROM, using the
 * appropriate chipset driver code. Note that X24LC32A_memcpy can only accept a
 * 16 bit quantity for the number of words, but WordCount is 32 bit because
 * some of the other devices might accept 32 bit quantities.  Hence the cast.
 *
 * @param	address					Start address to write to.
 * @param	WordCount				Number of words to write into flash.
 * @param	pData					Pointer to first word of source data.
 * @retval	EFLASHProgramStatus_t	Status code for write operation.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{952, 818} *pData can't be declared const.
static EFLASHProgramStatus_t Flash_SerialEEPROMWrite
			(const uint32_t address, const uint32_t WordCount, uint8_t* pData)
{
	EI2CStatus_t			status;
	EFLASHProgramStatus_t	ReturnValue;

	status = X24LC32A_memcpy(address, (uint16_t)WordCount, pData);

	// Note that we're disabling the lint warning for enum constant not used
	// within defaulted switch - status contains a number of other status
	// options, but we just have a catch all case for these.
	//lint -e{788}
	switch (status)
	{
		case I2C_COMPLETED_OK:
			ReturnValue = FLASH_PROGRAM_OK;
			break;

		default:
			ReturnValue = FLASH_PROGRAM_ERROR;
			break;
	}

	return ReturnValue;
}


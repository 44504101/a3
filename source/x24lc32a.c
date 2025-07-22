/*
 * x24lc32a.c
 *
 *  Created on: 2022��5��27��
 *      Author: l
 */
// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "common_data_types.h"
#include "i2c.h"
#include "x24lc32a.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

#define DEVICE_TYPE_IDENTIFIER	0x54u		///< 7 bit slave address (ignoring R/W).
#define DEVICE_ADDRESS_MASK		0x0000FFFFu	///< lowest 8 bits are device address.
#define SLAVE_ADDRESS_MASK		0x00000000u	///< bits [10:8] are slave address.
#define SLAVE_ADDRESS_SHIFT		8			///< shift by 8 bits.
#define X24LC32A_WRITE_PAGE_SIZE	32u			///< each page is 32 bytes.
#define X24LC32A_DEVICE_SIZE		4096u		///< total device size is 32k bytes.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static EI2CStatus_t local_memcpy(uint32_t StartAddress,
                                 uint16_t NumberOfWrites,
                                 const uint8_t * const p_source_buffer);

static uint16_t SlaveAddressGenerate(const uint32_t EntireAddress);
static uint16_t DeviceAddressGenerate(const uint32_t EntireAddress);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * X24LC32A_BlockRead reads from the EEPROM, starting at StartAddress.
 * StartAddress is a 32 bit address, and for this chip we use the least
 * significant 11 bits, and add in the device type identifier.
 * Note that the slave address does NOT include the R/!W bit - this is only
 * the seven bits (1010 A2 A1 A0 in this case).
 *
 * @param	StartAddress		Initial address to start reading from.
 * @param	NumberOfReads		Number of bytes to read from the device.
 * @param	pDestBuffer[]		Pointer to buffer to put data in.
 * @retval	EI2CStatus_t		I2C bus status.
 *
 */
// ----------------------------------------------------------------------------
EI2CStatus_t X24LC32A_BlockRead(const uint32_t StartAddress,
								const uint16_t NumberOfReads,
								uint8_t * const p_destination_buffer)
{
	EI2CStatus_t	status;
	uint16_t		DeviceAddress;
	uint16_t		SlaveAddress;

	// Generate slave and device addresses for this access.
	SlaveAddress = SlaveAddressGenerate(StartAddress);
	DeviceAddress = DeviceAddressGenerate(StartAddress);

	// Read from I2C device and return status.
	status = I2C_Read(SlaveAddress, DeviceAddress, NumberOfReads, p_destination_buffer);
	return status;
}
// ----------------------------------------------------------------------------
/**
 * X24LC32A_BlockWrite writes to the EEPROM, starting at StartAddress, and then
 * uses acknowledgement polling to check for write completion.
 * StartAddress is a 32 bit address, and for this chip we use the least
 * significant 11 bits, and add in the device type identifier.
 * Note that the slave address does NOT include the R/!W bit - this is only
 * the seven bits (1010 A2 A1 A0 in this case).
 * Also note that this function does not check for alignment to the page
 * boundary, or writing more than a certain number of words to the chip - this
 * is dealt with by the X24LC32A_memcmy function, which feeds data into this
 * function.
 *
 * @param	StartAddress		Initial address to start writing into.
 * @param	NumberOfWrites		Number of bytes to write into the device.
 * @param	pSourceBuffer[]		Pointer to buffer to get source data from.
 * @retval	EI2CStatus_t		I2C bus \ programming status.
 *
 */
// ----------------------------------------------------------------------------
EI2CStatus_t X24LC32A_BlockWrite(const uint32_t StartAddress,
								const uint16_t NumberOfWrites,
								const uint8_t * const p_source_buffer)
{
	EI2CStatus_t	status;
	uint16_t		DeviceAddress;
	uint16_t		SlaveAddress;

	// Generate slave and device addresses for this access.
	SlaveAddress = SlaveAddressGenerate(StartAddress);
	DeviceAddress = DeviceAddressGenerate(StartAddress);

	// Write to I2C device and return status.
	status = I2C_Write(SlaveAddress, DeviceAddress, NumberOfWrites, p_source_buffer);

	// If status is OK then poll for write complete.
	// Note that we set the timeout to zero to make the acknowledgement poll
	// function use the force timeout flag.
	if (status == I2C_COMPLETED_OK)
	{
		status = I2C_AckPoll(SlaveAddress, 0u);
	}

	return status;
}

// ----------------------------------------------------------------------------
/**
 * local_memcpy attempts to mimic the standard memcpy function for the device.
 * Data is written into the device, aligning to page boundaries as required.
 * local_memcpy ����ģ���豸�ı�׼ memcpy ������
 * ���ݱ�д���豸��������Ҫ��ҳ��߽���롣
 * @param	StartAddress		Initial address to start writing to.
 * @param	NumberOfWrites		Number of bytes to write into the device.
 * @param	p_source_buffer     Pointer to buffer containing source data.
 * @retval	EI2CStatus_t		I2C bus \ programming status.
 *
 */
// ----------------------------------------------------------------------------
static EI2CStatus_t local_memcpy(uint32_t StartAddress,
                                 uint16_t NumberOfWrites,
							     const uint8_t * const p_source_buffer)
{
	EI2CStatus_t	status = I2C_COMPLETED_OK;
	uint32_t		AddressMask;
	uint32_t		StartOffsetInPage;
	uint16_t		InternalWriteCounter;
    uint16_t        write_offset = 0u;

	// Setup address mask and work out whether start address is within a page
	// or appears nicely on a page boundary.
    // ���õ�ַ���벢ȷ����ʼ��ַ����ҳ���ڻ��Ǻܺõس�����ҳ��߽��ϡ�
	AddressMask = X24LC32A_WRITE_PAGE_SIZE - 1u;
	StartOffsetInPage = StartAddress & AddressMask;

	// If the address starts somewhere within a block, then write as many words
	// as are required to align the data (or just write the required words, if
	// there aren't more than will fill the block).
	// �����ַ�ӿ��ڵ�ĳ����ʼ����д�������������ľ����ܶ���֣�����ֻд��������֣����û�г�����������֣���
	if (StartOffsetInPage != 0u)
	{
		// Setup internal write counter for correct number of words to either
		// fill the first page, or just as many words as are required.
		// �����ڲ�д�������Ի�ȡ��ȷ������������һҳ�����߸�����Ҫ���þ����ܶ��������
		if (NumberOfWrites < (X24LC32A_WRITE_PAGE_SIZE - StartOffsetInPage) )
		{
			InternalWriteCounter = NumberOfWrites;
		}
		else
		{
		    // Note the cast - the result will never be more than 16 bits.
			// ��ע��ǿ��ת�� - �����Զ���ᳬ�� 16 λ��
		    //lint -e{921} Cast from uint32_t to uint16_t
			InternalWriteCounter = (uint16_t)(X24LC32A_WRITE_PAGE_SIZE - StartOffsetInPage);
		}

		// Write first page (might be only page).
		// д��һҳ������ֻ��һҳ����
		status = X24LC32A_BlockWrite(StartAddress,
		                           InternalWriteCounter,
		                           &p_source_buffer[write_offset]);

		// Update remaining number of bytes to write, start address and buffer pointer.
		// ����Ҫд���ʣ���ֽ�������ʼ��ַ�ͻ�����ָ�롣
		NumberOfWrites  -= InternalWriteCounter;
		StartAddress    += InternalWriteCounter;
		write_offset    += InternalWriteCounter;
	}

	// Having aligned the data to the page size, carry on writing in page sized
	// chunks (or skip this if there's less than a page to go).
	// ��������ҳ���С����󣬼���д��ҳ���С�Ŀ飨���Ҫ�ߵ�ҳ������һҳ���������˲��裩��
	while ( (NumberOfWrites >= X24LC32A_WRITE_PAGE_SIZE)
			&& (status == I2C_COMPLETED_OK) )
	{
		status = X24LC32A_BlockWrite(StartAddress,
		                           X24LC32A_WRITE_PAGE_SIZE,
		                           &p_source_buffer[write_offset]);

		NumberOfWrites  -= X24LC32A_WRITE_PAGE_SIZE;
		StartAddress    += X24LC32A_WRITE_PAGE_SIZE;
		write_offset    += X24LC32A_WRITE_PAGE_SIZE;
	}

	// If there's any more data to write (less than a page) then do it.
	// ������и�������Ҫд�루����һҳ��
	if ( (NumberOfWrites != 0u) && (status == I2C_COMPLETED_OK) )
	{
		status = X24LC32A_BlockWrite(StartAddress,
		                           NumberOfWrites,
		                           &p_source_buffer[write_offset]);
	}

	return status;
}

/**
 * Function pointer for memcpy function - set to local_memcpy as a default.
 */
//lint -e{956} Doesn't need to be volatile.
EI2CStatus_t (*X24LC32A_memcpy)(uint32_t StartAddress,
                              uint16_t NumberOfWrites,
                              const uint8_t * const p_source_buffer) = local_memcpy;


// ----------------------------------------------------------------------------
/**
 * X24LC32A_DeviceErase erases the entire device, by writing 0xFF to all locations.
 *
 * @retval	EI2CStatus_t		I2C bus \ programming status.
 *
 */
// ----------------------------------------------------------------------------
EI2CStatus_t X24LC32A_DeviceErase()
{
	uint8_t			DummyBlankArray[X24LC32A_WRITE_PAGE_SIZE];
	uint16_t		Counter;
	EI2CStatus_t	Status = I2C_ACKPOLL_TIMEOUT_EXCEEDED;
	uint32_t		Address = 0u;

	// Initialise dummy array.
	for (Counter = 0u; Counter < X24LC32A_WRITE_PAGE_SIZE; Counter++)
	{
		DummyBlankArray[Counter] = 0xFFu;
	}

	// Calculate number of pages in the device - this is the number of
	// page write operations which need to be performed.
	Counter = X24LC32A_DEVICE_SIZE / X24LC32A_WRITE_PAGE_SIZE;

	// Erase in blocks based on the page size.
	while (Counter != 0u)
	{
		Status = X24LC32A_memcpy(Address, X24LC32A_WRITE_PAGE_SIZE, DummyBlankArray);

		if (Status != I2C_COMPLETED_OK)
		{
			break;
		}

		Address += X24LC32A_WRITE_PAGE_SIZE;
		Counter--;
	}

	return Status;
}


// ----------------------------------------------------------------------------
/**
 * X24LC32A_ForceTimeoutFlagSet calls the function in the I2C driver to set the
 * force timeout flag, which will force the polling function to exit.
 *
 * @note
 * This assumes that a task that is higher priority than the one running the
 * polling function is calling this function.
 *
 */
// ----------------------------------------------------------------------------
void X24LC32A_ForceTimeoutFlagSet(void)
{
    I2C_AckPollTimeoutFlagSet();
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * SlaveAddressGenerate works out the 7 bit slave address for the EEPROM.
 * SlaveAddressGenerate ����� EEPROM �� 7 λ�ӵ�ַ��
 * @param	EntireAddress	32 bit address.
 * @retval	uint16_t		16 bit slave address (only bottom 7 bits used).
 *
 */
// ----------------------------------------------------------------------------
static uint16_t SlaveAddressGenerate(const uint32_t EntireAddress)
{
	uint16_t	SlaveAddress;

	// Mask off all address bits which don't form part of the slave address,
	// shift down into the LSB and add in the device type identifier.
    //lint -e{921} Cast from uint32_t to uint16_t, but it's ok.
	SlaveAddress = (uint16_t)(EntireAddress & SLAVE_ADDRESS_MASK);
	SlaveAddress = SlaveAddress >> SLAVE_ADDRESS_SHIFT;
	SlaveAddress |= DEVICE_TYPE_IDENTIFIER;

	return SlaveAddress;
}


// ----------------------------------------------------------------------------
/**
 * DeviceAddressGenerate works out the 16 bit device address for the EEPROM.
 * DeviceAddressGenerate ����� EEPROM �� 16 λ�豸��ַ��
 * @param	EntireAddress	32 bit address.
 * @retval	uint16_t		16 bit device address.
 *
 */
// ----------------------------------------------------------------------------
static uint16_t DeviceAddressGenerate(const uint32_t EntireAddress)
{
	uint16_t	DeviceAddress;

	// Mask off all address bits which don't form the device address.
    //lint -e{921} Cast from uint32_t to uint16_t, but it's ok.
	DeviceAddress = (uint16_t)(EntireAddress & DEVICE_ADDRESS_MASK);

	return DeviceAddress;
}


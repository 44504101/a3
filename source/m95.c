/*
 * m95.c
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */


// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "common_data_types.h"
#include "m95.h"
#include "spi.h"
#include "m95_prv.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

// These commands are taken from table 6 in the M95M01 data sheet.
#define M95_WRITE_ENABLE_COMMAND    0x06u       ///< Write enable command.
#define M95_WRITE_DISABLE_COMMAND   0x04u       ///< Write disable command.
#define M95_READ_STATUS_COMMAND     0x05u       ///< Read chip status command.
#define M95_WRITE_STATUS_COMMAND    0x01u       ///< Write chip status command.
#define M95_READ_COMMAND            0x03u       ///< Read command.
#define M95_WRITE_COMMAND           0x02u       ///< Write command.
#define M95_READ_ID_PAGE_COMMAND    0x83u       ///< Read page ID command.

#define M95_ID_PAGE_MAX_ADDRESS     0x000000FFu ///< Max page address.
#define M95_WIP_BIT_MASK            0x0001u     ///< Work In Progress bit mask.
#define M95_MAX_PAGE_SIZE           256u        ///< Maximum possible page size.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static EM95PollStatus_t local_memcpy(uint32_t StartAddress,
                                     uint32_t NumberOfWrites,
                                     const uint8_t * p_source_buffer);

static void SendAddressToDevice(const uint32_t Address);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module

/// M95 chip page size in bytes.
//lint -e{956} Doesn't need to be volatile.
static uint32_t mPageSizeInBytes;

/// M95 device size in bytes.
//lint -e{956} Doesn't need to be volatile.
static uint32_t mDeviceSizeInBytes;

/// Volatile flag to force a timeout during the polling function.
static volatile bool_t m_b_force_timeout = FALSE;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * M95_WriteEnableCommandSend sends the write enable command.
 * The SPISSTE bit is driven into the active and inactive state.
 *
 */
// ----------------------------------------------------------------------------
void M95_WriteEnableCommandSend(void)
{
    SPI_EEPROMActiveSet();
    SPI_Write(M95_WRITE_ENABLE_COMMAND);
    SPI_EEPROMInactiveSet();
}


// ----------------------------------------------------------------------------
/**
 * M95_WriteDisableCommandSend sends the write disable command.
 * The SPISSTE bit is driven into the active and inactive state.
 *
 */
// ----------------------------------------------------------------------------
void M95_WriteDisableCommandSend(void)
{
    SPI_EEPROMActiveSet();
    SPI_Write(M95_WRITE_DISABLE_COMMAND);
    SPI_EEPROMInactiveSet();
}


// ----------------------------------------------------------------------------
/**
 * M95_ReadStatusRegCommandSend sends the read status register command and
 * then performs a single read of the status register.
 * The SPISSTE bit is driven into the active and inactive state.
 *
 * Note the cast to uint8_t - the SPI_Read() function returns a 16 bit word
 * but the status is only 8 bits, so we throw the top 8 bits away.
 *
 */
// ----------------------------------------------------------------------------
uint8_t M95_ReadStatusRegCommandSend(void)
{
    uint8_t status;

    SPI_EEPROMActiveSet();
    SPI_Write(M95_READ_STATUS_COMMAND);
    status = (uint8_t)(SPI_Read(0u) & 0x00FFu); //lint !e921 Cast from uint16_t to uint8_t.
    SPI_EEPROMInactiveSet();

    return status;
}


// ----------------------------------------------------------------------------
/**
 * M95_WriteStatusRegCommandSend sends the write status register command,
 * followed by the required value to set the status register to.
 * The SPISSTE bit is driven into the active and inactive state.
 * Note that a Write Enable command must be sent before this command will work.
 *
 * @param   NewStatus   Status to write into status register.
 *
 */
// ----------------------------------------------------------------------------
void M95_WriteStatusRegCommandSend(const uint8_t NewStatus)
{
    SPI_EEPROMActiveSet();
    SPI_Write(M95_WRITE_STATUS_COMMAND);
    SPI_Write(NewStatus);
    SPI_EEPROMInactiveSet();
}


// ----------------------------------------------------------------------------
/**
 * M95_ReadCommandSend reads the desired number of bytes from the chip.
 * This is achieved by sending the read command, followed by the start address,
 * followed by reading x bytes.
 * Note that the destination buffer MUST be big enough to accept the number of
 * bytes which are being read!
 * The SPISSTE bit is driven into the active and inactive state.
 *
 * Note the cast to uint8_t - the SPI_Read() function returns a 16 bit word
 * but the data width of the chip itself is only 8 bits, so we throw the top
 * 8 bits away.
 *
 * @param   StartAddress        Address to start reading from.
 * @param   NumberOfReads       Number of bytes to read from the device.
 * @param   p_dest_buffer       Pointer to buffer to put read data in.
 *
 */
// ----------------------------------------------------------------------------
void M95_ReadCommandSend(const uint32_t StartAddress,
                            const uint32_t NumberOfReads,
                            uint8_t * const p_dest_buffer)
{
    uint32_t    Counter;

    SPI_EEPROMActiveSet();
    SPI_Write(M95_READ_COMMAND);
    SendAddressToDevice(StartAddress);

    // Read required number of bytes and put in buffer.
    // NOTE - no boundary checking for pointer!
    for (Counter = 0u; Counter < NumberOfReads; Counter++)
    {
        //lint -e{921} Cast from uint16_t to uint8_t.
        p_dest_buffer[Counter] = (uint8_t)(SPI_Read(0u) & 0x00FFu);
    }

    SPI_EEPROMInactiveSet();
}


// ----------------------------------------------------------------------------
/**
 * M95_WriteCommandSend writes the desired number of bytes into the chip.
 * This is achieved by sending the write command, followed by the start address,
 * followed by the required data.
 * The SPISSTE bit is driven into the active and inactive state.
 * Note that a Write Enable command must be sent before this command will work,
 * and this function doesn't 'know' about any address boundaries - this is
 * dealt with by the next layer up in the code.
 *
 * @param   StartAddress        Initial address to start writing to.
 * @param   NumberOfWrites      Number of bytes to write into the device.
 * @param   p_source_buffer     Pointer to buffer containing source data.
 *
 */
// ----------------------------------------------------------------------------
void M95_WriteCommandSend(const uint32_t StartAddress,
                            const uint32_t NumberOfWrites,
                            const uint8_t * const p_source_buffer)
{
    uint32_t    Counter;

    SPI_EEPROMActiveSet();
    SPI_Write(M95_WRITE_COMMAND);
    SendAddressToDevice(StartAddress);

    // Write required number of bytes into device.
    // NOTE - no boundary checking for device pages \ source overrun!
    for (Counter = 0u; Counter < NumberOfWrites; Counter++)
    {
        SPI_Write(p_source_buffer[Counter]);
    }

    SPI_EEPROMInactiveSet();
}


// ----------------------------------------------------------------------------
/**
 * M95_ReadIDCommandSend reads a number of bytes from the identification page.
 * This is achieved by sending the read ID command, followed by the start address,
 * followed by reading x bytes.
 * Note that the destination buffer MUST be big enough to accept the number of
 * bytes which are being read!
 * The SPISSTE bit is driven into the active and inactive state.
 *
 * Note the cast to uint8_t - the SPI_Read() function returns a 16 bit word
 * but the data width of the chip itself is only 8 bits, so we throw the top
 * 8 bits away.
 *
 * @param   StartAddress        Address to start reading from.
 * @param   NumberOfReads       Number of bytes to read from the device.
 * @param   p_dest_buffer       Pointer to buffer to put read data in.
 * @retval  bool_t              TRUE if parameters OK, FALSE if invalid.
 *
 */
// ----------------------------------------------------------------------------
bool_t M95_ReadIDCommandSend(const uint32_t StartAddress,
                                const uint32_t NumberOfReads,
                                uint8_t * const p_dest_buffer)
{
    uint32_t    Counter;
    bool_t      ReturnStatus = TRUE;

    // If any condition which would mean that we try and read beyond the end of
    // the ID page will occur, then don't allow any reading at all.
    if ( (NumberOfReads > mPageSizeInBytes)
            || (StartAddress > M95_ID_PAGE_MAX_ADDRESS)
            || ( (StartAddress + NumberOfReads) > (M95_ID_PAGE_MAX_ADDRESS + 1u)) )
    {
        ReturnStatus = FALSE;
    }
    else
    {
        SPI_EEPROMActiveSet();
        SPI_Write(M95_READ_ID_PAGE_COMMAND);

        SendAddressToDevice(StartAddress);

        // Read required number of bytes and put in buffer.
        // NOTE - no boundary checking for pointer!
        for (Counter = 0u; Counter < NumberOfReads; Counter++)
        {
            //lint -e{921} Cast from uint16_t to uint8_t.
            p_dest_buffer[Counter] = (uint8_t)(SPI_Read(0u) & 0x00FFu);
        }

        SPI_EEPROMInactiveSet();
    }

    return ReturnStatus;
}


// ----------------------------------------------------------------------------
/**
 * M95_WriteCompletePoll polls the device to check for write not in progress.
 * This function contains a timeout, which is specified as the number of times
 * the function can be called - if this is set to zero then it will run forever.
 *
 * @note
 * We exclude the reset of the force timeout flag at the start of this function
 * if we're running unit tests, otherwise we can never check for a timeout.
 *
 * @retval  EM95PollStatus_t    Enumerated return value.
 *
 */
// ----------------------------------------------------------------------------
EM95PollStatus_t M95_WriteCompletePoll(void)
{
    EM95PollStatus_t    M95PollStatus = M95_POLL_NO_WRITE_IN_PROGRESS;
    uint16_t            StatusRegister = 1u;

#ifndef UNIT_TEST_BUILD
    m_b_force_timeout = FALSE;
#endif

    // While the status register (masked) is not zero, this means that the
    // WIP bit is still set, so we haven't finished writing yet.
    while (StatusRegister != 0u)
    {
        // Read the status register and mask off everything except the WIP bit.
        StatusRegister = M95_ReadStatusRegCommandSend();
        StatusRegister &= M95_WIP_BIT_MASK;

        // If the force timeout flag is set then jump out,
        // assuming the status register is non-zero
        // (i.e we didn't finish this time round).
        if ( (m_b_force_timeout) && (StatusRegister != 0u) )
        {
            M95PollStatus = M95_POLL_TIMEOUT_EXCEEDED;
            break;
        }
    }

    return M95PollStatus;
}


// ----------------------------------------------------------------------------
// HIGHER LEVEL FUNCTIONS BELOW HERE
// ----------------------------------------------------------------------------
/**
 * M95_BlockRead uses the M95_ReadCommandSend function to read a number of
 * bytes from the device.  We do this so that this function can be mocked out
 * for testing, and it then 'matches' the BlockWrite function below (although
 * we could just have done the same thing to ReadCommandSend, but this seems
 * tidier).
 *
 * @param   StartAddress        Initial address to start reading from.
 * @param   NumberOfReads       Number of bytes to read from the device.
 * @param   p_dest_buffer       Pointer to buffer to put data in.
 *
 */
// ----------------------------------------------------------------------------
void M95_BlockRead(const uint32_t StartAddress,
                    const uint32_t NumberOfReads,
                    uint8_t * const p_dest_buffer)
{
    M95_ReadCommandSend(StartAddress, NumberOfReads, p_dest_buffer);
}


// ----------------------------------------------------------------------------
/**
 * M95_BlockWrite enables the device write, writes a number of bytes into the
 * device and then polls for write completion.  We do this so that this function
 * can be mocked out when we're testing the M95_memcpy function (rather than
 * having to mock out the enable, write and poll functions, or add lots of
 * expectations for these tests).
 * Note that this function doesn't 'know' about any address boundaries - this
 * is dealt with by the next layer up in the code.
 *
 * @param   StartAddress        Initial address to start writing to.
 * @param   NumberOfWrites      Number of bytes to write into the device.
 * @param   p_source_buffer     Pointer to buffer containing source data.
 * @retval  EM95PollStatus_t    Enumerated return value.
 *
 */
// ----------------------------------------------------------------------------
EM95PollStatus_t M95_BlockWrite(const uint32_t StartAddress,
                                const uint32_t NumberOfWrites,
                                const uint8_t * const p_source_buffer)
{
    EM95PollStatus_t    M95PollStatus;

    M95_WriteEnableCommandSend();
    M95_WriteCommandSend(StartAddress, NumberOfWrites, p_source_buffer);
    M95PollStatus = M95_WriteCompletePoll();

    return M95PollStatus;
}


// ----------------------------------------------------------------------------
/**
 * local_memcpy attempts to mimic the standard memcpy function for the device.
 * Data is written into the device, aligning to page boundaries as required.
 *
 * @param   StartAddress        Initial address to start writing to.
 * @param   NumberOfWrites      Number of bytes to write into the device.
 * @param   p_source_buffer     Pointer to buffer containing source data.
 * @retval  EM95PollStatus_t    Enumerated return value.
 *
 */
// ----------------------------------------------------------------------------
static EM95PollStatus_t local_memcpy(uint32_t StartAddress,
                                     uint32_t NumberOfWrites,
                                     const uint8_t * const p_source_buffer)
{
    EM95PollStatus_t    M95PollStatus = M95_POLL_NO_WRITE_IN_PROGRESS;
    uint32_t            AddressMask;
    uint32_t            StartOffsetInPage;
    uint32_t            InternalWriteCounter;
    uint32_t            write_offset = 0u;

    // Setup address mask and work out whether start address is within a page
    // or appears nicely on a page boundary.
    AddressMask = mPageSizeInBytes - 1u;
    StartOffsetInPage = StartAddress & AddressMask;

    // If the address starts somewhere within a block, then write as many words
    // as are required to align the data (or just write the required words, if
    // there aren't more than will fill the block).
    if (StartOffsetInPage != 0u)
    {
        // Setup internal write counter for correct number of words to either
        // fill the first page, or just as many words as are required.
        if (NumberOfWrites < (mPageSizeInBytes - StartOffsetInPage) )
        {
            InternalWriteCounter = NumberOfWrites;
        }
        else
        {
            InternalWriteCounter = mPageSizeInBytes - StartOffsetInPage;
        }

        // Write first page (might be only page).
        M95PollStatus = M95_BlockWrite(StartAddress,
                                       InternalWriteCounter,
                                       &p_source_buffer[write_offset]);

        // Update remaining number of bytes to write, start address and buffer pointer.
        NumberOfWrites  -= InternalWriteCounter;
        StartAddress    += InternalWriteCounter;
        write_offset    += InternalWriteCounter;
    }

    // Having aligned the data to the page size, carry on writing in page sized
    // chunks (or skip this if there's less than a page to go).
    while ( (NumberOfWrites >= mPageSizeInBytes)
            && (M95PollStatus == M95_POLL_NO_WRITE_IN_PROGRESS) )
    {
        M95PollStatus   = M95_BlockWrite(StartAddress,
                                         mPageSizeInBytes,
                                         &p_source_buffer[write_offset]);

        NumberOfWrites  -= mPageSizeInBytes;
        StartAddress    += mPageSizeInBytes;
        write_offset    += mPageSizeInBytes;
    }

    // If there's any more data to write (less than a page) then do it.
    if ( (NumberOfWrites != 0u) && (M95PollStatus == M95_POLL_NO_WRITE_IN_PROGRESS) )
    {
        M95PollStatus = M95_BlockWrite(StartAddress,
                                       NumberOfWrites,
                                       &p_source_buffer[write_offset]);
    }

    return M95PollStatus;
}

/**
 * Function pointer for memcpy function - set to local_memcpy as a default.
 */
//lint -e{956} Doesn't need to be volatile.
EM95PollStatus_t (*M95_memcpy)(const uint32_t StartAddress,
                               const uint32_t NumberOfWrites,
                               const uint8_t * const p_source_buffer) = local_memcpy;


// ----------------------------------------------------------------------------
/**
 * M95_DeviceSizeInitialise initialises the variables which hold the page
 * and device size for the device.  Note that this cannot be easily determined
 * automatically, as the address which is written to the device is either 1, 2
 * or 3 bytes depending of the actual device which is fitted.
 *
 * @param   PageSizeInBytes     Page size, in bytes.
 * @param   DeviceSizeInBytes   Device size, in bytes.
 *
 */
// ----------------------------------------------------------------------------
void M95_DeviceSizeInitialise(const uint32_t PageSizeInBytes,
                                const uint32_t DeviceSizeInBytes)
{
    mPageSizeInBytes = PageSizeInBytes;
    mDeviceSizeInBytes = DeviceSizeInBytes;
}


// ----------------------------------------------------------------------------
/**
 * M95_DevicePageSizeGet returns the page size, in bytes, which has been setup
 * using the initialise function and stored in mPageSizeInBytes.
 *
 * @retval  uint32_t    Page size, in bytes.
 *
 */
// ----------------------------------------------------------------------------
uint32_t M95_DevicePageSizeGet(void)
{
    return mPageSizeInBytes;
}


// ----------------------------------------------------------------------------
/**
 * M95_DeviceTotalSizeGet returns the total device size, in bytes, which has
 * been setup using the initialise function and stored in mDeviceSizeInBytes.
 *
 * @retval  uint32_t    Page size, in bytes.
 *
 */
// ----------------------------------------------------------------------------
uint32_t M95_DeviceTotalSizeGet(void)
{
    return mDeviceSizeInBytes;
}


// ----------------------------------------------------------------------------
/**
 * M95_DeviceErase erases the entire device, by writing 0xFF to all locations.
 *
 * @retval  EM95PollStatus_t    Enumerated return value.
 *
 */
// ----------------------------------------------------------------------------
EM95PollStatus_t M95_DeviceErase(void)
{
    uint8_t             DummyBlankArray[M95_MAX_PAGE_SIZE];
    uint32_t            Counter;
    EM95PollStatus_t    PollStatus = M95_POLL_TIMEOUT_EXCEEDED;
    uint32_t            Address = 0u;

    // Initialise dummy array.
    for (Counter = 0u; Counter < M95_MAX_PAGE_SIZE; Counter++)
    {
        DummyBlankArray[Counter] = 0xFFu;
    }

    // Calculate number of pages in the device - this is the number of
    // page write operations which need to be performed.
    Counter = mDeviceSizeInBytes / mPageSizeInBytes;

    // Erase in blocks based on the page size.
    while (Counter != 0u)
    {
        PollStatus = M95_memcpy(Address, mPageSizeInBytes, DummyBlankArray);

        if (PollStatus != M95_POLL_NO_WRITE_IN_PROGRESS)
        {
            break;
        }

        Address += mPageSizeInBytes;
        Counter--;
    }

    return PollStatus;
}


// ----------------------------------------------------------------------------
/**
 * M95_ForceTimeoutFlagSet set the force timeout flag, which will force
 * the polling function to exit.
 *
 * @note
 * This assumes that a task that is higher priority than the one running the
 * polling function is calling this function.
 *
 */
// ----------------------------------------------------------------------------
void M95_ForceTimeoutFlagSet(void)
{
    m_b_force_timeout = TRUE;
}


#ifdef UNIT_TEST_BUILD
// ----------------------------------------------------------------------------
/**
 * M95_ForceTimeoutFlagReset_TDD resets the force timeout flag.
 *
 * This is only required when unit testing, hence the conditional compilation.
 *
 */
// ----------------------------------------------------------------------------

void M95_ForceTimeoutFlagReset_TDD(void)
{
    m_b_force_timeout = FALSE;
}


// ----------------------------------------------------------------------------
/**
 * M95_MemcpyFunctionPtrReset_TDD resets the function pointer to point to the
 * 'proper' memcpy function.
 *
 * This is only required when unit testing, hence the conditional compilation.
 *
 */
// ----------------------------------------------------------------------------
void M95_MemcpyFunctionPtrReset_TDD(void)
{
    //lint -e{546} Suspicious use of & - it's correct, we want the address.
    M95_memcpy = &local_memcpy;
}
#endif


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * SendAddressToDevice writes the 24 bit address to the device, as 3 x writes
 * (bits 23:16, 16:8 and 7:0)
 *
 * @warning
 * This code doesn't cater for use on the very small devices (<= 4kbit) which
 * use an 8 bit address but add an extra address bit into the instruction word.
 *
 * @param   Address     24 bit address to write.
 *
 *
 */
// ----------------------------------------------------------------------------
static void SendAddressToDevice(const uint32_t Address)
{
    uint16_t    MaskedAddress;

    if (mDeviceSizeInBytes > 65536u)
    {
        // Generate top 8 address bits - shift down by 16 and mask off.
        //lint -e{921} Cast from uint32_t to uint16_t, but it's ok.
        MaskedAddress = (uint16_t)((Address >> 16u) & 0x000000FFu);
        SPI_Write(MaskedAddress);
    }

    // Generate middle 8 address bits - shift down by 8 and mask off.
    //lint -e{921} Cast from uint32_t to uint16_t, but it's ok.
    MaskedAddress = (uint16_t)((Address >> 8u) & 0x000000FFu);
    SPI_Write(MaskedAddress);

    // Generate bottom 8 address bits - just mask off.
    //lint -e{921} Cast from uint32_t to uint16_t, but it's ok.
    MaskedAddress = (uint16_t)(Address & 0x000000FFu);
    SPI_Write(MaskedAddress);
}

// ----------------------------------------------------------------------------
/*!
 * @file        XDImemory.c
 * @author      Fei Li
 * @date        October 2016
 * @brief		Handles the transactions between TSDnM and the serial EEPROM used
 * 				to store the XDI coefficients.
 *
 * @details     The serial EEPROM is not included in the recording system,
 * 				however the transactions with this device are performed through
 * 				the TSDnM opcode005 and opcode006.
 * 				The read and write operation mimic the behaviour of the
 * 				recording system when a read or a write request is performed
 * 				except that the data are read or written immediately.
 * 				The request status are then updated and the request semaphore released.
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "rsapi.h"
#include "i2c.h"
#include "x24lc32a.h"
#include "XDImemory.h"
#include "crc.h"

// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define DNI_PROM_ADDRESS_OFFSET		0x400u  ///< DnI PROM address offset.
#define XDI_MEMORY_SIZE				1024u   ///< 1024 bytes memory size.
#define WRITE_BUFFER_LENGTH_MSB_IDX	3u      ///< Index to the data length MSB in the write buffer.
#define WRITE_BUFFER_LENGTH_LSB_IDX	4u      ///< Index to the data length LSB in the write buffer.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static void   ReadBufferDataLeftShift(uint8_t* const p_buffer,
                                      const uint16_t bufferSize,
                                      const uint16_t leftShiftValue);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*!
 * XDIMEMORY_ReadRequest passes a read request on to the XDImemory.
 *
 * @note
 * All arguments to this function are pointers, and are used to return various
 * things to the calling function.
 *
 * @param   p_readBuffer    Pointer to the buffer to put the data in.
 * @param   p_readLength    Pointer to the variable to update with the length.
 * @param   p_readStatus    Pointer to the variable to update with the status.
 * @retval  bool_t          TRUE if request accepted OK, FALSE if any error.
 *
 */
// ----------------------------------------------------------------------------
bool_t XDIMEMORY_ReadRequest(uint8_t * const p_readBuffer,
                             uint16_t * const p_readLength,
                             rs_queue_status_t * const p_readStatus)
{
    const uint32_t  address = DNI_PROM_ADDRESS_OFFSET;
    EI2CStatus_t 	requestStatus;
    bool_t 			b_readRequestAcknowledge = FALSE;

    if ( (NULL != p_readBuffer) && (NULL != p_readLength) && (NULL != p_readStatus) )
    {
        // Read the all EEPROM content.
        requestStatus = X24LC32A_BlockRead(address,
                                         XDI_MEMORY_SIZE,
                                         p_readBuffer);

        // Update the read request status.
        if (I2C_COMPLETED_OK == requestStatus)
        {
            b_readRequestAcknowledge = TRUE;

            *p_readStatus = RS_QUEUE_REQUEST_COMPLETE;

            // Get the record length.
            //lint -e{921} -e{9033} cast from uint8_t to uint16_t.
            *p_readLength
                = (((uint16_t)p_readBuffer[0u] & 0x00FFu) << 8u) +
                    (uint16_t)(p_readBuffer[1u] & 0x00FFu);

            /*
             * If the data length is greater than XDI memory size,
             * the function returns all the memory content so that
             * the user can check it.
             */
            if (*p_readLength < XDI_MEMORY_SIZE)
            {
                // Shift the buffer content to keep only the data.
                ReadBufferDataLeftShift(p_readBuffer,
                                        *p_readLength ,
                                        2u);
            }
            else
            {
                *p_readLength = XDI_MEMORY_SIZE;
            }
        }
        else if (I2C_BUS_BUSY == requestStatus)
        {
            *p_readStatus = RS_QUEUE_REQUEST_IN_PROGRESS;
        }
        else
        {
            *p_readStatus = RS_QUEUE_REQUEST_FAILED;
        }
    }

    return b_readRequestAcknowledge;
}


// ----------------------------------------------------------------------------
/*!
 * XDIMEMORY_write_request passes a write request on to the XDImemory.
 *
 * @param   p_writeRequest Pointer to read request.
 * @retval  bool_t          TRUE if request accepted OK, FALSE if any error.
 *
 */
// ----------------------------------------------------------------------------
bool_t XDIMEMORY_WriteRequest(uint8_t * const p_writeBuffer,
                              const uint16_t numberOfBytesToWrite,
                              rs_queue_status_t * const p_writeStatus)
{
    const uint32_t  address 				  = DNI_PROM_ADDRESS_OFFSET;
    const uint16_t  totalNumberOfBytesToWrite = numberOfBytesToWrite + 2u;
    EI2CStatus_t 	requestStatus;
    bool_t 			b_writeRequestAcknowledge = FALSE;
    uint16_t                running_crc;
    uint8_t crc_length;

    if ( (NULL != p_writeBuffer) && (NULL != p_writeStatus) )
    {
        if (totalNumberOfBytesToWrite <= XDI_MEMORY_SIZE)
        {
            // Write the "record size" in the XDI memory (MSB, LSB).
            // These are the two extra bytes written just before the data section.
            //lint -e{921} cast from uint16_t to uint8_t.

            p_writeBuffer[0u] = 0xE1u;
            p_writeBuffer[1u] = (uint8_t)(72 & 0x00FFu);
            p_writeBuffer[2u] = (uint8_t)((72 >> 8u) & 0x00FFu);
            p_writeBuffer[WRITE_BUFFER_LENGTH_MSB_IDX]
                = (uint8_t)((numberOfBytesToWrite >> 8u) & 0x00FFu);

            //lint -e{921} cast from uint16_t to uint8_t.
            p_writeBuffer[WRITE_BUFFER_LENGTH_LSB_IDX]
                = (uint8_t)(numberOfBytesToWrite & 0x00FFu);


            // Ìí¼Ó CRC Ð£Ñé
            crc_length = numberOfBytesToWrite - 3;
            running_crc = CRC_CCITTOnByteCalculate(&p_writeBuffer, crc_length, 0x0000u);

            p_writeBuffer[crc_length] = (uint8_t)((running_crc >> 8u) & 0x00FFu);
            p_writeBuffer[crc_length + 1u] = (uint8_t)(running_crc & 0x00FFu);
            p_writeBuffer[crc_length + 2u] = 0x1A;
            // Copy the write buffer content into the 1Kbyte i2c EEPROM.
            requestStatus
                = X24LC32A_memcpy(address,
                                totalNumberOfBytesToWrite,
                                &p_writeBuffer[WRITE_BUFFER_LENGTH_MSB_IDX]);

            // Update the write request status.
            if (I2C_COMPLETED_OK == requestStatus)
            {
                b_writeRequestAcknowledge = TRUE;
                *p_writeStatus 	          = RS_QUEUE_REQUEST_COMPLETE;
            }
            else
            {
                *p_writeStatus = RS_QUEUE_REQUEST_FAILED;
            }
        }
    }

    return b_writeRequestAcknowledge;
}


// ----------------------------------------------------------------------------
/*!
 * XDIMEMORY_erase_request erases the XDI memory.
 *
 * Even though the device does not need to be erased before writing, we erase
 * the device here because it's useful for testing, to start with a blank
 * device.
 *
 * @retval  bool_t      TRUE if device erases OK, FALSE if some error.
 *
 */
// ----------------------------------------------------------------------------
bool_t XDIMEMORY_EraseRequest(void)
{
    EI2CStatus_t    requestStatus;
    bool_t          b_erasedOK = FALSE;

    requestStatus = X24LC32A_DeviceErase();

    if (I2C_COMPLETED_OK == requestStatus)
    {
        b_erasedOK = TRUE;
    }

    return b_erasedOK;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*!
 * ReadBufferDataLeftShift left shifts a buffer content "LeftShiftValue"
 *
 * @param   pBuffer 		Pointer to the buffer
 * @param	BufferSize		Buffer size.
 * @retval  LeftShiftValue  Left shift value.
 *
 */
// ----------------------------------------------------------------------------
static void ReadBufferDataLeftShift(uint8_t* const p_buffer,
                                    const uint16_t bufferSize,
                                    const uint16_t leftShiftValue)
{
    uint16_t loop;

    if (bufferSize > leftShiftValue)
    {
        for (loop = 0u; loop < (bufferSize - leftShiftValue); loop++)
        {
            p_buffer[loop] = p_buffer[loop + leftShiftValue];
        }
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

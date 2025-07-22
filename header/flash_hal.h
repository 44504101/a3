// ----------------------------------------------------------------------------
/**
 * @file    	flash_hal.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		19 Feb 2016
 * @brief		Header file for flash_hal.c
 * @details     This header file provides the function prototypes for which
 *              source code must be written by any project wishing to use the
 *              recording system.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SOURCE_FLASH_HAL_H_
#define SOURCE_FLASH_HAL_H_

#include "rsappconfig.h"
/**
 * Enumerated type for flash error status.
 *
 * Returned by the flash_hal_device_read(), flash_hal_device_write()
 * and flash_hal_device_erase() functions.
 *
 * @note
 * An invalid address error implies that something went wrong in the conversion
 * from logical to physical address, a write fail indicates any other error.
 *
 */
typedef enum
{
    FLASH_HAL_NO_ERROR,             ///< No error, operation completed OK.
    FLASH_HAL_INVALID_ADDRESS,      ///< Invalid address passed in.
    FLASH_HAL_WRITE_FAIL            ///< Flash write failed.
} flash_hal_error_t;

/**
 * Structure for holding logical address map information in.
 */
typedef struct
{
    storage_devices_t   device_to_use;  ///< Storage device to store the partition in.
    uint32_t            start_address;  ///< First logical address in the partition.
    uint32_t            end_address;    ///< Last logical address in the partition.
} flash_hal_logical_t;

/**
 * Structure for holding flash physical information in.
 *
 * @note
 * The block size is the minimum sized block of memory which can be
 * erased on the storage device - often called the sector size.
 */
typedef struct
{
    storage_devices_t   device_to_use;      ///< The storage device identifier.
    uint32_t            start_address;      ///< First physical address on the device.
    uint32_t            end_address;        ///< Last physical address on the device.
    uint32_t            block_size_bytes;   ///< Block size on the device.
} flash_physical_arrangement_t;


/**
 * flash_hal_initialise sets up the flash HAL.
 *
 * @param   p_logical_addresses Pointer to array of logical addresses to use.
 * @retval  bool_t              TRUE if initialised OK, FALSE if not.
 *
 */
bool_t              flash_hal_initialise(const flash_hal_logical_t * const p_logical_addresses);


/**
 * flash_hal_block_size_bytes_get returns the block size for each of the
 * available devices.
 *
 * The block size is the minimum size which can be erased on the device.
 *
 * @warning
 * This is the only function in the flash HAL which should be called
 * before the flash HAL is initialised.
 *
 * @note
 * The recording system needs this function to set up the logical addresses
 * such that a partition is a whole number of blocks.  Once the logical
 * addresses have been set up, the flash_hal_initialise() function should
 * then be called BEFORE using any of the other functions in this flash HAL.
 *
 * @param    device_identifier   Device to return block size for.
 * @retval  uint32_t            Block size in BYTES.
 *
 */
uint32_t            flash_hal_block_size_bytes_get
                        (const storage_devices_t device_identifier);


/**
 * flash_hal_device_read converts logical to physical address and then
 * calls the appropriate flash driver function to perform the read.
 *
 * @note
 * The logical start address is a BYTE ADDRESS.
 *
 * @param   logical_start_address       The logical start address for the read.
 * @param   number_of_bytes_to_read     The number of bytes to read.
 * @param   p_read_data                 Pointer to buffer to put read data in.
 * @retval  flash_hal_error_t           Enumerated value for read status.
 *
 */
flash_hal_error_t   flash_hal_device_read
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_read,
                         uint8_t * const p_read_data);


/**
 * flash_hal_device_write converts logical to physical address and then
 * calls the appropriate flash driver function to perform the write.
 *
 * @note
 * The logical start address is a BYTE ADDRESS.
 *
 * @param   logical_start_address       The logical start address for the write.
 * @param   number_of_bytes_to_write    The number of bytes to write.
 * @param   p_write_data                Pointer to buffer containing data to write.
 * @retval  flash_hal_error_t           Enumerated value for write status.
 *
 */
flash_hal_error_t   flash_hal_device_write
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_write,
                         const uint8_t * const p_write_data);


/**
 * flash_hal_device_erase converts logical to physical address and then
 * calls the appropriate flash driver function to perform the erase.
 *
 * The erase function doesn't care about the partitions (the HAL doesn't really
 * know about partitions) but it should check that we're not trying to erase
 * more of a device than actually exists.
 *
 * @note
 * The logical start address is a BYTE ADDRESS.
 *
 * @param   logical_start_address       The logical start address for the erase.
 * @param   number_of_bytes_to_erase    The number of bytes to erase.
 * @retval  flash_hal_error_t           Enumerated value for erase status.
 *
 */
flash_hal_error_t   flash_hal_device_erase
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_erase);


/**
 * flash_hal_device_blank_check converts logical to physical address and then
 * calls the appropriate flash driver function to check for the device being
 * blank.
 *
 * @note
 * The logical start address is a BYTE ADDRESS.
 *
 * @param   logical_start_address           The logical start address for the check.
 * @param   number_of_bytes_to_blank_check  The number of bytes to blank check.
 * @retval  bool_t                          TRUE if blank, FALSE if not blank.
 *
 */
bool_t              flash_hal_device_blank_check
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_blank_check);


/**
 * flash_hal_write_timeout_callbck is the callback function for the flash
 * write timeout.
 *
 * Based on which flash is currently being used, call the appropriate
 * function to force the timeout flag to be set (and stop the device polling).
 *
 * @note
 * The function prototype must match that specified by FreeRTOS, even though
 * we're not using the parameter in this case.
 *
 * @note
 * We use a void* instead of the FreeRTOS type TimerHandle_t to save us having
 * to include the FreeRTOS header files everywhere.
 *
 * @param   xTimer      Timer handle.
 *
 */
void                flash_hal_write_timeout_callbck(void* xTimer);


#endif /* SOURCE_FLASH_HAL_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

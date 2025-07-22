/*
 * flash_hal.c
 *
 *  Created on: 2022年5月27日
 *      Author: l
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "rsappconfig.h"
#include "flash_hal.h"
#include "flash_hal_prv.h"
#include "lld.h"            // chipset drivers for Spansion flash (main flash)
#include "m95.h"            // chipset drivers for M95M01 serial flash
#include "i2c.h"
#include "x24lc32a.h"         // chipset drivers for X24LC32A serial EEPROM
#include "buffer_utils.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define MAIN_FLASH_LOWER_DEVICE_MAX     0x04000000u     ///< maximum word address in device zero
#define M95_PAGE_SIZE_IN_BYTES          128u            ///< Page is 128 bytes
#define X24LC32A_PAGE_SIZE_IN_BYTES       32u             ///< Page is 16 bytes


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static bool_t   setup_address_mapping
                        (const flash_hal_logical_t * const p_logical_addresses);

static bool_t   check_physical_structure(void);

static bool_t   convert_from_logical_2_physical(const uint32_t logical_address,
                                                const uint32_t bytes_required,
                                                uint32_t * const p_physical_address,
                                                storage_devices_t * const p_device);

static void     main_flash_read(const uint32_t byte_address,
                                const uint32_t bytes_to_read,
                                uint8_t * const p_byte_data);

static flash_hal_error_t main_flash_write(const uint32_t byte_address,
                                          const uint32_t bytes_to_write,
                                          const uint8_t * const p_byte_data);

static flash_hal_error_t main_flash_partial_erase(const uint32_t byte_address,
                                                  const uint32_t bytes_to_erase);

static flash_hal_error_t serial_flash_partial_erase(const uint32_t byte_address,
                                                    const uint32_t bytes_to_erase);

static flash_hal_error_t eeprom_partial_erase(const uint32_t byte_address,
                                              const uint32_t bytes_to_erase);

static bool_t main_flash_blank_check(const uint32_t byte_address,
                                     const uint32_t bytes_to_check);

static bool_t serial_flash_blank_check(const uint32_t byte_address,
                                       const uint32_t bytes_to_check);

static bool_t eeprom_blank_check(const uint32_t byte_address,
                                 const uint32_t bytes_to_check);

static bool_t blank_check_buffer(const uint8_t * const p_buffer,
                                 const uint32_t length);

static bool_t check_one_flash_address_blank(const uint32_t word_address);

static bool_t check_one_flash_sector_blank(const uint32_t start_word_address);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

static const flash_physical_arrangement_t m_physical_addresses[]
                                              = FLASH_HAL_PHYSICAL_ADDRESSES;

//lint -e{956} Doesn't need to be volatile.
static bool_t                 m_b_flash_hal_initialised = FALSE;

//lint -e{956} Doesn't need to be volatile.
static address_translation_t  m_address_map[RS_CFG_MAX_NUMBER_OF_PARTITIONS];

//lint -e{956} Doesn't need to be volatile.
static storage_devices_t      m_current_device_used_for_write;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*!
 * flash_hal_initialise sets up the flash HAL.
 *
 * @param   p_logical_addresses Pointer to array of logical addresses to use.
 * @retval  bool_t              TRUE if initialised OK, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
bool_t flash_hal_initialise(const flash_hal_logical_t * const p_logical_addresses)
{
    bool_t  b_physical_structure_ok;
    bool_t  b_address_mapping_ok;

    m_b_flash_hal_initialised = FALSE;

    if (p_logical_addresses != NULL)
    {
        b_physical_structure_ok = check_physical_structure();

        if (b_physical_structure_ok)
        {
            b_address_mapping_ok = setup_address_mapping(p_logical_addresses);

            if (b_address_mapping_ok)
            {
                /* Set initialised flag only if physical AND mapping OK. */
                m_b_flash_hal_initialised = TRUE;
            }
        }
    }

    return m_b_flash_hal_initialised;
}


// ----------------------------------------------------------------------------
/*!
 * flash_hal_block_size_bytes_get returns the block size for each of the
 * available devices.
 * flash_hal_block_size_bytes_get 返回每个可用设备的块大小。
 * The block size is the minimum size which can be erased on the device.
 * 块大小是可以在设备上擦除的最小大小。
 * @param	device_identifier   Device to return block size for.
 * @retval	uint32_t            Block size in BYTES.
 *
 */
// ----------------------------------------------------------------------------
uint32_t flash_hal_block_size_bytes_get
                        (const storage_devices_t device_identifier)
{
    uint32_t    block_size_in_bytes;

    block_size_in_bytes
        = m_physical_addresses[device_identifier].block_size_bytes;

    return block_size_in_bytes;
}


// ----------------------------------------------------------------------------
/*!
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
// ----------------------------------------------------------------------------
flash_hal_error_t flash_hal_device_read
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_read,
                         uint8_t * const p_read_data)
{
    uint32_t            physical_address;
    storage_devices_t   physical_device;
    bool_t              b_converted_ok;
    flash_hal_error_t   read_status = FLASH_HAL_INVALID_ADDRESS;

    b_converted_ok = convert_from_logical_2_physical(logical_start_address,
                                                     number_of_bytes_to_read,
                                                     &physical_address,
                                                     &physical_device);

    if (b_converted_ok)
    {
        //lint -e{788} Not all enum types used in switch, but we have a default case.
        switch (physical_device)
        {
            /*
             * The main flash is a word-addressable device.
             * Only read from the main flash if the address is a word address
             * and the number of bytes is even (i.e. a whole number of words).
             */
            case STORAGE_DEVICE_MAIN_FLASH:
                if ( ((logical_start_address & 0x00000001u) == 0u)
                        && ((number_of_bytes_to_read & 0x000000001u) == 0u) )
                {
                    main_flash_read(physical_address,
                                    number_of_bytes_to_read,
                                    p_read_data);

                    read_status = FLASH_HAL_NO_ERROR;
                }
            break;

            /* The serial flash is a byte-addressable device.*/
            case STORAGE_DEVICE_SERIAL_FLASH:
                M95_BlockRead(physical_address,
                              number_of_bytes_to_read,
                              p_read_data);

                read_status = FLASH_HAL_NO_ERROR;
            break;

            /*
             * The I2C EEPROM is a byte-addressable device.
             * Discard return value from read - if it doesn't work we'll
             * deal with this in the layer above.
             * Also note the cast to uint16_t - number of bytes is only
             * 16 bits in the eeprom driver, but assuming the memory is
             * setup correctly you'll never try and read too many.
             */
            case STORAGE_DEVICE_I2C_EEPROM:
                //lint -e{920} -e{921} Cast from enum->void, uint32_t->uint16_t
                (void)X24LC32A_BlockRead(physical_address,
                                       (uint16_t)number_of_bytes_to_read,
                                       p_read_data);

                read_status = FLASH_HAL_NO_ERROR;
            break;

            default:
                /*
                 * Default case doesn't set the read status
                 * so we'll just return FLASH_HAL_INVALID_ADDRESS.
                 * Shouldn't ever get here as the conversion function
                 * will fail if we try and access a device which doesn't
                 * exist.
                 */
            break;
        }
    }

    return read_status;
}


// ----------------------------------------------------------------------------
/*!
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
// ----------------------------------------------------------------------------
flash_hal_error_t flash_hal_device_write
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_write,
                         const uint8_t * const p_write_data)
{
    uint32_t            physical_address;
    storage_devices_t   physical_device;
    bool_t              b_converted_ok;
    flash_hal_error_t   write_status = FLASH_HAL_INVALID_ADDRESS;
    EM95PollStatus_t    serial_flash_status;
    EI2CStatus_t        eeprom_status;

    b_converted_ok = convert_from_logical_2_physical(logical_start_address,
                                                     number_of_bytes_to_write,
                                                     &physical_address,
                                                     &physical_device);

    if (b_converted_ok)
    {
        //lint -e{788} Not all enum types used in switch, but we have a default case.
        switch (physical_device)
        {
            /*
             * The main flash is a word-addressable device.
             * Only write to the main flash if the address is a word address
             * and the number of bytes is even (i.e. a whole number of words).
             */
            case STORAGE_DEVICE_MAIN_FLASH:
                if ( ((logical_start_address & 0x00000001u) == 0u)
                        && ((number_of_bytes_to_write & 0x000000001u) == 0u) )
                {
                    m_current_device_used_for_write = STORAGE_DEVICE_MAIN_FLASH;

                    write_status = main_flash_write(physical_address,
                                                    number_of_bytes_to_write,
                                                    p_write_data);
                }
            break;

            /* The serial flash is a byte-addressable device.*/
            case STORAGE_DEVICE_SERIAL_FLASH:
                m_current_device_used_for_write = STORAGE_DEVICE_SERIAL_FLASH;

                serial_flash_status = M95_memcpy(physical_address,
                                                 number_of_bytes_to_write,
                                                 p_write_data);

                /* This is the only status which means the write was OK. */
                if (serial_flash_status == M95_POLL_NO_WRITE_IN_PROGRESS)
                {
                    write_status = FLASH_HAL_NO_ERROR;
                }
                else
                {
                    write_status = FLASH_HAL_WRITE_FAIL;
                }
            break;

            /*
             * The I2C EEPROM is a byte-addressable device.
             * Note the cast to uint16_t - number of bytes is only
             * 16 bits in the eeprom driver, but assuming the memory is
             * setup correctly you'll never try and write too many.
             */
            case STORAGE_DEVICE_I2C_EEPROM:
                m_current_device_used_for_write = STORAGE_DEVICE_I2C_EEPROM;

                //lint -e{921} Cast from uint32_t to uint16_t
                eeprom_status = X24LC32A_memcpy(physical_address,
                                              (uint16_t)number_of_bytes_to_write,
                                              p_write_data);

                if (eeprom_status == I2C_COMPLETED_OK)
                {
                    write_status = FLASH_HAL_NO_ERROR;
                }
                else
                {
                    write_status = FLASH_HAL_WRITE_FAIL;
                }
            break;

            default:
                /*
                 * Default case will return FLASH_HAL_INVALID_ADDRESS.
                 * Shouldn't ever get here as the conversion function
                 * will fail if we try and access a device which doesn't
                 * exist.
                 */
            break;
        }
    }

    return write_status;
}


// ----------------------------------------------------------------------------
/*!
 * flash_hal_device_erase converts logical to physical address and then
 * calls the appropriate flash driver function to perform the erase.
 *
 * The erase function doesn't care about the partitions (the HAL doesn't really
 * know about partitions) but it does check that we're not trying to erase more
 * of a device than actually exists.
 *
 * @note
 * The logical start address is a BYTE ADDRESS.
 *
 * @param   logical_start_address       The logical start address for the erase.
 * @param   number_of_bytes_to_erase    The number of bytes to erase.
 * @retval  flash_hal_error_t           Enumerated value for erase status.
 *
 */
// ----------------------------------------------------------------------------
flash_hal_error_t flash_hal_device_erase
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_erase)
{
     uint32_t            physical_address;
     storage_devices_t   physical_device;
     bool_t              b_converted_ok;
     flash_hal_error_t   erase_status = FLASH_HAL_INVALID_ADDRESS;
     uint32_t            sector_offset;
     uint32_t            sector_remainder;

     b_converted_ok = convert_from_logical_2_physical(logical_start_address,
                                                      number_of_bytes_to_erase,
                                                      &physical_address,
                                                      &physical_device);

     if (b_converted_ok)
     {
         sector_offset    = (physical_address - m_physical_addresses[physical_device].start_address)
                                % m_physical_addresses[physical_device].block_size_bytes;

         sector_remainder = number_of_bytes_to_erase
                                 % m_physical_addresses[physical_device].block_size_bytes;

         /* Only erase if we're doing an entire sector (or sectors). */
         if ( (sector_offset == 0u) && (sector_remainder == 0u) )
         {
             //lint -e{788} Not all enum types used in switch, but we have a default case.
             switch (physical_device)
             {
                 case STORAGE_DEVICE_MAIN_FLASH:
                     erase_status = main_flash_partial_erase(physical_address,
                                                             number_of_bytes_to_erase);
                 break;

                 case STORAGE_DEVICE_SERIAL_FLASH:
                     erase_status = serial_flash_partial_erase(physical_address,
                                                               number_of_bytes_to_erase);
                 break;

                 case STORAGE_DEVICE_I2C_EEPROM:
                     erase_status = eeprom_partial_erase(physical_address,
                                                         number_of_bytes_to_erase);
                 break;

                 default:
                     /*
                      * Default case returns FLASH_HAL_INVALID_ADDRESS.
                      * Shouldn't ever get here as the conversion function
                      * will fail if we try and access a device which doesn't
                      * exist.
                      */
                 break;
             }
         }
     }

     return erase_status;
}


// ----------------------------------------------------------------------------
/*!
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
// ----------------------------------------------------------------------------
bool_t flash_hal_device_blank_check
                        (const uint32_t logical_start_address,
                         const uint32_t number_of_bytes_to_blank_check)
{
    uint32_t                physical_address;
    storage_devices_t       physical_device;
    bool_t                  b_converted_ok;
    bool_t                  b_device_is_blank = FALSE;

    b_converted_ok = convert_from_logical_2_physical(logical_start_address,
                                                     number_of_bytes_to_blank_check,
                                                     &physical_address,
                                                     &physical_device);

    if (b_converted_ok)
    {
        //lint -e{788} Not all enum types used in switch, but we have a default case.
        switch (physical_device)
        {
            /*
             * The main flash is a word-addressable device.
             * Only blank check the main flash if the address is a word address
             * and the number of bytes is even (i.e. a whole number of words).
             */
            case STORAGE_DEVICE_MAIN_FLASH:
                if ( ((logical_start_address & 0x00000001u) == 0u)
                        && ((number_of_bytes_to_blank_check & 0x000000001u) == 0u) )
                {
                    b_device_is_blank
                        = main_flash_blank_check(physical_address,
                                                 number_of_bytes_to_blank_check);
                }
            break;

            /* The serial flash is a byte-addressable device.*/
            case STORAGE_DEVICE_SERIAL_FLASH:
                b_device_is_blank
                    = serial_flash_blank_check(physical_address,
                                               number_of_bytes_to_blank_check);
            break;

            /* The I2C EEPROM is a byte-addressable device.*/
            case STORAGE_DEVICE_I2C_EEPROM:
                b_device_is_blank
                    = eeprom_blank_check(physical_address,
                                         number_of_bytes_to_blank_check);
            break;

            default:
                /*
                 * Default case doesn't do anything so we return FALSE.
                 * Shouldn't ever get here as the conversion function
                 * will fail if we try and access a device which doesn't
                 * exist.
                 */
            break;
        }
    }

    return b_device_is_blank;
}


// ----------------------------------------------------------------------------
/**
 * flash_hal_write_timeout_callbck is the callback function for the flash
 * write timeout.
 *
 * Based on which flash is currently being used, we call the appropriate
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
// ----------------------------------------------------------------------------
/*
 * Disable Lint warnings for xTimer not referenced, could be declared const
 * and could be declared as pointing to const as we have to match the
 * FreeRTOS function prototype.
 */
//lint -e{715} -e{818} -e{952}
void flash_hal_write_timeout_callbck(void* xTimer)
{
    //lint -e{788} Not all enum types used in switch, but we have a default case.
    switch (m_current_device_used_for_write)
    {
        case STORAGE_DEVICE_MAIN_FLASH:
            lld_ForceTimeoutFlagSet();
        break;

        case STORAGE_DEVICE_SERIAL_FLASH:
            M95_ForceTimeoutFlagSet();
        break;

        case STORAGE_DEVICE_I2C_EEPROM:
            X24LC32A_ForceTimeoutFlagSet();
        break;

        default:
            /*
             * Default case does nothing - no function to call as
             * we don't know which device was being used.
             */
        break;
    }
}


// ----------------------------------------------------------------------------
/**
 * flash_hal_address_trans_ptr_get returns a pointer to the appropriate
 * entry in the array of address translation structures.
 *
 * @note
 * This function is not required according to flash_hal.h, because the header
 * file provided by the recording system doesn't know what we will actually do
 * in the implementation of the HAL itself, so doesn't specify that this
 * function needs to exist.
 *
 * This function should only be used when we're testing the recording system
 * using the console.
 *
 * @param   partition_index         The partition index
 * @retval  address_translation_t*  Pointer to address translation structure.
 */
// ----------------------------------------------------------------------------
const address_translation_t* flash_hal_address_trans_ptr_get
                                            (const uint16_t partition_index)
{
    const address_translation_t*    p_translation = NULL;

    if (partition_index < RS_CFG_MAX_NUMBER_OF_PARTITIONS)
    {
        p_translation = &m_address_map[partition_index];
    }

    return p_translation;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*!
 * setup_address_mapping creates the logical to physical address map for the
 * complete logical address range.
 * setup_address_mapping 为整个逻辑地址范围创建逻辑到物理地址映射。
 * For each partition in the recording system, an address map can be fetched
 * which holds the logical start and end addresses of the partition and
 * the physical device to use.
 * 对于记录系统中的每个分区，可以获取保存分区的逻辑开始和结束地址以及要使用的物理设备的地址映射。
 * Each set of logical addresses is converted into a physical start and end
 * address, a physical device and a physical adjustment (the conversion between
 * the logical address and the physical one) and all of this information is
 * stored in an array of adjustment structures.
 * 每组逻辑地址都被转换成物理开始和结束地址、物理设备和物理调整（逻辑地址和物理地址之间的转换），所有这些信息都存储在调整结构数组中。
 * @param   p_logical_addresses     Pointer to logical address map array.
 * @retval	bool_t      TRUE if map setup correctly, FALSE if not enough space.
 *
 */
// ----------------------------------------------------------------------------
static bool_t setup_address_mapping
                        (const flash_hal_logical_t * const p_logical_addresses)
{
    storage_devices_t   device;
    uint32_t            next_address_in_device[3];
    uint32_t            adjustment;
    uint8_t             map_counter;
    bool_t              b_mapping_completed_ok = TRUE;

    /* Setup the next physical address for each device. */
    next_address_in_device[0u] = m_physical_addresses[0u].start_address;
    next_address_in_device[1u] = m_physical_addresses[1u].start_address;
    next_address_in_device[2u] = m_physical_addresses[2u].start_address;

    for (map_counter = 0u;
            map_counter < RS_CFG_MAX_NUMBER_OF_PARTITIONS; map_counter++)
    {
        /* Get pointer to logical addresses for the partition. */
        device     = p_logical_addresses[map_counter].device_to_use;

        /*
         * Copy device and logical addresses across into address map.
         * This is what we search when trying to map a logical to physical
         * address - if the logical address lies within this range then
         * we apply the physical parameters below.
         */

        m_address_map[map_counter].device_to_use = device;

        m_address_map[map_counter].logical_start_address
            = p_logical_addresses[map_counter].start_address;

        m_address_map[map_counter].logical_end_address
            = p_logical_addresses[map_counter].end_address;

        /*
         * This is the amount we have to subtract from a
         * logical address to get the physical address.
         */
        adjustment = p_logical_addresses[map_counter].start_address - next_address_in_device[device];

        /* Calculate physical addresses based on logical and adjustment. */

        m_address_map[map_counter].physical_start_address
            = p_logical_addresses[map_counter].start_address - adjustment;

        m_address_map[map_counter].physical_end_address
            = p_logical_addresses[map_counter].end_address - adjustment;

        m_address_map[map_counter].physical_address_adjustment
            = adjustment;

        /*
         * The next physical address is the end address plus 1.
         * Remember that we're in bytes here.
         */
        next_address_in_device[device]
            = m_address_map[map_counter].physical_end_address + 1u;

        /*
         * Jump out if the physical address exceeds the actual device limits.
         * In this case the address mapping has failed, so we can't allow
         * any reading or writing to occur.
         */
        if (m_address_map[map_counter].physical_end_address > m_physical_addresses[device].end_address)
        {
            b_mapping_completed_ok = FALSE;
            break;
        }
    }

    return b_mapping_completed_ok;
}


// ----------------------------------------------------------------------------
/*!
 * check_physical_structure checks to make sure the enumerated types and
 * physical address devices are in the same order.
 *
 * This is required for the address mapping to work correctly, as we use
 * the enum as an index.
 *
 * @retval  bool_t      TRUE if OK, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t check_physical_structure(void)
{
    bool_t      b_physical_structure_is_ok = FALSE;

    /*
     * Check that the enumerated types are in the right order and so are
     * the physical address devices.
     *
     * Note that this test throws up lots of Lint warnings if everything is
     * OK - constant value Boolean, converting enum to int,
     * mismatched essential types and boolean always evaluated to TRUE.
     * This is ugly but necessary, as we can't use the preprocessor to check
     * enumerated types, so we'll disable the warnings here.
     *
     */
    //lint -e{506} -e{641} -e{9029} -e{774}
    if (   (STORAGE_DEVICE_MAIN_FLASH == 0)
        && (STORAGE_DEVICE_SERIAL_FLASH == 1)
        && (STORAGE_DEVICE_I2C_EEPROM == 2)
        && (m_physical_addresses[0u].device_to_use == STORAGE_DEVICE_MAIN_FLASH)
        && (m_physical_addresses[1u].device_to_use == STORAGE_DEVICE_SERIAL_FLASH)
        && (m_physical_addresses[2u].device_to_use == STORAGE_DEVICE_I2C_EEPROM)     )
    {
        b_physical_structure_is_ok = TRUE;
    }

    return b_physical_structure_is_ok;
}


// ----------------------------------------------------------------------------
/*!
 * convert_from_logical_2_physical takes a logical address and, using the
 * address mapping structure, converts this into a physical address in the
 * appropriate device.
 *
 * @param   logical_address     Logical address to be converted.
 * @param   bytes_required      Bytes required by whatever operation we're doing.
 * @param   p_physical_address  Pointer to physical address to update.
 * @param   p_device            Pointer to device to use to update.
 * @retval  bool_t              TRUE if conversion was OK, FALSE if any failure.
 *
 */
// ----------------------------------------------------------------------------
static bool_t convert_from_logical_2_physical(const uint32_t logical_address,
                                              const uint32_t bytes_required,
                                              uint32_t * const p_physical_address,
                                              storage_devices_t * const p_device)
{
    bool_t              b_converted_ok = FALSE;
    bool_t              b_found_map = FALSE;
    uint8_t             map_counter;
    uint32_t            physical_adjustment = 0u;
    storage_devices_t   device_to_use = STORAGE_DEVICE_MAIN_FLASH;
    uint32_t            start_physical_address;
    uint32_t            end_physical_address;

    /*
     * Only try and perform the address conversion
     * if the flash HAL is initialised and
     * the pointer arguments are not NULL.
     */
    if ( (m_b_flash_hal_initialised)
        && (p_physical_address != NULL) && (p_device != NULL) )
    {
        /*
         * Search through each address map looking for the logical address
         * to be within the start and end values and extract the physical
         * adjustment and the device to use.
         */
        for (map_counter = 0u;
                map_counter < RS_CFG_MAX_NUMBER_OF_PARTITIONS; map_counter++)
        {
            if ( (logical_address >= m_address_map[map_counter].logical_start_address)
                && (logical_address <= m_address_map[map_counter].logical_end_address) )
            {
                physical_adjustment
                    = m_address_map[map_counter].physical_address_adjustment;

                device_to_use
                    = m_address_map[map_counter].device_to_use;

                b_found_map = TRUE;
                break;
            }
        }

        /*
         * If we've found something then check to make sure the operation will
         * fit in the amount of physical space we've got, and if it will then
         * setup the physical address and device.
         */
        if (b_found_map)
        {
            start_physical_address = logical_address - physical_adjustment;

            /* Check for bytes required being zero to avoid underflow. */
            if (bytes_required == 0u)
            {
                end_physical_address = start_physical_address;
            }
            else
            {
                end_physical_address   = (start_physical_address + bytes_required) - 1u;
            }

            if (end_physical_address <= m_physical_addresses[device_to_use].end_address)
            {
                *p_physical_address = logical_address - physical_adjustment;
                *p_device           = device_to_use;

                b_converted_ok = TRUE;
            }
        }
    }

    return b_converted_ok;
}


// ----------------------------------------------------------------------------
/*!
 * main_flash_read reads data from the main flash.
 *
 * As the flash chipset driver reads in 16 bit words, we take each 16 bit word
 * and convert it to 2 x 8 bit bytes.
 *
 * @warning
 * This function must have an even number of bytes to read, and the byte address
 * must be word aligned, so the calling function must check for this.
 *
 * @param   byte_address        The byte address to read from.
 * @param   bytes_to_read       Number of bytes to read.
 * @param   p_byte_data         Pointer to byte array to put the read data in.
 *
 */
// ----------------------------------------------------------------------------
static void main_flash_read(const uint32_t byte_address,
                            const uint32_t bytes_to_read,
                            uint8_t * const p_byte_data)
{
    uint32_t    word_address;
    uint32_t    words_to_read;
    uint32_t    byte_offset = 0u;
    uint16_t    temp_read;

    word_address  = byte_address / 2u;
    words_to_read = bytes_to_read / 2u;

    while (words_to_read != 0u)
    {
        if (word_address < MAIN_FLASH_LOWER_DEVICE_MAX)
        {
            temp_read = lld_ReadOp(DEVICE_ZERO_BASE, word_address);
        }
        else
        {
            /*
             * lld_ReadOp's second argument is the offset into the device,
             * so we need to subtract the maximum address of the lower device
             * to get the desired offset.  Note that we have to disable the
             * Lint warning for cast from int to pointer (in DEVICE_ONE_BASE) -
             * this contravenes MISRA rule 11.4, but is a function of the way
             * the Spansion library code works, so is difficult to change.
             */
            //lint -e{9078} -e{923} Conversion between pointer and integer type.
            temp_read = lld_ReadOp(DEVICE_ONE_BASE,
                                   (word_address - MAIN_FLASH_LOWER_DEVICE_MAX) );
        }

        /* Split 16 bit word into bytes in little-endian fashion. */
        //lint -e{920} Ignoring return value, not used here as we use byte_offset.
        (void)BUFFER_UTILS_Uint16To8bitBuf(&p_byte_data[byte_offset], temp_read);
        byte_offset += 2u;
        word_address++;
        words_to_read--;
    }
}


// ----------------------------------------------------------------------------
/*!
 * main_flash_write writes data to the main flash.
 *
 * As the flash chipset driver writes in 16 bit words, we use the function
 * lld_memcpy_bytes in here, which takes each pair of bytes and converts to
 * a 16 bit word before writing to the flash.
 *
 * @warning
 * This function must have an even number of bytes to write, and the byte
 * address must be word aligned, so the calling function must check for this.
 *
 * @param   byte_address        The first byte address to write.
 * @param   bytes_to_write      Number of bytes to write.
 * @param   p_byte_data         Pointer to byte array containing data to write.
 * @retval  flash_hal_error_t   Enumerated value for flash status.
 *
 */
// ----------------------------------------------------------------------------
static flash_hal_error_t main_flash_write(const uint32_t byte_address,
                                          const uint32_t bytes_to_write,
                                          const uint8_t * const p_byte_data)
{
    uint32_t            word_address;
    uint32_t            words_to_write;
    DEVSTATUS           write_status;
    flash_hal_error_t   write_result = FLASH_HAL_WRITE_FAIL;

    word_address   = byte_address / 2u;
    words_to_write = bytes_to_write / 2u;

    if (word_address < MAIN_FLASH_LOWER_DEVICE_MAX)
    {
        /*
         * The maximum amount of data which the flash driver can handle in
         * one go is 65535 words.  In the recording system we'll never write
         * a TDR of more than 4k (plus the wrapper) so this isn't a problem.
         * Note the cast, as memcpy takes a uint16_t as it's argument.
         */
        //lint -e{921} Cast from uint32_t to uint16_t.
        write_status = lld_memcpy_bytes(DEVICE_ZERO_BASE,
                                        word_address,
                                        (uint16_t)words_to_write,
                                        p_byte_data);
    }
    else
    {
        /*
         * lld_memcpy's second argument is the offset into the device,
         * so we need to subtract the maximum address of the lower device
         * to get the desired offset.  Note that we have to disable the
         * Lint warning for cast from int to pointer (in DEVICE_ONE_BASE) -
         * this contravenes MISRA rule 11.4, but is a function of the way
         * the Spansion library code works, so is difficult to change.
         */
        //lint -e{9078} -e{923} Cast from int to pointer.
        //lint -e{921} Cast from uint32_t to uint16_t.
        write_status = lld_memcpy_bytes(DEVICE_ONE_BASE,
                                        (word_address - MAIN_FLASH_LOWER_DEVICE_MAX),
                                        (uint16_t)words_to_write,
                                        p_byte_data);
    }

    /* The only write status which means the write was OK is DEV_NOT_BUSY. */
    if (write_status == DEV_NOT_BUSY)
    {
        write_result = FLASH_HAL_NO_ERROR;
    }

    return write_result;
}


// ----------------------------------------------------------------------------
/*!
 * main_flash_partial_erase erases one or more sectors in the main flash.
 *
 * @warning
 * This function must have an address which is on a sector boundary, and
 * a number of bytes which is a multiple of the sector size, so the calling
 * function must check for this.
 *
 * @param   byte_address        The first byte address to erase.
 * @param   bytes_to_erase      Number of bytes to erase.
 * @retval  flash_hal_error_t   Enumerated value for flash status.
 *
 */
// ----------------------------------------------------------------------------
static flash_hal_error_t main_flash_partial_erase(const uint32_t byte_address,
                                                  const uint32_t bytes_to_erase)
{
    flash_hal_error_t   status = FLASH_HAL_NO_ERROR;
    uint32_t            word_address;
    uint32_t            number_of_sectors;
    DEVSTATUS           flash_erase_status;
    FLASHDATA *         p_device;
    ADDRESS             device_offset;
    bool_t              b_erased_ok = TRUE;

    word_address = byte_address / 2u;

    number_of_sectors = bytes_to_erase / m_physical_addresses[STORAGE_DEVICE_MAIN_FLASH].block_size_bytes;

    while (number_of_sectors != 0u)
    {
        if (word_address < MAIN_FLASH_LOWER_DEVICE_MAX)
        {
            p_device      = DEVICE_ZERO_BASE;
            device_offset = word_address;
        }
        else
        {
            //lint -e{9078} -e{923} Cast from int to pointer.
            p_device      = DEVICE_ONE_BASE;
            device_offset = word_address - MAIN_FLASH_LOWER_DEVICE_MAX;
        }

        /* If a sector is already blank no need to erase it. */
        if (check_one_flash_sector_blank(word_address))
        {
            flash_erase_status = DEV_NOT_BUSY;
        }
        else
        {
            flash_erase_status = lld_SectorEraseOp(p_device, device_offset);
        }

        /* Anything other than DEV_NOT_BUSY means that the erase failed. */
        if (flash_erase_status != DEV_NOT_BUSY)
        {
            b_erased_ok = FALSE;
            break;
        }

        /*
         * Increase the word address to the address of the first location
         * in the next sector, and go round again.
         */
        word_address += (m_physical_addresses[STORAGE_DEVICE_MAIN_FLASH].block_size_bytes / 2u);
        number_of_sectors--;
    }

    if (!b_erased_ok)
    {
        status = FLASH_HAL_WRITE_FAIL;
    }

    return status;
}


// ----------------------------------------------------------------------------
/*!
 * serial_flash_partial_erase erases one or more sectors in the serial flash.
 *
 * We do this by writing 0xFF's into the required locations.  There is no
 * function in the driver to do this, so we make one here to save having to
 * change the driver.
 *
 * The only slightly subtle thing in here is we align the writes to a page
 * boundary, to make it more efficient.  The page size is 128 bytes for the
 * chip we're using on the XPB board.
 *
 * @param   byte_address        The first byte address to erase.
 * @param   bytes_to_erase      Number of bytes to erase.
 * @retval  flash_hal_error_t   Enumerated value for flash status.
 *
 */
// ----------------------------------------------------------------------------
static flash_hal_error_t serial_flash_partial_erase(const uint32_t byte_address,
                                                    const uint32_t bytes_to_erase)
{
    flash_hal_error_t   erase_status = FLASH_HAL_NO_ERROR;
    EM95PollStatus_t    flash_status = M95_POLL_NO_WRITE_IN_PROGRESS;
    uint8_t             erase_buffer[M95_PAGE_SIZE_IN_BYTES];
    uint16_t            blank_counter;
    uint32_t            bytes_to_erase_remaining;
    uint32_t            start_offset_in_page;
    uint32_t            erase_counter;
    uint32_t            erase_address;

    /* Setup temporary write buffer with all blank locations. */
    for (blank_counter = 0u; blank_counter < M95_PAGE_SIZE_IN_BYTES;
            blank_counter++)
    {
        erase_buffer[blank_counter] = 0xFFu;
    }

    bytes_to_erase_remaining = bytes_to_erase;
    erase_address            = byte_address;
    start_offset_in_page     = erase_address & (M95_PAGE_SIZE_IN_BYTES - 1u);

    /*
     * If the address starts somewhere within a block then write as many words
     * as are required to align the data (or just write the required works, if
     * there aren't more than will fill the block).
     */
    if (start_offset_in_page != 0u)
    {
        /*
         * Setup internal write counter for correct number of words to either
         * fill the first page, or just as many words as are required.
         */
        if (bytes_to_erase < (M95_PAGE_SIZE_IN_BYTES - start_offset_in_page) )
        {
            erase_counter = bytes_to_erase;
        }
        else
        {
            erase_counter = M95_PAGE_SIZE_IN_BYTES - start_offset_in_page;
        }

        /* Write first page (might be only page). */
        flash_status = M95_BlockWrite(erase_address,
                                      erase_counter,
                                      &erase_buffer[0u]);

        /* Update remaining number of bytes to write and the address. */
        bytes_to_erase_remaining -= erase_counter;
        erase_address            += erase_counter;
    }

    /*
     * Having aligned the data to the page size, carry on writing in page sized
     * chunks (or skip this if there's less than a page to go).
     */
    while ( (bytes_to_erase_remaining >= M95_PAGE_SIZE_IN_BYTES)
            && (flash_status == M95_POLL_NO_WRITE_IN_PROGRESS) )
    {
        //lint -e{921} Cast from uint16_t to uint32_t of page size.
        flash_status = M95_BlockWrite(erase_address,
                                      (uint32_t)M95_PAGE_SIZE_IN_BYTES,
                                      &erase_buffer[0u]);

        bytes_to_erase_remaining -= M95_PAGE_SIZE_IN_BYTES;
        erase_address            += M95_PAGE_SIZE_IN_BYTES;
    }

    /* If there's any more data to write (less than a page) then do it. */
    if ( (bytes_to_erase_remaining != 0u)
            && (flash_status == M95_POLL_NO_WRITE_IN_PROGRESS) )
    {
        flash_status = M95_BlockWrite(erase_address,
                                      bytes_to_erase_remaining,
                                      &erase_buffer[0u]);
    }

    if (flash_status != M95_POLL_NO_WRITE_IN_PROGRESS)
    {
        erase_status = FLASH_HAL_WRITE_FAIL;
    }

    return erase_status;
}


// ----------------------------------------------------------------------------
/*!
 * eeprom_partial_erase erases one or more sectors in the eeprom.
 *
 * We do this by writing 0xFF's into the required locations.  There is no
 * function in the driver to do this, so we make one here to save having to
 * change the driver.
 *
 * The only slightly subtle thing in here is we align the writes to a page
 * boundary, to make it more efficient.  The page size is 16 bytes for the
 * chip we're using on the XPB board.
 *
 * @param   byte_address        The first byte address to erase.
 * @param   bytes_to_erase      Number of bytes to erase.
 * @retval  flash_hal_error_t   Enumerated value for flash status.
 *
 */
// ----------------------------------------------------------------------------
static flash_hal_error_t eeprom_partial_erase(const uint32_t byte_address,
                                              const uint32_t bytes_to_erase)
{
    flash_hal_error_t   erase_status  = FLASH_HAL_NO_ERROR;
    EI2CStatus_t        eeprom_status = I2C_COMPLETED_OK;
    uint8_t             erase_buffer[X24LC32A_PAGE_SIZE_IN_BYTES];
    uint16_t            blank_counter;
    uint32_t            bytes_to_erase_remaining;
    uint32_t            start_offset_in_page;
    uint32_t            erase_counter;
    uint32_t            erase_address;

    /* Setup temporary write buffer with all blank locations. */
    for (blank_counter = 0u; blank_counter < X24LC32A_PAGE_SIZE_IN_BYTES;
            blank_counter++)
    {
        erase_buffer[blank_counter] = 0xFFu;
    }

    bytes_to_erase_remaining = bytes_to_erase;
    erase_address            = byte_address;
    start_offset_in_page     = erase_address & (X24LC32A_PAGE_SIZE_IN_BYTES - 1u);

    /*
     * If the address starts somewhere within a block then write as many words
     * as are required to align the data (or just write the required works, if
     * there aren't more than will fill the block).
     */
    if (start_offset_in_page != 0u)
    {
        /*
         * Setup internal write counter for correct number of words to either
         * fill the first page, or just as many words as are required.
         */
        if (bytes_to_erase < (X24LC32A_PAGE_SIZE_IN_BYTES - start_offset_in_page) )
        {
            erase_counter = bytes_to_erase;
        }
        else
        {
            erase_counter = X24LC32A_PAGE_SIZE_IN_BYTES - start_offset_in_page;
        }

        /*
         * Write first page (might be only page).
         * Note the cast of the counter - the driver only accepts uint16_t,
         * but as we never erase more than the page size at a time, this is OK.
         */
        //lint -e{921} Cast from uint32_t to uint16_t.
        eeprom_status = X24LC32A_BlockWrite(erase_address,
                                          (uint16_t)erase_counter,
                                          &erase_buffer[0u]);

        /* Update remaining number of bytes to write and the address. */
        bytes_to_erase_remaining -= erase_counter;
        erase_address            += erase_counter;
    }

    /*
     * Having aligned the data to the page size, carry on writing in page sized
     * chunks (or skip this if there's less than a page to go).
     */
    while ( (bytes_to_erase_remaining >= X24LC32A_PAGE_SIZE_IN_BYTES)
            && (eeprom_status == I2C_COMPLETED_OK) )
    {
        eeprom_status = X24LC32A_BlockWrite(erase_address,
                                          X24LC32A_PAGE_SIZE_IN_BYTES,
                                          &erase_buffer[0u]);

        bytes_to_erase_remaining -= X24LC32A_PAGE_SIZE_IN_BYTES;
        erase_address            += X24LC32A_PAGE_SIZE_IN_BYTES;
    }

    /* If there's any more data to write (less than a page) then do it. */
    if ( (bytes_to_erase_remaining != 0u)
            && (eeprom_status == I2C_COMPLETED_OK) )
    {
        //lint -e{921} Cast from uint32_t to uint16_t, as above.
        eeprom_status = X24LC32A_BlockWrite(erase_address,
                                          (uint16_t)bytes_to_erase_remaining,
                                          &erase_buffer[0u]);
    }

    if (eeprom_status != I2C_COMPLETED_OK)
    {
        erase_status = FLASH_HAL_WRITE_FAIL;
    }

    return erase_status;
}


// ----------------------------------------------------------------------------
/*!
 * main_flash_blank_check reads data from the main flash and checks that it
 * is blank, using the sector blank check function as much as possible, for
 * speed.
 *
 * @warning
 * This function must have an even number of bytes to read, and the byte address
 * must be word aligned, so the calling function must check for this.
 *
 * @param   byte_address        The byte address to read from.
 * @param   bytes_to_check      Number of bytes to check.
 * @retval  bool_t              TRUE if device is blank, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t main_flash_blank_check(const uint32_t byte_address,
                                     const uint32_t bytes_to_check)
{
    const uint32_t  block_size_in_words
                        = m_physical_addresses[STORAGE_DEVICE_MAIN_FLASH].block_size_bytes / 2u;

    const uint32_t  start_address_in_words
                        = m_physical_addresses[STORAGE_DEVICE_MAIN_FLASH].start_address / 2u;

    uint32_t    word_address;
    uint32_t    words_to_check;
    bool_t      b_blank_check_ok = TRUE;
    uint32_t    sector_offset;
    uint32_t    words_to_end_of_sector = 0u;
    uint32_t    words_before_whole_sectors = 0u;
    uint32_t    whole_sectors;
    uint32_t    words_after_whole_sectors;

    word_address   = byte_address / 2u;
    words_to_check = bytes_to_check / 2u;

    /* The sector offset is how far, in words, we are into the sector. */
    sector_offset  = (word_address - start_address_in_words) % block_size_in_words;

    /*
     * If the sector offset is non-zero then we are not aligned with a sector
     * boundary, so we have to read 'some' words first before we can do the
     * faster sector blank checks.
     */
    if (sector_offset != 0u)
    {
        words_to_end_of_sector = block_size_in_words - sector_offset;

        /* Is there just one sector to read in, or more? */
        if (words_to_check < words_to_end_of_sector)
        {
            words_before_whole_sectors = words_to_check;
        }
        else
        {
            words_before_whole_sectors = words_to_end_of_sector;
        }
    }

    /*
     * Calculate the number of whole sectors to check.
     * We are deliberately using integer division here to round down.
     */
    words_to_check -= words_before_whole_sectors;
    whole_sectors   = words_to_check / block_size_in_words;

    /* Setup the number of words after the whole sectors to check. */
    words_to_check -= (whole_sectors * block_size_in_words);
    words_after_whole_sectors = words_to_check;

    /*
     * Having calculated words before, whole sectors and words after,
     * now do the actual blank checking.
     */

    /* Check any words before a whole sector, one word at a time. */
       while (words_before_whole_sectors != 0u)
    {
        b_blank_check_ok = check_one_flash_address_blank(word_address);

        /* Jump out of the while loop if the location wasn't blank. */
        if (!b_blank_check_ok)
        {
            break;
        }

        word_address++;
        words_before_whole_sectors--;
    }

    /* Assuming the above didn't fail, check all whole sectors. */
    if (b_blank_check_ok)
    {
        while (whole_sectors != 0u)
        {
            b_blank_check_ok = check_one_flash_sector_blank(word_address);

            /* Jump out of the while loop if the sector wasn't blank. */
            if (!b_blank_check_ok)
            {
                break;
            }

            word_address += block_size_in_words;
            whole_sectors--;
        }
    }

    /*
     * Assuming the above didn't fail, check any words after the whole
     * sectors, one word at a time.
     */
    if (b_blank_check_ok)
    {
        while (words_after_whole_sectors != 0u)
        {
            b_blank_check_ok = check_one_flash_address_blank(word_address);

            /* Jump out of the while loop if the location wasn't blank. */
            if (!b_blank_check_ok)
            {
                break;
            }

            word_address++;
            words_after_whole_sectors--;
        }
    }

    return b_blank_check_ok;
}


// ----------------------------------------------------------------------------
/*!
 * serial_flash_blank_check reads data from the serial flash and checks that it
 * is blank.
 *
 * This function reads in 'chunks' equal to the page size - reads don't have
 * to be aligned with the page, but it's more efficient to read a block rather
 * than read 1 byte at a time, because of the SPI overhead in reading.
 *
 * @param   byte_address        The byte address to read from.
 * @param   bytes_to_check      Number of bytes to check.
 * @retval  bool_t              TRUE if device is blank, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t serial_flash_blank_check(const uint32_t byte_address,
                                       const uint32_t bytes_to_check)
{
    uint8_t     buffer[M95_PAGE_SIZE_IN_BYTES];
    uint32_t    block_reads;
    uint32_t    remainder_reads;
    uint32_t    erase_address;
    bool_t      b_blank_check_ok = TRUE;

    block_reads     = bytes_to_check / M95_PAGE_SIZE_IN_BYTES;
    remainder_reads = bytes_to_check % M95_PAGE_SIZE_IN_BYTES;

    erase_address = byte_address;

    while ( (block_reads != 0u) && (b_blank_check_ok) )
    {
        //lint -e{921} Cast from uint16_t to uint32_t.
        M95_BlockRead(erase_address, (uint32_t)M95_PAGE_SIZE_IN_BYTES, &buffer[0u]);

        //lint -e{921} Cast from uint16_t to uint32_t.
        b_blank_check_ok = blank_check_buffer(&buffer[0u],
                                              (uint32_t)M95_PAGE_SIZE_IN_BYTES);

        erase_address += M95_PAGE_SIZE_IN_BYTES;
        block_reads--;
    }

    if ( (remainder_reads != 0u) && (b_blank_check_ok) )
    {
        M95_BlockRead(erase_address, remainder_reads, &buffer[0u]);

        b_blank_check_ok = blank_check_buffer(&buffer[0u],
                                              remainder_reads);
    }

    return b_blank_check_ok;
}


// ----------------------------------------------------------------------------
/*!
 * eeprom_blank_check reads data from the serial flash and checks that it
 * is blank.
 *
 * @param   byte_address        The byte address to read from.
 * @param   bytes_to_check      Number of bytes to check.
 * @retval  bool_t              TRUE if device is blank, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t eeprom_blank_check(const uint32_t byte_address,
                                 const uint32_t bytes_to_check)
{
    uint8_t     buffer[X24LC32A_PAGE_SIZE_IN_BYTES];
    uint32_t    block_reads;
    uint32_t    remainder_reads;
    uint32_t    erase_address;
    bool_t      b_blank_check_ok = TRUE;

    block_reads     = bytes_to_check / X24LC32A_PAGE_SIZE_IN_BYTES;
    remainder_reads = bytes_to_check % X24LC32A_PAGE_SIZE_IN_BYTES;

    erase_address = byte_address;

    while ( (block_reads != 0u) && (b_blank_check_ok) )
    {
        /*
         * Note that we discard the return value from the read
         * - we'll work out whether the read has worked by calling
         * the blank_check_buffer function.
         */
        //lint -e{920} Cast from enum to void.
        (void)X24LC32A_BlockRead(erase_address,
                               X24LC32A_PAGE_SIZE_IN_BYTES,
                               &buffer[0u]);

        //lint -e{921} Cast from uint16_t to uint32_t.
        b_blank_check_ok = blank_check_buffer(&buffer[0u],
                                              (uint32_t)X24LC32A_PAGE_SIZE_IN_BYTES);

        erase_address += X24LC32A_PAGE_SIZE_IN_BYTES;
        block_reads--;
    }

    if ( (remainder_reads != 0u) && (b_blank_check_ok) )
    {
        /*
         * Note the cast to uint16_t - the block read function can't read more
         * than 65535 in one go, but as we never read more than the page size
         * here, then we're ok.
         *
         * Also note that we discard the return value (same as above).
         */
        //lint -e{921} Cast from uint16_t to uint32_t.
        //lint -e{920} Cast from enum to void.
        (void)X24LC32A_BlockRead(erase_address,
                               (uint16_t)remainder_reads,
                               &buffer[0u]);

        b_blank_check_ok = blank_check_buffer(&buffer[0u],
                                              remainder_reads);
    }

    return b_blank_check_ok;
}


// ----------------------------------------------------------------------------
/*!
 * blank_check_buffer checks to see if a buffer is blank (contains only 0xFF's).
 *
 * @param   p_buffer    Pointer to buffer to check.
 * @param   length      Length of buffer to check.
 * @retval  bool_t      TRUE if buffer is blank, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t blank_check_buffer(const uint8_t * const p_buffer,
                                 const uint32_t length)
{
    uint32_t    blank_counter;
    bool_t      b_all_blank = TRUE;

    for (blank_counter = 0u; blank_counter < length; blank_counter++)
    {
        if (p_buffer[blank_counter] != 0xFFu)
        {
            b_all_blank = FALSE;
            break;
        }
    }

    return b_all_blank;
}


// ----------------------------------------------------------------------------
/*!
 * check_one_flash_address_blank checks a single location in the main flash
 * to see whether it's blank or not.
 *
 * @param   word_address    Word address to read and check.
 * @retval  bool_t          TRUE if location is blank, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t check_one_flash_address_blank(const uint32_t word_address)
{
    bool_t      b_location_is_blank = TRUE;
    uint16_t    value;

    if (word_address < MAIN_FLASH_LOWER_DEVICE_MAX)
    {
        value = lld_ReadOp(DEVICE_ZERO_BASE, word_address);
    }
    else
    {
        /*
         * lld_memcpy's second argument is the offset into the device,
         * so we need to subtract the maximum address of the lower device
         * to get the desired offset.  Note that we have to disable the
         * Lint warning for cast from int to pointer (in DEVICE_ONE_BASE) -
         * this contravenes MISRA rule 11.4, but is a function of the way
         * the Spansion library code works, so is difficult to change.
         */
        //lint -e{9078} -e{923} Cast from int to pointer.
        value = lld_ReadOp(DEVICE_ONE_BASE,
                           (word_address - MAIN_FLASH_LOWER_DEVICE_MAX) );
    }

    /* Any value other than 0xFFFF means the location isn't blank. */
    if (value != 0xFFFFu)
    {
        b_location_is_blank = FALSE;
    }

    return b_location_is_blank;
}


// ----------------------------------------------------------------------------
/*!
 * check_one_flash_sector_blank checks a sector in the main flash to see
 * whether it's blank or not, using the lld_BlankCheckOp function.
 *
 * @param   start_word_address  First address in the sector to blank check.
 * @retval  bool_t              TRUE if sector is blank, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t check_one_flash_sector_blank(const uint32_t start_word_address)
{
    bool_t      b_sector_is_blank = TRUE;
    DEVSTATUS   erase_status;

    if (start_word_address < MAIN_FLASH_LOWER_DEVICE_MAX)
    {
        erase_status = lld_BlankCheckOp(DEVICE_ZERO_BASE, start_word_address);
    }
    else
    {
        /*
         * lld_memcpy's second argument is the offset into the device,
         * so we need to subtract the maximum address of the lower device
         * to get the desired offset.  Note that we have to disable the
         * Lint warning for cast from int to pointer (in DEVICE_ONE_BASE) -
         * this contravenes MISRA rule 11.4, but is a function of the way
         * the Spansion library code works, so is difficult to change.
         */
        //lint -e{9078} -e{923} Cast from int to pointer.
        erase_status = lld_BlankCheckOp(DEVICE_ONE_BASE,
                                        (start_word_address - MAIN_FLASH_LOWER_DEVICE_MAX) );
    }

    /* Any response other than DEV_NOT_BUSY means the sector wasn't blank. */
    if (erase_status != DEV_NOT_BUSY)
    {
        b_sector_is_blank = FALSE;
    }

    return b_sector_is_blank;
}


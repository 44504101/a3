// ----------------------------------------------------------------------------
/**
 * @file        rspages.c
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        23 Feb 2016
 * @brief       Functions which support the code in rsapi.c
 * @details
 * Support functions for rsapi code, anything related to the pages within
 * the partitions.
 *
 * @note
 * These functions should only be called from the API code itself.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "rsappconfig.h"
#include "rspages.h"
#include "rspages_prv.h"
#include "rsapi.h"
#include "rssearch.h"
#include "flash_hal.h"
#include "crc.h"
#include "rspartition.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define PAGE_HEADER_FORMAT_CODE_OK  0x8Du       ///< Expected format code in page header.
#define PAGE_HEADER_ERROR_CODE_OK   0xFFu       ///< Expected error code in page header.
#define PAGE_HEADER_STATUS_CLOSED   0x6996u     ///< Status for a page which is closed.
#define PAGE_HEADER_STATUS_OPEN     0x7BB7u     ///< Status for a page which is open.
#define PAGE_HEADER_STATUS_BLANK    0xFFFFu     ///< Status for a page which is blank.

#define PAGE_HEADER_FORMAT_OFFSET   0u          ///< Offset in page header for format code.
#define PAGE_HEADER_PARID_OFFSET    1u          ///< Offset in page header for parameter ID.
#define PAGE_HEADER_CHECKSUM_OFFSET 2u          ///< Offset in page header for checksum.
#define PAGE_HEADER_STATUS_MSB      3u          ///< Offset in page header for MSB of status.
#define PAGE_HEADER_STATUS_LSB      4u          ///< Offset in page header for LSB of status.
#define PAGE_HEADER_ERROR_OFFSET    5u          ///< Offset in page header for error.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static bool_t check_area_is_blank(const uint8_t * const p_area,
                                  const uint16_t size_of_area);

static bool_t write_and_read_back(const uint32_t logical_start_address,
                                  const uint32_t number_of_bytes_to_write,
                                  const uint8_t * const p_write_data,
                                  const bool_t   b_read_back_requested);

static bool_t read_back_and_compare(const uint32_t logical_start_address,
                                    const uint32_t number_of_bytes_to_read,
                                    const uint8_t * const p_written_data);

static bool_t compare_buffers(const uint8_t * const p_buffer1,
                              const uint8_t * const p_buffer2,
                              const uint32_t length);

static rs_page_write_status_t write_page_data_handle_overlap
                                    (const rs_page_write_t * const p_write,
                                     uint32_t * const p_next_free_address);

static bool_t check_rsr_will_fit_in_partition
                                (const rs_page_write_t * const p_write_data);

static rs_page_status_t compare_header_with_addresses
                                    (const rs_header_status_t header_status,
                                     const uint32_t next_free_address,
                                     const uint32_t initial_read_address,
                                     const uint32_t next_page_address);

static rs_page_status_t check_next_page_is_blank
                            (const rs_header_data_t * const p_header_data);

static rs_header_status_t check_contents_of_page_header
                            (const uint8_t * const p_buffer,
                             const uint8_t partition_id);

static rs_header_status_t write_page_and_page_is_full
                                (const rs_page_write_t * const p_write,
                                 const uint32_t current_page_number);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * rspages_page_header_check loads a page header for a particular partition
 * and checks that it's OK.
 *
 * 'Allowable' return values for a good page are as follows:
 *      RS_HDR_PAGE_IS_CLOSED - means that the page is full but OK.
 *      RS_HDR_PAGE_IS_OPEN   - means that the page is partially used but OK.
 *      RS_HDR_PAGE_IS_EMPTY  - means that the page has no data in it yet
 *
 * @param   partition_logical_start_address     The partition start address.
 * @param   partition_logical_end_address       The partition end address.
 * @param   page_number_to_check                The page number to check.
 * @param   partition_id                        The ID for the partition.
 * @retval  rs_header_status_t  Enumerated value for any error found.
 *
 */
// ----------------------------------------------------------------------------
rs_header_status_t rspages_page_header_check
                        (const uint32_t partition_logical_start_address,
                         const uint32_t partition_logical_end_address,
                         const uint32_t page_number_to_check,
                         const uint8_t  partition_id)
{
    rs_header_status_t  return_value;
    uint32_t            read_address;
    uint32_t            last_potential_read_address;
    flash_hal_error_t   flash_read_status;
    uint8_t             page_buffer[PAGE_HEADER_LENGTH_BYTES];

    read_address = partition_logical_start_address
                    + (RS_CFG_PAGE_SIZE_KB
                                * 1024u
                                * page_number_to_check);

    last_potential_read_address
        = read_address + (PAGE_HEADER_LENGTH_BYTES - 1u);

    if (last_potential_read_address
            > partition_logical_end_address)
    {
        return_value = RS_HDR_INVALID_PAGE_NUMBER;
    }
    else
    {
        //lint -e{921} Cast to uint32_t to avoid prototype coercion on 16 bit platforms.
        flash_read_status = flash_hal_device_read(read_address,
                                                  (uint32_t)PAGE_HEADER_LENGTH_BYTES,
                                                  &page_buffer[0u]);

        if (flash_read_status != FLASH_HAL_NO_ERROR)
        {
            return_value = RS_HDR_FLASH_READ_ERROR;
        }
        else
        {
            return_value = check_contents_of_page_header(&page_buffer[0u],
                                                         partition_id);
        }
    }

    return return_value;
}


// ----------------------------------------------------------------------------
/**
 * rspages_page_header_write writes a new page header into the flash memory
 * and then reads it back to check that it's been written correctly.
 *
 * This function will return the following values:
 *      RS_HDR_HEADER_WRITE_OK      - means that the write \ read back was OK.
 *      RS_HDR_FLASH_WRITE_ERROR    - something went wrong.
 *      RS_HDR_INVALID_PARTITION_NUMBER - fatal error, invalid partition.
 *      RS_HDR_INVALID_PAGE_NUMBER      - fatal error, invalid page.
 *
 * @param   p_header_data       Pointer to header data structure.
 * @retval  rs_header_status_t  Enumerated value for any error found.
 *
 */
// ----------------------------------------------------------------------------
rs_header_status_t rspages_page_header_write
                        (const rs_header_data_t * const p_header_data)
{
    rs_header_status_t  return_value = RS_HDR_HEADER_WRITE_OK;
    uint32_t            write_address;
    uint32_t            last_potential_write_address;
    uint8_t             header_write[PAGE_HEADER_LENGTH_BYTES];
    uint8_t             header_offset;
    bool_t              b_write_completed_ok;

    if (p_header_data->partition_index >= RS_CFG_MAX_NUMBER_OF_PARTITIONS)
    {
        return_value = RS_HDR_INVALID_PARTITION_NUMBER;
    }
    else
    {
        write_address = p_header_data->partition_logical_start_addr
                            + (RS_CFG_PAGE_SIZE_KB
                                    * 1024u
                                    * p_header_data->page_number);

        last_potential_write_address
            = write_address + (PAGE_HEADER_LENGTH_BYTES - 1u);

        if (last_potential_write_address
                > p_header_data->partition_logical_end_addr)
        {
            return_value = RS_HDR_INVALID_PAGE_NUMBER;
        }
        else
        {
            header_write[0u] = p_header_data->format_code;
            header_write[1u] = p_header_data->partition_id;
            header_write[2u] = header_write[0u] + header_write[1u];

            //lint -e{921} Cast to 8 bits, just take the MSB here.
            header_write[3u] = (uint8_t)((p_header_data->status >> 8u) & 0x00FFu);

            //lint -e{921} Cast to 8 bits, just take the LSB here.
            header_write[4u] = (uint8_t)(p_header_data->status & 0x00FFu);

            header_write[5u] = p_header_data->error_code;

            //lint -e{921} Cast to 8 bits, just take the MSB here.
            header_write[6u] = (uint8_t)((p_header_data->error_address >> 8u) & 0x00FFu);

            //lint -e{921} Cast to 8 bits, just take the LSB here.
            header_write[7u] = (uint8_t)(p_header_data->error_address & 0x00FFu);

            for (header_offset = 8u;
                    header_offset < PAGE_HEADER_LENGTH_BYTES;
                    header_offset++)
            {
                header_write[header_offset] = 0xFFu;
            }

            //lint -e{921} Cast to uint32_t to avoid prototype coercion on 16 bit platforms.
            b_write_completed_ok = write_and_read_back(write_address,
                                                       (uint32_t)PAGE_HEADER_LENGTH_BYTES,
                                                       &header_write[0u],
                                                       TRUE);

            if (!b_write_completed_ok)
            {
                return_value = RS_HDR_HEADER_WRITE_ERROR;
            }
        }
    }

    return return_value;
}


// ----------------------------------------------------------------------------
/**
 * rspages_page_data_check loads a specific page from a partition and
 * checks that the page contents align with the header status.
 *
 * @note
 * Currently this function always tries and determines the next free address
 * in the page, regardless of the header status.  It may be that this is
 * inefficient, but allows for the API itself to determine whether a page
 * will be used or not.
 *
 * @note
 * This function uses the p_next_free_address pointer to update a variable
 * in the calling function.
 *
 * @param   p_header_data       Pointer to header data structure.
 * @param   p_next_free_address Pointer to next free address to update.
 * @retval  rs_header_status_t  Enumerated value for any error found.
 *
 */
// ----------------------------------------------------------------------------
rs_page_status_t rspages_page_data_check
                        (const rs_header_data_t * const p_header_data,
                         uint32_t * const p_next_free_address)
{
    rs_page_status_t    return_value = RS_PG_HEADER_PAGE_MISMATCH;
    uint32_t            initial_read_address;
    uint32_t            last_potential_read_address;
    uint32_t            number_of_bytes_to_read;
    uint32_t            next_free_address = 0xFFFFFFFFu;
    uint32_t            next_page_address;

    if (p_header_data->partition_index >= RS_CFG_MAX_NUMBER_OF_PARTITIONS)
    {
        return_value = RS_PG_INVALID_PARTITION_NUMBER;
    }
    else
    {
        initial_read_address = p_header_data->partition_logical_start_addr
                + (RS_CFG_PAGE_SIZE_KB * 1024u * p_header_data->page_number)
                + PAGE_HEADER_LENGTH_BYTES;

        next_page_address = p_header_data->partition_logical_start_addr
                                + (RS_CFG_PAGE_SIZE_KB * 1024u
                                    * (p_header_data->page_number + 1u) );

        last_potential_read_address = next_page_address - 1u;

        if (last_potential_read_address
                > p_header_data->partition_logical_end_addr)
        {
            return_value = RS_PG_INVALID_PAGE_NUMBER;
        }
        else
        {
            number_of_bytes_to_read = (RS_CFG_PAGE_SIZE_KB * 1024u)
                                        - PAGE_HEADER_LENGTH_BYTES;

            next_free_address = rssearch_find_next_free_address
                                    (initial_read_address, number_of_bytes_to_read);

            if (next_free_address == 0xFFFFFFFFu)
            {
                return_value = RS_PG_FLASH_READ_ERROR;
            }
            else
            {
                /* Check to make sure the header status and addresses match. */
                return_value = compare_header_with_addresses(p_header_data->header_status,
                                                             next_free_address,
                                                             initial_read_address,
                                                             next_page_address);

                /*
                 * If a page has space in it, then the next page should
                 * be empty, so check that it is.  This covers the situation
                 * where the last bit of the page is 0xFF but in fact this
                 * is just part of a record which spans a page.
                 */
                if (return_value == RS_PG_HEADER_OK_PAGE_HAS_SPACE)
                {
                    return_value = check_next_page_is_blank(p_header_data);
                }

                /* Update the next free address via the pointer.
                 * This is done for all cases where we actually tried
                 * to determine the address and got something.
                 */
                if (p_next_free_address != NULL)
                {
                    *p_next_free_address = next_free_address;
                }
            }
        }
    }

    return return_value;
}


// ----------------------------------------------------------------------------
/**
 * rspages_page_data_write writes a new tool data record (TDR) into the
 * flash memory, along with the wrapper to make up a recording system record
 * (RSR).
 *
 * The format of the RSR is as follows:
 *
 *     SYNC, REC ID (LSB\MSB), LEN (LSB\MSB), TDR, CRC (MSB\LSB), ENDSYNC
 *
 * SYNC is 0xE1 and ENDSYNC is 0x1A.
 * REC ID is the record ID.
 * LEN is the length of the TDR.
 * TDR is the tool data record, which is a variable length.
 * CRC is computed on SYNC, REC ID, LEN and TDR fields.
 *
 * @note
 * The RSR format deviates from that in the recording system specification,
 * in that we use an ENDSYNC character which is not specified.  Discussions
 * on the BB have shown that TSDnM can cope with this, and it makes it a lot
 * easier to find the end of an RSR (otherwise you can't tell the difference
 * between a blank location and a checksum which might be 0xFF or 0xFFFF).
 *
 * @warning
 * There are two versions of the recording system specification, with identical
 * version numbers, one which shows the REC ID and LEN as being big-endian, one
 * which shows them as little-endian.  The little-endian version is what TSDnM
 * is expecting.
 *
 * @param   p_write_data            Pointer to structure containing data to write.
 * @retval  rs_page_write_status_t  Enumerated value for any error found.
 *
 */
// ----------------------------------------------------------------------------
rs_page_write_status_t rspages_page_data_write
                            (const rs_page_write_t * const p_write_data)
{
    rs_page_write_status_t  status = RS_PG_WRITE_ERROR;
    uint16_t                running_crc;
    uint32_t                crc_length;
    uint16_t                tdr_length;
    uint32_t                write_address;
    bool_t                  b_rsr_will_fit;

    b_rsr_will_fit = check_rsr_will_fit_in_partition(p_write_data);

    /* Fail if RSR will not fit for any reason. */
    if (!b_rsr_will_fit)
    {
        status = RS_PG_WRITE_INVALID_ADDRESSES;
    }
    else
    {
        tdr_length = (p_write_data->bytes_to_write - RSAPI_BYTES_BEFORE_TDR)
                            - RSAPI_BYTES_AFTER_TDR;

        //lint -e{921} RSR_SYNC_CHARACTER includes a cast to uint8_t.
        p_write_data->p_write_buffer[0u] = RSR_SYNC_CHARACTER;

        //lint -e{921} Cast to 8 bits, just take the LSB here.
        p_write_data->p_write_buffer[1u] = (uint8_t)(p_write_data->record_id & 0x00FFu);

        //lint -e{921} Cast to 8 bits, just take the MSB here.
        p_write_data->p_write_buffer[2u] = (uint8_t)((p_write_data->record_id >> 8u) & 0x00FFu);

        //lint -e{921} Cast to 8 bits, just take the LSB here.
        p_write_data->p_write_buffer[3u] = (uint8_t)(tdr_length & 0x00FFu);

        //lint -e{921} Cast to 8 bits, just take the MSB here.
        p_write_data->p_write_buffer[4u] = (uint8_t)((tdr_length >> 8u) & 0x00FFu);

        //lint -e{921} Cast to uint32_t to force arithmetic on composite expression as 32 bit.
        crc_length = (uint32_t)p_write_data->bytes_to_write - RSAPI_BYTES_AFTER_TDR;

        running_crc = CRC_CCITTOnByteCalculate(p_write_data->p_write_buffer, crc_length, 0x0000u);

        //lint -e{921} Cast to 8 bits, just take the MSB here.
        p_write_data->p_write_buffer[crc_length] = (uint8_t)((running_crc >> 8u) & 0x00FFu);

        //lint -e{921} Cast to 8 bits, just take the LSB here.
        p_write_data->p_write_buffer[crc_length + 1u] = (uint8_t)(running_crc & 0x00FFu);

        //lint -e{921} RSR_ENDSYNC_CHARACTER includes a cast to uint8_t.
        p_write_data->p_write_buffer[crc_length + 2u] = RSR_ENDSYNC_CHARACTER;

        status = write_page_data_handle_overlap(p_write_data, &write_address);

        /*
         * Update the next address in the partition module.
         * Note that we do this regardless of whether the write worked or not,
         * because if the write failed we want to skip these potentially bad
         * locations.
         */
        //lint -e{920} Ignore the return value as this will always work.
        (void)rspartition_next_address_set(p_write_data->partition_index,
                                           write_address);
    }

    return status;
}


// ----------------------------------------------------------------------------
/**
 * rspages_page_details_calculate works fills in the output members of the
 * rs_page_details_t structure, based on the input members.
 *
 * @param   p_page_details      Pointer to rs_page_details_t structure.
 * @retval  bool_t              TRUE if calculated, FALSE if input error(s).
 *
 */
// ----------------------------------------------------------------------------
bool_t rspages_page_details_calculate(rs_page_details_t * const p_page_details)
{
    const uint32_t  page_size_in_bytes = RS_CFG_PAGE_SIZE_KB * 1024u;
    bool_t          b_calculation_ok = TRUE;
    uint32_t        number_of_pages;

    /* Fail if address isn't viable for some reason. */
    if ( (p_page_details->partition_logical_start_address
            > p_page_details->partition_logical_end_address)
        || (p_page_details->address_within_partition
                < p_page_details->partition_logical_start_address)
        || (p_page_details->address_within_partition
                > p_page_details->partition_logical_end_address) )
    {
        b_calculation_ok = FALSE;
    }

    p_page_details->distance_from_partition_start
        = p_page_details->address_within_partition
                - p_page_details->partition_logical_start_address;

    /* Deliberate use of integer division here to round down. */
    p_page_details->page_number
        = p_page_details->distance_from_partition_start / page_size_in_bytes;

    number_of_pages = (p_page_details->partition_logical_end_address
                        - p_page_details->partition_logical_start_address)
                            / page_size_in_bytes;
    number_of_pages++;

    p_page_details->maximum_number_of_pages = number_of_pages;

    /*
     * Work out the lower and upper addresses for the page within
     * which the address lies.  Remember to take into account
     * the page header, so the lower page boundary starts after this.
     */
    p_page_details->lower_address_within_page
        = p_page_details->partition_logical_start_address
            + (p_page_details->page_number * page_size_in_bytes)
            + PAGE_HEADER_LENGTH_BYTES;

    p_page_details->upper_address_within_page
        = p_page_details->partition_logical_start_address
            + ((p_page_details->page_number + 1u) * page_size_in_bytes) - 1u;

    /*
     * If the potential address is within the lower page boundary i.e. in
     * the page header then just set the distance to zero to avoid underflow.
     */
    if (p_page_details->address_within_partition
            < p_page_details->lower_address_within_page)
    {
        p_page_details->distance_to_lower_address = 0u;
    }
    else
    {
        p_page_details->distance_to_lower_address
            = p_page_details->address_within_partition
                    - p_page_details->lower_address_within_page;
    }

    p_page_details->distance_to_upper_address
            = p_page_details->upper_address_within_page
                    - p_page_details->address_within_partition;

    return b_calculation_ok;
}


#ifdef UNIT_TEST_BUILD
// ----------------------------------------------------------------------------
/**
 * rspages_unit_test_ptr_get returns a pointer to the unit test pointers
 * structure, for test purposes.
 * We use an ifdef to ensure that this pointer can't be accessed under
 * normal operation.
 *
 * @retval rsapi_unit_test_pointers_t*      Pointer to test structure.
 *
 */
// ----------------------------------------------------------------------------
rspages_unit_test_pointers_t* rspages_unit_test_ptr_get(void)
{
    //lint -e{956} Doesn't need to be volatile here. Pointers never change.
    static rspages_unit_test_pointers_t p_unit_test_structure =
    {
        check_area_is_blank,
        write_and_read_back,
        compare_buffers,
        write_page_data_handle_overlap,
        check_rsr_will_fit_in_partition
    };

    return &p_unit_test_structure;
}
#endif


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * check_area_is_blank checks that an array is blank.
 *
 * @param   p_area          Pointer to array to check.
 * @param   size_of_area    Length of array.
 * @retval  bool_t          TRUE if array is blank, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t check_area_is_blank(const uint8_t * const p_area,
                                  const uint16_t size_of_area)
{
    uint16_t    area_offset;
    bool_t      b_area_is_blank = TRUE;

    for (area_offset = 0u; area_offset < size_of_area; area_offset++)
    {
        if (p_area[area_offset] != RS_CFG_BLANK_LOCATION_CONTAINS)
        {
            b_area_is_blank = FALSE;
            break;
        }
    }

    return b_area_is_blank;
}


// ----------------------------------------------------------------------------
/**
 * write_and_read_back writes data to the flash memory and then reads it
 * back if requested to check that the write was successful.
 *
 * This function breaks the reading back into a number of passes, based on the
 * buffer size specified in RS_CFG_LOCAL_BLOCK_READ_SIZE.  The write is performed
 * as a single operation (at this level - the flash driver may do something
 * different but this doesn't matter here).
 *
 * @param   logical_start_address       Logical start address in flash for write.
 * @param   number_of_bytes_to_write    Number of bytes to write.
 * @param   p_write_data                Pointer to data to write into flash.
 * @param   b_read_back_requested       Flag indicating that the data written must be read back.
 * @retval  bool_t                      TRUE if write \ read back OK, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t write_and_read_back(const uint32_t logical_start_address,
                                  const uint32_t number_of_bytes_to_write,
                                  const uint8_t * const p_write_data,
                                  const bool_t   b_read_back_requested)
{
    bool_t              b_write_ok = FALSE;
    flash_hal_error_t   flash_write_status;
    uint32_t            whole_reads;
    uint32_t            remainder_reads;
    uint32_t            read_address;
    uint16_t            write_offset;
    bool_t              b_write_and_read_matched;
    uint32_t            expected_passes;
    uint16_t            matched_passes = 0u;
    bool_t              b_interim_read_back_failed = FALSE;

    flash_write_status = flash_hal_device_write(logical_start_address,
                                                number_of_bytes_to_write,
                                                p_write_data);

    if (flash_write_status == FLASH_HAL_NO_ERROR)
    {
        b_write_ok = TRUE;

        /* Read the data back if requested
         * and update the returned value.
         */
        if (b_read_back_requested)
        {
            whole_reads     = number_of_bytes_to_write / RS_CFG_LOCAL_BLOCK_READ_SIZE;
            remainder_reads = number_of_bytes_to_write % RS_CFG_LOCAL_BLOCK_READ_SIZE;

            expected_passes = whole_reads;
            read_address    = logical_start_address;
            write_offset    = 0u;

            /* Carry out read \ compare operations on RS_CFG_LOCAL_BLOCK_READ_SIZE. */
            while ( (whole_reads != 0u) && (!b_interim_read_back_failed) )
            {
                //lint -e{921} Cast to uint32_t to avoid prototype coercion on 16 bit platforms.
                b_write_and_read_matched
                = read_back_and_compare(read_address,
                                        (uint32_t)RS_CFG_LOCAL_BLOCK_READ_SIZE,
                                        &p_write_data[write_offset]);

                if (b_write_and_read_matched)
                {
                    matched_passes++;
                }
                else
                {
                    b_interim_read_back_failed = TRUE;
                }

                whole_reads--;
                read_address += RS_CFG_LOCAL_BLOCK_READ_SIZE;
                write_offset += RS_CFG_LOCAL_BLOCK_READ_SIZE;
            }

            /* Carry out read \ compare on whatever is left after the whole reads. */
            if ( (remainder_reads != 0u) && (!b_interim_read_back_failed) )
            {
                expected_passes++;

                b_write_and_read_matched
                = read_back_and_compare(read_address,
                                        remainder_reads,
                                        &p_write_data[write_offset]);

                if (b_write_and_read_matched)
                {
                    matched_passes++;
                }
            }

            if (matched_passes != expected_passes)
            {
                b_write_ok = FALSE;
            }
        }
    }

    return b_write_ok;
}


// ----------------------------------------------------------------------------
/**
 * read_back_and_compare reads a block of data back from the flash and compares
 * it with another set of data to check that the two blocks match.
 *
 * @param   logical_start_address       Logical start address in flash for write.
 * @param   number_of_bytes_to_read     Number of bytes to read back.
 * @param   p_written_data              Pointer to data which was written.
 * @retval  bool_t                      TRUE if read back OK, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t read_back_and_compare(const uint32_t logical_start_address,
                                    const uint32_t number_of_bytes_to_read,
                                    const uint8_t * const p_written_data)
{
    bool_t              b_write_ok = FALSE;
    flash_hal_error_t   flash_read_status;
    uint8_t             header_read[RS_CFG_LOCAL_BLOCK_READ_SIZE];

    flash_read_status
        = flash_hal_device_read(logical_start_address,
                                number_of_bytes_to_read,
                                &header_read[0u]);

    if (flash_read_status == FLASH_HAL_NO_ERROR)
    {
        b_write_ok = compare_buffers(p_written_data,
                                     &header_read[0u],
                                     number_of_bytes_to_read);
    }

    return b_write_ok;
}


// ----------------------------------------------------------------------------
/**
 * compare_buffers compares the contents of two buffers of type uint8_t.
 *
 * @param   p_buffer1       Pointer to first buffer.
 * @param   p_buffer2       Pointer to second buffer.
 * @param   length          Length of buffers.
 * @retval  bool_t          TRUE if buffers match, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t compare_buffers(const uint8_t * const p_buffer1,
                              const uint8_t * const p_buffer2,
                              const uint32_t length)
{
    uint32_t    offset;
    bool_t      b_buffers_match = FALSE;

    if ( (p_buffer1 != NULL) && (p_buffer2 != NULL) )
    {
        for (offset = 0u; offset < length; offset++)
        {
            if (p_buffer1[offset] != p_buffer2[offset])
            {
                break;
            }
        }

        if (offset == length)
        {
            b_buffers_match = TRUE;
        }
    }

    return b_buffers_match;
}


// ----------------------------------------------------------------------------
/**
 * write_page_data_handle_overlap writes a block of page data to a page and
 * manages any overlap into the next page.
 *
 * @note
 * This function uses the p_next_free_address pointer to pass the next free
 * address which can be written to into the calling function.
 *
 * @warning
 * This function is called after we've already established that there is
 * sufficient space in the partition to write the entire RSR.  Therefore
 * there is no need to check the range of addresses in here. This makes
 * this function simpler, but if it is re-used elsewhere handle with care!
 *
 * @param   p_write                 Pointer to data to write.
 * @param   p_next_free_address     Pointer to return next address to use.
 * @retval  rs_page_status_t        Enumerated value for write status.
 *
 */
// ----------------------------------------------------------------------------
static rs_page_write_status_t write_page_data_handle_overlap
                                    (const rs_page_write_t * const p_write,
                                     uint32_t * const p_next_free_address)
{
    bool_t                  b_write_ok;
    rs_page_details_t       page_details;
    uint32_t                free_space_in_page;
    uint32_t                next_free_address;
    uint32_t                remainder_to_write;
    rs_page_write_status_t  status = RS_PG_WRITE_ERROR;
    bool_t                  b_filled_page = FALSE;

    page_details.partition_logical_start_address = p_write->partition_logical_start_addr;
    page_details.partition_logical_end_address   = p_write->partition_logical_end_addr;
    page_details.address_within_partition        = p_write->next_free_addr;

    //lint -e{920} Ignore return value as we know page details are valid here.
    (void)rspages_page_details_calculate(&page_details);

    /* Distance doesn't take into account the current address. */
    free_space_in_page = page_details.distance_to_upper_address + 1u;

    /* Does the write fit within a single page? */
    if (p_write->bytes_to_write <= free_space_in_page)
    {
        //lint -e{921} Cast to uint32_t to avoid prototype coercion on 16 bit platforms.
        b_write_ok = write_and_read_back(p_write->next_free_addr,
                                         (uint32_t)p_write->bytes_to_write,
                                         p_write->p_write_buffer,
                                         p_write->b_read_back_write_command);

        /*
         * Calculate the next free address irrespective of whether the
         * write works or not - if it fails we want to skip these
         * addresses in future writes as the flash might be damaged.
         */
        next_free_address = p_write->next_free_addr + p_write->bytes_to_write;

        if (next_free_address > page_details.upper_address_within_page)
        {
            b_filled_page = TRUE;

            next_free_address = page_details.upper_address_within_page
                + PAGE_HEADER_LENGTH_BYTES + 1u;

            /*
             * The page is full so write the page header for the next page.
             * Ignore the return value here - if the write of the header fails
             * we will still carry on using the memory.
             */
            //lint -e{920} Cast from enum to void
            (void)write_page_and_page_is_full(p_write, page_details.page_number);
        }
    }
    /*
     * If write won't fit within a single page then split it.
     */
    else
    {
        b_filled_page = TRUE;

        /* Write the first page.*/
        b_write_ok = write_and_read_back(p_write->next_free_addr,
                                         free_space_in_page,
                                         p_write->p_write_buffer,
                                         p_write->b_read_back_write_command);

        /*
         * Write the next page header now we want to use the next page.
         * Ignore the return value here - if the write of the header fails
         * we will still carry on using the memory.
         */
        //lint -e{920} Cast from enum to void
        (void)write_page_and_page_is_full(p_write, page_details.page_number);

        /*
         * Calculate the next free address irrespective of whether the
         * first write worked or not - if it failed we want to skip these
         * addresses in future writes as the flash might be damaged
         */
        next_free_address  = page_details.upper_address_within_page
                                + PAGE_HEADER_LENGTH_BYTES + 1u;

        if (b_write_ok)
        {
            remainder_to_write = p_write->bytes_to_write - free_space_in_page;

            /* Write the second page. */
            b_write_ok = write_and_read_back(next_free_address,
                                             remainder_to_write,
                                             &p_write->p_write_buffer[free_space_in_page],
                                             p_write->b_read_back_write_command);

            /*
             * Update next free address irrespective of whether the second
             * write worked or not - if it failed we want to skip these
             * addresses in future writes as the flash might be damaged.
             *
             * Note that we don't need to check for this address being
             * into the next page as this isn't possible - a TDR will
             * only ever span two pages.
             */
            next_free_address += remainder_to_write;
        }
    }

    if (b_write_ok)
    {
        if (b_filled_page)
        {
            status = RS_PG_WRITE_OK_PAGE_FULL;
        }
        else
        {
            status = RS_PG_WRITE_OK;
        }
    }

    /* Update the next free address in the calling function. */
    *p_next_free_address = next_free_address;

    return status;
}


// ----------------------------------------------------------------------------
/**
 * check_rsr_will_fit_in_partition checks to make sure that the entire RSR
 * will fit in whatever space is available in the partition.
 *
 * @param   p_write_data    Pointer to data to write.
 * @retval  bool_t          TRUE if there is space, FALSE if any error.
 *
 */
// ----------------------------------------------------------------------------
static bool_t check_rsr_will_fit_in_partition
                                (const rs_page_write_t * const p_write_data)
{
    rs_page_details_t   page_details;
    bool_t              b_page_ok;
    uint32_t            free_space_in_page;
    uint32_t            next_page_address;
    uint32_t            remainder_to_write;

    page_details.partition_logical_start_address
                            = p_write_data->partition_logical_start_addr;
    page_details.partition_logical_end_address
                            = p_write_data->partition_logical_end_addr;
    page_details.address_within_partition
                            = p_write_data->next_free_addr;

    b_page_ok = rspages_page_details_calculate(&page_details);

    if (b_page_ok)
    {
        /* Distance doesn't take into account the current address. */
        free_space_in_page = page_details.distance_to_upper_address + 1u;

        /* If there's space in the current page then that's ok. */
        if (p_write_data->bytes_to_write <= free_space_in_page)
        {
            b_page_ok = TRUE;
        }
        /*
         * Otherwise check to make sure the remainder of the write
         * will fit in the next page.
         */
        else
        {
            /* This is the first address we can write to in the next page. */
            next_page_address = page_details.upper_address_within_page
                                    + PAGE_HEADER_LENGTH_BYTES + 1u;

            remainder_to_write = p_write_data->bytes_to_write - free_space_in_page;

            /*
             * This address MUST exist within the partition for the split write
             * to work (as it's the final address which needs to be written to)
             * so if rspages_page_details_calculate returns FALSE
             * then the next page isn't big enough \ doesn't exist.
             */
            page_details.address_within_partition
                            = next_page_address + remainder_to_write - 1u;

            b_page_ok = rspages_page_details_calculate(&page_details);
        }
    }

    return b_page_ok;
}


// ----------------------------------------------------------------------------
/**
 * compare_header_with_addresses compares the header status with various
 * addresses to make sure what the header 'says' matches with the addresses.
 *
 * @param   header_status           The header status.
 * @param   next_free_address       The next free address in the page.
 * @param   initial_read_address    The start of the data in the current page.
 * @param   next_page_address       The next page address.
 * @retval  rs_page_status_t        Page status.
 *
 */
// ----------------------------------------------------------------------------
static rs_page_status_t compare_header_with_addresses
                                    (const rs_header_status_t header_status,
                                     const uint32_t next_free_address,
                                     const uint32_t initial_read_address,
                                     const uint32_t next_page_address)
{
    rs_page_status_t    return_value = RS_PG_HEADER_PAGE_MISMATCH;

    /*lint -e{788} Not all enum constants used in switch.
     * The default case catches all fatal errors.
     *
     * Any situation which we do not handle explicitly
     * will return RS_PG_HEADER_PAGE_MISMATCH.
     */
    switch (header_status)
    {
        case RS_HDR_HEADER_IS_BLANK:
            if (next_free_address == initial_read_address)
            {
                return_value = RS_PG_HEADER_AND_PAGE_BLANK;
            }
        break;

        case RS_HDR_PAGE_IS_CLOSED:
            if (next_free_address == next_page_address)
            {
                return_value = RS_PG_HEADER_OK_PAGE_IS_FULL;
            }
        break;

        case RS_HDR_PAGE_IS_OPEN:
            if (next_free_address == initial_read_address)
            {
                return_value = RS_PG_HEADER_OK_PAGE_IS_EMPTY;
            }
            else if (next_free_address != next_page_address)
            {
                return_value = RS_PG_HEADER_OK_PAGE_HAS_SPACE;
            }
            else
            {
                ;   /* Extra else for MISRA compliance - do nothing. */
            }
        break;

        case RS_HDR_PAGE_IS_EMPTY:
            if (next_free_address == initial_read_address)
            {
                return_value = RS_PG_HEADER_OK_PAGE_IS_EMPTY;
            }
            else if (next_free_address < next_page_address)
            {
                return_value = RS_PG_HEADER_OK_PAGE_HAS_SPACE;
            }
            else
            {
                ;   /* Extra else for MISRA compliance - do nothing. */
            }
        break;

        /* An undefined header isn't too bad, so let it pass.
         * To get an undefined header, everything else in the
         * header must have checked out OK because of the order
         * in which the header is parsed.
         */
        case RS_HDR_PAGE_IS_UNDEFINED:
            if (next_free_address == initial_read_address)
            {
                return_value = RS_PG_HEADER_OK_PAGE_IS_EMPTY;
            }
            else if (next_free_address != next_page_address)
            {
                return_value = RS_PG_HEADER_OK_PAGE_HAS_SPACE;
            }
            else
            {
                return_value = RS_PG_HEADER_OK_PAGE_IS_FULL;
            }
        break;

        /* An error code fail in the header probably means that
         * somewhere in the page we had a write failure, so mark
         * the page as having errors somewhere.  There is a risk
         * that if it's 'just' a header failure then we'll mark
         * the block as unusable by mistake.
         */
        case RS_HDR_HEADER_ERROR_CODE_FAIL:
            return_value = RS_PG_HEADER_OK_PAGE_HAS_ERRORS;
        break;

        default:
            return_value = RS_PG_HEADER_ERROR;
        break;
    }

    return return_value;
}


// ----------------------------------------------------------------------------
/**
 * check_next_page_is_blank makes sure that the page after the current one
 * is blank.
 *
 * This function is only called if the page has space in it, so the page
 * status will be RS_PG_HEADER_OK_PAGE_HAS_SPACE.  If the following page
 * has space in it, the return status is still RS_PG_HEADER_OK_PAGE_HAS_SPACE.
 *
 * @param   p_header_data       Pointer to header data for current page \ partition.
 * @retval  rs_page_status_t    Page status, which might be modified.
 */

// ----------------------------------------------------------------------------
static rs_page_status_t check_next_page_is_blank
                            (const rs_header_data_t * const p_header_data)
{
    uint32_t            last_potential_read_address;
    uint32_t            new_page_first_read_address;
    uint32_t            new_page_free_address;
    uint32_t            number_of_bytes_to_read;
    rs_page_status_t    return_value = RS_PG_HEADER_OK_PAGE_HAS_SPACE;


    /* Last address is the address of the end of the second page. */
    last_potential_read_address = p_header_data->partition_logical_start_addr
                                    + (RS_CFG_PAGE_SIZE_KB * 1024u
                                        * (p_header_data->page_number + 2u) )
                                    - 1u;

    /* Only check if there are actually more pages. */
    if (last_potential_read_address <= p_header_data->partition_logical_end_addr)
    {
        new_page_first_read_address
            = p_header_data->partition_logical_start_addr
                + (RS_CFG_PAGE_SIZE_KB * 1024u * (p_header_data->page_number + 1u))
                + PAGE_HEADER_LENGTH_BYTES;

        number_of_bytes_to_read     = (RS_CFG_PAGE_SIZE_KB * 1024u)
                                        - PAGE_HEADER_LENGTH_BYTES;

        new_page_free_address = rssearch_find_next_free_address
                                     (new_page_first_read_address,
                                      number_of_bytes_to_read);

        /*
         * If free address is the start of the page then the
         * entire page is blank, otherwise the page didn't
         * have space in it, so do something about it.
         *
         * There are three situations where we can have space
         * in a page, when the header says the page is open,
         * empty or undefined, so deal with these.
         */
        if (new_page_free_address != new_page_first_read_address)
        {
            /*
             * If the header is undefined then change the
             * return value to say the page is full.
             * This is not an error.
             */
            if (p_header_data->header_status == RS_HDR_PAGE_IS_UNDEFINED)
            {
                return_value = RS_PG_HEADER_OK_PAGE_IS_FULL;
            }
            /* Otherwise there's been a mismatch (error). */
            else
            {
                return_value = RS_PG_HEADER_PAGE_MISMATCH;
            }
        }
    }

    return return_value;
}


// ----------------------------------------------------------------------------
/**
 * check_contents_of_page_header checks to make sure that the contents of
 * the page header are correct.
 *
 * @param   p_buffer            Pointer to buffer containing page header.
 * @param   partition_id        The partition ID which the header refers to.
 * @retval  rs_header_status_t  Page header status.
 *
 */
// ----------------------------------------------------------------------------
static rs_header_status_t check_contents_of_page_header
                                            (const uint8_t * const p_buffer,
                                             const uint8_t partition_id)
{
    rs_header_status_t  return_value;
    bool_t              b_header_is_blank;
    uint8_t             checksum;
    uint16_t            status;

    b_header_is_blank = check_area_is_blank(p_buffer, PAGE_HEADER_LENGTH_BYTES);

    checksum = p_buffer[PAGE_HEADER_FORMAT_OFFSET]
                   + p_buffer[PAGE_HEADER_PARID_OFFSET];

    if (b_header_is_blank)
    {
        return_value = RS_HDR_HEADER_IS_BLANK;
    }
    else if (checksum != p_buffer[PAGE_HEADER_CHECKSUM_OFFSET])
    {
        return_value = RS_HDR_HEADER_CHECKSUM_FAIL;
    }
    else if (p_buffer[PAGE_HEADER_PARID_OFFSET] != partition_id)
    {
        return_value = RS_HDR_HEADER_PARTITION_ID_FAIL;
    }
    else if (p_buffer[PAGE_HEADER_FORMAT_OFFSET] != PAGE_HEADER_FORMAT_CODE_OK)
    {
        return_value = RS_HDR_HEADER_FORMAT_CODE_FAIL;
    }
    else if (p_buffer[PAGE_HEADER_ERROR_OFFSET] != PAGE_HEADER_ERROR_CODE_OK)
    {
        return_value = RS_HDR_HEADER_ERROR_CODE_FAIL;
    }
    else
    {
        /* Status is organised MSB first in the page buffer. */

        //lint -e{921} Cast from 8 to 16 bits as we're combining two bytes.
        status = (uint16_t)p_buffer[PAGE_HEADER_STATUS_LSB] & 0x00FFu;

        //lint -e{921} Cast from 8 to 16 bits as we're combining two bytes.
        status |= (((uint16_t)p_buffer[PAGE_HEADER_STATUS_MSB] << 8u) & 0xFF00u);

        switch (status)
        {
            case PAGE_HEADER_STATUS_CLOSED:
                return_value = RS_HDR_PAGE_IS_CLOSED;
            break;

            case PAGE_HEADER_STATUS_OPEN:
                return_value = RS_HDR_PAGE_IS_OPEN;
            break;

            case PAGE_HEADER_STATUS_BLANK:
                return_value = RS_HDR_PAGE_IS_EMPTY;
            break;

            /* Anything other than the above is undefined. */
            default:
                return_value = RS_HDR_PAGE_IS_UNDEFINED;
            break;
        }
    }

    return return_value;
}


// ----------------------------------------------------------------------------
/**
 * write_page_and_page_is_full is called when a page has been written and
 * is now full.  This function updates the running page counters and writes
 * the header of the next page.
 *
 * @param   p_write             Pointer to write data for page.
 * @param   p_details           Pointer to extra page details.
 * @retval  rs_header_status_t  Page header status.
 *
 */
// ----------------------------------------------------------------------------
static rs_header_status_t write_page_and_page_is_full
                                (const rs_page_write_t * const p_write,
                                 const uint32_t current_page_number)
{
    rs_header_data_t    header_data;
    rs_header_status_t  header_write;

    /* Update the running page counters as we've filled a page. */
    rspartition_flag_page_as_full(p_write->partition_index);

    /* Write the next page header now we want to use the next page. */
    header_data.partition_index = p_write->partition_index;
    header_data.partition_id    = p_write->partition_id;
    header_data.partition_logical_start_addr
                                = p_write->partition_logical_start_addr;
    header_data.partition_logical_end_addr
                                = p_write->partition_logical_end_addr;
    header_data.format_code     = 0x8Du;
    header_data.status          = 0x6996u;
    header_data.error_code      = 0xFFu;
    header_data.error_address   = 0xFFFFu;
    header_data.page_number     = current_page_number + 1u;

    header_write = rspages_page_header_write(&header_data);

    return header_write;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

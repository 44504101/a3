// ----------------------------------------------------------------------------
/**
 * @file        rssearch.c
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        6 Apr 2016
 * @brief       Support functions for searching the recording system.
 * @details
 * Support functions for the recording system, anything related to searching.
 *
 * @note
 * These functions should only be called from other recording system functions,
 * not directly as if they were part of the API.
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
#include "rsapi.h"
#include "rssearch.h"
#include "rssearch_prv.h"
#include "rspages.h"
#include "flash_hal.h"
#include "crc.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define RSR_TDR_OFFSET_FROM_SYNC    3u      ///< Start of TDR is 3 away from SYNC.
#define RSR_TDR_EXTRA_LENGTH        2u      ///< TDR length field is 2 bytes.
#define RSR_CRC_EXTRA_LENGTH        5u      ///< SYNC, IDx2, LENx2.
#define RSR_WRAPPER_SIZE_OVERHEAD   8u      ///< SYNC, IDx2, LENx2, CRCx2, ENDSYNC

/// The size of the find buffer - twice the size of the largest possible RSR.
#define RSR_FIND_BUFFER_SIZE        (2u * (RS_CFG_MAX_TDR_SIZE_BYTES + RSR_WRAPPER_SIZE_OVERHEAD) )

/// A blank character in the RSR will be an 0xFF.
#define RSR_BLANK_CHARACTER         0xFFu


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static uint16_t count_blanks_from_end(const uint8_t * const p_area,
                                      const uint16_t size_of_area);

static uint8_t partition_memory_read_setup
                        (const rssearch_internal_memory_t * const p_memory_data,
                         uint32_t * const p_read_addresses,
                         uint32_t * const p_bytes_to_read);

static uint8_t partition_memory_read_setup_fwd
                    (const rs_page_details_t * const p_page_details,
                     uint32_t * const p_read_addresses,
                     uint32_t * const p_bytes_to_read);

static uint8_t partition_memory_read_setup_bwd
                    (const rs_page_details_t * const p_page_details,
                     uint32_t * const p_read_addresses,
                     uint32_t * const p_bytes_to_read);

static uint32_t read_partition_data(const uint32_t * const p_read_address,
                                    const uint32_t * const p_bytes_to_read,
                                    const uint8_t number_of_reads);

static bool_t search_for_valid_rsr_in_buffer
                    (const rssearch_internal_search_t * const p_internal_data,
                     rssearch_rsr_local_data_t * const p_local_data);

static bool_t check_for_record_and_instance
                    (const rssearch_internal_check_t * const p_internal_data,
                     uint32_t * const p_instance_counter);

static uint16_t convert_msb_lsb_8bits_into_16bits
                                (const uint8_t * const p_buffer);

static uint16_t convert_lsb_msb_8bits_into_16bits
                                (const uint8_t * const p_buffer);

static bool_t calc_next_search_address
                            (const rssearch_search_data_t * const p_search_data,
                             const uint32_t * const p_read_address,
                             const uint32_t * const p_bytes_to_read,
                             const uint8_t number_of_reads,
                             const uint16_t last_valid_search_index,
                             uint32_t * const p_next_search_start_address);

static uint32_t calc_next_search_address_fwd
                            (const uint32_t * const p_read_address,
                             const uint32_t * const p_bytes_to_read,
                             const uint8_t number_of_reads,
                             const uint16_t last_valid_search_index);

static uint32_t calc_next_search_address_bwd
                            (const uint32_t * const p_read_address,
                             const uint32_t * const p_bytes_to_read,
                             const uint8_t number_of_reads,
                             const uint16_t last_valid_search_index);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/*
 * Note that the first three of these variables need to be volatile because
 * they are only used within this module and are not changed from outside it,
 * so we can disable all the lint warnings for 956 (non const, non volatile
 * static or external variable)
 *
 * The final variable is volatile because it is changed by a callback
 * function so the loop in which it is running may not know that.
 */

/// Search buffer to load data into to search, looking for a valid RSR.
//lint -e{956}
static uint8_t              m_rsr_search_buffer[RSR_FIND_BUFFER_SIZE];

/// Flag which says whether a valid RSR has been found or not.
//lint -e{956}
static bool_t               mb_rsr_is_valid = FALSE;

/// Structure containing results from RSR search.
//lint -e{956}
static rssearch_rsr_info_t  m_rsr_info = {NULL, NULL, 0u, 0u, 0u};

/// Volatile flag to force a search timeout (triggered by callback).
static volatile bool_t      mb_rssearch_timeout = FALSE;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * rssearch_find_next_free_address searches back through a contiguous area of
 * memory looking for the start of a blank section.
 *
 * A blank section is defined as a contiguous section which all contains 0xFF
 * - there is a risk with this approach that the last data byte may be an 0xFF
 * as well, so this situation needs to be checked for elsewhere.
 * Unfortunately the recording system specification doesn't cater for this.
 *
 * @param   logical_start_address       Start address of contiguous area.
 * @param   number_of_bytes_to_check    Number of contiguous bytes to check.
 * @retval  uint32_t                    Start of blank area or 0xFFFFFFFF for fail.
 *
 */
// ----------------------------------------------------------------------------
uint32_t rssearch_find_next_free_address(const uint32_t logical_start_address,
                                         const uint32_t number_of_bytes_to_check)
{
    uint32_t            next_free_address = 0xFFFFFFFFu;
    flash_hal_error_t   flash_read_status;
    uint8_t             block_buffer[RS_CFG_LOCAL_BLOCK_READ_SIZE];
    uint32_t            whole_blocks_to_read;
    uint32_t            remainder_to_read;
    uint32_t            logical_read_address;
    uint32_t            blanks_from_end = 0u;
    uint32_t            total_blanks_from_end = 0u;
    bool_t              b_found_used_data = FALSE;
    bool_t              b_flash_ok = TRUE;

    whole_blocks_to_read = number_of_bytes_to_check / RS_CFG_LOCAL_BLOCK_READ_SIZE;
    remainder_to_read    = number_of_bytes_to_check % RS_CFG_LOCAL_BLOCK_READ_SIZE;

    /* Setup the initial address to start at.
     * We might underflow if we haven't got a whole page, but it doesn't
     * matter because the remainder code doesn't use this logical_read_address.
     */
    logical_read_address = (logical_start_address + number_of_bytes_to_check)
                                - RS_CFG_LOCAL_BLOCK_READ_SIZE;

    /* Read whole blocks until we've done them all or the flash fails. */
    while ( (whole_blocks_to_read != 0u) && (b_flash_ok) )
    {
        //lint -e{921} Cast to uint32_t to avoid prototype coercion on 16 bit platforms.
        flash_read_status = flash_hal_device_read(logical_read_address,
                                                  (uint32_t)RS_CFG_LOCAL_BLOCK_READ_SIZE,
                                                  &block_buffer[0u]);

        if (flash_read_status != FLASH_HAL_NO_ERROR)
        {
            b_flash_ok = FALSE;
        }
        else
        {
            blanks_from_end = count_blanks_from_end(&block_buffer[0u],
                                                    RS_CFG_LOCAL_BLOCK_READ_SIZE);

            total_blanks_from_end += blanks_from_end;

            /* Jump out as soon as we find something not completely blank. */
            if (blanks_from_end != RS_CFG_LOCAL_BLOCK_READ_SIZE)
            {
                b_found_used_data = TRUE;
                break;
            }

            logical_read_address -= RS_CFG_LOCAL_BLOCK_READ_SIZE;
            whole_blocks_to_read--;
        }
    }

    /* Only do the remainder if we haven't already found some used data.
     * There will only ever be a single remainder, so read from the
     * logical start address.
     */
    if ( (remainder_to_read != 0u) && (!b_found_used_data) && (b_flash_ok) )
    {
        flash_read_status = flash_hal_device_read(logical_start_address,
                                                  remainder_to_read,
                                                  &block_buffer[0u]);

        if (flash_read_status != FLASH_HAL_NO_ERROR)
        {
            b_flash_ok = FALSE;
        }
        else
        {
            //lint -e{921} Cast remainder to 16 bit - buffer isn't bigger than this.
            blanks_from_end = count_blanks_from_end(&block_buffer[0u],
                                                    (uint16_t)remainder_to_read);

            total_blanks_from_end += blanks_from_end;
        }
    }

    /* Any flash error and we simply return an address of 0xFFFFFFFF,
     * otherwise calculate the next free address.
     */
    if (b_flash_ok)
    {
        next_free_address = (logical_start_address + number_of_bytes_to_check)
                                - total_blanks_from_end;
    }

    return next_free_address;
}


// ----------------------------------------------------------------------------
/**
 * rssearch_find_valid_RSR_start searches back through a contiguous area of
 * memory looking for the start of a valid recording system record (RSR).
 *
 * @warning
 * This function relies on the fact that an RSR will only ever span two
 * pages in the recording system, as we assume that the page size will be
 * at least the same as the TDR size.  There is a check in rsapi.c to enforce
 * this.
 *
 * @warning
 * This function is not thread safe as it uses the m_rsr_search_buffer and
 * accesses the flash directly.  This function must therefore only be used
 * during initialisation (when no tasks are running) or via the read \ write
 * gate keeper task in the API.
 *
 * @param   p_search_data   Pointer to search data structure
 * @retval  bool_t          TRUE if found valid RSR.
 *
 */
// ----------------------------------------------------------------------------
bool_t rssearch_find_valid_RSR_start
                (const rssearch_search_data_t * const p_search_data)
{
    uint32_t                    read_address[2];
    uint32_t                    bytes_to_read[2];
    uint32_t                    instance = 0u;
    uint8_t                     number_of_reads;
    bool_t                      b_finished_searching = FALSE;
    bool_t                      b_checked_entire_buffer;
    bool_t                      b_found_valid_rsr;
    bool_t                      b_found_correct_record_and_instance;
    uint16_t                    last_valid_search_index;
    rssearch_internal_memory_t  memory_data;
    rssearch_internal_search_t  search_data;
    rssearch_internal_check_t   check_data;
    rssearch_rsr_local_data_t   local_data;

    /* Assume the worst - the RSR is not valid. */
    mb_rsr_is_valid = FALSE;

    /*
     * Reset the timeout flag before we start searching
     * (unless we're unit testing, because we can't change
     * it using the callback during the test).
     */
#ifndef UNIT_TEST_BUILD
    mb_rssearch_timeout = FALSE;
#endif

    /*
     * Fail if problems with the addresses.
     */
    if ( (p_search_data->partition_logical_start_address
            > p_search_data->partition_logical_end_address)
        || (p_search_data->search_start_address
            < p_search_data->partition_logical_start_address)
        || (p_search_data->search_start_address
            > p_search_data->partition_logical_end_address) )
    {
        ;
    }
    else
    {
        memory_data.search_direction                = p_search_data->search_direction;
        memory_data.partition_logical_start_address = p_search_data->partition_logical_start_address;
        memory_data.partition_logical_end_address   = p_search_data->partition_logical_end_address;
        memory_data.search_start_address            = p_search_data->search_start_address;

        search_data.search_direction                = p_search_data->search_direction;

        check_data.required_record_instance         = p_search_data->required_record_instance;
        check_data.b_match_record_id                = p_search_data->b_match_record_id;
        check_data.required_record_id               = p_search_data->required_record_id;

        while (!mb_rssearch_timeout && !b_finished_searching)
        {
            number_of_reads = partition_memory_read_setup(&memory_data,
                                                          &read_address[0u],
                                                          &bytes_to_read[0u]);

            /*
             * Read the partition data and put in in the RSR buffer.
             * Note the cast of the return value - we will only ever
             * read up to twice the RSR size, which is ~8k.
             */
            //lint -e{921} Cast to uint16_t.
            search_data.bytes_read_into_buffer
                = (uint16_t)read_partition_data(&read_address[0u],
                                                &bytes_to_read[0u],
                                                number_of_reads);

            /*
             * Abort if something went wrong with the partition read.
             * We use a break here to reduce the nesting of the
             * code below slightly. Execution will jump to the code
             * after the end of the while (!b_finished_searching) loop.
             */
            if (search_data.bytes_read_into_buffer == 0u)
            {
                break;
            }

            if (search_data.search_direction == RSSEARCH_FORWARDS)
            {
                last_valid_search_index = 0u;
            }
            else
            {
                last_valid_search_index = search_data.bytes_read_into_buffer;
            }

            b_checked_entire_buffer = FALSE;

            /* Loop round checking the buffer, looking for valid \ matching RSR's. */
            while (!b_checked_entire_buffer)
            {
                search_data.search_start_index = last_valid_search_index;

                /*
                 * Need to decrement the search start index for a backwards
                 * search otherwise we just find the same RSR as we did before.
                 */
                if ((search_data.search_direction == RSSEARCH_BACKWARDS)
                        && (search_data.search_start_index != 0u))
                {
                    search_data.search_start_index--;
                }

                b_found_valid_rsr = search_for_valid_rsr_in_buffer(&search_data, &local_data);

                if (b_found_valid_rsr)
                {
                    /*
                     * Update the last valid search index
                     * as we've found a valid RSR.
                     */

                    /*
                     * If searching forwards then start the next search at the
                     * location after the end of the RSR we've just found.
                     */
                    if (search_data.search_direction == RSSEARCH_FORWARDS)
                    {
                        last_valid_search_index = local_data.last_searched_index + 1u;
                    }
                    else
                    {
                        last_valid_search_index = local_data.last_searched_index;
                    }

                    b_found_correct_record_and_instance = check_for_record_and_instance(&check_data, &instance);

                    /*
                     * If valid RSR and correct record \ instance then
                     * we're done, so set the module level flag.
                     * The b_finished_searching flag jumps us out of
                     * the while (!b_finished_searching) loop
                     * The break statement jumps us out of the
                     * while (!b_checked_entire_buffer) loop.
                     */
                    if (b_found_correct_record_and_instance)
                    {
                        mb_rsr_is_valid = TRUE;
                        b_finished_searching = TRUE;
                        break;
                    }
                }

                /* Have we reached the end of the buffer? */
                if ( (local_data.number_of_bytes_checked == local_data.maximum_check_size)
                        || (last_valid_search_index == 0u) )
                {
                    b_finished_searching
                        = calc_next_search_address(p_search_data,
                                                   &read_address[0u],
                                                   &bytes_to_read[0u],
                                                   number_of_reads,
                                                   last_valid_search_index,
                                                   &memory_data.search_start_address);

                    /*
                     * Jump out of the while (!b_checked_entire_buffer)
                     * loop and read in next memory block or stop.
                     */
                    b_checked_entire_buffer = TRUE;
                }
            }
        }
    }

    return mb_rsr_is_valid;
}


// ----------------------------------------------------------------------------
/**
 * rssearch_valid_rsr_pointer_get returns a pointer to the RSR info structure.
 *
 * As a sanity check, this returns a NULL pointer if the mb_rsr_is_valid flag
 * is FALSE.
 *
 * @retval rssearch_rsr_info_t*     Pointer to RSR info structure.
 *
 */
// ----------------------------------------------------------------------------
const rssearch_rsr_info_t* rssearch_valid_rsr_pointer_get(void)
{
    const rssearch_rsr_info_t*    p_rsr_info = NULL;

    if (mb_rsr_is_valid)
    {
        p_rsr_info = &m_rsr_info;
    }

    return p_rsr_info;
}


// ----------------------------------------------------------------------------
/**
 * rssearch_timeout_callback is the callback function for the search timeout.
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
void rssearch_timeout_callback(void* xTimer)
{
    mb_rssearch_timeout = TRUE;
}


#ifdef UNIT_TEST_BUILD
// ----------------------------------------------------------------------------
/**
 * rssearch_unit_test_ptr_get returns a pointer to the unit test pointers
 * structure, for test purposes.
 * We use an ifdef to ensure that this pointer can't be accessed under
 * normal operation.
 *
 * @retval rsapi_unit_test_pointers_t*      Pointer to test structure.
 *
 */
// ----------------------------------------------------------------------------
rssearch_unit_test_pointers_t* rssearch_unit_test_ptr_get(void)
{
    //lint -e{956} Doesn't need to be volatile nere.  Pointers never change.
    static rssearch_unit_test_pointers_t p_unit_test_structure =
    {
        &m_rsr_search_buffer[0u],
        &mb_rsr_is_valid,
        &mb_rssearch_timeout,
        &m_rsr_info,

        count_blanks_from_end,
        partition_memory_read_setup,
        read_partition_data,
        search_for_valid_rsr_in_buffer,
        check_for_record_and_instance,
        convert_msb_lsb_8bits_into_16bits
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
 * count_blanks_from_end counts the number of contiguous blank locations
 * from the end of an array, working towards the start of the array.
 *
 * @param   p_area          Pointer to array to check.
 * @param   size_of_area    Length of array.
 * @retval  uint16_t        Number of contiguous blank locations.
 *
 */
// ----------------------------------------------------------------------------
static uint16_t count_blanks_from_end(const uint8_t * const p_area,
                                      const uint16_t size_of_area)
{
    uint16_t    area_offset = size_of_area;
    uint16_t    contiguous_blanks = 0u;

    while (area_offset != 0u)
    {
        if (p_area[area_offset - 1u] == RS_CFG_BLANK_LOCATION_CONTAINS)
        {
            contiguous_blanks++;
            area_offset--;
        }
        else
        {
            break;
        }
    }

    return contiguous_blanks;
}


// ----------------------------------------------------------------------------
/**
 * partition_memory_read_setup sets up the required reads to get enough data
 * from the partition memory so that at least one RSR should be contained in
 * the m_rsr_search_buffer.
 *
 * This function will read RSR_FIND_BUFFER_SIZE bytes from the recording memory
 * starting at the search start address and working forwards or backwards
 * depending on the search direction.
 *
 * If we've reached the bottom or top of the partition itself then the read is
 * truncated, as an RSR will not span partitions.
 *
 * This function skips any page headers which it encounters along the way,
 * splitting the read into two chunks, one before and one after the header.
 *
 * @param   p_memory_data       Pointer to memory data structure.
 * @param   p_read_addresses    Pointer to array to write read addresses into.
 * @param   p_bytes_to_read     Pointer to array to write bytes to read into.
 * @retval  uint8_t             Number of reads required.
 *
 */
// ----------------------------------------------------------------------------
static uint8_t partition_memory_read_setup
                        (const rssearch_internal_memory_t * const p_memory_data,
                         uint32_t * const p_read_addresses,
                         uint32_t * const p_bytes_to_read)
{
    uint8_t             number_of_reads = 0u;
    rs_page_details_t   page_details;

    if ((p_memory_data != NULL) && (p_read_addresses != NULL)
            && (p_bytes_to_read != NULL))
    {
        page_details.partition_logical_start_address = p_memory_data->partition_logical_start_address;
        page_details.partition_logical_end_address   = p_memory_data->partition_logical_end_address;
        page_details.address_within_partition        = p_memory_data->search_start_address;

        //lint -e{920} Ignore return value as we know page details are valid here.
        (void)rspages_page_details_calculate(&page_details);

        switch (p_memory_data->search_direction)
        {
            case RSSEARCH_FORWARDS:
               number_of_reads = partition_memory_read_setup_fwd(&page_details,
                                                                 p_read_addresses,
                                                                 p_bytes_to_read);
            break;

            case RSSEARCH_BACKWARDS:
                number_of_reads = partition_memory_read_setup_bwd(&page_details,
                                                                  p_read_addresses,
                                                                  p_bytes_to_read);
            break;

            /* The default case isn't testable but is required just in case. */
            default:
                number_of_reads = 0u;
            break;
        }
    }

    return number_of_reads;
}


// ----------------------------------------------------------------------------
/**
 * partition_memory_read_setup_fwd is called from the
 * partition_memory_read_setup() function when searching forwards.
 *
 * @param   p_page_details      Pointer to page details structure.
 * @param   p_read_addresses    Pointer to array to write read addresses into.
 * @param   p_bytes_to_read     Pointer to array to write bytes to read into.
 * @retval  uint8_t             Number of reads required.
 *
 */
// ----------------------------------------------------------------------------
static uint8_t partition_memory_read_setup_fwd
                            (const rs_page_details_t * const p_page_details,
                             uint32_t * const p_read_addresses,
                             uint32_t * const p_bytes_to_read)
{
    uint8_t     number_of_reads = 1u;
    uint32_t    last_page_number;

    last_page_number = p_page_details->maximum_number_of_pages - 1u;

    /* Does the read fit somewhere in a page with no shortening of the read? */
    if (p_page_details->distance_to_upper_address >= RSR_FIND_BUFFER_SIZE)
    {
        p_bytes_to_read[0u]  = RSR_FIND_BUFFER_SIZE;

        /*
         * If the start address is in the page header
         * then start at the lower page boundary.
         */
        if (p_page_details->address_within_partition < p_page_details->lower_address_within_page)
        {
            p_read_addresses[0u] = p_page_details->lower_address_within_page;
        }
        else
        {
            p_read_addresses[0u] = p_page_details->address_within_partition;
        }
    }
    /* Does the read fall off the top of the page and this is the last page? */
    else if ((p_page_details->distance_to_upper_address < RSR_FIND_BUFFER_SIZE)
                && (p_page_details->page_number == last_page_number))
    {
        p_bytes_to_read[0u]  = p_page_details->distance_to_upper_address + 1u;
        p_read_addresses[0u] = p_page_details->address_within_partition;
    }
    /* Does the read fall off the top of the page but we can read the next page? */
    else if ((p_page_details->distance_to_upper_address < RSR_FIND_BUFFER_SIZE)
                && (p_page_details->page_number < last_page_number))
    {
        p_bytes_to_read[0u]  = p_page_details->distance_to_upper_address + 1u;
        p_read_addresses[0u] = p_page_details->address_within_partition;

        p_bytes_to_read[1u]  = RSR_FIND_BUFFER_SIZE - p_bytes_to_read[0u];
        p_read_addresses[1u] = p_page_details->upper_address_within_page + PAGE_HEADER_LENGTH_BYTES + 1u;

        number_of_reads = 2u;
    }
    /* Any other condition is a mistake, so don't read anything. */
    else
    {
        number_of_reads = 0u;
    }

    return number_of_reads;
}


// ----------------------------------------------------------------------------
/**
 * partition_memory_read_setup_bwd is called from the
 * partition_memory_read_setup() function when searching backwards.
 *
 * @param   p_page_details      Pointer to page details structure.
 * @param   p_read_addresses    Pointer to array to write read addresses into.
 * @param   p_bytes_to_read     Pointer to array to write bytes to read into.
 * @retval  uint8_t             Number of reads required.
 *
 */
// ----------------------------------------------------------------------------
static uint8_t partition_memory_read_setup_bwd
                            (const rs_page_details_t * const p_page_details,
                             uint32_t * const p_read_addresses,
                             uint32_t * const p_bytes_to_read)
{
    uint8_t     number_of_reads = 1u;

    /*
     * Does the read fit somewhere in a page with no adjustments necessary?
     * Note that when reading backwards we don't include the search start
     * address - this is to ensure that we always start on a word boundary.
     */
    if (p_page_details->distance_to_lower_address >= RSR_FIND_BUFFER_SIZE)
    {
        p_bytes_to_read[0u]  = RSR_FIND_BUFFER_SIZE;
        p_read_addresses[0u] = p_page_details->address_within_partition - RSR_FIND_BUFFER_SIZE;
    }
    /* Does the read fall off the bottom of the page and this is the first page? */
    else if ((p_page_details->distance_to_lower_address < RSR_FIND_BUFFER_SIZE)
                && (p_page_details->page_number == 0u))
    {
        p_bytes_to_read[0u]  = p_page_details->distance_to_lower_address;
        p_read_addresses[0u] = p_page_details->partition_logical_start_address + PAGE_HEADER_LENGTH_BYTES;

        /* If we're in the header then there's no read to be done. */
        if (p_page_details->distance_to_lower_address == 0u)
        {
            p_bytes_to_read[0u] = 0u;
            number_of_reads = 0u;
        }
    }
    /* Does the read fall off the bottom of the page but we can read the previous page? */
    else if ((p_page_details->distance_to_lower_address < RSR_FIND_BUFFER_SIZE)
                && (p_page_details->page_number > 0u))
    {
        /*
         * Special case if we're in the header - just read an equal
         * number of bytes from the previous and current pages.
         */
        if (p_page_details->distance_to_lower_address == 0u)
        {
            /* Setup read for the current page (from the page boundary). */
            p_bytes_to_read[1u]  = RSR_FIND_BUFFER_SIZE / 2u;
            p_read_addresses[1u] = p_page_details->lower_address_within_page;

            /* Number of bytes to read from previous page. */
            p_bytes_to_read[0u]  = RSR_FIND_BUFFER_SIZE / 2u;
        }
        else
        {
            /* Setup read for the current page (from the page boundary). */
            p_bytes_to_read[1u]  = p_page_details->distance_to_lower_address;
            p_read_addresses[1u] = p_page_details->lower_address_within_page;

            /* Number of bytes to read from previous page. */
            p_bytes_to_read[0u]  = RSR_FIND_BUFFER_SIZE - p_bytes_to_read[1u];
        }

        /*
         * Calculate start address of current page and then subtract the
         * number of bytes which we need to read from the previous page
         * to arrive at the read address for the previous page.
         */
        p_read_addresses[0u] = p_page_details->lower_address_within_page - PAGE_HEADER_LENGTH_BYTES;
        p_read_addresses[0u] -= p_bytes_to_read[0u];

        number_of_reads = 2u;
    }
    /* Any other condition is a mistake, so don't read anything. */
    else
    {
        number_of_reads = 0u;
    }

    return number_of_reads;
}


// ----------------------------------------------------------------------------
/**
 * read_partition_data does the actual read from the flash memory.
 * This uses a pair of arrays which hold the addresses to read from and the
 * number of contiguous bytes to read from each address, and writes to
 * the module scope variable m_rsr_search_buffer.
 *
 * @param   p_read_address      Pointer to array containing read addresses.
 * @param   p_bytes_to_read     Pointer to array containing bytes to read.
 * @param   number_of_reads     Number of reads required.
 * @retval  uint32_t            Total number of bytes read (or zero if error).
 *
 */
// ----------------------------------------------------------------------------
static uint32_t read_partition_data(const uint32_t * const p_read_address,
                                    const uint32_t * const p_bytes_to_read,
                                    const uint8_t number_of_reads)
{
    uint32_t            total_number_of_bytes_read = 0u;
    flash_hal_error_t   flash_status;
    uint8_t             read_counter;
    uint32_t            write_offset = 0u;
    uint32_t            erase_counter;

    if (number_of_reads == 0u)
    {
        total_number_of_bytes_read = 0u;
    }
    else
    {
        for (erase_counter = 0u; erase_counter < RSR_FIND_BUFFER_SIZE;
                erase_counter++)
        {
            m_rsr_search_buffer[erase_counter] = RSR_BLANK_CHARACTER;
        }

        for (read_counter = 0u; read_counter < number_of_reads; read_counter++)
        {
            flash_status = flash_hal_device_read(p_read_address[read_counter],
                                                 p_bytes_to_read[read_counter],
                                                 &m_rsr_search_buffer[write_offset]);

            total_number_of_bytes_read += p_bytes_to_read[read_counter];

            /* Jump out if any flash read error - no fancy retries. */
            if (flash_status != FLASH_HAL_NO_ERROR)
            {
                total_number_of_bytes_read = 0u;
                break;
            }

            write_offset += p_bytes_to_read[read_counter];
        }
    }

    return total_number_of_bytes_read;
}


// ----------------------------------------------------------------------------
/**
 * search_for_valid_rsr_in_buffer searches backwards or forwards through the
 * m_rsr_search_buffer, looking for a valid RSR and a matching record ID
 * and an instance of that record ID.
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
 * @note
 * This function uses the module scope buffer m_rsr_search_buffer, hence
 * no arguments relating to the buffer are required.  It is assumed that this
 * buffer has been loaded with something useful before this function is called.
 *
 * @note
 * The pointer p_local_data points to a structure which is used for variables
 * variables which we want to be able to check using the unit tests, or use in
 * the calling function.  It's not terribly elegant, but it's better than
 * sprinkling printf's all over this function or having a global structure.
 *
 * @param   p_internal_data     Pointer to internal data structure to use.
 * @param   p_local_data        Pointer to working data which is modified.
 * @retval  bool_t              TRUE if valid RSR found, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t search_for_valid_rsr_in_buffer
                    (const rssearch_internal_search_t * const p_internal_data,
                     rssearch_rsr_local_data_t * const p_local_data)
{
    uint16_t    search_index;
    uint16_t    tdr_offset;
    uint16_t    crc_offset;
    uint16_t    crc_length;
    bool_t      b_rsr_is_valid = FALSE;

    search_index = p_internal_data->search_start_index;

    /* Make sure the search is actually going to work. */
    if ((p_internal_data->bytes_read_into_buffer > RSR_FIND_BUFFER_SIZE)
            || (search_index >= p_internal_data->bytes_read_into_buffer))
    {
        p_local_data->maximum_check_size = 0u;
    }
    else
    {
        if (p_internal_data->search_direction == RSSEARCH_BACKWARDS)
        {
            p_local_data->maximum_check_size = search_index + 1u;
        }
        else
        {
            p_local_data->maximum_check_size
                = p_internal_data->bytes_read_into_buffer - search_index;
        }
    }

    /*
     * Go through the search buffer in the required direction until we've
     * either found a valid RSR or checked the whole buffer.
     */
    for (p_local_data->number_of_bytes_checked = 0u;
            p_local_data->number_of_bytes_checked < p_local_data->maximum_check_size;
            p_local_data->number_of_bytes_checked++)
    {
        /* If a sync has been found, check that it's part of a valid RSR... */
        if (m_rsr_search_buffer[search_index] == RSR_SYNC_CHARACTER)
        {
            tdr_offset = search_index + RSR_TDR_OFFSET_FROM_SYNC;

            /* If the TDR lies within the buffer then extract the TDR length. */
            if (tdr_offset < p_internal_data->bytes_read_into_buffer)
            {
                m_rsr_info.tdr_length = convert_lsb_msb_8bits_into_16bits(&m_rsr_search_buffer[tdr_offset]);

                crc_offset = tdr_offset + m_rsr_info.tdr_length + RSR_TDR_EXTRA_LENGTH;

                /*
                 * If the CRC lies within the buffer (plus a space for the ENDSYNC
                 * then calculate the CRC from the buffer and extract the expected value.
                 */
                if (crc_offset < (p_internal_data->bytes_read_into_buffer - 1u))
                {
                    crc_length = m_rsr_info.tdr_length + RSR_CRC_EXTRA_LENGTH;

                    //lint -e{921} Cast to uint32_t to avoid prototype coercion.
                    p_local_data->calculated_crc
                        = CRC_CCITTOnByteCalculate(&m_rsr_search_buffer[search_index],
                                                   (uint32_t)crc_length,
                                                   0x0000u);

                    p_local_data->extracted_crc
                        = convert_msb_lsb_8bits_into_16bits(&m_rsr_search_buffer[crc_offset]);

                    /*
                     * If the checksum matches and there's an ENDSYNC then this
                     * is a valid RSR so extract all of the information.
                     */
                    if ((p_local_data->calculated_crc == p_local_data->extracted_crc)
                            && (m_rsr_search_buffer[crc_offset + 2u] == RSR_ENDSYNC_CHARACTER))
                    {
                        m_rsr_info.crc            = p_local_data->calculated_crc;
                        m_rsr_info.record_id      = convert_lsb_msb_8bits_into_16bits(&m_rsr_search_buffer[search_index + 1u]);
                        m_rsr_info.p_start_of_rsr = &m_rsr_search_buffer[search_index];
                        m_rsr_info.p_start_of_tdr = &m_rsr_search_buffer[search_index + 5u];

                        if (p_internal_data->search_direction == RSSEARCH_BACKWARDS)
                        {
                            p_local_data->last_searched_index = search_index;
                        }
                        else
                        {
                            p_local_data->last_searched_index = crc_offset + 2u;
                        }

                        b_rsr_is_valid = TRUE;
                        break;
                    }
                }
            }
        }

        if (p_internal_data->search_direction == RSSEARCH_BACKWARDS)
        {
            search_index--;
        }
        else
        {
            search_index++;
        }
    }

    return b_rsr_is_valid;
}


// ----------------------------------------------------------------------------
/**
 * check_for_record_and_instance checks to see whether the record ID and
 * instance of the record ID match the requirements.
 *
 * @note
 * This function modifies the value of the instance counter which is passed
 * in using a pointer.  This was (probably) preferable to having a static
 * counter for the instance within the function which needed to be reset
 * the first time through etc.
 *
 * @param   p_internal_data     Pointer to internal data structure to use.
 * @param   p_instance_counter  Pointer to instance counter.
 * @retval  bool_t              TRUE if valid RSR found, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t check_for_record_and_instance
                    (const rssearch_internal_check_t * const p_internal_data,
                     uint32_t * const p_instance_counter)
{
    bool_t  b_rsr_match = FALSE;
    bool_t  b_instance_check_required = FALSE;

    /* Do we need to match the record ID? */
    if (p_internal_data->b_match_record_id)
    {
        /* If the record ID's match, only then do we check the instance. */
        if (m_rsr_info.record_id == p_internal_data->required_record_id)
        {
            b_instance_check_required = TRUE;
        }
    }
    /* If record ID match not required just check for the instance to match. */
    else
    {
        b_instance_check_required = TRUE;
    }

    if (b_instance_check_required)
    {
        if (*p_instance_counter == p_internal_data->required_record_instance)
        {
            b_rsr_match = TRUE;
        }
        /*
         * Increment the instance counter if it doesn't match.
         * Note the use of parenthesis - incrementing the CONTENTS.
         */
        else
        {
            (*p_instance_counter)++;
        }
    }

    return b_rsr_match;
}


// ----------------------------------------------------------------------------
/**
 * convert_msb_lsb_8bits_into_16bits converts two successive 8 bit words in a
 * buffer (arranged as MSB, LSB) into a single 16 bit word.
 *
 * @note
 * Make sure that the pointer actually pointer to something sensible before
 * calling this function.  There is no way to check this.
 *
 * @param   p_buffer        Pointer to the buffer containing the 8 bit words.
 * @retval  uint16_t        16 bit result.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{661} -e{662} Access \ creation of out-of-bounds pointer.  All ok.
static uint16_t convert_msb_lsb_8bits_into_16bits
                                (const uint8_t * const p_buffer)
{
    uint16_t    result;

    /* Get the MSB into the bottom 8 bits of result. */
    result = p_buffer[0u];

    /*
     * Shift up into the correct position.  Note the cast - this avoids
     * a Lint warning about loss of precision (24 bits to 16 bits) and is
     * safe because we only want the 8 LSB's which are now shifted into
     * the 8 MSB's.
     */
    //lint -e{921} Cast to uint16_t.
    result = (uint16_t)(result << 8u);

    /* Add the LSB - we now have a 16 bit result. */
    result += ((uint16_t)p_buffer[1u] & 0x00FFu);

    return result;
}


// ----------------------------------------------------------------------------
/**
 * convert_lsb_msb_8bits_into_16bits converts two successive 8 bit words in a
 * buffer (arranged as LSB, MSB) into a single 16 bit word.
 *
 * @note
 * Make sure that the pointer actually pointer to something sensible before
 * calling this function.  There is no way to check this.
 *
 * @param   p_buffer        Pointer to the buffer containing the 8 bit words.
 * @retval  uint16_t        16 bit result.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{661} -e{662} Access \ creation of out-of-bounds pointer.  All ok.
static uint16_t convert_lsb_msb_8bits_into_16bits
                                (const uint8_t * const p_buffer)
{
    uint16_t    result;

    /* Get the MSB into the bottom 8 bits of result. */
    result = p_buffer[1u];

    /*
     * Shift up into the correct position.  Note the cast - this avoids
     * a Lint warning about loss of precision (24 bits to 16 bits) and is
     * safe because we only want the 8 LSB's which are now shifted into
     * the 8 MSB's.
     */
    //lint -e{921} Cast to uint16_t.
    result = (uint16_t)(result << 8u);

    /* Add the LSB - we now have a 16 bit result. */
    result += ((uint16_t)p_buffer[0u] & 0x00FFu);

    return result;
}


// ----------------------------------------------------------------------------
/**
 * calc_next_search_address calculates the next address to start searching
 * for a new RSR from, based on what we read last time and what we found.
 *
 * @note
 * This function uses the p_next_search_start_address to update the calling
 * function with the next address to start searching from, if further
 * searching is required.
 *
 * @param   p_search_data       Pointer to structure containing search data.
 * @param   p_read_address      Pointer to array containing read addresses.
 * @param   p_bytes_to_read     Pointer to array containing bytes to read.
 * @param   number_of_reads     Number of reads which were carried out.
 * @param   last_valid_search_index     Index into read array where end of valid RSR found.
 * @param   p_next_search_start_address Pointer to location to update with next address.
 * @retval  bool_t              TRUE if finished searching, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t calc_next_search_address
                            (const rssearch_search_data_t * const p_search_data,
                             const uint32_t * const p_read_address,
                             const uint32_t * const p_bytes_to_read,
                             const uint8_t number_of_reads,
                             const uint16_t last_valid_search_index,
                             uint32_t * const p_next_search_start_address)
{
    bool_t      b_finished_searching = FALSE;
    uint32_t    next_search_start_address;

    if (p_search_data->search_direction == RSSEARCH_FORWARDS)
    {
        next_search_start_address
                = calc_next_search_address_fwd(p_read_address,
                                               p_bytes_to_read,
                                               number_of_reads,
                                               last_valid_search_index);

        /*
         * Have we fallen off the end of the partition by
         * adjusting the start address?  If so, we're done.
         */
        if (next_search_start_address >= p_search_data->partition_logical_end_address)
        {
            b_finished_searching = TRUE;
        }
    }
    else
    {
        next_search_start_address
                = calc_next_search_address_bwd(p_read_address,
                                               p_bytes_to_read,
                                               number_of_reads,
                                               last_valid_search_index);

        /*
         * Have we fallen off the start of the partition by
         * adjusting the start address?  If so, we're done.
         */
        if (next_search_start_address <=
                (p_search_data->partition_logical_start_address
                        + PAGE_HEADER_LENGTH_BYTES))
        {
            b_finished_searching = TRUE;
        }
    }

    if (!b_finished_searching)
    {
        *p_next_search_start_address = next_search_start_address;
    }

    return b_finished_searching;
}


// ----------------------------------------------------------------------------
/**
 * calc_next_search_address_fwd calculates the next address to start searching
 * for a new RSR from, based on what we read last time and what we found.
 *
 * This function deals with a forward search i.e. the next address will be
 * higher than the current address.
 *
 * @param   p_read_address      Pointer to array containing read addresses.
 * @param   p_bytes_to_read     Pointer to array containing bytes to read.
 * @param   number_of_reads     Number of reads which were carried out.
 * @param   last_valid_search_index     Index into read array where end of valid RSR found.
 * @retval  uint32_t            Next search address.
 *
 */
// ----------------------------------------------------------------------------
static uint32_t calc_next_search_address_fwd
                            (const uint32_t * const p_read_address,
                             const uint32_t * const p_bytes_to_read,
                             const uint8_t number_of_reads,
                             const uint16_t last_valid_search_index)
{
    uint32_t    next_search_address;

    /* If we only did one read to fill the buffer... */
    if (number_of_reads == 1u)
    {
        /*
         * Index of zero means that no valid RSR was found within the buffer.
         * In this case we set the start address to the next block in flash
         */
        if (last_valid_search_index == 0u)
        {
            next_search_address = p_read_address[0u] + p_bytes_to_read[0u];
        }
        /*
         * Otherwise we set the start address to whichever location
         * yielded the last valid search (generally the ENDSYNC character).
         */
        else
        {
            next_search_address = p_read_address[0u] + last_valid_search_index;
        }
    }
    /* If we needed two reads to fill the buffer... */
    else
    {
        /*
         * Index of zero means that no valid RSR was found within the buffer.
         * In this case we set the start address to the next block in flash.
         * As this was a split read, use the second read address \ bytes.
         */
        if (last_valid_search_index == 0u)
        {
            next_search_address = p_read_address[1u] + p_bytes_to_read[1u];
        }
        else
        {
            /*
             * If the last valid search location was within the
             * first read which was carried out, set the start
             * address using the first read address.
             */
            if (last_valid_search_index < p_bytes_to_read[0u])
            {
                next_search_address = p_read_address[0u] + last_valid_search_index;
            }
            /*
             * If the last valid search location was within the
             * second read which was carried out, set the start
             * address using the second read address.
             */
            else
            {
                next_search_address = p_read_address[1u] + (last_valid_search_index - p_bytes_to_read[0u]);
            }
        }
    }

    return next_search_address;
}


// ----------------------------------------------------------------------------
/**
 * calc_next_search_address_bwd calculates the next address to start searching
 * for a new RSR from, based on what we read last time and what we found.
 *
 * This function deals with a backwards search i.e. the next address will be
 * lower than the current address.
 *
 * @note
 * The return value from this function, the next search start address, will
 * be adjusted so that we don't actually read this address again elsewhere,
 * so we don't need to do the adjustment here.
 *
 * @param   p_read_address      Pointer to array containing read addresses.
 * @param   p_bytes_to_read     Pointer to array containing bytes to read.
 * @param   number_of_reads     Number of reads which were carried out.
 * @param   last_valid_search_index     Index into read array where end of valid RSR found.
 * @retval  uint32_t            Next search address.
 *
 */
// ----------------------------------------------------------------------------
static uint32_t calc_next_search_address_bwd
                            (const uint32_t * const p_read_address,
                             const uint32_t * const p_bytes_to_read,
                             const uint8_t number_of_reads,
                             const uint16_t last_valid_search_index)
{
    uint32_t    next_search_address;
    uint32_t    total_bytes_read;

    /* If we only did one read to fill the buffer... */
    if (number_of_reads == 1u)
    {
        /*
         * Index of maximum means that no valid RSR was found within the buffer.
         * In this case we set the start address to the last read address.
         */
        if (last_valid_search_index == p_bytes_to_read[0u])
        {
            next_search_address = p_read_address[0u];
        }
        /*
         * Otherwise we set the start address to the location before
         * whichever location yielded the last valid search.
         */
        else
        {
            next_search_address = p_read_address[0u] + last_valid_search_index;
        }
    }
    /* If we needed two reads to fill the buffer... */
    else
    {
        total_bytes_read = p_bytes_to_read[0u] + p_bytes_to_read[1u];

        /*
         * Index of maximum means that no valid RSR was found within the buffer.
         * In this case we set the start address to the last read address.
         */
        if (last_valid_search_index == total_bytes_read)
        {
            next_search_address = p_read_address[0u];
        }
        /*
         * Otherwise we set the start address to whichever location
         * yielded the last valid search.
         */
        else
        {
            /*
             * If the last valid search location was within the
             * first read which was carried out, set the start
             * address using the first read address.
             */
            if (last_valid_search_index < p_bytes_to_read[0u])
            {
                next_search_address = p_read_address[0u] + last_valid_search_index;
            }
            /*
             * If the last valid search location was within the
             * second read which was carried out, set the start
             * address using the second read address.
             */
            else
            {
                next_search_address = p_read_address[1u] + (last_valid_search_index - p_bytes_to_read[0u]);
            }
        }
    }

    return next_search_address;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

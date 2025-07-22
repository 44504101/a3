// ----------------------------------------------------------------------------
/**
 * @file    	rspartition.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Apr 2016
 * @brief		Functions which support the code in rsapi.c - partition related.
 * @details
 * Support functions for rsapi code, anything related to the partitions
 * (not the pages within the partitions, just the partitions).
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
#include "rsapi.h"
#include "rspages.h"
#include "rspartition.h"
#include "rspartition_prv.h"
#include "rssearch.h"
#include "flash_hal.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static void partition_counters_clear
                                (rs_partition_info_t * const p_partition);

static void update_progress_counter(uint8_t * const p_counter,
                                    const uint8_t new_value);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/**
 * Array of rs_partition_info_t structures, one for each partition.
 *
 * This is initialised with the values set up in rsappconfig.h.
 */
//lint -e{956} Doesn't need to be volatile here. Is only read from outside.
static rs_partition_info_t  m_rs_partition_info[RS_CFG_MAX_NUMBER_OF_PARTITIONS]
                                                = RS_CFG_PARTITION_SETTINGS;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * rspartition_addresses_calculate calculates the start and end addresses for
 * each partition, based on the number of pages and the size of each block on
 * the device.
 *
 * We assume that the logical addresses are contiguous, and the
 * partitions appear in the logical address range in the same order as they
 * appear in the partition info array.
 *
 * This function uses the flash_hal_block_size_bytes_get() function, BEFORE
 * the flash HAL is initialised.  This is the only function in the flash HAL
 * which can be called before initialising - we have to do this to set up all
 * the logical addresses which the flash HAL then uses to calculate the mapping
 * between logical and physical.
 *
 * @note
 * m_rs_partition_info is updated by this function.
 *
 */
// ----------------------------------------------------------------------------
void rspartition_addresses_calculate(void)
{
    const uint32_t      page_size_in_bytes = RS_CFG_PAGE_SIZE_KB * 1024u;
    uint32_t            block_size_in_bytes;
    uint32_t            number_of_pages;
    storage_devices_t   device_to_use;
    uint16_t            partition;
    uint32_t            previous_partition_end_address = 0u;
    uint32_t            bytes_in_partition;
    uint32_t            pages_per_block;
    uint32_t            padding_bytes = 0u;

    for (partition = 0u; partition < RS_CFG_MAX_NUMBER_OF_PARTITIONS;
            partition++)
    {
        device_to_use       = m_rs_partition_info[partition].device_to_use;
        number_of_pages     = m_rs_partition_info[partition].number_of_pages;
        block_size_in_bytes = flash_hal_block_size_bytes_get(device_to_use);

        /* Are blocks bigger than pages? */
        if (block_size_in_bytes > page_size_in_bytes)
        {
            /* If the block size is a multiple of the page size
             * (which it should normally be, everything should be 2^n)
             * then just check for full blocks.
             */
            if ( (block_size_in_bytes % page_size_in_bytes) == 0u)
            {
                pages_per_block = block_size_in_bytes / page_size_in_bytes;

                /* If the required number of pages doesn't result in perfectly
                 * full blocks then we need to adjust the number of pages by
                 * adding how ever many pages are needed to fill the block.
                 */
                if ( (number_of_pages % pages_per_block) != 0u)
                {
                    number_of_pages +=
                        pages_per_block - (number_of_pages % pages_per_block);
                }

                bytes_in_partition = number_of_pages * page_size_in_bytes;
            }
            /* If the block size isn't a multiple of the page size,
             * then we need to pad to give us full blocks.
             */
            else
            {
                bytes_in_partition = number_of_pages * page_size_in_bytes;

                padding_bytes = block_size_in_bytes
                                    - (bytes_in_partition % block_size_in_bytes);

                /* Add an extra page if there's enough space. */
                if (padding_bytes > page_size_in_bytes)
                {
                    number_of_pages++;
                    bytes_in_partition += page_size_in_bytes;
                    padding_bytes -= page_size_in_bytes;
                }
            }
        }
        /* If here then pages are bigger than blocks. */
        else
        {
            bytes_in_partition = number_of_pages * page_size_in_bytes;

            /* Pad to fill up the rest of a block if it's not a multiple.
             * Note that if it is, we always full a whole number of blocks,
             * so there's no need to add extra pages as in the above code. */
            if ( (page_size_in_bytes % block_size_in_bytes) != 0u)
            {
                padding_bytes = block_size_in_bytes
                                    - (bytes_in_partition % block_size_in_bytes);
            }
        }

        bytes_in_partition += padding_bytes;

        /* Update number of pages in case it's been modified. */
        m_rs_partition_info[partition].number_of_pages = number_of_pages;

        /* Next partition starts where the last one finished. */
        m_rs_partition_info[partition].start_address
            = previous_partition_end_address;

        m_rs_partition_info[partition].end_address
            = m_rs_partition_info[partition].start_address
                + bytes_in_partition - 1u;

        /* Setup end address for next time round. */
        previous_partition_end_address
            = m_rs_partition_info[partition].end_address + 1u;
    }
}


// ----------------------------------------------------------------------------
/**
 * rspartition_bisection_search_do uses a bisection search to find the next
 * page which can be written to.
 *
 * @note
 * This function needs various members of m_rs_partition_info to have been
 * initialised before use - id, number_of_pages, start_address and end_address.
 *
 * Once the function has finished, the rest of m_rs_partition_info (for the
 * partition which is being checked) will be updated.
 *
 * The bisection search can setup the following error codes:
 *      RS_ERR_NO_ERROR                     (returns TRUE)
 *      RS_ERR_PARTITION_IS_FULL            (returns TRUE)
 *      RS_ERR_PARTITION_NEEDS_FORMAT       (returns FALSE)
 *
 * @warning
 * This code does NOT check for an invalid partition_index, so ensure that
 * the calling function does so.
 *
 * @param   partition_index     Partition number to check (0,1,2 etc).
 * @retval  bool_t              TRUE if search was OK, partition is ready.
 *
 */
// ----------------------------------------------------------------------------
bool_t rspartition_bisection_search_do(const uint8_t partition_index)
{
    const uint32_t          page_length_in_bytes = (RS_CFG_PAGE_SIZE_KB * 1024u);
    rs_partition_info_t*    p_partition;
    uint32_t                lower_page_to_check = 0u;
    uint32_t                upper_page_to_check;
    uint32_t                page_to_check;
    uint32_t                previous_page_to_check = 0xFFFFFFFFu;
    bool_t                  b_done = FALSE;
    uint32_t                page_start_address;
    uint32_t                next_page_start_address;
    bool_t                  b_page_is_blank;
    rs_error_t              rs_error = RS_ERR_NO_ERROR;
    bool_t                  b_partition_ready_to_use = TRUE;
    uint32_t                next_free_address;
    bool_t                  b_next_free_address_search_required = FALSE;

    /*
     * Use a pointer just to make the code tidier - having to use
     * m_rs_partition_info[partition_index] each time is messy!
     */
    p_partition = &m_rs_partition_info[partition_index];

    partition_counters_clear(p_partition);
    p_partition->next_available_address     = 0xFFFFFFFFu;

    upper_page_to_check = p_partition->number_of_pages - 1u;

    while (!b_done)
    {
        page_to_check = (lower_page_to_check + upper_page_to_check) / 2u;

        /*
         * Once the algorithm checks the same page twice in succession,
         * the bisection part of the search is complete.
         * This page contains data somewhere, need to look for it.
         */
        if (page_to_check == previous_page_to_check)
        {
            b_next_free_address_search_required = TRUE;
            b_done = TRUE;
        }
        else
        {
            page_start_address  = p_partition->start_address
                                   + (RS_CFG_PAGE_SIZE_KB * 1024u * page_to_check);

            /* Check for the page being blank, including the header. */
            b_page_is_blank = flash_hal_device_blank_check(page_start_address,
                                                           page_length_in_bytes);

            if (b_page_is_blank)
            {
                upper_page_to_check = page_to_check - 1u;

                /* Special case where the memory is unformatted, so all blank. */
                if (page_to_check == 0u)
                {
                    rs_error = RS_ERR_PARTITION_NEEDS_FORMAT;
                    p_partition->blank_headers_and_pages = p_partition->number_of_pages;
                    b_partition_ready_to_use = FALSE;
                    b_done = TRUE;
                }
            }
            else
            {
                lower_page_to_check = page_to_check + 1u;
            }

            previous_page_to_check = page_to_check;
        }
    }

    /*
     * Search for the next free address in the page...
     * Note that if the page is completely full then the
     * search function returns the address immediately after the page.
     */
    if (b_next_free_address_search_required)
    {
        /*
         * Search through the data portion of the page,
         * looking for the next free address (hence we adjust the arguments
         * passed into the find function to reflect the start address \ size of
         * the data portion of the page).
         * This function returns the address of the first location
         * in the next page if the page is full.
         */
        //lint -e{644} Page start address will have been initialised before use.
        next_free_address
            = rssearch_find_next_free_address((page_start_address + PAGE_HEADER_LENGTH_BYTES),
                                              (page_length_in_bytes - PAGE_HEADER_LENGTH_BYTES) );

        next_page_start_address = page_start_address + page_length_in_bytes;

        /* Have we fallen off the end of the partition?  If so, partition is full. */
        if (next_free_address > p_partition->end_address)
        {
            rs_error = RS_ERR_PARTITION_IS_FULL;
            p_partition->free_pages = 0u;
            p_partition->full_pages = p_partition->number_of_pages;
        }
        /*
         * Have we fallen off the end of the page?  If so, set the address to
         * the first location after the page header for the next page and
         * adjust the page counters accordingly.
         */
        else if (next_free_address == next_page_start_address)
        {
            next_free_address += PAGE_HEADER_LENGTH_BYTES;
            p_partition->next_available_address = next_free_address;
            p_partition->full_pages = page_to_check + 1u;
            p_partition->free_pages = (p_partition->number_of_pages - page_to_check) - 1u;
        }
        /* Otherwise we have a valid next address within the current page. */
        else
        {
            p_partition->next_available_address = next_free_address;
            p_partition->full_pages = page_to_check;
            p_partition->free_pages = p_partition->number_of_pages - page_to_check;
        }
    }

    /* Update the error status in the partition now we've finished. */
    p_partition->partition_error_status = rs_error;

    return b_partition_ready_to_use;
}


// ----------------------------------------------------------------------------
/**
 * rspartition_format_partition formats a partition.  This erases all the data
 * in the partition and then writes the page headers.
 *
 * The progress counter is updated during the formatting, as follows:
 *
 *      0 - initial value
 *      1 - starting to erase the device
 *     29 - finished the erase
 *     30 - starting to blank check the device
 *     49 - finished the blank check
 *     50 - starting to write page headers
 *    100 - done
 *
 * @note
 * This function doesn't check for the recording system having been initialised,
 * but as this function should only be called from the recording system API
 * (which does check, then this is OK.
 *
 * @note
 * In contravention of the recording system specification, we only write the
 * first page header at startup, not each page header.
 *
 * @note
 * The status field in the page header is set to 'closed' (0x6996) on startup
 * because we do not re-write any of the memory (to avoid issues with data
 * retention at temperature).
 *
 * @param   partition_index     Partition index relating to partition to format.
 * @param   p_progress_counter  Pointer to progress counter (0-100)
 * @retval  rs_error_t          Enumerated value for error code.
 *
 */
// ----------------------------------------------------------------------------
rs_error_t rspartition_format_partition(const uint8_t partition_index,
                                        uint8_t * const p_progress_counter)
{
    rs_error_t                  format_status = RS_ERR_UNIT_TEST_DEFAULT_VAL;
    uint32_t                    number_of_bytes;
    const rs_partition_info_t*  p_partition;
    flash_hal_error_t           flash_error;
    bool_t                      b_partition_is_blank;
    rs_header_data_t            header_data;
    rs_header_status_t          write_status;

    update_progress_counter(p_progress_counter, 0u);

    if (partition_index >= RS_CFG_MAX_NUMBER_OF_PARTITIONS)
    {
        format_status = RS_ERR_BAD_PARTITION_INDEX;
    }
    else
    {

        /* Use a pointer just to make the code tidier - having to use
         * m_rs_partition_info[partition_index] each time is messy!
         */
        p_partition = &m_rs_partition_info[partition_index];

        number_of_bytes = RS_CFG_PAGE_SIZE_KB
                            * 1024u
                            * p_partition->number_of_pages;

        /* Starting the erase so set the progress counter to 1. */
        update_progress_counter(p_progress_counter, 1u);

        flash_error = flash_hal_device_erase(p_partition->start_address,
                                             number_of_bytes);

        /* Finished the erase so set the progress counter to 29. */
        update_progress_counter(p_progress_counter, 29u);

        if (flash_error == FLASH_HAL_NO_ERROR)
        {
            /* Starting the blank check so set the progress counter to 30. */
            update_progress_counter(p_progress_counter, 30u);

            b_partition_is_blank = flash_hal_device_blank_check
                                        (p_partition->start_address,
                                         number_of_bytes);

            /* Finished the blank check so set the progress counter to 49. */
            update_progress_counter(p_progress_counter, 49u);

            if (b_partition_is_blank)
            {
                header_data.partition_index = partition_index;
                header_data.partition_id    = p_partition->id;
                header_data.partition_logical_start_addr
                                            = p_partition->start_address;
                header_data.partition_logical_end_addr
                                            = p_partition->end_address;
                header_data.format_code     = 0x8Du;

                /*
                 * Set status to closed to avoid re-writing header
                 * once the page has been used.
                 */
                header_data.status          = 0x6996u;

                header_data.error_code      = 0xFFu;
                header_data.error_address   = 0xFFFFu;
                header_data.page_number     = 0u;

                /* Starting the header write so set the progress counter to 50. */
                update_progress_counter(p_progress_counter, 50u);

                write_status = rspages_page_header_write(&header_data);

                if (write_status == RS_HDR_HEADER_WRITE_OK)
                {
                    format_status = RS_ERR_NO_ERROR;
                }
                else
                {
                    format_status = RS_ERR_HEADER_WRITE_FAILURE;
                }
            }
            else
            {
                format_status = RS_ERR_PARTITION_ERASE_FAILURE;
            }
        }
        else
        {
            format_status = RS_ERR_PARTITION_ERASE_FAILURE;
        }
    }

    if (format_status == RS_ERR_NO_ERROR)
    {
        /* Finished successfully so set the progress counter to 100. */
        update_progress_counter(p_progress_counter, 100u);
    }

    return format_status;
}


// ----------------------------------------------------------------------------
/**
 * rspartition_check_partition_id checks to make sure that a particular
 * partition ID relates  to an actual partition index.
 *
 * @param   partition_id    Partition ID to check.
 * @retval  uint16_t        Partition index relating to partition ID.
 *
 */
// ----------------------------------------------------------------------------
uint16_t rspartition_check_partition_id(const uint8_t partition_id)
{
    uint16_t    partition_index;

    for (partition_index = 0u;
            partition_index < RS_CFG_MAX_NUMBER_OF_PARTITIONS;
            partition_index++)
    {
        if (m_rs_partition_info[partition_index].id == partition_id)
        {
            break;
        }
    }

    if (partition_index == RS_CFG_MAX_NUMBER_OF_PARTITIONS)
    {
        partition_index = RSPARTITION_INDEX_BAD_ID_VALUE;
    }

    return partition_index;
}


// ----------------------------------------------------------------------------
/**
 * rspartition_flag_page_as_full adjusts the number of free \ full
 * pages for a particular partition.  Once there are no free pages, the
 * partition error status is changed to RS_ERR_PARTITION_IS_FULL.
 *
 * @param   partition_index     Partition index of partition with full page.
 *
 */
// ----------------------------------------------------------------------------
void rspartition_flag_page_as_full(const uint8_t partition_index)
{
    if ((partition_index < RS_CFG_MAX_NUMBER_OF_PARTITIONS)
            && (m_rs_partition_info[partition_index].free_pages != 0u))
    {
        m_rs_partition_info[partition_index].free_pages--;
        m_rs_partition_info[partition_index].full_pages++;

        if (m_rs_partition_info[partition_index].free_pages == 0u)
        {
            m_rs_partition_info[partition_index].partition_error_status
                = RS_ERR_PARTITION_IS_FULL;
        }
    }
}


// ----------------------------------------------------------------------------
/**
 * rspartition_next_address_set sets up the next available address which can
 * be written in a partition.
 *
 * @note
 * This code only checks for the address being within the partition limits,
 * as the partition module doesn't know anything about page header addresses
 * etc.
 *
 * @param   partition_index     Partition index of partition.
 * @param   next_free_address   Next free address in the partition.
 *
 */
// ----------------------------------------------------------------------------
bool_t rspartition_next_address_set(const uint8_t partition_index,
                                    const uint32_t next_free_address)
{
    bool_t  b_address_updated_ok = FALSE;

    if ( (partition_index < RS_CFG_MAX_NUMBER_OF_PARTITIONS)
            && (next_free_address >=
                    m_rs_partition_info[partition_index].start_address)
            && (next_free_address <=
                    m_rs_partition_info[partition_index].end_address))
    {
        m_rs_partition_info[partition_index].next_available_address
                                                        = next_free_address;
        b_address_updated_ok = TRUE;
    }

    return b_address_updated_ok;
}


// ----------------------------------------------------------------------------
/**
 * rspartition_partition_ptr_get returns a const pointer to the required index
 * in the partition info structure.
 *
 * @param   partition_index         Index into partition info structure.
 * @retval  rs_partition_info_t*    Pointer to structure.
 *
 */
// ----------------------------------------------------------------------------
const rs_partition_info_t* rspartition_partition_ptr_get
                                            (const uint8_t partition_index)
{
    const rs_partition_info_t* p_partition = NULL;

    if (partition_index < RS_CFG_MAX_NUMBER_OF_PARTITIONS)
    {
        p_partition = &m_rs_partition_info[partition_index];
    }

    return p_partition;
}


#ifdef UNIT_TEST_BUILD
// ----------------------------------------------------------------------------
/**
 * rspartition_unit_test_ptrs_get returns a pointer to the unit test pointers
 * structure, for test purposes.
 * We use an ifdef to ensure that this pointer can't be accessed under
 * normal operation.
 *
 * @retval rspartition_unit_test_ptrs_t*    Pointer to test structure.
 *
 */
// ----------------------------------------------------------------------------
rspartition_unit_test_ptrs_t* rspartition_unit_test_ptrs_get(void)
{
    //lint -e{956} Doesn't need to be volatile here. Pointers never change.
    static rspartition_unit_test_ptrs_t p_unit_test_structure =
    {
        &m_rs_partition_info[0u],
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
 * partition_counters_clear clears all page counters relating to a partition.
 *
 * @param   p_partition     Pointer to partition.
 *
 */
// ----------------------------------------------------------------------------
static void partition_counters_clear(rs_partition_info_t * const p_partition)
{
    p_partition->free_pages                 = 0u;
    p_partition->full_pages                 = 0u;
    p_partition->unusable_pages             = 0u;
    p_partition->error_pages                = 0u;
    p_partition->blank_headers_and_pages    = 0u;
}


// ----------------------------------------------------------------------------
/**
 * update_progress_counter updates the progress counter variable.
 *
 * @param   p_counter       Pointer to the progress counter.
 * @param   new_value       Value to update the progress counter with.
 *
 */
// ----------------------------------------------------------------------------
static void update_progress_counter(uint8_t * const p_counter,
                                    const uint8_t new_value)
{
    if (p_counter != NULL)
    {
        *p_counter = new_value;
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

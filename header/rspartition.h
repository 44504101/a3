// ----------------------------------------------------------------------------
/**
 * @file    	rspartition.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Apr 2016
 * @brief		Header file for rspartition.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SOURCE_RSPARTITION_H_
#define SOURCE_RSPARTITION_H_

#include "rsapi.h"
#include "rsappconfig.h"
/// Return value from rspartition_check_partition_id() for an invalid ID.
#define RSPARTITION_INDEX_BAD_ID_VALUE  0xFFFFu

/**
 * Structure to hold parameters of interest for each recording partition.
 */
typedef struct
{
    /* These values are defined in rssappconfig.h for each partition. */
    uint8_t             id;                 ///< Partition ID.
    uint32_t            number_of_pages;    ///< Number of pages in the partition.
    storage_devices_t   device_to_use;      ///< ID of device to store the partition in.

    /* These values are derived by the recording system and updated as we go along. */
    uint32_t    start_address;              ///< First logical address in partition.
    uint32_t    end_address;                ///< Last logical address in partition.
    rs_error_t  partition_error_status;     ///< Error status of the partition.
    uint32_t    next_available_address;     ///< Next available logical address in partition.

    uint32_t    free_pages;                 ///< Number of pages which can have data written into them.
    uint32_t    full_pages;                 ///< Number of pages which are full and cannot be written to.
    uint32_t    unusable_pages;             ///< Number of pages with corrupted header.
    uint32_t    error_pages;                ///< Number of pages with error flagged.
    uint32_t    blank_headers_and_pages;    ///< Number of blank header \ page combinations.
} rs_partition_info_t;


void        rspartition_addresses_calculate(void);

bool_t      rspartition_bisection_search_do(const uint8_t partition_index);

rs_error_t  rspartition_format_partition(const uint8_t partition_index,
                                         uint8_t * const p_progress_counter);

uint16_t    rspartition_check_partition_id(const uint8_t partition_id);

void        rspartition_flag_page_as_full(const uint8_t partition_index);

bool_t      rspartition_next_address_set(const uint8_t partition_index,
                                         const uint32_t next_free_address);

const rs_partition_info_t* rspartition_partition_ptr_get(const uint8_t partition_index);


#endif /* SOURCE_RSPARTITION_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

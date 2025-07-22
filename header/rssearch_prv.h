// ----------------------------------------------------------------------------
/**
 * @file        rssearch_prv.h
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        6 Apr 2016
 * @brief       Header file for rssearch_prv.c
 * @note        Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SOURCE_RSSEARCH_PRV_H_
#define SOURCE_RSSEARCH_PRV_H_

/**
 * Internal structure used in rssearch.c for memory related operations.
 */
typedef struct
{
    rs_search_direction_t   search_direction;                   ///< Forwards or backwards.
    uint32_t                partition_logical_start_address;    ///< Logical start address.
    uint32_t                partition_logical_end_address;      ///< Logical end address.
    uint32_t                search_start_address;               ///< Search start address.
} rssearch_internal_memory_t;

/**
 * Internal structure used in rssearch.c for search related operations.
 */
typedef struct
{
    rs_search_direction_t   search_direction;           ///< Forwards or backwards.
    uint16_t                search_start_index;         ///< Index at which to start searching.
    uint16_t                bytes_read_into_buffer;     ///< Number of bytes in the search buffer.
} rssearch_internal_search_t;

/**
 * Internal structure used in rssearch.c for record checking related operations.
 */
typedef struct
{
    uint32_t                required_record_instance;   ///< Record instance to find.
    bool_t                  b_match_record_id;          ///< Flag to say whether to match record ID or not.
    uint16_t                required_record_id;         ///< Expected record ID if we're trying to match.
} rssearch_internal_check_t;

/**
 * Internal structure used in rssearch.c for local data relating to the RSR.
 */
typedef struct
{
    uint16_t    maximum_check_size;             ///< Maximum size of data buffer to check.
    uint16_t    number_of_bytes_checked;        ///< Number of bytes which have been checked.
    uint16_t    distance_to_end_of_buffer;      ///< Distance between current position and end of buffer.
    uint16_t    extracted_crc;                  ///< CRC extracted from RSR.
    uint16_t    calculated_crc;                 ///< CRC calculated on RSR.
    uint16_t    last_searched_index;            ///< Last index we searched for.
} rssearch_rsr_local_data_t;


#ifdef UNIT_TEST_BUILD

/**
 * Structure for recording system, for unit testing.
 * This allows the unit tests to check all results via a single pointer.
 */
typedef struct
{
    uint8_t*                p_rsr_search_buffer;
    bool_t*                 pb_rsr_is_valid;
    volatile bool_t*        pb_rssearch_timeout;
    rssearch_rsr_info_t*    p_rsr_info;

    // Function pointers to the various static functions to test directly.
    uint16_t    (*p_count_blanks_from_end)(const uint8_t * const p_area,
                                           const uint16_t size_of_area);

    uint8_t     (*p_partition_memory_read_setup)
                            (const rssearch_internal_memory_t * const p_memory_data,
                             uint32_t * const p_read_addresses,
                             uint32_t * const p_bytes_to_read);

    uint32_t    (*p_read_partition_data)
                            (const uint32_t * const p_read_address,
                             const uint32_t * const p_bytes_to_read,
                             const uint8_t number_of_reads);

    bool_t      (*p_search_for_valid_rsr_in_buffer)
                            (const rssearch_internal_search_t * const p_internal_data,
                             rssearch_rsr_local_data_t * const p_local_data);

    bool_t      (*p_check_for_record_and_instance)
                    (const rssearch_internal_check_t * const p_internal_data,
                     uint32_t * const p_instance_counter);

    uint16_t    (*p_convert_msb_lsb_8bits_into_16bits)
                                    (const uint8_t * const p_buffer);

} rssearch_unit_test_pointers_t;

rssearch_unit_test_pointers_t* rssearch_unit_test_ptr_get(void);

#endif /* UNIT_TEST_BUILD */


#endif /* SOURCE_RSSEARCH_PRV_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

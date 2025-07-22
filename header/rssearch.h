// ----------------------------------------------------------------------------
/**
 * @file        rssearch.h
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        6 Apr 2016
 * @brief       Header file for rssearch.c
 * @note        Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SOURCE_RSSEARCH_H_
#define SOURCE_RSSEARCH_H_

/**
 * Structure used by rssearch_find_valid_RSR_start() to specify the search
 * details.
 */
typedef struct
{
    rs_search_direction_t   search_direction;                   ///< Backwards or forwards.
    uint32_t                partition_logical_start_address;    ///< Logical start address.
    uint32_t                partition_logical_end_address;      ///< Logical end address.
    uint32_t                search_start_address;               ///< Search start address.
    uint32_t                required_record_instance;           ///< Record instance to find.
    bool_t                  b_match_record_id;                  ///< Flag to say whether to match record ID or not.
    uint16_t                required_record_id;                 ///< Expected record ID if we're trying to match.
} rssearch_search_data_t;

/**
 * Structure returned by rssearch_valid_rsr_pointer_get containing all
 * relevant details of the valid RSR which has been found.
 */
typedef struct
{
    uint8_t*    p_start_of_rsr;     ///< Pointer to the start of the RSR in the local buffer.
    uint8_t*    p_start_of_tdr;     ///< Pointer to the start of the TDR in the local buffer.
    uint16_t    record_id;          ///< The record ID which has been found.
    uint16_t    tdr_length;         ///< The length of the TDR which has been found.
    uint16_t    crc;                ///< The CRC of the RSR which has been found.
} rssearch_rsr_info_t;


uint32_t rssearch_find_next_free_address
                    (const uint32_t logical_start_address,
                     const uint32_t number_of_bytes_to_check);

bool_t   rssearch_find_valid_RSR_start
                    (const rssearch_search_data_t * const p_search_data);

const rssearch_rsr_info_t* rssearch_valid_rsr_pointer_get(void);

void     rssearch_timeout_callback(void* xTimer);

#endif /* SOURCE_RSSEARCH_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

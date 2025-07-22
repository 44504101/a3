// ----------------------------------------------------------------------------
/**
 * @file    	rspages.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		17 Feb 2016
 * @brief		Header file for rspages.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SOURCE_RSPAGES_H_
#define SOURCE_RSPAGES_H_

/// RSR SYNC character - this occurs at the start of each RSR.
#define RSR_SYNC_CHARACTER          0xE1u

/// RSR endSYNC character - this occurs at the end of each RSR.
#define RSR_ENDSYNC_CHARACTER       0x1Au

/// The page header is 16 bytes long.
#define PAGE_HEADER_LENGTH_BYTES    16u

/**
 * Enumerated types for all possible status messages relating to the
 * page headers in the recording system.
 */
typedef enum
{
    RS_HDR_INVALID_PARTITION_NUMBER,
    RS_HDR_INVALID_PAGE_NUMBER,
    RS_HDR_HEADER_IS_BLANK,
    RS_HDR_HEADER_CHECKSUM_FAIL,
    RS_HDR_HEADER_PARTITION_ID_FAIL,
    RS_HDR_HEADER_FORMAT_CODE_FAIL,
    RS_HDR_HEADER_ERROR_CODE_FAIL,
    RS_HDR_PAGE_IS_CLOSED,
    RS_HDR_PAGE_IS_OPEN,
    RS_HDR_PAGE_IS_UNDEFINED,
    RS_HDR_PAGE_IS_EMPTY,
    RS_HDR_FLASH_READ_ERROR,
    RS_HDR_HEADER_WRITE_ERROR,
    RS_HDR_HEADER_WRITE_OK
} rs_header_status_t;

/**
 * Structure to encapsulate information relating to the page header.
 */
typedef struct
{
    uint8_t             partition_index;                ///< Partition index.
    uint8_t             partition_id;                   ///< Partition ID.
    uint32_t            partition_logical_start_addr;   ///< Partition logical start address.
    uint32_t            partition_logical_end_addr;     ///< Partition logical end address.
    uint32_t            page_number;                    ///< Page number in partition.
    rs_header_status_t  header_status;                  ///< Page header status.

    uint8_t             format_code;                    ///< The format code.
    uint16_t            status;                         ///< The status word.
    uint8_t             error_code;                     ///< The error code.
    uint16_t            error_address;                  ///< The error address.
} rs_header_data_t;

/**
 * Enumerated types for all possible status messages relating to the
 * page data itself in the recording system.
 *
 * Fatal errors are those which the recording system cannot recover from.
 */
typedef enum
{
    RS_PG_INVALID_PARTITION_NUMBER,     ///< Invalid partition number during function (fatal error).
    RS_PG_INVALID_PAGE_NUMBER,          ///< Invalid page number during function (fatal error).
    RS_PG_FLASH_READ_ERROR,             ///< Error while reading page from flash (fatal error).
    RS_PG_HEADER_AND_PAGE_BLANK,        ///< Header and page are both completely blank.
    RS_PG_HEADER_PAGE_MISMATCH,         ///< Header and page don't match (fatal error).
    RS_PG_HEADER_ERROR,                 ///< Header error (fatal error).
    RS_PG_HEADER_OK_PAGE_HAS_ERRORS,    ///< Header OK but page has errors (fatal error).
    RS_PG_HEADER_OK_PAGE_IS_FULL,       ///< Header and page are OK, page is full.
    RS_PG_HEADER_OK_PAGE_HAS_SPACE,     ///< Header and page are OK, page has free space.
    RS_PG_HEADER_OK_PAGE_IS_EMPTY       ///< Header and page are OK, page is empty.
} rs_page_status_t;

/**
 * Structure for requesting page details using rspages_page_details_calculate().
 */
typedef struct
{
    /* Inputs */
    uint32_t    partition_logical_start_address;    ///< The partition logical start address.
    uint32_t    partition_logical_end_address;      ///< The partition logical end address.
    uint32_t    address_within_partition;           ///< The address within the partition.

    /* Outputs */
    uint32_t    distance_from_partition_start;  ///< Distance between address and partition start.
    uint32_t    page_number;                    ///< Page number which address falls in.
    uint32_t    maximum_number_of_pages;        ///< Total number of pages in the partition.
    uint32_t    lower_address_within_page;      ///< The lower address of the 'active' page.
    uint32_t    upper_address_within_page;      ///< The upper address of the 'active' page.
    uint32_t    distance_to_lower_address;      ///< Distance from address to lower address.
    uint32_t    distance_to_upper_address;      ///< Distance from address to upper address.
} rs_page_details_t;

/**
 * Structure for specifying the data to write using rspages_page_data_write().
 */
typedef struct
{
    uint8_t     partition_index;                ///< Partition index of partition to write into.
    uint8_t     partition_id;                   ///< Partition ID of partition to write into.
    uint32_t    partition_logical_start_addr;   ///< Logical start address of partition to write into.
    uint32_t    partition_logical_end_addr;     ///< Logical end address of partition to write into.
    uint32_t    next_free_addr;                 ///< Next free address to write to in partition.
    uint16_t    record_id;                      ///< Record ID to be written.
    uint8_t *   p_write_buffer;                 ///< Pointer to the write buffer.
    uint16_t    bytes_to_write;                 ///< Number of bytes to write.
    bool_t      b_read_back_write_command;      ///< Flag sets to read back what has been written.
} rs_page_write_t;

/**
 * Enumerated types for status relating to page writes.
 * Returned by rspages_page_data_write().
 */
typedef enum
{
    RS_PG_WRITE_INVALID_ADDRESSES,              ///< Write fails with invalid addresses.
    RS_PG_WRITE_OK,                             ///< Write is OK.
    RS_PG_WRITE_OK_PAGE_FULL,                   ///< Write is OK, page is now full.
    RS_PG_WRITE_ERROR                           ///< Write fails with some sort of error.
} rs_page_write_status_t;


rs_header_status_t rspages_page_header_check
                            (const uint32_t partition_logical_start_address,
                             const uint32_t partition_logical_end_address,
                             const uint32_t page_number_to_check,
                             const uint8_t  partition_id);

rs_header_status_t      rspages_page_header_write
                            (const rs_header_data_t * const p_header_data);

rs_page_status_t        rspages_page_data_check
                            (const rs_header_data_t * const p_header_data,
                             uint32_t * const p_next_free_address);

rs_page_write_status_t  rspages_page_data_write
                            (const rs_page_write_t * const p_write_data);

bool_t                  rspages_page_details_calculate
                            (rs_page_details_t * const p_page_details);

#endif /* SOURCE_RSPAGES_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

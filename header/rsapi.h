// ----------------------------------------------------------------------------
/**
 * @file    	rsapi.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		17 Feb 2016
 * @brief		Header file for rsapi.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SOURCE_RSAPI_H_
#define SOURCE_RSAPI_H_

#define RSAPI_BYTES_BEFORE_TDR  5u  ///< Space required in write buffer before TDR.
#define RSAPI_BYTES_AFTER_TDR   3u  ///< Space required in write buffer after TDR.

/**
 * Enumerated type for all possible recording system errors.
 *
 * Note that functions do not have to return all possible values from this list.
 * Generally, either RS_ERR_NO_ERROR or some of the errors will be fine.
 */
typedef enum
{
    RS_ERR_NO_ERROR                 = 0,
    RS_ERR_FLASH_READ_ERROR         = 1,
    RS_ERR_PARTITION_IS_FULL        = 2,
    RS_ERR_PARTITION_NEEDS_FORMAT   = 3,
    RS_ERR_NOT_INITIALISED_YET      = 4,
    RS_ERR_BAD_PARTITION_INDEX      = 5,
    RS_ERR_PARTITION_ERASE_FAILURE  = 6,
    RS_ERR_HEADER_WRITE_FAILURE     = 7,
    RS_ERR_BAD_READ_QUEUE           = 8,
    RS_ERR_BAD_PARTITION_ID         = 9,
    RS_ERR_BAD_WRITE_QUEUE          = 10,
    RS_ERR_READ_WRITE_TASK_RUNNING  = 11,
    RS_ERR_BAD_FORMAT_QUEUE         = 12,
    RS_ERR_UNIT_TEST_DEFAULT_VAL    = 1000
} rs_error_t;


/**
 * Enumerated type for possible status messages when reading, writing
 * or formatting.
 *
 * Note that RS_QUEUE_INCOMPATIBLE_ALIGNMENT is only used when trying to add
 * a write, any other failure when adding to the queue results in
 * RS_QUEUE_COULD_NOT_ADD_TO_QUEUE being used.
 * There is a specific error for alignment failure because it's important.
 *
 */
typedef enum
{
    RS_QUEUE_INCOMPATIBLE_ALIGNMENT,    ///< The number of bytes does not align correctly (write only).
    RS_QUEUE_COULD_NOT_ADD_TO_QUEUE,    ///< The request could not be added to the queue for some reason.
    RS_QUEUE_REQUEST_IN_QUEUE,          ///< The request was OK and has been placed in the appropriate queue.
    RS_QUEUE_REQUEST_IN_PROGRESS,       ///< The request has been moved from the queue and is in progress.
    RS_QUEUE_REQUEST_FAILED,            ///< The request failed to be carried out for some reason.
    RS_QUEUE_REQUEST_COMPLETE           ///< The request completed successfully.
} rs_queue_status_t;


/**
 * Structure for the recording system configuration.
 */
typedef struct
{
    /* These values are setup during initialisation. */
    uint16_t    spec_level;             ///< Specification revision.
    uint16_t    code_version;           ///< Software version.
    uint16_t    board_type;             ///< ID of memory board.
    uint16_t    number_of_partitions;   ///< Maximum number of partitions which are supported.
    uint16_t    page_size_kb;           ///< Page size in kilobytes.
    uint32_t    total_pages;            ///< Number of pages in entire memory space.

    /* These values are updated as we go along... */
    uint32_t    accessible_pages;       ///< Number of "accessible" pages.
    uint32_t    unusable_pages;         ///< Number of pages with corrupted header.
    uint32_t    error_pages;            ///< Number of pages with error flagged.
} rs_configuration_t;


/**
 * Enumerated list of possible search directions.
 */
typedef enum
{
    RSSEARCH_FORWARDS,                  ///< Search forwards through the recording system.
    RSSEARCH_BACKWARDS                  ///< Search backwards through the recording system.
} rs_search_direction_t;


/**
 * Enumerated list of possible queues.
 */
typedef enum
{
    RS_QUEUE_ID_READ    = 0,            ///< Read queue identifier.
    RS_QUEUE_ID_WRITE,                  ///< Write queue identifier.
    RS_QUEUE_ID_FORMAT,                 ///< Format queue identifier.
    RS_QUEUE_ID_COUNT                   ///< The overall queue counter identifier.
} rs_queue_identifiers_t;


/*!
 * Read request structure.
 *
 * This is used by the read request and the internal read queue.
 *
 * Note the queue specific variables - these are not needed when
 * making the read request itself, but are used by the read queue.
 * This saves having a slightly different structure for the queue itself,
 * which might be (more) confusing - the trade off is that the queue structure
 * is slightly bigger than it might otherwise be.
 *
 */
typedef struct
{
    /* Inputs */
    uint8_t                 partition_id;           ///< ID of partition to read from.
    rs_search_direction_t   search_direction;       ///< Enumerated type for search direction.
    uint32_t                record_instance;        ///< Record instance to find.
    bool_t                  b_match_record_id;      ///< Match record ID?
    uint16_t                record_id;              ///< Record ID to match, if flag set.

    /* Outputs */
    uint8_t *               p_read_buffer;          ///< Pointer to buffer to copy read data into.
    uint16_t *              p_read_length;          ///< Pointer to length variable to update.
    rs_queue_status_t *     p_read_status;          ///< Pointer to read status word.
    void*                   p_read_semaphore;       ///< Handle to read semaphore.

    /* Queue specific variables - don't set these up when making a request */
    uint8_t                 partition_index;        ///< Queue uses partition index, not ID.
} rs_read_request_t;


/*!
 * Write request structure.
 *
 * This is used by the write request and the internal write queue.
 *
 * @note
 * The write buffer must be large enough to allow for the RSR to be
 * added to the buffer around the TDR by the recording system
 * (RSAPI_BYTES_BEFORE_TDR + RSAPI_BYTES_AFTER_TDR), and the TDR must
 * start at index RSAPI_BYTES_BEFORE_TDR in the write buffer.
 *
 * Note the queue specific variables - these are not needed when
 * making the write request itself, but are used by the write queue.
 * This saves having a slightly different structure for the queue itself,
 * which might be (more) confusing - the trade off is that the queue structure
 * is slightly bigger than it might otherwise be.
 *
 */
typedef struct
{
    /* Inputs */
    uint8_t             partition_id;               ///< ID of partition to write into.
    uint16_t            record_id;                  ///< Record ID of data to write.
    uint8_t *           p_write_buffer;             ///< Pointer to start of buffer containing data to write.
    uint16_t            tdr_bytes_to_write;         ///< Number of bytes of TDR to write (excluding RSR wrapper).
    bool_t              b_read_back_required;       ///< Flag set to read back the memory after a write operation.

    /* Outputs */
    rs_queue_status_t * p_write_status;             ///< Pointer to write status word.
    void*               p_write_semaphore;          ///< Handle to write semaphore.

    /* Queue specific variables - don't set these up when making a request */
    uint8_t             partition_index;            ///< Queue uses partition index, not ID.
} rs_write_request_t;


/*!
 * Format request structure.
 *
 * This is used by the format request and the internal format queue.
 *
 */
typedef struct
{
    /* Inputs */
    uint8_t             partition_id;               ///< ID of partition to format.

    /* Outputs */
    rs_queue_status_t * p_format_status;            ///< Pointer to format status word.
    void*               p_format_semaphore;         ///< Handle for format semaphore.

    /* Queue specific variables - don't set these up when making a request */
    uint8_t             partition_index;            ///< Queue uses partition index, not ID.
} rs_format_request_t;


bool_t      rsapi_recording_system_init(void);

rs_error_t  rsapi_partition_format_request
                        (const rs_format_request_t * const p_format_request);

uint8_t     rsapi_partition_format_prog_get(void);

rs_error_t  rsapi_partition_status_get(uint8_t partition_id);

rs_error_t  rsapi_read_request(const rs_read_request_t * const p_read_request);

rs_error_t  rsapi_write_request(const rs_write_request_t * const p_write_request);

void        rsapi_readwrite_task(void * p_task_parameters);

bool_t      rsapi_query_if_task_enabled(void);

void        rsapi_task_enable(void);

void        rsapi_task_disable(void * const p_disable_semaphore);

const rs_configuration_t*   rsapi_configuration_pointer_get(void);

uint16_t    rsapi_queue_items_waiting_get(const rs_queue_identifiers_t identifier);

#endif /* SOURCE_RSAPI_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

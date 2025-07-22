// ----------------------------------------------------------------------------
/**
 * @file        rsapi_prv.h
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        19 Feb 2016
 * @brief       Private header file for rsapi.c
 * @details     This is a private header file for rsapi.c - it contains
 *              anything which doesn't need to be visible to other modules as
 *              part of the API itself, or things which are only visible when
 *              running the units tests.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef SOURCE_RSAPI_PRV_H_
#define SOURCE_RSAPI_PRV_H_

/*
 * Generate an error if the number of partitions is greater than 8 bits,
 * as the counters within the code are generally uint8_t when used with this.
 */
#if RS_CFG_MAX_NUMBER_OF_PARTITIONS > 255u
#error "Maximum number of partitions cannot be greater than 255"
#endif


/* Generate an error if the page size is less than the maximum TDR size.
 * The search algorithms are not designed to cope with this, and this is
 * unlikely to be something which is actually required.
 */
#if (RS_CFG_PAGE_SIZE_KB * 1024u) < RS_CFG_MAX_TDR_SIZE_BYTES
#error "Page size cannot be less than maximum TDR size!"
#endif


/*
 * Generate an error if the maximum TDR exceeds 16 bits.
 * The search algorithms are not designed to cope with this - it's unlikely
 * that this is an issue as the recording system specification gives the
 * maximum TDR size as 4k.
 */
#if RS_CFG_MAX_TDR_SIZE_BYTES > 65535u
#error "Maximum TDR size exceeds 16 bit - recording system not designed to cope with this"
#endif


/*
 * Generate an error if the page size is too large for 32 bit operation
 * (the internal working of the recording system is mainly 32 bits at present).
 * Note that this is a crude test - there are more tests to check that the
 * logical address doesn't overflow 32 bits.
 */
#if RS_CFG_PAGE_SIZE_KB > 0x3FFFFFu
#error "Page size will overflow 32 bits when converted to bytes"
#endif


/**
 * Enumerated type for the different states which the read \ write task
 * state engine can occupy.
 */
typedef enum
{
    RSAPI_STATE_IDLE_READ_CHECK,    ///< Task is idling, check for any reads.
    RSAPI_STATE_IDLE_WRITE_CHECK,   ///< Task is idling, check for any writes.
    RSAPI_STATE_IDLE_FORMAT_CHECK,  ///< Task is idling, check for any formatting.
    RSAPI_STATE_READ_REQUIRED,      ///< A read is required.
    RSAPI_STATE_READ_IN_PROGRESS,   ///< A read is in progress.
    RSAPI_STATE_WRITE_REQUIRED,     ///< A write is required.
    RSAPI_STATE_WRITE_IN_PROGRESS   ///< A write is in progress.
} rsapi_read_write_task_state_t;


#ifdef UNIT_TEST_BUILD
/**
 * Structure for read \ write task, for unit testing.
 * THis allows the unit tests to check all results via a single structure.
 */
typedef struct
{
    rsapi_read_write_task_state_t   initial_state;
    rsapi_read_write_task_state_t   final_state;

    rs_read_request_t               read_queue_data;
    rs_write_request_t              write_queue_data;
    rssearch_search_data_t          search_data;
    rs_page_write_t                 write_data;
} rsapi_task_test_t;
#endif /* UNIT_TEST_BUILD */

#endif /* SOURCE_RSAPI_PRV_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

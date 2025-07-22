// ----------------------------------------------------------------------------
/**
 * @file        rspages_prv.h
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        23 Feb 2016
 * @brief       Private header file for rspages.c
 * @details     This is a private header file for rscore.c - it contains
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
#ifndef SOURCE_RSPAGES_PRV_H_
#define SOURCE_RSPAGES_PRV_H_

#ifdef UNIT_TEST_BUILD

/**
 * Structure for recording system, for unit testing.
 * This allows the unit tests to check all results via a single pointer.
 */
typedef struct
{
    // Function pointer to the various static functions to test directly.
    bool_t      (*p_check_area_is_blank)(const uint8_t * const p_area,
                                         const uint16_t size_of_area);

    bool_t      (*p_write_and_read_back)(const uint32_t logical_start_address,
                                         const uint32_t number_of_bytes_to_write,
                                         const uint8_t * const p_write_data,
                                         const bool_t   b_read_back_requested);

    bool_t      (*p_compare_buffers)(const uint8_t * const p_buffer1,
                                     const uint8_t * const p_buffer2,
                                     const uint32_t length);

    rs_page_write_status_t  (*p_write_page_data_handle_overlap)
                                    (const rs_page_write_t * const p_write,
                                     uint32_t * const p_next_free_address);

    bool_t      (*p_check_rsr_will_fit_in_partition)
                                    (const rs_page_write_t * const p_write_data);

} rspages_unit_test_pointers_t;

rspages_unit_test_pointers_t* rspages_unit_test_ptr_get(void);

#endif /* UNIT_TEST_BUILD */

#endif /* SOURCE_RSPAGES_PRV_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

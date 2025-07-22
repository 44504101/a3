// ----------------------------------------------------------------------------
/**
 * @file        rspartition_prv.h
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        04 Apr 2016
 * @brief       Private header file for rspartition.c
 * @details     This is a private header file for rspartition.c - it contains
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
#ifndef SOURCE_RSPARTITION_PRV_H_
#define SOURCE_RSPARTITION_PRV_H_

#ifdef UNIT_TEST_BUILD

/**
 * Structure for recording system, for unit testing.
 * This allows the unit tests to check all results via a single pointer.
 */
typedef struct
{
    rs_partition_info_t*    p_partitions;

} rspartition_unit_test_ptrs_t;

rspartition_unit_test_ptrs_t* rspartition_unit_test_ptrs_get(void);

#endif /* UNIT_TEST_BUILD */

#endif /* SOURCE_RSAPI_PARTITION_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

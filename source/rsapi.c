// ----------------------------------------------------------------------------
/**
 * @file        rsapi.c
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        17 Feb 2016
 * @brief       Recording system module for RSS tools.
 * @details
 * This is the top level of the recording system module for RSS tools,
 * containing all the API functions.  These are the only functions in the
 * recording system module which other code should call.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
/* Disable warnings for variables could be defined at block scope as we need
 * the variables to have module scope to allow the unit tests to access them.
 */
//lint -save
//lint -esym(9003, m_last_wake_time)
//lint -esym(9003, m_task_test)
//lint -esym(9003, m_logical_address_map)
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "rsappconfig.h"

/* Use the Free\OpenRTOS queues, semaphores etc for the moment - the intention
 * would be to re-factor this to include an RTOS abstraction layer, but there
 * isn't time to do this now.
 */

#include "rsapi.h"
#include "rssearch.h"
#include "rspartition.h"
#include "rspages.h"
#include "flash_hal.h"
#include "rsapi_prv.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define SPEC_LEVEL                  0x00AAu;        ///< Meets S-406011 rev AA.
#define CODE_VERSION                0x0101u;        ///< Version 1.01

///< Maximum event number the recording system can handle.
#define MAX_EVENT_NUMBER            RS_CFG_READ_QUEUE_LENGTH  + \
                                    RS_CFG_WRITE_QUEUE_LENGTH +  \
                                    RS_CFG_MAX_NUMBER_OF_PARTITIONS

///< Initial recording system event number.
#define INITIAL_EVENT_NUMBER        0u


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static void check_partition_before_use(const uint8_t partition_index);

static void queue_status_update(rs_queue_status_t * const p_status,
                                const rs_queue_status_t new_status,
                                void * const p_semaphore);

static rsapi_read_write_task_state_t read_required_state_do
                            (const rs_read_request_t * const p_read_request,
                             rssearch_search_data_t * const p_search_data);

static rsapi_read_write_task_state_t read_in_progress_state_do
                            (const rs_read_request_t * const p_read_request,
                             const rssearch_search_data_t * const p_search_data);

static rsapi_read_write_task_state_t write_required_state_do
                            (const rs_write_request_t * const p_write_request,
                             rs_page_write_t * const p_write_data);

static rsapi_read_write_task_state_t write_in_progress_state_do
                            (const rs_write_request_t * const p_write_request,
                             const rs_page_write_t * const p_write_data);

static rsapi_read_write_task_state_t format_check_state_do(void);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/*
 * Note that none of these variables need to be volatile because they are
 * only used within this module and are not changed from outside it, so we
 * can disable all the lint warnings for 956 (non const, non volatile static
 * or external variable)
 */

/// The configuration structure for the recording system.
//lint -e{956}
static rs_configuration_t   m_rs_config;

/// Flag to say whether the recording system has been initialised or not.
//lint -e{956}
static bool_t               m_b_recording_system_has_been_initialised = FALSE;


/**
 * Structure holding logical address mapping for each partition.
 * Extracted from the partition module and stored in here for use by
 * the flash_hal_initialise() function.
 */
//lint -e{956}
static flash_hal_logical_t  m_logical_address_map[RS_CFG_MAX_NUMBER_OF_PARTITIONS];

/// Flag to enable or disable the read \ write task.
//lint -e{956}
static bool_t               m_b_rw_task_enabled = FALSE;

/// Flag to request that the read \ write task be disabled.
//lint -e{956}
static bool_t               m_b_rw_task_disable_request = FALSE;

/// Partition format progress counter, 0 to 100%.
//lint -e{956}
static uint8_t              m_partition_format_progress = 0u;

#ifdef UNIT_TEST_BUILD
/**
 * Structure to hold variables which we use for testing the read \ write task.
 */
//lint -e{956}
static rsapi_task_test_t    m_task_test;
#endif


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * rsapi_recording_system_init initialises the recording system.
 *
 * @note
 * The OpenRTOS queue functions can only handle a queue item size of 256 bytes.
 * This is probably not a real problem, just something to be aware of.
 * We cast the item size to 8 bits whenever we use it, which is ok.
 *
 * This function sets up all structures which are related to the recording
 * system, and creates the read and write queues which are used to request
 * reads and writes from the recording system.
 *
 * @retval  bool_t      TRUE if recording system initialised OK, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
bool_t rsapi_recording_system_init(void)
{
    uint8_t                     partition_counter;
    const rs_partition_info_t * p_partition;
    bool_t                      b_flash_hal_initialised_ok;

    m_rs_config.spec_level              = SPEC_LEVEL;
    m_rs_config.code_version            = CODE_VERSION;
    m_rs_config.board_type              = RS_CFG_BOARD_TYPE;
    m_rs_config.number_of_partitions    = RS_CFG_MAX_NUMBER_OF_PARTITIONS;
    m_rs_config.page_size_kb            = RS_CFG_PAGE_SIZE_KB;
    m_rs_config.total_pages             = 0u;
    m_rs_config.accessible_pages        = 0u;
    m_rs_config.unusable_pages          = 0u;
    m_rs_config.error_pages             = 0u;

    /*
     * Now calculate the start and end logical addresses for each partition.
     * This also calculates the total number of pages in the memory.
     */
    rspartition_addresses_calculate();

    /*
     *  Now extract the logical addresses for each partition
     *  and copy into the logical address map which the flash HAL uses.
     */
    for (partition_counter = 0u;
            partition_counter < RS_CFG_MAX_NUMBER_OF_PARTITIONS;
            partition_counter++)
    {
        p_partition = rspartition_partition_ptr_get(partition_counter);

        m_logical_address_map[partition_counter].device_to_use
            = p_partition->device_to_use;

        m_logical_address_map[partition_counter].start_address
            = p_partition->start_address;

        m_logical_address_map[partition_counter].end_address
            = p_partition->end_address;
    }

    /* Initialise the flash HAL before we need to use it. */
    b_flash_hal_initialised_ok = flash_hal_initialise(&m_logical_address_map[0u]);

    /* Only check the partitions if the HAL initialised correctly,
     * otherwise it means the addresses were wrong somehow so we
     * can't read\write properly.
     */
    if (b_flash_hal_initialised_ok)
    {
        /* Check each partition to make sure it's in good order. */
        for (partition_counter = 0u;
                partition_counter < RS_CFG_MAX_NUMBER_OF_PARTITIONS;
                partition_counter++)
        {
            check_partition_before_use(partition_counter);
        }

    }
    m_b_recording_system_has_been_initialised = TRUE;
    return m_b_recording_system_has_been_initialised;
}


// ----------------------------------------------------------------------------
/**
 * rsapi_partition_format_request requests that a partition be formatted.
 * The format request is added to the format queue, and the read \ write task
 * will then perform the format when not reading or writing.
 *
 * @note
 * partition_index is 16 bits so we can return a max value for a bad partition,
 * but we cast to 8 bits before updating the partition index in the input struct.
 * This is safe because we're already discarded invalid ID's, so we disable the
 * Lint warning.
 *
 * @param   p_format_request    Pointer to the format request structure.
 * @retval  rs_error_t          Enumerated value for error code.
 *
 */
// ----------------------------------------------------------------------------
rs_error_t rsapi_partition_format_request
                        (const rs_format_request_t * const p_format_request)
{
    rs_error_t          format_request_status = RS_ERR_BAD_FORMAT_QUEUE;
    rs_queue_status_t   queue_status_to_update = RS_QUEUE_COULD_NOT_ADD_TO_QUEUE;
    uint16_t            partition_index;
    rs_format_request_t format_data;

    /* Don't allow format if recording system not initialised yet. */
    if (!m_b_recording_system_has_been_initialised)
    {
        format_request_status = RS_ERR_NOT_INITIALISED_YET;
    }
    else if (p_format_request != NULL)
    {
        partition_index
            = rspartition_check_partition_id(p_format_request->partition_id);

        if (partition_index == RSPARTITION_INDEX_BAD_ID_VALUE)
        {
            format_request_status = RS_ERR_BAD_PARTITION_ID;
        }
        else
        {
            /*
             * Setup the queue structure and send to the queue.
             * We cast the partition index from 16 bits to 8 bits.
             * We only need to use 8 bits here, we only need 16 bits
             * above so we can return 0xFFFF if the ID was bad.
             */
            format_data.partition_id       = p_format_request->partition_id;
            //lint -e{921} Cast from uint16_t to uint8_t
            format_data.partition_index    = (uint8_t)partition_index;
            format_data.p_format_status    = p_format_request->p_format_status;
            format_data.p_format_semaphore = p_format_request->p_format_semaphore;

        }
    }
    else
    {
        ;   // Extra else for MISRA compliance - just return bad format queue.
    }

    return format_request_status;
}


// ----------------------------------------------------------------------------
/**
 * rsapi_partition_format_prog_get return the progress of the partition
 * format function, which should be between 0 and 100%.
 *
 * @retval  uint8_t     Partition format progress, between 0 and 100%.
 *
 */
// ----------------------------------------------------------------------------
uint8_t rsapi_partition_format_prog_get(void)
{
    return m_partition_format_progress;
}


// ----------------------------------------------------------------------------
/**
 * rsapi_partition_status_get
 *
 * @param   partition_id   Standardised partition (type) number.
 *
 * @retval  rs_error_t     Partition status (OK, Full, Unformatted...etc...)
 *
 */
// ----------------------------------------------------------------------------
rs_error_t  rsapi_partition_status_get(uint8_t partition_id)
{
    rs_error_t                 partition_status = RS_ERR_BAD_PARTITION_ID;
    uint16_t                   partition_index;
    const rs_partition_info_t* partition_ptr;

    // Convert standardised partition ID into a recording system index
    partition_index = rspartition_check_partition_id(partition_id);

    // If this ID has been implemented on this specific recording system
    if (partition_index != RSPARTITION_INDEX_BAD_ID_VALUE)
    {
        // Access the partition parameters
        partition_ptr = rspartition_partition_ptr_get(partition_index);

        // Get the status of the partition
        partition_status = partition_ptr->partition_error_status;
    }

    return partition_status;
}


// ----------------------------------------------------------------------------
/**
 * rsapi_read_request handles read requests from other tasks.
 *
 * @note
 * This function does not do the read, it just adds the request to the read queue
 * (but only if the recording system has been initialised and the partition does
 * not need to be formatted).
 *
 * @note
 * It is allowed to use NULL pointers for the read buffer and semaphore - the
 * read task will simply ignore these but will still perform the read, as this
 * is sometimes useful.
 *
 * @note
 * partition_index is 16 bits so we can return a max value for a bad partition,
 * but we cast to 8 bits before passing to rspartition_partition_ptr_get.  This
 * is safe because we're already discarded invalid ID's, so we disable the Lint
 * warning.
 *
 * @param   p_read_request      Pointer to the read request structure.
 * @retval  rs_error_t          Enumerated value for error code.
 *
 */
// ----------------------------------------------------------------------------
rs_error_t rsapi_read_request(const rs_read_request_t * const p_read_request)
{
    rs_error_t          read_request_status = RS_ERR_BAD_READ_QUEUE;
    uint16_t            partition_index;
    rs_queue_status_t   queue_status_to_update = RS_QUEUE_COULD_NOT_ADD_TO_QUEUE;
    const rs_partition_info_t*    p_partition_info;

    if (!m_b_recording_system_has_been_initialised)
    {
        read_request_status = RS_ERR_NOT_INITIALISED_YET;
    }
    return read_request_status;
}


// ----------------------------------------------------------------------------
/**
 * rsapi_write_request handles write requests from other tasks.
 *
 * @note
 * This function does not do the write, it just adds the request to the
 * queue (but only if the recording system has been initialised and the
 * partition does not need to be formatted).
 *
 * @note
 * It is allowed to use NULL pointers for the write buffer and semaphore - the
 * write task will simply ignore these but will still perform the write, as this
 * is sometimes useful.  For the case of a NULL write buffer pointer, the write
 * task will write 0xFF's. @TODO Doesn't do this at the moment!
 *
 * @note
 * partition_index is 16 bits so we can return a max value for a bad partition,
 * but we cast to 8 bits before passing to rspartition_partition_ptr_get.  This
 * is safe because we're already discarded invalid ID's, so we disable the Lint
 * warning.
 *
 * @param   p_write_request     Pointer to the write request structure.
 * @retval  rs_error_t          Enumerated value for error code.
 *
 */
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * rsapi_readwrite_task is the read \ write \ formatting task.
 *
 * @note
 * This task assumes the use of FreeRTOS \ OpenRTOS, which means that the
 * local variables will always retain their values as the function never
 * actually exits.  The m_b_read_write_task_enabled is used to enabled \ disable
 * the read \ write portion of the task, and check \ do any formatting which
 * is required.
 *
 * @note
 * To allow the task to be unit tested, we use conditional compilation to
 * add a break statement to jump us out of the infinite loop while running
 * unit tests, and to allow us to access the local variables before they go out
 * of scope.
 *
 * @warning
 * This task is disabled by making a request to disable.  This request is only
 * processed in the idle state of the task.  This mechanism ensures that any
 * current read \ write operation has completed before disabling the recording
 * system.  Care needs to be taken if a higher priority task is waiting for
 * the recording system to be disabled, as simply polling for the flag
 * m_b_rw_task_enabled to be set to FALSE in the higher priority task will
 * cause this task to be blocked and the flag will never be set.
 *
 * @param   p_task_parameters   Pointer to any parameters passed into the task.
 *
 */
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * rsapi_query_if_task_enabled returns whether the read \ write task is enabled
 * or not.
 *
 * @retval  bool_t      TRUE if read \ write task enabled, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
bool_t rsapi_query_if_task_enabled(void)
{
    return m_b_rw_task_enabled;
}


// ----------------------------------------------------------------------------
/**
 * rsapi_task_enable sets the flag to enable the read \ write task.
 *
 */
// ----------------------------------------------------------------------------
void rsapi_task_enable(void)
{
    m_b_rw_task_enabled = TRUE;
}


// ----------------------------------------------------------------------------
/**
 * rsapi_task_disable sets the flag to request that the read \ write task
 * is disabled.  The read \ write task will then disable itself once the
 * current operation has finished, and post a semaphore (if one is provided);
 *
 * @param   p_disable_semaphore     Handle to semaphore to post when disabled.
 *
 */
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/**
 * rsapi_configuration_pointer_get returns a const pointer to the configuration
 * structure.
 *
 * @retval rs_configuration_t*      Constant pointer to m_rs_config.
 *
 */
// ----------------------------------------------------------------------------
const rs_configuration_t* rsapi_configuration_pointer_get(void)
{
    return &m_rs_config;
}


// ----------------------------------------------------------------------------
/*!
 * rsapi_queue_items_waiting_get returns the number of messages which are
 * waiting a particular queue to be processed, to give an idea of whether
 * the queue is being emptied correctly or not.
 *
 * @param   identifier      Enumerated type for the queue to query.
 * @retval  uint16_t        Number of messages waiting in the queue.
 *
 */
// ----------------------------------------------------------------------------
uint16_t rsapi_queue_items_waiting_get(const rs_queue_identifiers_t identifier)
{

    //lint -e{921} Cast to standard type to avoid using FreeRTOS types everywhere.
    return (uint16_t)0;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * check_partition_before_use makes sure that a partition is fit for use.
 *
 * This uses a bisection search to find the next page which can be written to.
 *
 * @note
 * This function needs various members of m_rs_partition_info to have been
 * initialised before use - id, number_of_pages, start_address and end_address.
 *
 * @param   partition_index     Partition number to check (0,1,2 etc).
 *
 */
// ----------------------------------------------------------------------------
static void check_partition_before_use(const uint8_t partition_index)
{
    const rs_partition_info_t*  p_partition;

    /* Only check partition if index is valid. */
    if (partition_index < RS_CFG_MAX_NUMBER_OF_PARTITIONS)
    {
        //lint -e{920} Ignoring return value as the only failure is unformatted.
        (void)rspartition_bisection_search_do(partition_index);

        /* Index is valid so fetch pointer to partition information. */
        p_partition = rspartition_partition_ptr_get(partition_index);

        /* Update all 'total' page counters - note that the
         * partition index will be valid here so the individual
         * values will have been updated by the search functions.
         */
        m_rs_config.total_pages         += p_partition->number_of_pages;
        m_rs_config.accessible_pages    += p_partition->free_pages;
        m_rs_config.accessible_pages    += p_partition->full_pages;
        m_rs_config.unusable_pages      += p_partition->unusable_pages;
        m_rs_config.error_pages         += p_partition->error_pages;
    }
}


// ----------------------------------------------------------------------------
/**
 * queue_status_update updates the status variable pointed to by p_status,
 * and posts a semaphore using the p_semaphore pointer.
 *
 * @param   p_status    Pointer to status variable to update.
 * @param   new_status  New status valid to update the status variable with.
 * @param   p_semaphore Pointer to semaphore to post.
 *
 */
// ----------------------------------------------------------------------------
static void queue_status_update(rs_queue_status_t * const p_status,
                                const rs_queue_status_t new_status,
                                void * const p_semaphore)
{
    if (p_status != NULL)
    {
        *p_status = new_status;
    }

    if (p_semaphore != NULL)
    {
        /*
         * Give the semaphore to the blocked task if the request
         * failed or the request was complete (i.e. to unblock).
         * Any other status means that we're still trying to do something.
         */
        if ( (new_status == RS_QUEUE_REQUEST_FAILED)
                || (new_status == RS_QUEUE_REQUEST_COMPLETE))
        {
            /*
             * Discard the return value of the semaphore give as there's
             * not much we can do here if it didn't work.
             */
            //lint -e{920} Cast from short to void.
            (void)xSemaphoreGive(p_semaphore);
        }
    }
}


// ----------------------------------------------------------------------------
/**
 * read_required_state_do is part of the read \ write task state engine, and
 * is called when a read is required.
 *
 * This function sets up the search data structure (via the p_search_data
 * pointer) if the partition is valid.
 *
 * @note
 * This function uses the p_search_data pointer to setup the structure
 * in the calling function.
 *
 * @param   p_read_request      Pointer to read request data structure.
 * @param   p_search_data       Pointer to search data structure to set up.
 * @retval  rsapi_read_write_task_state_t   New task state.
 *
 */
// ----------------------------------------------------------------------------
static rsapi_read_write_task_state_t read_required_state_do
                            (const rs_read_request_t * const p_read_request,
                             rssearch_search_data_t * const p_search_data)
{
    const rs_partition_info_t*      p_partition;
    rsapi_read_write_task_state_t   new_state;

    p_partition = rspartition_partition_ptr_get(p_read_request->partition_index);

    if (p_partition != NULL)
    {
        p_search_data->search_direction                = p_read_request->search_direction;
        p_search_data->partition_logical_start_address = p_partition->start_address;
        p_search_data->partition_logical_end_address   = p_partition->end_address;

        if (p_search_data->search_direction == RSSEARCH_FORWARDS)
        {
            p_search_data->search_start_address = p_partition->start_address;
        }
        else
        {
            p_search_data->search_start_address = p_partition->next_available_address;
        }
    }


    return RSAPI_STATE_WRITE_IN_PROGRESS;
}


// ----------------------------------------------------------------------------
/**
 * read_in_progress_state_do is part of the read \ write task state engine, and
 * is called when a read is in progress.
 *
 * This function sets up the timeout timer and then searches for the appropriate
 * record using the search data structure.
 *
 *
 * @param   p_read_request      Pointer to read request data structure.
 * @param   p_search_data       Pointer to search data structure.
 * @retval  rsapi_read_write_task_state_t   New task state.
 *
 */
// ----------------------------------------------------------------------------
static rsapi_read_write_task_state_t read_in_progress_state_do
                            (const rs_read_request_t * const p_read_request,
                             const rssearch_search_data_t * const p_search_data)
{
    rs_queue_status_t           queue_status_to_update;
    bool_t                      b_read_ok;
    const rssearch_rsr_info_t*  p_valid_rsr;
    uint16_t                    copy_counter;



    return RSAPI_STATE_IDLE_READ_CHECK;
}


// ----------------------------------------------------------------------------
/**
 * write_required_state_do is part of the read \ write task state engine, and
 * is called when a write is required.
 *
 * This function sets up the timeout timer and the write data structure
 * (via the p_write_data pointer) if the partition is valid.
 *
 * @note
 * This function uses the p_write_data pointer to setup the structure
 * in the calling function.
 *
 * @param   p_write_request     Pointer to read request data structure.
 * @param   p_write_data        Pointer to write data structure to set up.
 * @retval  rsapi_read_write_task_state_t   New task state.
 *
 */
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/**
 * write_in_progress_state_do is part of the read \ write task state engine, and
 * is called when a write is in progress.
 *
 * This function writes the required data into the recording memory, using
 * the rspages_page_data_write() function.
 *
 *
 * @param   p_write_request     Pointer to read request data structure.
 * @param   p_write_data        Pointer to write data structure.
 * @retval  rsapi_read_write_task_state_t   New task state.
 *
 */
// ----------------------------------------------------------------------------
static rsapi_read_write_task_state_t write_in_progress_state_do
                            (const rs_write_request_t * const p_write_request,
                             const rs_page_write_t * const p_write_data)
{
    rs_page_write_status_t          page_write_status;

    page_write_status = rspages_page_data_write(p_write_data);

    if ((page_write_status == RS_PG_WRITE_OK)
            || (page_write_status == RS_PG_WRITE_OK_PAGE_FULL))
    {
        queue_status_update(p_write_request->p_write_status,
                            RS_QUEUE_REQUEST_COMPLETE,
                            p_write_request->p_write_semaphore);
    }
    else
    {
        queue_status_update(p_write_request->p_write_status,
                            RS_QUEUE_REQUEST_FAILED,
                            p_write_request->p_write_semaphore);
    }

    /*
     * Always go back to the idle read state when a write has finished.
     * It doesn't matter whether the write was successful or not.
     */
    return RSAPI_STATE_IDLE_READ_CHECK;
}


// ----------------------------------------------------------------------------
/**
 * check_for_format_required is called from the read \ write \ format task,
 * and checks to see whether there is anything in the format queue.
 *
 * If there's something in the queue, a format is carried out.
 *
 * @retval  rsapi_read_write_task_state_t   The new state.
 */
// ----------------------------------------------------------------------------
static rsapi_read_write_task_state_t format_check_state_do(void)
{

    rs_format_request_t             format_queue_data;
    rs_error_t                      format_status;
    rsapi_read_write_task_state_t   new_state = RSAPI_STATE_IDLE_READ_CHECK;
    rs_queue_status_t               status_to_update_on_completion;
    const rs_partition_info_t*      p_partition;
    rs_error_t                      status_before_format;

    /*
     * If there's something in the queue then we need to do a format.
     * The format request has already been validated, so we can just go ahead.
     */
    return new_state;
}


// ----------------------------------------------------------------------------
//lint -restore
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
/**
 * @file        acqmtc_dsp_b/src/opcode221.c
 * @author		Benoit LeGuevel
 * @date        11 Oct. 2013
 * @brief       Handles the opcode 221 processing : Get erase status.
 * @details
 * Get the erase status of the latest erased device. We firts call the
 * DSP_B_MODE_deviceErasedGet() function to know what is this device.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/

// ----------------------------------------------------------------------------

#include "opcode221.h"
#include "lld.h"



// ----------------------------------------------------------------------------
/*!
 * ¶ÁÈ¡FLASH²Á³ý×´Ì¬.
 *
 * @note
 * This function using various #defines in the lld.h file, which is rather
 * loose in its types, so we disable lots of MISRA warnings here for the
 * various transgressions.  The code works OK, it's just messy.
 *
 * @param   device                      Device to poll ('0' or '1').
 * @retval  main_flash_status_t    Enumerated status value.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{921, 923, 9027, 9029, 9078, 9117, 9130}
static main_flash_status_t main_flash_poll(const char_t device)
{
    main_flash_status_t return_value;
    FLASHDATA status_register;

    if (device == '0')
    {
        /* Send the status register read command. */
        lld_StatusRegReadCmd(DEVICE_ZERO_BASE);

        /* Read from the status register. */
        status_register = lld_ReadOp(DEVICE_ZERO_BASE, (ADDRESS) 0u);
    }
    else if (device == '1')
    {
        // Send the status register read command.
        lld_StatusRegReadCmd(DEVICE_ONE_BASE);

        // Read from the status register.
        status_register = lld_ReadOp(DEVICE_ONE_BASE, (ADDRESS) 0u);
    }
    else
    {
        status_register = 0u;
    }

    /*
     * If the flash is BUSY then all other bits in the status register are
     * invalid, so just return that the device is busy.
     */
    if ((status_register & DEV_RDY_MASK) != DEV_RDY_MASK)
    {
        return_value = FLASH_POLL_BUSY;
    }
    /*
     * Otherwise flash is not busy, but there might be other things in the
     * status register which are important, so check all the different options.
     */
    else
    {
        /* If erase suspend bit is set then return erase suspended code. */
        if ((status_register & DEV_ERASE_SUSP_MASK) == DEV_ERASE_SUSP_MASK)
        {
            return_value = FLASH_POLL_ERASE_SUSPENDED;
        }
        /*
         * If erase fail bit is set then check for sector locked, and either
         * return erase fail or sector locked code.
         */
        else if ((status_register & DEV_ERASE_MASK) == DEV_ERASE_MASK)
        {
            return_value = FLASH_POLL_ERASE_FAIL;

            if ((status_register & DEV_SEC_LOCK_MASK) == DEV_SEC_LOCK_MASK)
            {
                return_value = FLASH_POLL_SECTOR_LOCKED;
            }
        }
        /*
         * If program fail bit is set then check for sector locked, and either
         * return program fail or sector locked code.
         */
        else if ((status_register & DEV_PROGRAM_MASK) == DEV_PROGRAM_MASK)
        {
            return_value = FLASH_POLL_PROGRAM_FAIL;

            if ((status_register & DEV_SEC_LOCK_MASK) == DEV_SEC_LOCK_MASK)
            {
                return_value = FLASH_POLL_SECTOR_LOCKED;
            }
        }
        /*
         * If program aborted bit is set then return program aborted code.
         * Note that #define in lld.h is for a reserved bit,but it is used!
         */
        else if ((status_register & DEV_RFU_MASK) == DEV_RFU_MASK)
        {
            return_value = FLASH_POLL_PROGRAM_ABORTED;
        }
        /* If program suspended bit is set then return program suspended code. */
        else if ((status_register & DEV_PROGRAM_SUSP_MASK)
                == DEV_PROGRAM_SUSP_MASK)
        {
            return_value = FLASH_POLL_PROGRAM_SUSPENDED;
        }
        /*
         * If sector lock bit is set (on its own) then return sector locked code.
         * Note that this bit probably can't be set on its own - it should go
         * with either program or erase fail, which are dealt with above.
         */
        else if ((status_register & DEV_SEC_LOCK_MASK) == DEV_SEC_LOCK_MASK)
        {
            return_value = FLASH_POLL_SECTOR_LOCKED;
        }
        else
        {
            return_value = FLASH_POLL_NOT_BUSY;
        }
    }

    return return_value;
}

// ----------------------------------------------------------------------------
/**
 * opcod217 erases the flash memory and send back the write command status
 * The command has to retrieve the last device erase and then get its status.
 *
 * @param   pCommand            Pointer to the command.
 * @param   pResponse           Pointer to the response.
 */

// ----------------------------------------------------------------------------
//lint -e{715} pCommand not referenced (but prototype must be the same for all opcodes)
void opcode221_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,Timer_t* timer)
{
    main_flash_status_t EraseStatus = FLASH_POLL_BUSY;

    EraseStatus = main_flash_poll('1');
    /*device = DSP_B_MODE_deviceErasedGet();

    switch (device)
    {
        case (CONFIG_FIELD_BLOCK):
            EraseStatus                        = CONFIG_FLASH_eraseStatus_get();
            pResponse->bOpcodeResponseRequired = TRUE;
            break;

        case (CONFIG_ENG_BLOCK):
            EraseStatus                        = CONFIG_FLASH_eraseStatus_get();
            pResponse->bOpcodeResponseRequired = TRUE;
            break;

        case (RECORDING_FLASH):

            // If the DSP is in FLASH ERASE mode and if the flash is not erased => FLASH_BUSY
            if ( TRUE == RECORDING_FLASH_EraseStatusGet() )
            {
                EraseStatus = FLASH_POLL_NOT_BUSY;
            }
            else
            {
                EraseStatus = FLASH_POLL_BUSY;
            }

            pResponse->bOpcodeResponseRequired = TRUE;
            break;

        case (SURVEY_TRAJECTORY_PARTITIONS):
            EraseStatus                        = FLASH_POLL_NOT_BUSY;
            pResponse->bOpcodeResponseRequired = TRUE;
            break;

        default:
            pResponse->bOpcodeResponseRequired = FALSE;
            break;
    }*/


    // update the response status
    if ( FLASH_POLL_NOT_BUSY == EraseStatus )
    {
        //pResponse->status = OPCODE_NO_ERROR;
        loader_MessageSend( LOADER_OK, 0, "" );
    }
    else
    {
        //pResponse->status = OPCODE_MEMORY_BUSY;
        loader_MessageSend( 6, 0, "" );
    }
    Timer_TimerReset(timer);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode221.c;5 */
/*       1*[1792972] 17-FEB-2014 14:55:26 (GMT) BLeguevel */
/*         "Configuration and recording system implemented on the DSP_B downhole code" */
/*       2*[1794435] 29-APR-2014 15:18:27 (GMT) BLeguevel */
/*         "DSP_B code formatting" */
/*       3*[1842524] 27-JUN-2014 16:00:50 (GMT) BLeguevel */
/*         "DSP_B code modifications to match with the new RS485COM module implemented in the common code" */
/*       4*[1846566] 30-JUL-2014 12:07:39 (GMT) BLeguevel */
/*         "High level code modification in order to fit with the recording flash polling mechanism" */
/*       5*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode221.c;5 */

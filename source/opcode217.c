// ----------------------------------------------------------------------------
/**
 * @file        acqmtc_dsp_b/src/opcode217.c
 * @author		Benoit LeGuevel
 * @date        11 Oct. 2013
 * @brief       Handles the opcode 217 processing : Erase Flash Memory.
 * @details
 * The function is compatible with the Toolscope CRS implementation:
 * Thus the first argument following the opcode is the block identifier
 * which define which device or partition has to be erased.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/

// ----------------------------------------------------------------------------

#include "opcode217.h"
#include "opcode221.h"
#include "lld.h"
#include "m95.h"
#include "XDImemory.h"


#define BLOCK_ID_OFFSET         0u  ///< Block to erase identifier offset.

// ----------------------------------------------------------------------------
/**
 * opcod217 erases the flash memory and sends back the write command status
 * The command format is <opcode><blockIdentifier>
 *               blockIdentifier :  0x02        =>      FIELD_BLOCK,
 *                                  0x04        =>      ENGINEERING_BLOCK,
 *                                  [05;63]     =>      SURVEY AND TRAJECTORY partitions,
 *                                  0xFF        =>      RECORDING MEMORY.
 * The configuration memory Blocks can be erased only if the DSP is in COM page.
 * The device actually erased has to be recored, the opcode221 (get erase status)
 * needs to know which was the last device erased.
 *
 * @param   pCommand            Pointer to the command
 * @param   pResponse           Pointer to the response
 */
// ----------------------------------------------------------------------------

void opcode217_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,Timer_t* timer)
{
    //EFLASHPollStatus_t EraseStatus = FLASH_POLL_NOT_BUSY;
    const uint16_t     blockIdentifier = (uint16_t)message->dataPtr[BLOCK_ID_OFFSET] & 0xFFu;
    EM95PollStatus_t m95EraseStatus;
    bool_t XDIEraseStatus = FALSE;
    main_flash_status_t EraseStatus;
    // Record the device erased and send the command
    switch (blockIdentifier)
    {
        case 2:  //²Á³ýSPI EPPROM
            m95EraseStatus = M95_DeviceErase();
            if (m95EraseStatus == M95_POLL_NO_WRITE_IN_PROGRESS)
            {
                loader_MessageSend( LOADER_OK, 0, "" );
            }
            else
            {
                loader_MessageSend( LOADER_TIMEOUT, 0, "" );
            }
            break;

        case 4: //²Á³ýI2C EEPROM
            XDIEraseStatus = XDIMEMORY_EraseRequest();
            if (XDIEraseStatus)
            {
                loader_MessageSend(LOADER_OK, 0, "" );
            }
            else
            {
                loader_MessageSend(LOADER_INVALID_MESSAGE, 0, "" );
            }
            break;

        case (0xFFu) :    // Recording memory
                loader_MessageSend(LOADER_OK, 0, "" );
                lld_ChipEraseCmd(DEVICE_ZERO_BASE);  //²Á³ýFLASH0
                lld_ChipEraseCmd(DEVICE_ONE_BASE);   //²Á³ýFLASH1
                Timer_TimerSet(timer, 600000);
        	break;

        default :
            // Survey and trajectory partition erase
            // Sectors[0:31], the block identifier for the sector 0 is 5
                if ( (blockIdentifier > 4u) && (blockIdentifier < 37u) )
                {
                    lld_ChipEraseCmd(DEVICE_ZERO_BASE);  //²Á³ýFLASH0
                }

                // TODO patch to remove after Toolscope modification (only 32 sectors to erase)
                if (blockIdentifier > 65u)
                {
                    //EraseStatus = FLASH_POLL_SECTOR_LOCKED;
                }
            break;
    }

    // update the response status
    /*if ( FLASH_POLL_NOT_BUSY == EraseStatus )
    {
        loader_MessageSend( LOADER_OK, 0, "" );
    }
    else
    {
        loader_MessageSend( LOADER_OK, 0, "" );
        //pResponse->status = OPCODE_MEMORY_BUSY ;
    }*/
    Timer_TimerReset(timer);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode217.c;6 */
/*       1*[1792972] 17-FEB-2014 14:55:26 (GMT) BLeguevel */
/*         "Configuration and recording system implemented on the DSP_B downhole code" */
/*       2*[1794435] 29-APR-2014 15:18:27 (GMT) BLeguevel */
/*         "DSP_B code formatting" */
/*       3*[1842524] 27-JUN-2014 16:00:50 (GMT) BLeguevel */
/*         "DSP_B code modifications to match with the new RS485COM module implemented in the common code" */
/*       4*[1846566] 30-JUL-2014 12:07:39 (GMT) BLeguevel */
/*         "High level code modification in order to fit with the recording flash polling mechanism" */
/*       5*[1849590] 01-SEP-2014 08:50:48 (GMT) BLeguevel */
/*         "DSP_B mode,c file modification, implementation of a state machine" */
/*       6*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode217.c;6 */

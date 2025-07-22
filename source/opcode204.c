// ----------------------------------------------------------------------------
/**
 * @file        acqmtc_dsp_b/src/opcode204.c
 * @author		Benoit LeGuevel
 * @date        04 Dec. 2013
 * @brief       Handles the opcode 204 processing : Get Dpoint value.
 * @details
 * The opcode is followed by two arguments defining the lower Dpoint index and the upper Dpoint index
 * to fetch. The index go from 0 t0 255.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------

#include "opcode204.h"
#include "rspages.h"
#include "rspartition.h"
#include "flash_hal.h"

#define PARAM_LOW_OFFSET    0u			///< Lower Dpoint argument buffer index.
#define PARAM_HIGH_OFFSET   1u			///< Upper Dpoint argument buffer index.

//-----------------------------------------------------------------------------
//static uint8_t m_read_config_buffer[1024];   //读取配置数据

// ----------------------------------------------------------------------------
/**
 * opcode204 gets the Dpoints value from a Dpoint list going
 * from the Lower Dpoint index to the higher Dpoint index
 * The Dpoints are coded on 16bits. They are sent on
 * the RS485 bus starting from the lower Dpoint to
 * the upper one. For each Dpoint the sending format
 * is <LSB><MSB>
 * When certain Dpoints are read, they have to be cleared.
 * It is the case for the toolface vector Dpoints and the
 * status word Dpoints.
 *
 * @param   pCommand            Pointer to the command
 * @param   pResponse           Pointer to the response
 * @retval                      N.A
 */
// ----------------------------------------------------------------------------
void opcode204_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,Timer_t* timer)
{
    const uint16_t LowerDpointIndex = (uint16_t)message->dataPtr[PARAM_LOW_OFFSET];     //lint !e960 pointer arithmetic
    //const uint16_t UpperDpointIndex = (uint16_t)message->dataPtr[PARAM_HIGH_OFFSET];    //lint !e960 pointer arithmetic
    uint8_t bufferOffset = 105 + 4 * LowerDpointIndex;
    uint8_t buffer[4];
    uint32_t startAddress = 0x00002010;
    startAddress += bufferOffset;

    /*
    // 在第一次获取数据
    if(LowerDpointIndex == 0){
        if(rspartition_partition_ptr_get((uint8_t)1)->partition_error_status == RS_ERR_PARTITION_NEEDS_FORMAT){
            loader_MessageSend(LOADER_FORMAT_IN_PROGRESS, 0, "");
            return;
        }
        flash_hal_device_read(0x00002010, (uint32_t)524u, &m_read_config_buffer[0]);
    }
    bufferOffset = 105 + 4 * LowerDpointIndex;
    buffer[0] = m_read_config_buffer[bufferOffset];
    buffer[1] = m_read_config_buffer[bufferOffset+1];
    buffer[2] = m_read_config_buffer[bufferOffset+2];
    buffer[3] = m_read_config_buffer[bufferOffset+3];
*/


    // 判断配置分区是否格式化
    if(rspartition_partition_ptr_get(1)->partition_error_status == RS_ERR_PARTITION_NEEDS_FORMAT){
        loader_MessageSend(LOADER_FORMAT_IN_PROGRESS, 0, "");
        return;
    }
    // 判断配置分区是否包含写入数据
    if(rspartition_partition_ptr_get(1)->next_available_address < 0x0000221C){
        loader_MessageSend(LOADER_PARAMETER_OUT_OF_RANGE, 0, "");
        return;
    }

    if(flash_hal_device_read(startAddress, (uint32_t)4u, &buffer[0]) == FLASH_HAL_NO_ERROR){
        loader_MessageSend( LOADER_OK, 4, (char*)buffer );
    }else{
        loader_MessageSend(LOADER_INVALID_MESSAGE, 0, "");
    }

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode204.c;9 */
/*       1*[1792972] 17-FEB-2014 14:55:26 (GMT) BLeguevel */
/*         "Configuration and recording system implemented on the DSP_B downhole code" */
/*       2*[1794435] 29-APR-2014 15:18:27 (GMT) BLeguevel */
/*         "DSP_B code formatting" */
/*       3*[1842524] 27-JUN-2014 16:00:50 (GMT) BLeguevel */
/*         "DSP_B code modifications to match with the new RS485COM module implemented in the common code" */
/*       4*[1849907] 12-AUG-2014 12:14:16 (GMT) BLeguevel */
/*         "ToolfaceVector module implementation, reset the TF vector counters" */
/*       5*[1863246] 27-OCT-2014 14:57:13 (GMT) BLeguevel */
/*         "ACQ_STAT status word implementation" */
/*       6*[1867389] 04-NOV-2014 12:02:32 (GMT) BLeguevel */
/*         "Enable page switch  on opcodes 204, 205 and 202." */
/*       7*[1875174] 16-DEC-2014 15:21:51 (GMT) BLeguevel */
/*         "Latch the ACQ_STAT2, MTC_STAT, PWR_STAT status words in the DPS_B StatusWord.c module" */
/*       8*[1895737] 20-APR-2015 13:18:47 (GMT) SHaworth */
/*         "Rename d-points to specify that they are ACQ\MTC d-points" */
/*       9*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode204.c;9 */

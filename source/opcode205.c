// ----------------------------------------------------------------------------
/**
 * @file        acqmtc_dsp_b/src/opcode205.c
 * @author		Benoit LeGuevel
 * @date        04 Dec. 2013
 * @brief       Handles the opcode 205 processing : Set Dpoint value.
 * @details
 * The opcode is followed by 3 arguments: The Lower Dpoint index, the upper Dpoint index and the set of values
 * used to update the Dpoint contents.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */

// ----------------------------------------------------------------------------
#include "opcode205.h"
#include "rspages.h"
#include "rspartition.h"
#include "sci.h"
#include "buffer_utils.h"

#define PARAM_LOW_OFFSET        0u		///< Lower Dpoint index.
#define PARAM_HIGH_OFFSET       1u		///< Upper Dpoint index.
#define OPCODE_205_DATA_OFFSET  2u		///< Index of the first byte used to update the Dpoints.

static uint8_t m_write_config_buffer[1024];
static uint8_t bufferOffset = 5;    //索引偏移量
uint8_t channal_num[20] = { 0x11u, 0x0Cu, 0x09u, 0x01u, 0x09u, 0x0Eu, 0x05u,
                            0x07u, 0x12u, 0x07u, 0x0Au, 0x09u, 0x02u, 0x05u,
                            0x04u, 0x08u, 0x10u, 0x19u, 0x15u, 0x10u }; //每个记录组中需要记录的通道个数
// ----------------------------------------------------------------------------
/**
 * opcode205 sets the Dpoints value of the Dpoint list going
 * from the Lower Dpoint index to the higher Dpoint index.
 * The values are received as bytes, for a Dpoint, the reception
 * format is <LSB><MSB>.
 *
 * @note
 * The function fetches the reception buffer in order to update
 * the Dpoints between [LowerIndex; UpperIndex] independently of the
 * message length.
 *
 * @note
 * The RS485 baud rate can be changed through the Dpoint "DP_BAUD_RATE".
 * Only if the DSP is in DUMP_FLASH mode(To avoid any downhole issue), when
 * this Dpoint is set the RS485 communication baud rate is changed.
 *
 * @param   pCommand            Pointer to the command
 * @param   pResponse           Pointer to the response
 * @retval                      N.A
 */

// ----------------------------------------------------------------------------
void opcode205_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
                       Timer_t* timer)
{
    rs_page_write_t p_write_data;
    const rs_partition_info_t* p_partition_info;
    const uint16_t ConfigIndex = (uint16_t) message->dataPtr[0];
    const uint8_t *ConfigValue = &message->dataPtr[1];
    uint8_t p_buffer[10], i, j;

    if (ConfigIndex == 0)
    {
        for (i = 0; i < 20; i++)
        {
            BUFFER_UTILS_Uint16To8bitBuf(&p_buffer[0], 200u);
            m_write_config_buffer[bufferOffset] = p_buffer[0];
            bufferOffset += 1u;
            m_write_config_buffer[bufferOffset] = p_buffer[1];
            bufferOffset += 1u;
        }
        for (j = 0; j < 20; j++)
        {
            BUFFER_UTILS_Uint16To8bitBuf(&p_buffer[0], 200u);
            m_write_config_buffer[bufferOffset] = p_buffer[0];
            bufferOffset += 1u;
            m_write_config_buffer[bufferOffset] = p_buffer[1];
            bufferOffset += 1u;
            m_write_config_buffer[bufferOffset] = (uint8_t) channal_num[j];
            bufferOffset += 1u;
        }
    }

    bufferOffset = 105 + 4 * ConfigIndex;
    m_write_config_buffer[bufferOffset] = ConfigValue[0];
    m_write_config_buffer[bufferOffset + 1] = ConfigValue[1];
    m_write_config_buffer[bufferOffset + 2] = ConfigValue[2];
    m_write_config_buffer[bufferOffset + 3] = ConfigValue[3];

    if (ConfigIndex == 102) //bufferOffset == 524u)  //共104个配置值，此处写102是指第103个数，由于第104个配置在前面已经写进来了，所以102指的是现在的最后一个也就是第103个
    {
        p_write_data.partition_id = (uint8_t) 7u;
        p_write_data.record_id = (uint16_t) 28u;    //配置参数在记录系统中的工作id为  28
        p_write_data.partition_index = rspartition_check_partition_id(7); // 获取分区对应索引值
        p_partition_info = rspartition_partition_ptr_get(
                p_write_data.partition_index);
        p_write_data.partition_logical_start_addr =
                p_partition_info->start_address;
        p_write_data.partition_logical_end_addr =
                p_partition_info->end_address;
        p_write_data.next_free_addr = 8208;
        p_write_data.p_write_buffer = &m_write_config_buffer;
        p_write_data.bytes_to_write = 524;
        rspages_page_data_write(&p_write_data);
    }
    loader_MessageSend( LOADER_OK, 0, "");
    Timer_TimerReset(timer);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode205.c;8 */
/*       1*[1792972] 17-FEB-2014 14:55:26 (GMT) BLeguevel */
/*         "Configuration and recording system implemented on the DSP_B downhole code" */
/*       2*[1794435] 29-APR-2014 15:18:27 (GMT) BLeguevel */
/*         "DSP_B code formatting" */
/*       3*[1842524] 27-JUN-2014 16:00:50 (GMT) BLeguevel */
/*         "DSP_B code modifications to match with the new RS485COM module implemented in the common code" */
/*       4*[1852025] 06-AUG-2014 08:08:26 (GMT) BLeguevel */
/*         "Opcode205 modification allowing page switch when the DPpoints[00-02] are requested" */
/*       5*[1867389] 04-NOV-2014 12:02:32 (GMT) BLeguevel */
/*         "Enable page switch  on opcodes 204, 205 and 202." */
/*       6*[1895737] 20-APR-2015 13:18:47 (GMT) SHaworth */
/*         "Rename d-points to specify that they are ACQ\MTC d-points" */
/*       7*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*       8*[1942355] 02-FEB-2016 10:48:08 (GMT) BLeguevel */
/*         "Fix the acqmtc_dsp_b_V2_1 alpha baseline issues" */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode205.c;8 */

// ----------------------------------------------------------------------------
/**
 * @file        acqmtc_dsp_b/src/opcode206.c
 * @author		Benoit LeGuevel
 * @date        10 Oct. 2013
 * @brief       Handles the opcode 206 processing : Write in flash.
 * @details
 * The function is compatible with the Toolscope CRS implementation:
 * Thus the first argument following the opcode is the block identifier
 * which define which device or partition has to be programmed. The other arguments
 * are : the packet size in byte, the address coded on 16 bits and a pointer to the
 * buffer to copy in  the flash.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#include "opcode208.h"
#include "XDImemory.h"
#include "rspages.h"
#include "rspartition.h"

#define BLOCK_ID_OFFSET     	0u		///< Block identifier offset.
#define PACKET_SIZE_OFFSET  	1u		///< Packet size to copy (number of bytes).
#define ADDRESS_LOW_OFFSET  	2u		///< Address LSB offset.
#define ADDRESS_HIGH_OFFSET 	3u		///< Address MSB offset.
#define OPCODE_206_DATA_OFFSET  4u		///< offset to the data to copy in flash.
// Local function declaration
static uint8_t m_write_coeff_SPI_buffer[1024];    //存储校正矩阵
static uint16_t spi_coeffBufferOffset = 73 + 5u;
static uint16_t checkNum = 0;
// ----------------------------------------------------------------------------
/**
 * opcod206 copies the content of the message in a flash memory
 * and send back the write command status.
 * The command format is <206><blockIdentifier><PacketSize><StartAddressLSB><StartAddressMSB><...Data...>
 * The block identifiers 2 and 4 are used to read data from the field
 * and engineering configuration block. These two blocks are located in the
 * configuration memory. The data are written only if the DSP is in COM page
 * The block identifiers [5-36] are used to record the survey and the trajectory
 * data in the Recording_Flash memory.
 *
 * @param   pCommand            Pointer to the command
 * @param   pResponse           Pointer to the response
 */
// ----------------------------------------------------------------------------
void opcode208_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,Timer_t* timer)
{
    const uint16_t blockIdentifier = (uint16_t)message->dataPtr[BLOCK_ID_OFFSET];
    const uint16_t packetSize      = (uint16_t)message->dataPtr[PACKET_SIZE_OFFSET];
    //const uint32_t address         = (uint32_t)message->dataPtr[ADDRESS_LOW_OFFSET] + ((uint32_t)message->dataPtr[ADDRESS_HIGH_OFFSET] << 8);	//lint !e960 pointer arithmetic
    uint8_t*  const pDataByte   = message->dataPtr + OPCODE_206_DATA_OFFSET;
    uint16_t Offset = 5;
    uint16_t configIndex = 0u;
    rs_page_write_t p_write_data;
    switch (blockIdentifier)
    {
        case 0:   //写入加表温度校正
            for(configIndex = 0;configIndex < 86;configIndex++){
                m_write_coeff_SPI_buffer[configIndex] = 0;
            }
            for (configIndex = 0u; configIndex < packetSize; configIndex++)
                {
                    m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = pDataByte[configIndex];
                    checkNum += pDataByte[configIndex];
                    spi_coeffBufferOffset++;
                }
            loader_MessageSend( LOADER_OK, 0, "" );
            break;
        case 1:  //写入加表偏置
            for (configIndex = 0u; configIndex < packetSize; configIndex++)
            {
                m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = pDataByte[configIndex];
                checkNum += pDataByte[configIndex];
                spi_coeffBufferOffset++;
            }
            loader_MessageSend( LOADER_OK, 0, "" );
            break;
        case 2:  //写入磁表温度校正
            for (configIndex = 0u; configIndex < packetSize; configIndex++)
            {
                m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = pDataByte[configIndex];
                checkNum += pDataByte[configIndex];
                spi_coeffBufferOffset++;
            }
            loader_MessageSend( LOADER_OK, 0, "" );
            break;
        case 3:  //写入磁表偏置
            for (configIndex = 0u; configIndex < packetSize; configIndex++)
            {
                m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = pDataByte[configIndex];
                checkNum += pDataByte[configIndex];
                spi_coeffBufferOffset++;
            }
            loader_MessageSend( LOADER_OK, 0, "" );
            break;
        case 4:
            for (configIndex = 0u; configIndex < packetSize; configIndex++)
            {
                m_write_coeff_SPI_buffer[Offset + configIndex] = pDataByte[configIndex];
                checkNum += pDataByte[configIndex];
            }
            checkNum -= pDataByte[16];
            checkNum -= pDataByte[17];
            if(!(checkNum == (pDataByte[16] * 256 + pDataByte[17]))){
                loader_MessageSend( LOADER_VERIFY_FAILED, 0, "" );
                spi_coeffBufferOffset = 73+5;
                checkNum = 0;
           }else{
               loader_MessageSend( LOADER_OK, 0, "" );
           }
            break;
        case 5:
            for (configIndex = 0u; configIndex < packetSize; configIndex++)
            {
                m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = pDataByte[configIndex];
                checkNum += pDataByte[configIndex];
                spi_coeffBufferOffset++;
            }
            if(spi_coeffBufferOffset == 494){
                p_write_data.partition_id = (uint8_t) 0u;
                p_write_data.record_id = (uint16_t) 71u;    //校正矩阵在记录系统中的工作id为  71
                p_write_data.partition_index = rspartition_check_partition_id(0); // 获取分区对应索引值
                p_write_data.partition_logical_start_addr = 0;
                p_write_data.partition_logical_end_addr = 8191;
                p_write_data.next_free_addr = 16;
                p_write_data.p_write_buffer = m_write_coeff_SPI_buffer;
                p_write_data.bytes_to_write = spi_coeffBufferOffset + 3;
                if(rspages_page_data_write(&p_write_data) == RS_PG_WRITE_OK){
                    loader_MessageSend( LOADER_OK, 0, "" );
                }else{
                    loader_MessageSend( LOADER_VERIFY_FAILED, 0, "" );
                }
            }else{
                loader_MessageSend( LOADER_VERIFY_FAILED, 0, "" );
            }
            //bool_t writeStates = XDIMEMORY_WriteRequest(&m_write_coeff_SPI_buffer[0],spi_coeffBufferOffset,RS_QUEUE_REQUEST_IN_PROGRESS);
            spi_coeffBufferOffset = 73+5;
            checkNum = 0;
            break;
        default:  //写入Serial Number 和时间   12-16为时间： 年 月 日 时 分
//            for (configIndex = 0u; configIndex < 24; configIndex++)
//            {
//                if(configIndex == 0){
//                    m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = 0x40;
//                }
//                if(configIndex == 1){
//                    m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = 0xE0;
//                }
//                m_write_coeff_SPI_buffer[spi_coeffBufferOffset] = 0;
//                //spi_coeffBufferOffset++;
//            }
            Offset = 5+18;
            for (configIndex = 0u; configIndex < packetSize; configIndex++)
            {
                m_write_coeff_SPI_buffer[Offset + configIndex] = pDataByte[configIndex];
                checkNum += pDataByte[configIndex];
            }
            checkNum -= pDataByte[16];
            checkNum -= pDataByte[17];
            if(!(checkNum == (pDataByte[16] * 256 + pDataByte[17]))){
                loader_MessageSend( LOADER_VERIFY_FAILED, 0, "" );
                spi_coeffBufferOffset = 73+5;
                checkNum = 0;
                return;
            }else{
                loader_MessageSend( LOADER_OK, 0, "" );
            }
            break;
    }
    Timer_TimerReset(timer);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode206.c;6 */
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
/*       6*[1942355] 02-FEB-2016 10:48:08 (GMT) BLeguevel */
/*         "Fix the acqmtc_dsp_b_V2_1 alpha baseline issues" */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode206.c;6 */

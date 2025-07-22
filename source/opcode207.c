// ----------------------------------------------------------------------------
/**
 * @file        acqmtc_dsp_b/src/opcode207.c
 * @author		Benoit LeGuevel
 * @date        10 Oct. 2013
 * @brief       Handles the opcode 207 processing : Read flash
 * @details
 * The function is compatible with the Toolscope CRS implementation:
 * Thus the first argument following the opcode is the block identifier
 * which define which device or partition has to be read. The other arguments
 * are : the packet size in byte, the address coded on 16 bits.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/

// ----------------------------------------------------------------------------

#include "opcode207.h"
#include "XDImemory.h"
#include "rsapi.h"

#define CPU_CONFIG_DPOINTS_BLOCK  0xFEu	///< CPU Dpoint block identifier.

#define BLOCK_ID_OFFSET     		   0u		///< Block identifier offset.
#define OPCODE_207_PACKET_SIZE_OFFSET  1u		///< Packet size(in bytes) offset.
#define OPCODE_207_ADDRESS_LOW_OFFSET  2u		///< Address LSB offset.
#define OPCODE_207_ADDRESS_HIGH_OFFSET 3u		///< Address MSB offset.
#define DNI_PROM                0xFFu           ///< DnI PROM Toolscope identifier.


// ----------------------------------------------------------------------------
/**
 * opcod207 reads the content of a memory block
 * and send back the data in the buffer pointed by pResponse->bufferPointer
 * The command format is <207><blockIdentifier><PacketSize><StartAddressLSB><StartAddressMSB>.
 * The block identifiers 2 and 4 are used to read data from the field
 * and engineering configuration block. These two block are located in the
 * configuration memory.
 * The block identifiers [5-36] are used to read the survey and the trajectory
 * data in the Recording_Flash memory.
 *
 * @param   pCommand        Pointer to the command
 * @param   pResponse       Pointer to the response
 */

// ----------------------------------------------------------------------------

void opcode207_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,Timer_t* timer)
{
    const uint16_t blockIdentifier = (uint16_t)message->dataPtr[BLOCK_ID_OFFSET] & 0xFFu;                   //lint !e960 pointer arithmetic
    const uint16_t packetSize      = (uint16_t)message->dataPtr[OPCODE_207_PACKET_SIZE_OFFSET] & 0xFFu;     //lint !e960 pointer arithmetic
    const uint32_t address         = (uint32_t)(message->dataPtr[OPCODE_207_ADDRESS_LOW_OFFSET])            //lint !e960 pointer arithmetic
                                       + ((uint32_t)(message->dataPtr[OPCODE_207_ADDRESS_HIGH_OFFSET] ) << 8u); //lint !e960 pointer arithmetic
    uint8_t m_read_coeff_iic_buffer[100] = {0};    //´æ´¢Ð£Õý¾ØÕó
        // if the reception buffer is wide enough to get the answer
        switch (blockIdentifier)
        {
            /*case FIELD_BLOCK :                      // Config flash
                CONFIG_FLASH_read(blockIdentifier, address, packetSize, pDataByte);
                break;

            case ENGINEERING_BLOCK :                // Config flash
                CONFIG_FLASH_read(blockIdentifier, address, packetSize, pDataByte);
                break;

            case SURVEY_PARTITION_ID :              // Recording flash
                RECORDING_FLASH_surveyOrTrajectoryRead(blockIdentifier, address, packetSize, pDataByte);
                break;*/

            case DNI_PROM :
                XDIMEMORY_ReadRequest(&m_read_coeff_iic_buffer[0], packetSize, RS_QUEUE_REQUEST_IN_PROGRESS);
                break;

           /* case CPU_CONFIG_DPOINTS_BLOCK :
                 CONFIG_FLASH_CPUconfigDpointsGet(address, packetSize, pDataByte);
                break;*/

            default :                                 // Recording flash

                // TODO : Might need to compute the address considering the block number and the address
                // as the block number computed on the CPU board is computed assuming the sector size
                // is 64Kbyte (here it is 128Kbyte)
                // Trajectory
                //RECORDING_FLASH_surveyOrTrajectoryRead(blockIdentifier, address, packetSize, pDataByte);
                break;
        }
        uint8_t testData[16] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1};
        loader_MessageSend( LOADER_OK, packetSize, &testData[0]);

    Timer_TimerReset(timer);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode207.c;7 */
/*       1*[1792972] 17-FEB-2014 14:55:26 (GMT) BLeguevel */
/*         "Configuration and recording system implemented on the DSP_B downhole code" */
/*       2*[1794435] 29-APR-2014 15:18:27 (GMT) BLeguevel */
/*         "DSP_B code formatting" */
/*       3*[1842524] 27-JUN-2014 16:00:50 (GMT) BLeguevel */
/*         "DSP_B code modifications to match with the new RS485COM module implemented in the common code" */
/*       4*[1846566] 30-JUL-2014 12:07:39 (GMT) BLeguevel */
/*         "High level code modification in order to fit with the recording flash polling mechanism" */
/*       5*[1852048] 12-AUG-2014 12:13:53 (GMT) BLeguevel */
/*         "Opcode207 modification allowing to read the CPU Dpoints stored in the configFlash.c module" */
/*       6*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*       7*[1942355] 02-FEB-2016 10:48:08 (GMT) BLeguevel */
/*         "Fix the acqmtc_dsp_b_V2_1 alpha baseline issues" */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode207.c;7 */

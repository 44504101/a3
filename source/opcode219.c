// ----------------------------------------------------------------------------
/**
 * @file        acqmtc_dsp_b/src/opcode219.c
 * @author		Benoit LeGuevel
 * @date        04 December 2014
 * @brief       Handles the opcode 219 processing : Dump a Recording Flash segment.
 * @details
 * This opcode is used by Toolscope when the a TSIM1 is connected.
 * The dump speed is then 115200 bauds.
 * Toolscope performs the memory dump requesting 256 word packets.
 * This is called a segment.
 * The segment to dump is identified on two bytes (16 words).
 * This leads to a maximum memory size : 2^16 * 256(words) = 16Mwords, 32Mbytes.
 *
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/

// ----------------------------------------------------------------------------

#include "common_data_types.h"
#include "opcode219.h"
#include "flash_hal.h"
#include "rspartition.h"

#define OPCODE_219_ADDRESS_LOW_OFFSET  	0u		///< Address LSB offset.
#define OPCODE_219_ADDRESS_HIGH_OFFSET 	1u		///< Address MSB offset.
#define OPCODE_219_PACKET_SIZE_OFFSET  	2u		///< Packet size offset.

#define SEGMENT_SIZE_IN_WORDS	512u	///< Segment size in words.

uint8_t responseBuffer[1032];
extern uint8_t selectPartitionIndex;
// ----------------------------------------------------------------------------
/**
 * opcod219 reads the content of a logging memory segment (Fixed or Circular partition segment).
 * Sends back the data in the buffer pointed by pResponse->bufferPointer
 * The command format is <219><StartAddressLSB><StartAddressMSB><PacketSize(words)>.
 *
 * @param   pCommand        Pointer to the command
 * @param   pResponse       Pointer to the response
 */
// ----------------------------------------------------------------------------
void opcode219_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,Timer_t* timer){
	uint16_t 				WordCount;
	uint16_t 				ByteCount;
	flash_hal_error_t   flash_read_status;
	uint32_t address = 0u;

	// Get the number of words to dump
	//WordCount = (uint16_t)message->dataPtr[OPCODE_219_PACKET_SIZE_OFFSET] & 0xFFu;		    //lint !e960 pointer arithmetic
	WordCount = (uint16_t)message->dataPtr[4] & 0xFFu;
	// If the command number of word to dump is 0 the
	// it means that 256 words have to be dumped (see Xceed Toolscope).
	if (0u == WordCount )
	{
		WordCount = 256u;
	}

	ByteCount = WordCount * 2u;

	// Computes the physical address, determine the chip where dumping the data.
//	address = (uint32_t)(message->dataPtr[OPCODE_219_ADDRESS_LOW_OFFSET])								//lint !e960 pointer arithmetic
//	        + ((uint32_t)(message->dataPtr[OPCODE_219_ADDRESS_HIGH_OFFSET] ) << 8u);					//lint !e960 pointer arithmetic

	address = (uint32_t)(message->dataPtr[OPCODE_219_ADDRESS_LOW_OFFSET])                               //lint !e960 pointer arithmetic
	            + ((uint32_t)(message->dataPtr[OPCODE_219_ADDRESS_HIGH_OFFSET] ) << 8u)
	            + ((uint32_t)(message->dataPtr[2] ) << 16u)
	            + ((uint32_t)(message->dataPtr[3] ) << 24u);

	address = address * SEGMENT_SIZE_IN_WORDS;
	const rs_partition_info_t *p_partition = rspartition_partition_ptr_get(selectPartitionIndex);
	address += p_partition->start_address;
    flash_read_status = flash_hal_device_read(address,ByteCount,&responseBuffer[0u]);

	if(flash_read_status == FLASH_HAL_NO_ERROR){
		loader_MessageSend( LOADER_OK, ByteCount, (char*)responseBuffer );
	}
	else                                              // The opcode is not processed
	{
		loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
	}
	Timer_TimerReset(timer);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode219.c;3 */
/*       1*[1873747] 09-DEC-2014 13:45:12 (GMT) BLeguevel */
/*         "Implement the opcode219" */
/*       2*[1900307] 22-MAY-2015 09:31:10 (GMT) BLeguevel */
/*         "Add Doxygen comments." */
/*       3*[1942355] 02-FEB-2016 10:48:08 (GMT) BLeguevel */
/*         "Fix the acqmtc_dsp_b_V2_1 alpha baseline issues" */
/*+- OmniWorks Replacement History - fe_dhs`dev28335`tool`crs`acqmtc_dsp_b`src:opcode219.c;3 */

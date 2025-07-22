/*
 * opcode046.c
 *
 *  Created on: 2022年5月27日
 *      Author: l
 */

#include "common_data_types.h"
#include "loader_state.h"
#include "timer.h"
#include "comm.h"
#include "opcode046.h"
#include "tool_specific_config.h"
#include "tool_specific_hardware.h"
#include "rspartition.h"
#include "flash_hal.h"
#include "sci.h"
#include "buffer_utils.h"
#include "crc.h"
#include "iocontrolcommon.h"
// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define CMD_TYPE_OFFSET     				0u							///< Command type offset.
#define SEND_CMD_OFFSET     				1u							///< Send command offset.

#define FAST_DUMP_START_CMD					2u							///< Start fast dump command value.
#define END_DUMP							3u							///< End of dump command value.
#define SEND_PACKET_CMD						4u							///< Send packet command value.
#define ANOTHER_SEND_PACKET_CMD				5u							///< Another send packet command value.

#define START_DUMP_ADDRESS					0x07310000u					///< Start memory dump address.120651776

#define MEMORY_PAGE_SIZE					256u                      	///< Recording memory page size (in words).
#define TRANSMIT_BUFFER_SIZE    			(MEMORY_PAGE_SIZE * 2u)   	///< Transmit buffer size (in bytes).
#define EXTRA_BYTE_NUMBER       			11u						  	///< Number of extra bytes to transmit.
#define START_CHAR              			0x01u						///< Message start character.
#define STOP_CHAR              				0x1Au						///< Message last character.
#define INITIAL_CRC_VALUE      				0x00u						///< CRC initial value.
#define SLAVE_ADRESS_DSP_B      			0xFDu						///< DSP_B slave address.

#define RECORDING_SYSTEM_STOP_TICK_TIMEOUT  20u		  					///< Default recording system stop timeout in tick counts.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static sendFrameState_t fast_dump_sendFrameRun(const uint8_t* const pMessage);
static void 			fast_dump_initialise(const Ebaud_rate_t baudRate);

static uint16_t			sentCommandDecode(const uint8_t* const pMessage,
									uint32_t* const pLogicalAddress,
									uint16_t* const pCrc);
static void 			transmitBuffer_Initialise(uint32_t* const pLogicalAddress,
									  	  	  	  const uint32_t ByteNumber,
												  uint8_t* const pTransmitBuffer,
												  uint16_t* const pCrc);
static void 			lastFrame_Transmit(uint8_t* const pTransmitBuffer, const uint16_t crc);

void SSB_BufferTransmitStart(const uint8_t * const p_bufferToTransmit,const uint16_t numberOfBytesToTransmit);
// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/// Fast dump state machine.
//lint -e{956} Doesn't need to be volatile.
static sendFrameState_t mState = FAST_DUMP_INITIAL;

/// Logging memory physical address.
//lint -e{956} Doesn't need to be volatile.
static uint32_t		mLogicalAddress;

/// Number of read sequences.
//lint -e{956} Doesn't need to be volatile.
static uint16_t     mNumberOfReads;

/// Current DSP_B baud rate.
//lint -e{956} Doesn't need to be volatile.
static Ebaud_rate_t mCurrentBaudRate;

/// Fast dump baud rate.
//lint -e{956} Doesn't need to be volatile.
static Ebaud_rate_t mFastDumpRate;

/// Memory dump start address.
//lint -e{956} Doesn't need to be volatile.
static uint32_t    	mLoggingMemoryStartAddress;

/// Pointer to the temporary buffer location used as "even frame" buffer.
//lint -e{956} Doesn't need to be volatile.
static uint8_t*    mpEvenTransmitBuffer 		= NULL;

/// Pointer to the temporary buffer location used as "odd frame" buffer.
//lint -e{956} Doesn't need to be volatile.
static uint8_t*    mpOddTransmitBuffer  		= NULL;

/// Computed crc.
//lint -e{956} Doesn't need to be volatile.
static uint16_t    mCrc 						= INITIAL_CRC_VALUE;

static uint8_t buffer[512];     //传输数据缓冲块
static uint8_t mTemporaryBuffer[1032];
uint8_t selectPartitionIndex;
void opcode46_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
        Timer_t* timer){
	const uint8_t commandType          = message->dataPtr[CMD_TYPE_OFFSET];
	const uint8_t* const pSendCommand  = &message->dataPtr[SEND_CMD_OFFSET];
	const uint8_t baudIndex  = message->dataPtr[SEND_CMD_OFFSET];
	uint32_t DumpRate;
	switch (commandType)
	{
	    case SEND_CMD_OFFSET:
	        //fast_dump_initialise(921600);
	        switch(baudIndex){
	        case 0:
	            DumpRate = 4800;
	            break;
	        case 1:
	            DumpRate = 9600;
	                       break;
	        case 2:
	            DumpRate = 19200;
	                        break;
	        case 3:
	            DumpRate = 38400;
	                        break;
	        case 4:
	            DumpRate = 57600;
	                        break;
	        case 5:
	            DumpRate = 76800;
	                        break;
	        case 6:
	            DumpRate = 115200;
	                        break;
	        case 7:
	            DumpRate = 921600;
	                        break;
	        default:
	            DumpRate = 57600;
	            break;
	        }

	        if(!SCI_BaudRateSet(SCI_B, 58982400u, (uint32_t)DumpRate)){
	            loader_MessageSend( LOADER_INVALID_MESSAGE, 0, "");
	        }else{
	            loader_MessageSend( LOADER_OK, 0, "" );
	        }
	    break;
		case FAST_DUMP_START_CMD:

		    // Set the dump speed to 921600 Bytes/second.
			//fast_dump_initialise(SSB_TX_BAUD_RATE_921600);


			// We use the temporary buffer as two ping-pong buffers, we first make sure its size fits with the needs of the opcode.
			if (1)//pCommand->TemporaryBufferSize >= (2u * (TRANSMIT_BUFFER_SIZE + 3u)) )
			{
				//Assign the Odd and the Even buffer pointers to some temporary buffer locations.
				mpOddTransmitBuffer 	= &mTemporaryBuffer[0u];
				mpEvenTransmitBuffer 	= &mTemporaryBuffer[TRANSMIT_BUFFER_SIZE + 3u];
			    loader_MessageSend( LOADER_OK, 0, "" );
			}
			else
			{
				// Just avoid problem with the unit tests.
				mpOddTransmitBuffer 	= NULL;
				mpEvenTransmitBuffer 	= NULL;

				// At that stage Toolscope should stop the dump process,
				loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "");
			}
		break;


		case END_DUMP:
			// Set the RS485 speed back to their original values.
		    if(!SCI_BaudRateSet(SCI_B, 58982400u, (uint32_t)mCurrentBaudRate)){
                loader_MessageSend( LOADER_INVALID_MESSAGE, 0, "");
            }else{
                loader_MessageSend( LOADER_OK, 0, "" );
            }
		break;

		case SEND_PACKET_CMD:
			// In case Toolscope continues the Dump process.
			// One test should be sufficient as the two buffers are assigned together.
			if ( (mpOddTransmitBuffer != NULL) && (mpEvenTransmitBuffer != NULL) )
			{
				// Loop until the complete frame is completed.
				while (FAST_DUMP_INITIAL != fast_dump_sendFrameRun(pSendCommand))
				{
					; // Just loop.
				}

				// No answer except the dump frame is expected.
			}
			else
			{
			    loader_MessageSend( LOADER_PARAMETER_OUT_OF_RANGE, 0, "" );
			}
		break;

		case ANOTHER_SEND_PACKET_CMD:
		{
		    const uint8_t index  = message->dataPtr[SEND_CMD_OFFSET];
		    const rs_partition_info_t* p_partition = rspartition_partition_ptr_get(index);
		    selectPartitionIndex = index;
			buffer[0] = (uint8_t)p_partition->id;
			BUFFER_UTILS_Uint32To8bitBuf(&buffer[1],p_partition->next_available_address);
			Uint16 length = 5;
			loader_MessageSend( LOADER_OK, length, (char*)buffer );
		}
		break;
		default :
			// Nothing, just need a comment to make Lint happy.

		break;
	}
	// reset the timer
    Timer_TimerReset(timer);
}


// ------------------------------------------------------------------------
/**
 * FAST_DUMP_initialise sets the fast dump baud rate value, gets the current baud rate,
 * gets the logging memory start address.
 *
 * @param       baudRate        baud rate set on the opcode 0x2E 0x01.
 */
// ------------------------------------------------------------------------
static void fast_dump_initialise(const Ebaud_rate_t baudRate)
{
	// Set the logging memory start address.
	mLoggingMemoryStartAddress 	   = START_DUMP_ADDRESS;

    mFastDumpRate                  = baudRate;
    mCurrentBaudRate               = SSB_BusBaudRateGet();
}


// ------------------------------------------------------------------------
/**
 * fast_dump_sendFrameRun manages the different steps of a logging memory dump transmission.
 *
 * @note
 * This is not the most efficient way to process
 * a dump command but it allows to check
 * the parameters at each cycle and so it suits well with TDD.
 *
 * @param       pMessage    Pointer to the message received.
 * @retval      state       Packet transmission state.
 */
// ------------------------------------------------------------------------
static sendFrameState_t fast_dump_sendFrameRun(const uint8_t* const pMessage)

{
    switch (mState)
    {
        case FAST_DUMP_INITIAL:
            //SSB_BusBaudRateSet(mFastDumpRate);
            mCrc         	= INITIAL_CRC_VALUE;
            mNumberOfReads 	= sentCommandDecode(pMessage,&mLogicalAddress, &mCrc);
            mState        	= FAST_DUMP_FIRST_FRAME;
            break;

        case FAST_DUMP_FIRST_FRAME:
            //lint -e{921} Cast to uint32_t to avoid prototype coercion.
            transmitBuffer_Initialise(&mLogicalAddress,
                                      (uint32_t)TRANSMIT_BUFFER_SIZE,
            						  &mpEvenTransmitBuffer[0u],
            						  &mCrc);
            mNumberOfReads--;
            //loader_MessageSend( LOADER_OK, 517, mpOddTransmitBuffer[0u] );
            SSB_BufferTransmitStart(&mpOddTransmitBuffer[0u], 10u);	   	// send the 10 first bytes.
            mState = FAST_DUMP_EVEN_FRAME;
            break;

        case FAST_DUMP_EVEN_FRAME:
        	// freeRTOS waiting for the end of transmission event.
        	if (SSB_TransmitDoneCheckAndWait())
        	{
        		//Send new frame
        	    SSB_BufferTransmitStart(&mpEvenTransmitBuffer[0u],
        		                        TRANSMIT_BUFFER_SIZE);

        		//lint -e{921} Cast to uint32_t to avoid prototype coercion.
        		transmitBuffer_Initialise(&mLogicalAddress,
        		                          (uint32_t)TRANSMIT_BUFFER_SIZE,
        								  &mpOddTransmitBuffer[0u],
        								  &mCrc);
        		mNumberOfReads--;

        		/*
        		 * The last frame is always sent from the odd buffer since the
        		 * packet size is a multiple of 512 (words) and the page size
        		 * is 256 (words)
        		 */
        		if ( 0u == mNumberOfReads )
        		{
        			mState = FAST_DUMP_LAST_FRAME;

        			lastFrame_Transmit(&mpOddTransmitBuffer[0u], mCrc);
        		}
        		else
        		{
        			mState = FAST_DUMP_ODD_FRAME;
        		}
        	}
        	break;

        case FAST_DUMP_ODD_FRAME:
        	// freeRTOS waiting for the end of transmission event.
        	if (SSB_TransmitDoneCheckAndWait())
        	{
        		//Send new frame
        	    SSB_BufferTransmitStart(&mpOddTransmitBuffer[0u],
        		                        TRANSMIT_BUFFER_SIZE);

        		//lint -e{921} Cast to uint32_t to avoid prototype coercion.
        		transmitBuffer_Initialise(&mLogicalAddress,
        		                          (uint32_t)TRANSMIT_BUFFER_SIZE,
        								  &mpEvenTransmitBuffer[0u],
        								  &mCrc);
        		mNumberOfReads--;


        		// 雷戈修改，在这块应该也判断一下当前是否把数据读取完毕
        		/*if ( 0u == mNumberOfReads )
                {
                    mState = FAST_DUMP_LAST_FRAME;

                    lastFrame_Transmit(&mpOddTransmitBuffer[0u], mCrc);
                }
                else
                {
                    transmitBuffer_Initialise(&mLogicalAddress,
                                                              (uint32_t)TRANSMIT_BUFFER_SIZE,
                                                              &mpEvenTransmitBuffer[0u],
                                                              &mCrc);
                    mState = FAST_DUMP_EVEN_FRAME;
                }*/
        		mState = FAST_DUMP_EVEN_FRAME;
        	}
        	break;

        case FAST_DUMP_LAST_FRAME:
        	// freeRTOS waiting for the end of transmission event.
        	if (SSB_TransmitDoneCheckAndWait())
        	{
        		//Send last frame
        		// + 3 extra characters : <CRC_MSB><CRC_LSB><CTRL_Z>
        	    SSB_BufferTransmitStart(&mpOddTransmitBuffer[0u],
        		                        (TRANSMIT_BUFFER_SIZE + 3u));

        		mState = FAST_DUMP_END;
        	}
        	break;

        case FAST_DUMP_END:
        	// freeRTOS waiting for the end of transmission event.
        	if (SSB_TransmitDoneCheckAndWait())
        	{

        	    SSB_BusInReceiveModeSet();
        		mState = FAST_DUMP_INITIAL;
        	}
        	break;

        default :
        	// Nothing, just need a comment here for Lint.
            break;
    }

    return mState;
}


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Local function definition
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
/**
 * lastFrame_Transmit sends the last frame, adds to the message the last bytes; CRC and CTRL_Z.
 *
 * @param pTransmitBuffer     Pointer to the transmit buffer.
 * @param crc                 CRC value.
 */
// ------------------------------------------------------------------------
//lint -e{921} Cast to uint8_t as buffer holds uint8_t's
static void lastFrame_Transmit(uint8_t* const pTransmitBuffer, const uint16_t crc)
{
    pTransmitBuffer[TRANSMIT_BUFFER_SIZE]      = (uint8_t)( (crc & 0xFF00u) >> 8u );    //CRC MSB
    pTransmitBuffer[TRANSMIT_BUFFER_SIZE + 1u] = (uint8_t)(crc & 0x00FFu);              //CRC LSB
    pTransmitBuffer[TRANSMIT_BUFFER_SIZE + 2u] = STOP_CHAR;
}


// ------------------------------------------------------------------------
/**
 * sentCommandDecode gets the Toolscope command and computes the start address and the frame size.
 * Initialises the 10 first characters of the frame pMessage[0] frame size (Kbyte).
 *
 * @note
 * Toolscope always requests a number of Kbyte (512 words minimum).
 * As the MEMORY_PAGE_SIZE is 256 words, the number of read is equal
 * to NumberOfWordRequested/MEMORY_PAGE_SIZE. The rest of  the division
 * is NULL.
 * @note
 * The address sent by Tooslcope is a byte address, it needs to be converted in a word
 * address in order to read the recording flash data.
 *
 * @param       pMessage            Pointer to the received message (command).
 * @param       pLogicalAddress     Pointer to the memory logical address.
 * @param       pCrc                Pointer to the crc value.
 * @retval      mNumberOfReads      Number of read sequences.
 */
// ------------------------------------------------------------------------
static uint16_t sentCommandDecode(const uint8_t* const pMessage,
                                  uint32_t* const pLogicalAddress,
                                  uint16_t* const pCrc)
{
    // Packet size and start address computation
    const uint16_t ByteToRead   = pMessage[0u] * 1024u;

    const uint16_t byteCount    = ByteToRead + EXTRA_BYTE_NUMBER;

    //lint -e{921} Casts to avoid violating essential type mode.
    uint32_t startAddress = ((uint32_t)pMessage[3u] << 24u);

    //lint -e{921} Casts to avoid violating essential type mode.
    startAddress +=         ((uint32_t)pMessage[2u] << 16u);

    //lint -e{921} Casts to avoid violating essential type mode.
    startAddress +=         ((uint32_t)pMessage[1u] << 8u);

    mNumberOfReads = ByteToRead / TRANSMIT_BUFFER_SIZE;

    // Computes the physical address
    *pLogicalAddress = mLoggingMemoryStartAddress + startAddress;

    // Initialise the 10 first bytes of the packet.
    mpOddTransmitBuffer[0u] = START_CHAR;								// Start character.
    mpOddTransmitBuffer[1u] = SLAVE_ADRESS_DSP_B; 						// Slave address.

    // Add the byte count to the buffer, LSB then MSB.
    //lint -e{920} Ignoring return value, not used here.
    (void)BUFFER_UTILS_Uint16To8bitBuf(&mpOddTransmitBuffer[2u], byteCount);

    mpOddTransmitBuffer[4u] = 0u;
    mpOddTransmitBuffer[5u] = pMessage[0u];								// Packet size.
    mpOddTransmitBuffer[6u] = pMessage[1u]; 							// Address, byte 1.
    mpOddTransmitBuffer[7u] = pMessage[2u]; 							// Address, byte 2.
    mpOddTransmitBuffer[8u] = pMessage[3u]; 							// Address, byte 3.
    mpOddTransmitBuffer[9u] = pMessage[4u]; 							// Address, byte 4.

    // Compute the CRC on the 10 first bytes.
    //lint -e{921} Cast to uint32_t to avoid prototype coercion.
    /**pCrc = CheckNum_Calculate(&mpOddTransmitBuffer[0u],
                                     (uint32_t)10u,
                                     *pCrc);*/
    *pCrc = CRC_CCITTOnByteCalculate(&mpOddTransmitBuffer[0u],
                                     (uint32_t)10u,
                                     *pCrc);
    return mNumberOfReads;
}


// ------------------------------------------------------------------------
/**
 * transmitBuffer_Initialise reads the content of the recording memory,
 * computes the crc and  updates the next physical address.
 *
 * @param   pPhysicalAddress    Recording memory address.
 * @param   ByteNumber          Number of bytes to read.
 * @param   pTransmitBuffer     Pointer to the transmission buffer.
 * @param   pCrc                Pointer to the CRC value.
 */
// ------------------------------------------------------------------------
static void transmitBuffer_Initialise(uint32_t* const pLogicalAddress,
                                      const uint32_t ByteNumber,
                                      uint8_t* const pTransmitBuffer,
                                      uint16_t* const pCrc)
{
	// Read the data from the logging flash
	//lint -e{920} CAst from enum to void - discard the return value.
	(void)flash_hal_device_read(*pLogicalAddress, ByteNumber, pTransmitBuffer);

	// Compute the CRC.
	//*pCrc = CheckNum_Calculate(pTransmitBuffer, ByteNumber, *pCrc);
	*pCrc = CRC_CCITTOnByteCalculate(pTransmitBuffer, ByteNumber, *pCrc);

	// We assume that the logging flash is word addressable
	*pLogicalAddress += ByteNumber;
}

void SSB_BufferTransmitStart(const uint8_t * const p_bufferToTransmit,
                             const uint16_t numberOfBytesToTransmit)
{
    // Set the bus in Tx mode.设置总线（bus）为传输模式
    IOCONTROLCOMMON_RS485ReceiverDisable();  //接收失能（关）

    //lint -e{920} cast from int to void, returned value not used.
    //lint -e{921} cast from uint32_t to uint32_t in MASTER_CLOCK_FREQUENCY.
/*    (void)SCI_BaudRateSet(SCI_B,
                          (uint32_t)58982400u,
                          m_ssbBaudRates[m_ssb.baudRateIdentifier].transmitterBaudRate);*/

    IOCONTROLCOMMON_RS485TransmitterEnable(); //传输使能（开）

    // Initiate the transmission. 初始化传输
    SCI_TxStart(SCI_B, p_bufferToTransmit, numberOfBytesToTransmit);
}

bool_t SSB_TransmitDoneCheckAndWait(void)
{
    bool_t  b_transmitDone = FALSE;
    int16_t status = FALSE;

    // 100mS timeout - worst case.
    Timer_Wait(50);
    /*
     * If the status from the semaphore is OK then poll for transmit
     * complete.  This isn't super efficient, but by this point there is
     * only a single character which is being moved through the shift resister,
     * worst case for this is 174uS at 57600 baud.  SSB is only used on the
     * surface, so the CPU isn't busy, so polling isn't so bad here.
     */
    if (1 == status)
    {
        while (!b_transmitDone)
        {
            b_transmitDone = SCI_TxDoneCheck(SCI_B);
        }
    }
    /* Otherwise just check once. */
    else
    {
        b_transmitDone = SCI_TxDoneCheck(SCI_B);
    }

    return b_transmitDone;
}

void SSB_BusInReceiveModeSet(void)
{
    // Disable the transmitter.
    IOCONTROLCOMMON_RS485TransmitterDisable();

    //lint -e{920} cast from int to void, returned value not used.
    //lint -e{921} cast from uint32_t to uint32_t in MASTER_CLOCK_FREQUENCY.
/*    (void)SCI_BaudRateSet(SCI_B,
                          (uint32_t)58982400u,
                          m_ssbBaudRates[m_ssb.baudRateIdentifier].receiverBaudRate);*/

    //SCI_RxBufferInitialise(SCI_B, (uint8_t*)mSSBReceiveInterruptBuffer, 513u);

    // Enable the receiver.
    IOCONTROLCOMMON_RS485ReceiverEnable();
}

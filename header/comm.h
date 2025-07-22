/*******************************************************************
 *
 *    DESCRIPTION: Handles transmission and buffering of opcode
 *					commands.
 *
 *    AUTHOR: Sterling Wei (based on Scott DiPasque's portable bootloader)
 *
 *    HISTORY:
 *
 *******************************************************************/
#ifndef COMM_H_INCLUDED
#define COMM_H_INCLUDED
#include "common_data_types.h"
// Status codes

/** Message received is valid, and no error occured as a result of the message */
#define LOADER_OK						0u

/** Full message received, but opcode is invalid */
#define LOADER_INVALID_OPCODE			2u

/** Message was corrupted upon reception */
#define LOADER_INVALID_MESSAGE			3u

/** Communications timeout occurred while waiting for character */
#define LOADER_TIMEOUT					4u

/** Response to opcode39, subfield 1 indicating that memory erasure or programming is currently in progress */
#define LOADER_FORMAT_IN_PROGRESS		6u

/** Cannot format (acquisition is enables)*/
#define LOADER_CANNOT_FORMAT			7u

/** Full message received, but the subfield is of an unexpected size */
#define LOADER_WRONG_NUM_PARAMETERS		9u

/**  Full canopen message received but length greater thann 0x1E */
#define LOADER_CAN_LENGTH_ERR			5u

/**  Full canopen message received but checksum error */
#define LOADER_CAN_CKS_ERR				10u

/** Full message received, but one of the values in the subfield is outside of the expected range */
#define LOADER_PARAMETER_OUT_OF_RANGE	27u

/** Full message received, but CRC or checksum verification requested by the message failed */
#define LOADER_VERIFY_FAILED			29u


typedef enum
{
    MESSAGE_OK,         // A message was fully received and verified.
    MESSAGE_ERROR,      // A message was received, but was in error (checksum or address).
    MESSAGE_TIMEOUT,   	// No message was received before the timeout period ended.
    MESSAGE_INCOMPLETE	// Full message not received yet (generally from debug port).
} EMessageStatus_t;

typedef enum
{
    BUS_SSB,
    BUS_ISB,
    BUS_CAN,
    BUS_DEBUG,
    BUS_UNDEFINED
}EBusType_t;

typedef struct OpcodePacket
{
    unsigned char address;      // Slave address
    Uint16 length;              // Length of the message
    unsigned char opcode;       // Opcode of the command
    Uint16 dataLengthInBytes;   // Length in bytes of the subfield of the message
    unsigned char* dataPtr;     // Array of bytes of the subfield of the message
    Uint16 checksum;            // checksum of the message
} OpcodePacket_t;


/*typedef struct OpcodePacket
{
    unsigned char address;      // 从机地址
    Uint16 length;              // 消息长度
    unsigned char opcode;       // 操作码
    Uint16 dataLengthInBytes;   // 帧中操作字段的长度
    unsigned char* dataPtr;     // 存放帧中操作数据地址
    Uint16 checksum;            // 消息校验和
} OpcodePacket_t;   */

typedef OpcodePacket_t LoaderMessage_t;

/** Maximum length of a message for SSB or CAN message*/
#define COMM_MAX_LENGTH 512

// Global variables:
extern EBusType_t 		gBusCOM;
extern unsigned char 	gRxBuffer[COMM_MAX_LENGTH];

// Function prototypes:
LoaderMessage_t* 	loader_waitForMessage(Timer_t *timer);
void 				loader_MessageSend(Uint8 Status, Uint16 LengthOfDataInBytes, char* pData);


#endif

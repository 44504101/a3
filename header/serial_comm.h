#ifndef _ssbcomm
#define _ssbcomm
/*******************************************************************
 *
 *    DESCRIPTION: Handles transmission and buffering of opcode
 *					commands.  Does not interpret opcodes
 *
 *    AUTHOR: Sterling Wei (based on code by Scott DiPasquale)
 *
 *    HISTORY:
 *
 *******************************************************************/

#define SERIAL_STARTCHAR                    0x01    // Start character
#define SERIAL_ENDCHAR                      0x1A    // End character
#define SERIAL_MAX_LENGTH                   512     // Maximum length of a message
#define SERIAL_HEADER_LENGTH                6u      // Length of header, including checksum
#define COMM_TIMEOUT                        10u     // 10 mS wait for any character other than SERIAL_STARTCHAR before timing out
#define RS485_ENPIN_TOGGLE_TO_RX_DELAY      8u      // 8 milli-seconds wait (spec require minimum 7 ms)


LoaderMessage_t*    serial_LoaderMessagePointerGet(void);
bool_t              serial_StartCharacterReceivedCheck(EBusType_t busType);
EMessageStatus_t 	serial_MessageWait(Timer_t* pExternalTimer, bool_t bFoundStartCharacterAlready, EBusType_t busType);
void                serial_MessageSend(Uint8 status, Uint16 length, char * data, EBusType_t busType);
const Timer_t* 		serial_CommTimerPointerGet(void);
void                serial_SlaveAddressSet(uint8_t NewAddress, EBusType_t busType);
void                serial_AltSlaveAddressSet(const uint8_t NewAddress, const EBusType_t busType);
uint8_t             serial_SlaveAddressGet(EBusType_t busType);

#endif

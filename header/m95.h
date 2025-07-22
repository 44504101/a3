/*
 * m95.h
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

#ifndef HEADER_M95_H_
#define HEADER_M95_H_

/// Enumerated list of the possible M95 memory status on a write operation.
typedef enum
{
	M95_POLL_UNKNOWN_ERROR 			= 0,		///< Unknown error.
	M95_POLL_NO_WRITE_IN_PROGRESS 	= 1,		///< No write error.
	M95_POLL_TIMEOUT_EXCEEDED 		= 2			///< Timeout.
} EM95PollStatus_t;

void 		M95_WriteEnableCommandSend(void);
void 		M95_WriteDisableCommandSend(void);
uint8_t 	M95_ReadStatusRegCommandSend(void);
void 		M95_WriteStatusRegCommandSend(const uint8_t NewStatus);
void		M95_ReadCommandSend(const uint32_t StartAddress,
									const uint32_t NumberOfReads,
									uint8_t * const p_dest_buffer);
void 		M95_WriteCommandSend(const uint32_t StartAddress,
									const uint32_t NumberOfWrites,
									const uint8_t * const p_source_buffer);
bool_t 		M95_ReadIDCommandSend(const uint32_t StartAddress,
						    		const uint32_t NumberOfReads,
									uint8_t * const p_dest_buffer);

EM95PollStatus_t M95_WriteCompletePoll(void);

void        M95_BlockRead(const uint32_t StartAddress,
                            const uint32_t NumberOfReads,
                            uint8_t * const p_dest_buffer);

EM95PollStatus_t M95_BlockWrite(const uint32_t StartAddress,
                                    const uint32_t NumberOfWrites,
                                    const uint8_t * const p_source_buffer);

//lint -e{956} Doesn't need to be volatile.
extern EM95PollStatus_t (*M95_memcpy)(uint32_t StartAddress,
                                      uint32_t NumberOfWrites,
                                      const uint8_t * const p_source_buffer);

void 		M95_DeviceSizeInitialise(const uint32_t PageSizeInBytes,
										const uint32_t DeviceSizeInBytes);

uint32_t 			M95_DevicePageSizeGet(void);
uint32_t 			M95_DeviceTotalSizeGet(void);
EM95PollStatus_t	M95_DeviceErase(void);

void                M95_ForceTimeoutFlagSet(void);

#endif /* HEADER_M95_H_ */

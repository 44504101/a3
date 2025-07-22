/*
 * x24lc32a.h
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

#ifndef HEADER_X24LC32A_H_
#define HEADER_X24LC32A_H_

#include "i2c.h"

EI2CStatus_t	X24LC32A_BlockRead(const uint32_t StartAddress,
									const uint16_t NumberOfReads,
									uint8_t * const p_destination_buffer);

EI2CStatus_t	X24LC32A_BlockWrite(const uint32_t StartAddress,
									const uint16_t NumberOfWrites,
									const uint8_t * const p_source_buffer);

//lint -e{956} Doesn't need to be volatile.
extern EI2CStatus_t	(*X24LC32A_memcpy)(uint32_t StartAddress,
            	                     uint16_t NumberOfWrites,
								     const uint8_t * const p_source_buffer);

EI2CStatus_t	X24LC32A_DeviceErase(void);

void            X24LC32A_ForceTimeoutFlagSet(void);

#endif /* HEADER_X24LC32A_H_ */

// ----------------------------------------------------------------------------
/**
 * @file        XDImemory.h
 * @author      Fei Li.com
 * @date        October 2016
 * @brief       Header file for XDImemory.c
 * @note        Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef XDI_MEMORY_H_
#define XDI_MEMORY_H_

#include "rsapi.h"

bool_t XDIMEMORY_ReadRequest(uint8_t * const p_readBuffer,
                             uint16_t * const p_readLength,
                             rs_queue_status_t * const p_readStatus);

bool_t XDIMEMORY_WriteRequest(uint8_t * const p_writeBuffer,
                              const uint16_t numberOfBytesToWrite,
                              rs_queue_status_t * const p_writeStatus);

bool_t XDIMEMORY_EraseRequest(void);

#endif /* XDI_MEMORY_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

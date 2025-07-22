// ----------------------------------------------------------------------------
/**
 * @file    common/src/genericIO.h
 * @author  Fei Li (LIF@xsyu.edu.cn)
 * @brief   Header file for genericIO.c
 * @note	Please refer to the .c file for a detailed functional description.
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 */
// ----------------------------------------------------------------------------
#ifndef genericIO_H_
#define genericIO_H_

//lint -e{956} Doesn't need to be volatile.
extern void (*genericIO_16bitWrite)(const uint32_t address, const uint16_t data);

//lint -e{956} Doesn't need to be volatile.
extern uint16_t (*genericIO_16bitRead)(const uint32_t address);

//lint -e{956} Doesn't need to be volatile.
extern void (*genericIO_32bitWrite)(const uint32_t address, const uint32_t data);

//lint -e{956} Doesn't need to be volatile.
extern uint32_t (*genericIO_32bitRead)(const uint32_t address);

void genericIO_16bitMaskBitSet(const uint32_t address, const uint16_t mask);
void genericIO_16bitMaskBitClear(const uint32_t address, const uint16_t mask);
void genericIO_32bitMaskBitSet(const uint32_t address, const uint32_t mask);
void genericIO_32bitMaskBitClear(const uint32_t address, const uint32_t mask);

#endif /* genericIO_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


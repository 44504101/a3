/*
 * extflash.h
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

#ifndef HEADER_EXTFLASH_H_
#define HEADER_EXTFLASH_H_

extern	uint16_t (*EXTFLASH_ExternalFlashRead)(const uint32_t address);   			///< Pointer to the external flash read function.
extern	void (*EXTFLASH_ExternalFlashWrite)(uint32_t address, const uint16_t data); ///< Pointer to the external flash write function.


#endif /* HEADER_EXTFLASH_H_ */

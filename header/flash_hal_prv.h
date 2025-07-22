/*
 * flash_hal_prv.h
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

#ifndef HEADER_FLASH_HAL_PRV_H_
#define HEADER_FLASH_HAL_PRV_H_

typedef struct
{
    storage_devices_t   device_to_use;
    uint32_t            logical_start_address;
    uint32_t            logical_end_address;
    uint32_t            physical_start_address;
    uint32_t            physical_end_address;
    uint32_t            physical_address_adjustment;
} address_translation_t;

extern const address_translation_t* flash_hal_address_trans_ptr_get(const uint16_t partition_index);

#endif /* HEADER_FLASH_HAL_PRV_H_ */

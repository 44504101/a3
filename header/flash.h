// ----------------------------------------------------------------------------
/**
 * @file    	acqmtc_dsp_b/src/flash.h
 * @author		Fei Li
 * @date		4 Dec 2012
 * @brief		Header file for flash.c
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Xi'an Shiyou Univ., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef FLASH_H_
#define FLASH_H_

#include "common_data_types.h"

/// Enumerated list of the flash devices.
typedef enum FLASHDeviceNumber
{
	FLASH_DEVICE_PARALLEL 	= 0,	///< Parallel flash.
	FLASH_DEVICE_SERIAL		= 1,	///< SPI flash.
	FLASH_DEVICE_EEPROM 	= 2		///< I2C flash.
} EFLASHDeviceNumber_t;

/// Enumerated flash write status.
typedef enum FLASHProgramStatus
{
	FLASH_PROGRAM_UNKNOWN_ERROR 		= 0,	///< Unknown error.
	FLASH_PROGRAM_OK 					= 1,	///< Program Ok.
	FLASH_PROGRAM_ERROR 				= 2,	///< Program error.
	FLASH_PROGRAM_SECTOR_LOCKED_ERROR 	= 3,	///< Sector locked.
	FLASH_PROGRAM_UNKNOWN_DEVICE 		= 4		///< Unknown device.
} EFLASHProgramStatus_t;

/// Enumerated flash poll status.
typedef enum FLASHPollStatus
{
	FLASH_POLL_NOT_BUSY,						///< Flash free.
	FLASH_POLL_BUSY,							///< Flash busy.
	FLASH_POLL_ERASE_SUSPENDED,					///< Flash erase suspended.
	FLASH_POLL_ERASE_FAIL,						///< Flash erase failure.
	FLASH_POLL_PROGRAM_FAIL,					///< Flash program failure.
	FLASH_POLL_PROGRAM_ABORTED,					///< Flash program aborted.
	FLASH_POLL_PROGRAM_SUSPENDED,				///< Flash program suspended.
	FLASH_POLL_SECTOR_LOCKED					///< Flash sector locked.
} EFLASHPollStatus_t;

void					FLASH_Read(const EFLASHDeviceNumber_t device,
										const uint32_t address,
										const uint32_t WordCount,
										void* pData);

EFLASHProgramStatus_t	FLASH_Write(const EFLASHDeviceNumber_t device,
										const uint32_t address,
										const uint32_t WordCount,
										void* pData);

EFLASHPollStatus_t 		FLASH_Erase(const EFLASHDeviceNumber_t DeviceType,
										const uint16_t DeviceNumber);

EFLASHPollStatus_t		FLASH_ExternalFlashPoll(const uint16_t DeviceNumber);

#endif /* FLASH_H_ */

// ----------------------------------------------------------------------------
/**
 * @file      acqmtc_dsp_b/src/configFlash.h
 * @author	  Fei Li
 * @date      27 August 2013
 * @brief     Header file for configFlash.c
 * @note      Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Xi'an Shiyou Univ., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */

// ----------------------------------------------------------------------------

#ifndef CONFIG_FLASH_H
#define CONFIG_FLASH_H

#include "common_data_types.h"
#include "flash.h"

#define FIELD_BLOCK                         2u					///< Field block identifier.
#define ENGINEERING_BLOCK                   4u					///< Engineering block identifier.

// Field block : 17 pages, 1 for the header, 16 for the data
#define FIELD_CONFIG_START_ADDRESS          0u					///< Start address of the Field block.
#define FIELD_CONFIG_END_ADDRESS            0x10FFu				///< End address of the Field block.

// Engineering block : 17 pages, 1 for the header, 16 for the data
#define ENGINEERING_CONFIG_START_ADDRESS    0x1100u				///< Start address of the Engineering block.
#define ENGINEERING_CONFIG_END_ADDRESS      0x21FFu				///< End address of the Engineering block.
#define CONFIGURATION_DATA_OFFSET           (uint32_t)0x100u	///< Offset from which the data are recorded in a block.
#define CONFIG_MEM_PAGE_SIZE                256u  				///< Configuration memory page size(in bytes).

// Configuration header description. Number of bytes per element
#define HEADER_ID_SIZE              16u						///< Block ID size (in bytes).
#define HEADER_CHECKSUM_SIZE        2u						///< Block checksum size (in bytes).
#define NUMBER_OF_ENTRIES_SIZE      2u						///< Block number of entries size (in bytes).
#define HEADER_DATE_SIZE            20u						///< Block date size (in bytes).
#define HEADER_SIZE                 ( HEADER_ID_SIZE + HEADER_CHECKSUM_SIZE + NUMBER_OF_ENTRIES_SIZE + HEADER_DATE_SIZE )  ///< Block header size (in bytes).
#define HEADER_LOCATION             (uint32_t)0u            ///< Header offset.
#define IDENTFIER_OFFSET            (uint32_t)0u			///< Block identifier offset.
#define CHECKSUM_OFFSET             (uint32_t)14u			///< Checksum offset.
#define NUMBER_OF_ENTRIES_OFFSET    (uint32_t)16u			///< Number of entries offset.
#define CREATION_DATE_OFFSET        (uint32_t)18u			///< Date of configuration creation offset.

// Configuration Dpoint characteristics
#define NUMBER_OF_BYTES_PER_CONFIG_DPOINT   8u				///< Number of bytes per configuration Dpoint.
#define FIELD_TYPE                          0u				///< Field Dpoint type identifier.
#define ENG_TYPE                            1u				///< Engineering Dpoint type identifier.

// CPU Dpoints
#define NUMBER_OF_CPU_DPOINTS               9u				///< Number of CPU Dpoint recorded on the ACQ-MTC configuration memory.
#define OPTIMAL_H  							450u			///< Optimal Total H index.
#define OPTIMAL_G  							451u			///< Optimal Total G index.
#define OPTIMAL_MAG_DIP 					452u			///< Optimal Mag dip.
#define TOTAL_CORRECTION 					453u			///< Total correction.
#define DISTANCE_BTWN_DNIS 					458u 			///< Distance between Xceed and Telescope D&Is.



/// Structure of a configuration block header.
typedef struct configurationHeader
{
    uint8_t  Identifier[HEADER_ID_SIZE];					///< Block identifier, Field or Engineering.
    uint16_t Checksum;										///< Checksum.
    uint16_t NumberOfEntries;								///< Number of entries.
    uint8_t  Date[HEADER_DATE_SIZE];						///< Date of creation.
} configurationHeader_t;

/// Structure of a configuration Dpoint.
typedef struct ConfigDpoint
{
    uint16_t Index;											///< Configuration index.
    uint16_t Value;											///< Value.
    uint16_t Type;											///< Type, Field or Engineering Dpoint.
    uint16_t spare;											///< Spare byte.
} ConfigDpoint_t;


void 					CONFIG_FLASH_read(const uint16_t blockIdentifier, const uint32_t address, const uint16_t packetSize,
                                 uint8_t*  pDataByte);
EFLASHProgramStatus_t 	CONFIG_FLASH_write(const uint16_t blockIdentifier, const uint32_t address,
                                                   const uint16_t packetSize, uint8_t* const pDataByte);
EFLASHPollStatus_t 		CONFIG_FLASH_erase(const uint16_t blockIdentifier);

EFLASHPollStatus_t 		CONFIG_FLASH_eraseStatus_get(void);
configurationHeader_t 	CONFIG_FLASH_headerRead(const uint16_t blockIdentifier);
ConfigDpoint_t 			CONFIG_FLASH_DpointGet(const uint16_t blockIdentifier, const uint32_t address);
const uint16_t*			CONFIG_FLASH_CPUconfigDpointPointerGet(const uint16_t index);
bool_t 					CONFIG_FLASH_CPUconfigDpointsSet(const uint16_t index, const uint16_t value);
void 					CONFIG_FLASH_CPUconfigDpointsGet(const uint32_t address, const uint16_t packetSize, uint8_t* const pDataByte);

#if defined (PLATFORM_PC_GCC_COMPILER) || defined (PLATFORM_PC_TI_COMPILER)
 void CONFIG_FLASH_eraseStatus_set_TDD(const EFLASHPollStatus_t eraseStatus);
#endif

#endif


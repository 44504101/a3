// ----------------------------------------------------------------------------
/**
 * @file    	ToolSpecificConfig.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		23 Jan 2014
 * @brief		Tool specific configuration file for Xceed bootloader.
 * @note		This assumes that most of the code is from the SDRM bootloader.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef TOOLSPECIFICCONFIG_H_
#define TOOLSPECIFICCONFIG_H_

#include "utils.h"                          // Need this for endianness enum
#include "xpb_bootloader_blinfo.h"

#define I_AM_THE_BOOTLOADER                 // Conditionally compile common code for bootloader

#define COMM_SSB							// SSB bus is required.
#define COMM_DEBUG							// Debug port is required.

#define SSB_SLAVE_ADDRESS			0xFD	// Dummy address for code to use as default.
#define ISB_SLAVE_ADDRESS           0x42    // Dummy address for code to use as default.
#define SSB_SLAVE_ADDRESS_DSP_A		0x8C    // Xceed (on XPB) / Xcel DSP A slave address.
#define SSB_SLAVE_ADDRESS_DSP_B		0xFD    // Xcel DSP B slave address.
#define ALT_SSB_SLAVE_ADDRESS_DSP_B 0xFD    // Xceed (on XPB) DSP B slave address.

#define SELF_TEST_LENGTH            7
#define JUMP_TO_APP_WITH_BAD_CRC    FALSE
#define WAITMODE_TIMEOUT            5000	// 5,000 milliseconds, or 5 seconds
#define LOADERMODE_TIMEOUT          120000  // give plenty of time for surface to re-program
#define BAD_APP_CRC_TIMEOUT         120000  // give plenty of time for surface to re-program

#define BOOTLOADER_START_ADDRESS    0x338000                    // Bootloader in flash sector A.
#define BOOTLOADER_END_ADDRESS      0x33FF7F
#define BOOTLOADER_LENGTH           (BOOTLOADER_END_ADDRESS - BOOTLOADER_START_ADDRESS)
#define BOOTLOADER_CRC_ADDRESS      BOOTLOADER_END_ADDRESS      // CRC in final bootloader location.

#define APPLICATION_START_ADDRESS   0x300000                    // Application in flash sectors C,D,E,F,G & H.
#define APPLICATION_END_ADDRESS     0x32FFFF
#define APPLICATION_LENGTH          (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS)
#define APPLICATION_CRC_ADDRESS     APPLICATION_END_ADDRESS     // CRC one off the end of the app

#define PARAMETER_START_ADDRESS     0x330000                    // Parameters in flash sector B.
#define PARAMETER_END_ADDRESS       0x337FFF
#define PARAMETER_LENGTH            (PARAMETER_END_ADDRESS - PARAMETER_START_ADDRESS)
#define PARAMETER_CRC_ADDRESS       PARAMETER_END_ADDRESS       // CRC in final application location.

#define CONFIG_START_ADDRESS        0                           // Configuration partition not used - set to zero.
#define CONFIG_END_ADDRESS          0
#define CONFIG_LENGTH               (CONFIG_END_ADDRESS - CONFIG_START_ADDRESS)     // Length must be zero for not used.
#define CONFIG_CRC_ADDRESS          CONFIG_END_ADDRESS


#define TARGET_ENDIAN_TYPE          LITTLE_ENDIAN
#define DOWNLOAD_ENDIANESS          BIG_ENDIAN      //endianess when downloading data
#define UPLOAD_ENDIANESS            BIG_ENDIAN


#define BASELINE_NAME               "dummy baseline"
#define BASELINE_DATE               "Thursday, January 1, 1970 00:00:00"
/* Define Identity for Opcode 2
 * To ensure backward compatibility with the classic Toolscope, the format is as follows:
 *  aaabbbbbbcccdddefff
 *  where   aaa is the sub type (either "BL " or "bE " for good \ bad CRCs of bootloader itself and application code)
 *          bbbbbb is the version (always "XCEED ")
 *          ccc is the software revision, major version
 *          ddd is the software revision, minor version
 *          e   is the software revision, baseline type (alpha, beta, commercial)
 *          fff is the software revision, build number
 *
 * The preprocessor is used to append the appropriate number of leading zeroes to the
 * various numbers - OmniWorks doesn't zero pad the strings, but we want a constant
 * length of string for all possible replies.
 *
 */
#if (BASELINE_MAJOR_VERSION < 10)
#define MAJOR_VERSION           "00"BASELINE_MAJOR_VERSION_STRING
#elif (BASELINE_MAJOR_VERSION < 100)
#define MAJOR_VERSION           "0"BASELINE_MAJOR_VERSION_STRING
#else
#define MAJOR_VERSION           BASELINE_MAJOR_VERSION_STRING
#endif

#if (BASELINE_MINOR_VERSION < 10)
#define MINOR_VERSION           "00"BASELINE_MINOR_VERSION_STRING
#elif (BASELINE_MINOR_VERSION < 100)
#define MINOR_VERSION           "0"BASELINE_MINOR_VERSION_STRING
#else
#define MINOR_VERSION           BASELINE_MINOR_VERSION_STRING
#endif

#if (BASELINE_BUILD < 10)
#define BUILD_VERSION           "00"BASELINE_BUILD_STRING
#elif (BASELINE_BUILD < 100)
#define BUILD_VERSION           "0"BASELINE_BUILD_STRING
#else
#define BUILD_VERSION           BASELINE_BUILD_STRING
#endif

#define BOOTLOADER_BOARD_ID     "BL XPB   "MAJOR_VERSION""MINOR_VERSION""BASELINE_TYPE_STRING""BUILD_VERSION
#define BOOTLOADER_BOARD_ID_ERR "bE XPB   "MAJOR_VERSION""MINOR_VERSION""BASELINE_TYPE_STRING""BUILD_VERSION

#define BOARD_ID_LENGTH         ((sizeof BOOTLOADER_BOARD_ID)-1)

/// Describes RAM section where promloader is copied into before running it (RAML0 - RAML4).
/// This needs to match the area allocated via the promloader linker file.
#define PROMLOADER_RAM_START_ADDRESS     0x8000
#define PROMLOADER_RAM_SIZE              0x4000

#define BUFFER_BASE_ADDRESS         0xF000
#define BUFFER_LENGTH               0x1000

// Boot sector masks - these are the flash sectors in the 28335 which each partition is stored in.
#define BOOT_SECTOR_MASK        (SECTORA)
#define APPLICATION_SECTOR_MASK (SECTORC | SECTORD | SECTORE | SECTORF | SECTORG| SECTORH)
#define PARAMETER_SECTOR_MASK   (SECTORB)
#define CONFIG_SECTOR_MASK      0x0000              // Sector not used, so set mask to zero.

#define ALLOW_BOOTLOADER_PROGRAMMING    FALSE
#define ALLOW_INCREMENTAL_FLASH_WRITE   TRUE


#endif /* TOOLSPECIFICCONFIG_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


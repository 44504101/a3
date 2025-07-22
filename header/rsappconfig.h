/*
 * rsappconfig.h
 *
 *  Created on: 2022年5月27日
 *      Author: l
 */

#ifndef HEADER_RSAPPCONFIG_H_
#define HEADER_RSAPPCONFIG_H_

#define RS_CFG_BOARD_TYPE                   1u      ///< First XPB board
#define RS_CFG_MAX_NUMBER_OF_PARTITIONS     7u      ///< 7 partitions
#define RS_CFG_BLANK_LOCATION_CONTAINS      0xFFu   ///< Blank is 0xFF

/**
 * Define the periodicity of the recording system gatekeeper task.
 * This is how often the read \ write task runs and will access the memory.
 */
#define RS_CFG_TASK_PERIODICITY_MS  10u


/**
 * Toolscope cannot handle pages of different sizes, so care needs to be taken
 * to ensure that this page size can be accommodated by all storage devices in
 * the system (i.e. don't make the page size larger than the total capacity
 * of any single storage device).  A small page size does not affect the
 * recording or dumping rate of the recording system, but may have an impact
 * on the (off-line) decoding of the dump file by Toolscope.Toolscope
 * 无法处理不同大小的页面，因此需要注意确保该页面大小可以被系统中的所有存储设备容纳（即不要使页面大小大于任何单个存储设备的总容量） .
 * 较小的页面大小不会影响记录系统的记录或转储率，但可能会影响 Toolscope 对转储文件的（离线）解码。
 */
#define RS_CFG_PAGE_SIZE_KB         8u

/**
 * Number of right shifts required to get the page number.
 * For a 8Kbyte page size, this shift value is 13 (2^13 = 8192).
 */
#define RS_PAGE_NBR_SHIFT			13u


/**
 * The maximum size of the Tool Data Record.  This is normally 4k (as per the
 * recording standard), but there may be instances where a very small memory
 * is used and the page size and TDR have to both be scaled down).
 * This is also related to the size of the buffer allocated in the record
 * search module, where we have a buffer of approximately twice this size.
 * Keep this small for testing otherwise it's tedious to setup test buffers.
 * 工具数据记录的最大大小。这通常是4k（根据记录标准），但可能会有使用非常小内存的情况，页面大小和TDR都必须缩小）。
 * 这还与记录搜索模块中分配的缓冲区大小有关，我们在该模块中有一个大约为该大小两倍的缓冲区。
 * 为了测试，请将其保持在较小的范围内，否则设置测试缓冲区会很繁琐。
 * @warning
 * Set to a non-standard size initially to save memory.
 *
 */
#define RS_CFG_MAX_TDR_SIZE_BYTES   1024u


/**
 * Define the size of block which will be read in one go when trying to
 * setup the recording system.  This is related to how much spare RAM there
 * is on a stack to allocate to a buffer which data is read into.
 * Keep this small for testing otherwise it's tedious to setup test buffers.
 */
#define RS_CFG_LOCAL_BLOCK_READ_SIZE 32u


/**
 * Define the number of reads which can be queued.
 */
#define RS_CFG_READ_QUEUE_LENGTH    4u


/**
 * Define the number of writes which can be queued.
 */
#define RS_CFG_WRITE_QUEUE_LENGTH   40u


/**
 * Define the read timeout, in milliseconds.
 * This is the maximum amount of time the recording system may take
 * to find a record somewhere in the recording memory.
 */
#define RS_CFG_READ_QUEUE_TIMEOUT_MS        30000uL


/**
 * Define the write timeout, in milliseconds.
 * This is the maximum amount of time the recording system may take
 * to write a record to the recording memory.
 */
#define RS_CFG_WRITE_QUEUE_TIMEOUT_MS       100u


/**
 * Enumerated type for all storage devices
 * which could be used by the recording system.
 */
typedef enum
{
    STORAGE_DEVICE_MAIN_FLASH       = 0,    ///< Identifier for main flash.
    STORAGE_DEVICE_SERIAL_FLASH     = 1,    ///< Identifier for serial flash.
    STORAGE_DEVICE_I2C_EEPROM       = 2		///< Identifier for I2c eeprom.
} storage_devices_t;


/**
 * Partition IDs
 */
#define RS_PARTITION_CALIBRATION    0u
#define RS_PARTITION_CONFIGURATION  7u
#define RS_PARTITION_MWD            11u
#define RS_PARTITION_STATIC_SURVEYS 12u
#define RS_PARTITION_TRAJECTORY     13u
#define RS_PARTITION_BURST_DATA     14u
#define RS_PARTITION_ALL_OTHER      15u


/**
 * Partition settings - these are loaded into an array of structures,
 * of type rs_partition_info_t (see rspartition.h).
 * Only the first three elements of the structure need to be initialised here,
 * the partition ID, the number of pages in the partition and the device in
 * which the partition is to be stored.
 * All other values are setup by the recording system itself, so can be
 * initialised to zero or a default value.
 *
 * @note
 * The number of pages in the partition is the number of pages of size
 * RS_CFG_PAGE_SIZE_KB.  The recording system will ensure that
 * a partition will fill at least one block of the particular storage device,
 * as this is the minimum amount of space which can be erased on the device.
 * It may therefore be that a partition is enlarged to make sure this works,
 * by increasing the number of pages, otherwise it would not be possible to
 * erase a partition without partially erasing the adjacent partition.
 *
 * @warning
 * It is the responsibility of whoever is setting up this file to ensure
 * that the partition settings used will actually fit in the physical space
 * available.
 *
 */
#define RS_CFG_PARTITION_SETTINGS                                                                                          \
{                                                                                                                          \
    { RS_PARTITION_CALIBRATION,    1u,     STORAGE_DEVICE_SERIAL_FLASH, 0u, 0u, RS_ERR_NO_ERROR, 0u, 0u, 0u, 0u, 0u, 0u }, \
    { RS_PARTITION_CONFIGURATION,  7u,     STORAGE_DEVICE_SERIAL_FLASH, 0u, 0u, RS_ERR_NO_ERROR, 0u, 0u, 0u, 0u, 0u, 0u }, \
    { RS_PARTITION_MWD,            128u,   STORAGE_DEVICE_MAIN_FLASH,   0u, 0u, RS_ERR_NO_ERROR, 0u, 0u, 0u, 0u, 0u, 0u }, \
    { RS_PARTITION_STATIC_SURVEYS, 256u,   STORAGE_DEVICE_MAIN_FLASH,   0u, 0u, RS_ERR_NO_ERROR, 0u, 0u, 0u, 0u, 0u, 0u }, \
    { RS_PARTITION_TRAJECTORY,     2304u,  STORAGE_DEVICE_MAIN_FLASH,   0u, 0u, RS_ERR_NO_ERROR, 0u, 0u, 0u, 0u, 0u, 0u }, \
    { RS_PARTITION_BURST_DATA,     12032u, STORAGE_DEVICE_MAIN_FLASH,   0u, 0u, RS_ERR_NO_ERROR, 0u, 0u, 0u, 0u, 0u, 0u }, \
    { RS_PARTITION_ALL_OTHER,      18048u, STORAGE_DEVICE_MAIN_FLASH,   0u, 0u, RS_ERR_NO_ERROR, 0u, 0u, 0u, 0u, 0u, 0u }, \
}


/**
 * Flash HAL physical addresses, one per storage device in the system.
 * These are loaded into an array of structures, of type
 * flash_physical_arrangement_t (see flash_hal_prv.h)
 * Flash HAL 物理地址，系统中每个存储设备一个。
 * 它们被加载到一个结构数组中，类型为 flash_physical_arrangement_t（参见 flash_hal_prv.h）
 * @warning
 * This array must be setup in the same order as the enumerated type
 * storage_devices_t, as we use the type to index the array.
 * It seems wasteful to include the storage device here, but we use it as a
 * sanity check to make sure that the array has been setup in the correct order.
 * 该数组必须按照与枚举类型 storage_devices_t 相同的顺序设置，因为我们使用该类型来索引数组。
 * 在此处包含存储设备似乎很浪费，但我们将其用作完整性检查以确保阵列已按正确的顺序设置。
 * @note
 * All addresses are in BYTES.
 *
 */
#define FLASH_HAL_PHYSICAL_ADDRESSES                                \
{                                                                   \
    { STORAGE_DEVICE_MAIN_FLASH,   0u,   0x0FFFFFFFu, 131072u },    \
    { STORAGE_DEVICE_SERIAL_FLASH, 0u,   0x0000FFFFu,      1u },    \
    { STORAGE_DEVICE_I2C_EEPROM,   0u,   0x00008000u,      1u },    \
}

#endif /* HEADER_RSAPPCONFIG_H_ */

// ----------------------------------------------------------------------------
/**
 * @file    	common/src/genericIO.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		28 Mar 2012
 * @brief		Generic IO driver for 16 and 32 bit read \ write operations.
 *
 * @note
 * All functions use the form _Impl and are declared as static because we're
 * using a function pointer to access these functions.
 *
 * @note
 * Lint complains about the casting of the address (a uint32_t) into a pointer,
 * generating the following warnings:
 *     511 - size incompatibility in cast from uint32_t (4 bytes) to uint16_t* (2 bytes)
 *     923 - cast from uint32_t to pointer
 *    9078 - conversion between a pointer and integer type
 *
 * Warning 511 appears because Lint is setup so that it thinks that pointers are
 * only 16 bits wide - we do this because of the 22 bit pointer limit below, to
 * make sure we get warnings for everything.
 *
 * Warnings 923 and 9078 are safe to disable because we have to convert into a
 * pointer somehow to access the hardware itself, as long as the following warning
 * is taken into account:
 *
 * @warning
 * On the 28335, pointers can only be a maximum of 22 bits, so it will not be
 * possible to address the entire 32 bit address range using this code.
 *
 * @warning
 * Don't be tempted to rename this file io.c - this is a GCC standard filename,
 * and the code will compile but won't work!
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "genericIO.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static void     IO_16bitWrite_Impl(const uint32_t address, const uint16_t data);
static uint16_t IO_16bitRead_Impl(const uint32_t address);
static void     IO_32bitWrite_Impl(const uint32_t address, const uint32_t data);
static uint32_t IO_32bitRead_Impl(const uint32_t address);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * IO_16bitWrite_Impl performs a 16 bit write.
 *
 * @param	address		32 bit address to write data into.
 * @param	data		16 bit data to write into 'address'.
 *
*/
// ----------------------------------------------------------------------------
static void IO_16bitWrite_Impl(const uint32_t address, const uint16_t data)
{
	volatile uint16_t *p;

	p  = (uint16_t*)address;		//lint !e511 !e923 !e9078
	*p = data;
}

/// Defining instance of the global function pointer genericIO_16bitWrite.
/// The pointer is initialised to point to IO_16bitWrite_Impl.
//lint -e{956} External variable.  Pointer doesn't change so it's fine.
void (*genericIO_16bitWrite)(const uint32_t address, const uint16_t data) = IO_16bitWrite_Impl;


// ----------------------------------------------------------------------------
/**
 * IO_16bitRead_Impl performs a 16 bit read.
 *
 * @param	address		32 bit address to read data from.
 * @retval	uint16_t	16 bit data read from 'address'.
 *
*/
// ----------------------------------------------------------------------------
static uint16_t IO_16bitRead_Impl(const uint32_t address)
{
	volatile const uint16_t *p;

	p = (uint16_t*)address;			//lint !e511 !e923 !e9078
	return *p;
}

/// Defining instance of the global function pointer genericIO_16bitRead.
/// The pointer is initialised to point to IO_16bitRead_Impl.
//lint -e{956} External variable.  Pointer doesn't change so it's fine.
uint16_t (*genericIO_16bitRead)(const uint32_t address) = IO_16bitRead_Impl;


// ----------------------------------------------------------------------------
/**
 * IO_32bitWrite_Impl performs a 32 bit write.
 *
 * @param	address		32 bit address to write data into.
 * @param	data		32 bit data to write into 'address'.
 *
*/
// ----------------------------------------------------------------------------
static void IO_32bitWrite_Impl(const uint32_t address, const uint32_t data)
{
	volatile uint32_t *p;

	p  = (uint32_t*)address;		//lint !e511 !e923 !e9078
	*p = data;
}

/// Defining instance of the global function pointer genericIO_32bitWrite.
/// The pointer is initialised to point to IO_32bitWrite_Impl.
//lint -e{956} External variable.  Pointer doesn't change so it's fine.
void (*genericIO_32bitWrite)(const uint32_t address, const uint32_t data) = IO_32bitWrite_Impl;


// ----------------------------------------------------------------------------
/**
 * IO_32bitRead_Impl performs a 32 bit read.
 *
 * @param	address		32 bit address to read data from.
 * @retval	uint32_t	32 bit data read from 'address'.
 *
*/
// ----------------------------------------------------------------------------
static uint32_t IO_32bitRead_Impl(const uint32_t address)
{
	volatile const uint32_t *p;

	p = (uint32_t*)address;			//lint !e511 !e923 !e9078
	return *p;
}

/// Defining instance of the global function pointer genericIO_32bitRead.
/// The pointer is initialised to point to IO_32bitRead_Impl.
//lint -e{956} External variable.  Pointer doesn't change so it's fine.
uint32_t (*genericIO_32bitRead)(const uint32_t address) = IO_32bitRead_Impl;


// ----------------------------------------------------------------------------
/**
 * genericIO_16bitMaskBitSet performs a read-modify-write to set bits.
 * Note that we don't clear the required bits before setting, or check to see
 * whether the bits are already set, so there will always be a read and a write
 * operation performed.
 *
 * @param	address		32 bit address to read data from.
 * @param	mask		16 bit mask - holds the bits to set.
 *
*/
// ----------------------------------------------------------------------------
void genericIO_16bitMaskBitSet(const uint32_t address, const uint16_t mask)
{
	uint16_t	result;

	result = genericIO_16bitRead(address);
	result |= mask;
	genericIO_16bitWrite(address, result);
}


// ----------------------------------------------------------------------------
/**
 * genericIO_16bitMaskBitClear performs a read-modify-write to clear bits.
 * Note that we always clear the required bits, even if they are not set, so
 * there will always be a read and a write operation performed.
 *
 * @param	address		32 bit address to read data from.
 * @param	mask		16 bit mask - holds the bits to clear.
 *
*/
// ----------------------------------------------------------------------------
void genericIO_16bitMaskBitClear(const uint32_t address, const uint16_t mask)
{
	uint16_t	result;

	result = genericIO_16bitRead(address);
	result &= ~mask;
	genericIO_16bitWrite(address, result);
}


// ----------------------------------------------------------------------------
/**
 * genericIO_32bitMaskBitSet performs a read-modify-write to set bits.
 * Note that we don't clear the required bits before setting, or check to see
 * whether the bits are already set, so there will always be a read and a write
 * operation performed.
 *
 * @param	address		32 bit address to read data from.
 * @param	mask		32 bit mask - holds the bits to set.
 *
*/
// ----------------------------------------------------------------------------
void genericIO_32bitMaskBitSet(const uint32_t address, const uint32_t mask)
{
	uint32_t	result;

	result = genericIO_32bitRead(address);
	result |= mask;
	genericIO_32bitWrite(address, result);
}


// ----------------------------------------------------------------------------
/**
 * genericIO_32bitMaskBitClear performs a read-modify-write to clear bits.
 * Note that we always clear the required bits, even if they are not set, so
 * there will always be a read and a write operation performed.
 *
 * @param	address		32 bit address to read data from.
 * @param	mask		32 bit mask - holds the bits to clear.
 *
*/
// ----------------------------------------------------------------------------
void genericIO_32bitMaskBitClear(const uint32_t address, const uint32_t mask)
{
	uint32_t	result;

	result = genericIO_32bitRead(address);
	result &= ~mask;
	genericIO_32bitWrite(address, result);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

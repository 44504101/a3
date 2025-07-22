/*
 * extflash.c
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "common_data_types.h"
#include "extflash.h"
#include "genericIO.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

#define XZCS7_ADDRESS_ZONE	            0x00200000u     ///< Internal DSP write bit
#define GPADAT_ADDRESS		            0x00006FC0u		///< Address for GPADAT register
#define GPATOGGLE_ADDRESS	            0x00006FC6u		///< Address for GPATOGGLE register
#define GPIO_BIT_MASK	                0x07F00000u		///< bit mask to clear GPIO[26:20]
#define OUT_OF_RANGE_MASK			    0xFFF00000u		///< bit mask for out of range addresses


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module

static void SetupTopAddressBits(const uint32_t full_address);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * Extflash_ExternalFlashRead_Impl reads 16 bit data from a 32 bit address.
 * We use the form _Impl and declare the function as static because we're using
 * a function pointer to access this function (to allow for redirection under TDD).
 *
 * The flash are mapped into address space XZCS7, and uses the full 20 bit DSP
 * address space (XA0 - XA19), plus GPIO20-GPIO26.  GPIO26 selects flash 1 or 2.
 *
 * @param	address		Address to read from.
 * @retval	uint16_t	Value obtained by reading from 'address'.
 *
*/
// ----------------------------------------------------------------------------
static uint16_t Extflash_ExternalFlashRead_Impl(uint32_t address)
{
	uint16_t		value;

	// Set address bits 26:20
	SetupTopAddressBits(address);

	// Clear any address bits which are out of range, so address rolls over.
	// Note that this also clears the GPIO bits from the address, as these
	// have already been dealt with.
	address &= ~OUT_OF_RANGE_MASK;

	// Set bit in internal DSP address to map into zone 7.
	address |= XZCS7_ADDRESS_ZONE;

	value = genericIO_16bitRead(address);

	return value;
}

/// This is the defining instance of the global function pointer FLASH_ExternalFlashRead.
/// The pointer is initialised to point to Flash_ExternalFlashRead_Impl.
//lint -e{956} Doesn't need to be volatile.
uint16_t (*EXTFLASH_ExternalFlashRead)(uint32_t address) = Extflash_ExternalFlashRead_Impl;


// ----------------------------------------------------------------------------
/**
 * Extflash_ExternalFlashWrite_Impl writes 16 bit data into a 32 bit address.
 * We use the form _Impl and declare the function as static because we're using
 * a function pointer to access this function (to allow for redirection under TDD).
 *
 * The flash are mapped into address space XZCS7, and uses the full 20 bit DSP
 * address space (XA0 - XA19), plus GPIO20-GPIO26.  GPIO26 selects flash 1 or 2.
 *
 * @param	address		Address to write to.
 * @param	data		Data to write into 'address'.
 *
*/
// ----------------------------------------------------------------------------
static void Extflash_ExternalFlashWrite_Impl(uint32_t address, const uint16_t data)
{
	// Set address bits 26:20
	SetupTopAddressBits(address);

	// Clear any address bits which are out of range, so address rolls over.
	// Note that this also clears the GPIO bits from the address, as these
	// have already been dealt with.
	address &= ~OUT_OF_RANGE_MASK;

	// Set bit in internal DSP address to map into zone 7.
	address |= XZCS7_ADDRESS_ZONE;

	genericIO_16bitWrite(address, data);
}

/// This is the defining instance of the global function pointer FLASH_ExternalFlashWrite.
/// The pointer is initialised to point to Flash_ExternalFlashWrite_Impl.
//lint -e{956} Doesn't need to be volatile.
void (*EXTFLASH_ExternalFlashWrite)(uint32_t address, const uint16_t data) = Extflash_ExternalFlashWrite_Impl;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * SetupTopAddressBits sets up GPIO[26:20] to address the required flash device.
 *
 * @param	full_address	Address to access.
 *
*/
// ----------------------------------------------------------------------------
static void SetupTopAddressBits(const uint32_t full_address)
{
	uint32_t	required_gpio_address;
	uint32_t	current_gpio_address;
	uint32_t	pins_to_change;

	// Generate top address bits from whole address.
	// Any higher bits are ignored here.
	required_gpio_address = full_address & GPIO_BIT_MASK;

	// Read port and mask off all extraneous bits to generate the current
	// state of GPIO[26:20].  Note the cast of GPADAT_ADDRESS to 32 bits
	// - as the value is < 65536 the compiler thinks it's a uint16_t.
	//lint -e{921} Cast from unsigned in to unsigned long.
	current_gpio_address  = genericIO_32bitRead( (uint32_t)GPADAT_ADDRESS);
	current_gpio_address &= GPIO_BIT_MASK;

	// Generate pins to toggle - the XOR of the current and required address
	// sets any pin which needs to change to a 1.
	pins_to_change = required_gpio_address ^ current_gpio_address;

	// Toggle required bits - GPIO[26:20] now set correctly.
	// Note the cast of GPATOGGLE_ADDRESS to 32 bits
    // - as the value is < 65536 the compiler thinks it's a uint16_t.
    //lint -e{921} Cast from unsigned in to unsigned long.
	genericIO_32bitWrite( (uint32_t)GPATOGGLE_ADDRESS, pins_to_change);
}

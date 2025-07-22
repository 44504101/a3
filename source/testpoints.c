// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/testpoints.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		4 Jun 2013
 * @brief		Test point driver for TI's 28335 DSP.
 * @details
 * Test point driver code for 28335 DSP.  The intention of this driver is to
 * facilitate code profiling using testpoints, where the testpoints which are
 * used are programmable.  There is a testpoint array, mTestpointArray, which
 * holds the addresses of the various GPIO set, clear and toggle registers,
 * and a bit mask to activate the required bit.  Functions are provided to
 * initialise the array to suit the particular application, and then the
 * user code calls TESTPOINTS_Set, TESTPOINTS_Clear or TESTPOINTS_Toggle.
  *
 * Un-used entries in the testpoint array point to a dummy variable in RAM,
 * so if the user code ensures that a unique offset is used for each different
 * 'instance' of using testpoints (so, use TESTPOINTS_xxx(0) only in one place
 * in the code, TESTPOINTS_xxx(1) in another place etc), it is possible to only
 * activate a particular testpoint, by setting the others in the array to
 * un-used.
 *
 * @warning
 * Clearly, this code has some impact on the overall processor utilisation -
 * testing in a very simple case, at a clock frequency of 58.9824MHz yields the
 * following results for time spent setting \ clearing \ toggling a testpoint:
 * ~330nS with this module and the calling module(s) build at -O3 and offset checking disabled.
 * ~355nS with this module built at -O3 and offset checking disabled.
 * ~440nS with this module built at -O3 and offset checking enabled.
 * ~510nS with this module built with no optimisation and offset checking disabled.
 * ~580nS with this module built with no optimisation and offset checking enabled.
 *
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "DSP28335_device.h"
#include "testpoints.h"
#include "testpointoffsets.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define OFFSET_CHECKING_ENABLED     ///< Comment out for faster, but less 'safe' code.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/**
 * Dummy variable which is used to initialised the testpoint array.
 * This means that any testpoint which is altered without setting up correctly
 * will just modify this variable, and won't break anything.
 */
//lint -e{956} Doesn't need to be volatile.
static uint32_t		mDummyVariable;

/// Local array of testpoint structures, initialised to point to the dummy variable.
//lint -e{956} Doesn't need to be volatile.
static Testpoints_t mTestpointArray[MAXIMUM_NUMBER_OF_TESTPOINTS] =
{
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu},
	{&mDummyVariable, &mDummyVariable, &mDummyVariable, 0u, 0xFFFFu}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * TESTPOINTS_Initialise sets up an entry in the test point array, to set \
 * clear or toggle the appropriate bit for the GPIO required.
 *
 * @param	Offset		Index into test point array.
 * @param	GpioNumber	GPIO number - 0 to 87.
 * @retval	bool_t		TRUE if entry could be initialised, FALSE if failed.
 *
 */
// ----------------------------------------------------------------------------
bool_t TESTPOINTS_Initialise(const uint16_t Offset, const uint16_t GpioNumber)
{
	bool_t		bSetupOK = TRUE;
	uint16_t	BitNumber = 0u;

	// If Offset exceeds maximum number of testpoint locations then error.
	if (Offset >= MAXIMUM_NUMBER_OF_TESTPOINTS)
	{
		bSetupOK = FALSE;
	}
	// Otherwise try and initialise the testpoint array.
	else
	{
		// Store the GPIO bit number.
		mTestpointArray[Offset].GpioBitNumber = GpioNumber;

		// Select the correct port (A,B or C) depending on the bit number.
		if (GpioNumber < 32u)
		{
			BitNumber = GpioNumber;
			mTestpointArray[Offset].pSetRegister = &GpioDataRegs.GPASET.all;
			mTestpointArray[Offset].pClearRegister = &GpioDataRegs.GPACLEAR.all;
			mTestpointArray[Offset].pToggleRegister = &GpioDataRegs.GPATOGGLE.all;
		}
		else if (GpioNumber < 64u)
		{
			BitNumber = GpioNumber - 32u;
			mTestpointArray[Offset].pSetRegister = &GpioDataRegs.GPBSET.all;
			mTestpointArray[Offset].pClearRegister = &GpioDataRegs.GPBCLEAR.all;
			mTestpointArray[Offset].pToggleRegister = &GpioDataRegs.GPBTOGGLE.all;
		}
		else if (GpioNumber < 88u)
		{
			BitNumber = GpioNumber - 64u;
			mTestpointArray[Offset].pSetRegister = &GpioDataRegs.GPCSET.all;
			mTestpointArray[Offset].pClearRegister = &GpioDataRegs.GPCCLEAR.all;
			mTestpointArray[Offset].pToggleRegister = &GpioDataRegs.GPCTOGGLE.all;
		}
		// If the bit number is invalid then reset the bit number in the
		// array to the default value and set the OK flag to FALSE (not OK!).
		else
		{
			mTestpointArray[Offset].GpioBitNumber = 0xFFFFu;
			bSetupOK = FALSE;
		}

		// If the OK flag is TRUE then generate the bit mask for the bit number.
		if (bSetupOK)
		{
		    //lint -e{921} Cast to uint32_t before bit shifting to avoid truncation.
			mTestpointArray[Offset].BitMask = (uint32_t)1u << BitNumber;
		}
	}

	return bSetupOK;
}


// ----------------------------------------------------------------------------
/**
 * TESTPOINTS_ArrayReset resets the required entry in the test point array
 * to the default values.
 *
 * @param	Offset		Index into test point array.
  * @retval	bool_t		TRUE if entry could be reset, FALSE if failed.
 *
 */
// ----------------------------------------------------------------------------
bool_t TESTPOINTS_ArrayReset(const uint16_t Offset)
{
	bool_t	bResetOK = FALSE;

	if (Offset < MAXIMUM_NUMBER_OF_TESTPOINTS)
	{
		mTestpointArray[Offset].GpioBitNumber = 0xFFFFu;
		mTestpointArray[Offset].BitMask = 0u;
		mTestpointArray[Offset].pSetRegister = &mDummyVariable;
		mTestpointArray[Offset].pClearRegister = &mDummyVariable;
		mTestpointArray[Offset].pToggleRegister = &mDummyVariable;
		bResetOK = TRUE;
	}

	return bResetOK;
}


// ----------------------------------------------------------------------------
/**
 * TESTPOINTS_ArrayQuery returns the GPIO number which a particular array
 * element has been set to - this will be 0xFFFF if uninitialised, or 0xFFFE
 * if an invalid offset has been received.
 *
 * @param	Offset		Index into test point array.
 * @retval	uint16_t	GPIO number.
 *
 */
// ----------------------------------------------------------------------------
uint16_t TESTPOINTS_ArrayQuery(const uint16_t Offset)
{
	uint16_t	ReturnValue = 0xFFFEu;

	if (Offset < MAXIMUM_NUMBER_OF_TESTPOINTS)
	{
		ReturnValue = mTestpointArray[Offset].GpioBitNumber;
	}

	return ReturnValue;
}


// ----------------------------------------------------------------------------
/**
 * TESTPOINTS_Set sets the appropriate GPIO pin, based on the test point
 * array which has already been initialised.
 *
 * @param	Offset		Index into test point array for test point to set.
 *
 */
// ----------------------------------------------------------------------------
void TESTPOINTS_Set(const uint16_t Offset)
{
#ifdef OFFSET_CHECKING_ENABLED
	if (Offset < MAXIMUM_NUMBER_OF_TESTPOINTS)
	{
		*mTestpointArray[Offset].pSetRegister = mTestpointArray[Offset].BitMask;
	}
#else
	*mTestpointArray[Offset].pSetRegister = mTestpointArray[Offset].BitMask;
#endif
}


// ----------------------------------------------------------------------------
/**
 * TESTPOINTS_Clear clears sets the appropriate GPIO pin, based on the test#
 * point array which has already been initialised.
 *
 * @param	Offset		Index into test point array for test point to clear.
 *
 */
// ----------------------------------------------------------------------------
void TESTPOINTS_Clear(const uint16_t Offset)
{
#ifdef OFFSET_CHECKING_ENABLED
	if (Offset < MAXIMUM_NUMBER_OF_TESTPOINTS)
	{
		*mTestpointArray[Offset].pClearRegister = mTestpointArray[Offset].BitMask;
	}
#else
	*mTestpointArray[Offset].pClearRegister = mTestpointArray[Offset].BitMask;
#endif
}


// ----------------------------------------------------------------------------
/**
 * TESTPOINTS_Toggle toggle the appropriate GPIO pin, based on the test point
 * array which has already been initialised.
 *
 * @param	Offset		Index into test point array for test point to toggle.
 *
 */
// ----------------------------------------------------------------------------
void TESTPOINTS_Toggle(const uint16_t Offset)
{
#ifdef OFFSET_CHECKING_ENABLED
	if (Offset < MAXIMUM_NUMBER_OF_TESTPOINTS)
	{
		*mTestpointArray[Offset].pToggleRegister = mTestpointArray[Offset].BitMask;
	}
#else
	*mTestpointArray[Offset].pToggleRegister = mTestpointArray[Offset].BitMask;
#endif
}


// ----------------------------------------------------------------------------
/**
 * TESTPOINTS_ArrayPointerGet returns a pointer to the appropriate location
 * in the test point array, so the array can be checked to see it it's been
 * initialised correctly.
 *
 * @param	Offset			Index into test point array.
 * @retval	Testpoints_t*	Pointer to index in test point array.
 *
 */
// ----------------------------------------------------------------------------
const Testpoints_t* TESTPOINTS_ArrayPointerGet(const uint16_t Offset)
{
	return &mTestpointArray[Offset];
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

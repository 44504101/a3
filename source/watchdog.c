// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/watchdog.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		5 Jul 2012
 * @brief		Functions to control the watchdog timer on TI's 28335 DSP.
 * @details
 * Allow the watchdog to be enabled and disabled, the watchdog prescaler to be
 * adjusted, the watchdog override to be locked and a software reset generated.
 * Also allows the watchdog to be kicked.
 *
 * Note that the watchdog cannot be disabled if the WDOVERRIDE bit is clear -
 * the default state of this bit is set when the device is reset, and the bit is
 * only cleared if it is specifically written to.  Also note that all watchdog
 * control registers are protected, so EALLOW \ EDIS must be used before and
 * after writing to any of these registers.
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
#include "DSP28335_device.h"
#include "watchdog.h"
#include "genericIO.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

/* Note - disable Lint warnings for the casts here - need 32 bit arguments. */

/// Address for SCSR register
#define SCSR_ADDRESS	            (/*lint -e(921)*/(uint32_t)0x00007022u)

/// Address for WDCR register
#define WDCR_ADDRESS	            (/*lint -e(921)*/(uint32_t)0x00007029u)

#define SCSR_WDOVERRIDE_BIT_MASK	0x0001u		///< WDOVERRIDE is bit 0
#define WDCR_WDDIS_BIT_MASK         0x0040u     ///< WDDIS is bit 6
#define WDCR_WDFLAG_BIT_MASK        0x0080u     ///< WDFLAG is bit 7


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static bool_t CheckWatchdogOverrideBit(void);
static void   CheckPreviousResetType(const uint16_t controlRegister);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

//lint -e{956} Doesn't need to be volatile.
static bool_t   m_b_lastResetWasFromWatchdog;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * WATCHDOG_Disable will disable the watchdog timer (and set the watchdog
 * prescaler back to the default value of divide by 1).
 *
 * @retval	bool_t	Boolean value = TRUE if watchdog has been disabled.
 *
 */
// ----------------------------------------------------------------------------
bool_t WATCHDOG_Disable(void)
{
    bool_t      bDisabledOK = FALSE;
    bool_t      bWatchdogOverrideBitSet;
    uint16_t    RequiredData;
    uint16_t    controlRegister;

    // Read from the control register.
    controlRegister = genericIO_16bitRead(WDCR_ADDRESS);

    // Check the WDFLAG bit and set the m_b_lastResetWasFromWatchdog flag.
    CheckPreviousResetType(controlRegister);

    // check the WDOVERRIDE bit
    bWatchdogOverrideBitSet = CheckWatchdogOverrideBit();

    // if the WDOVERRIDE bit is set, then we can disable the watchdog.
    // (or if the watchdog is disabled already, in which case the override
    // bit doesn't matter because we're not trying to change the WDDIS bit).
    // The WDDIS bit is set to 1 to indicate that the watchdog is already disabled.
    if (bWatchdogOverrideBitSet || ( (controlRegister & WDCR_WDDIS_BIT_MASK) != 0u) )
    {
        // Setup all bits to write into the WDCR register, and write them
        // (register is protected so need to use EALLOW and EDIS).
        //lint -e{835, 845, 921}
        RequiredData =
            ( (uint16_t)1u << 7 ) |	    // 1: clear WDFLAG bit (by writing 1)
            ( (uint16_t)1u << 6 ) |	    // 1: disable watchdog
            ( (uint16_t)1u << 5 ) |     // 1: }
            ( (uint16_t)0u << 4 ) |     // 0: } watchdog check, must be 101
            ( (uint16_t)1u << 3 ) |     // 1: }
            ( (uint16_t)0u << 0 );	    // 000: prescaler divide by 1 (bits 2:0)

        EALLOW;
        genericIO_16bitWrite(WDCR_ADDRESS, RequiredData);
        EDIS;

        bDisabledOK = TRUE;
    }

    return bDisabledOK;
}


// ----------------------------------------------------------------------------
/**
 * WATCHDOG_Enable will enable the watchdog timer and set the required prescaler.
 *
 * @param   RequiredPrescaler   Enumerated value for required prescaler to set.
 * @retval	bool_t	            Boolean value = TRUE if watchdog has been enabled.
 *
 */
// ----------------------------------------------------------------------------
bool_t WATCHDOG_Enable(const EWatchdogPrescalers_t RequiredPrescaler)
{
    bool_t      bEnabledOK = FALSE;
    bool_t      bWatchdogOverrideBitSet;
    uint16_t    RequiredData;
    uint16_t    controlRegister;

    // Read from the control register.
    controlRegister = genericIO_16bitRead(WDCR_ADDRESS);

    // Check the WDFLAG bit and set the m_b_lastResetWasFromWatchdog flag.
    CheckPreviousResetType(controlRegister);

    // check the WDOVERRIDE bit
    bWatchdogOverrideBitSet = CheckWatchdogOverrideBit();

    // if the WDOVERRIDE bit is set, then we can enable the watchdog.
    // (or if the watchdog is enabled already, in which case the override
    // bit doesn't matter because we're not trying to change the WDDIS bit).
    // The WDDIS bit is set to 0 to indicate that the watchdog is already enabled.
    if (bWatchdogOverrideBitSet || ( (controlRegister & WDCR_WDDIS_BIT_MASK) == 0u) )
    {
        // Setup all bits to write into the WDCR register, and write them
        // (register is protected so need to use EALLOW and EDIS).
        //lint -e{835, 845, 921}
        //lint -e{930} Cast of enum prescaler to uint16_t OK, all values +ve.
        RequiredData =
            ( (uint16_t)1u << 7 ) |	    // 1: clear WDFLAG bit (by writing 1)
            ( (uint16_t)0u << 6 ) |	    // 0: enable watchdog
            ( (uint16_t)1u << 5 ) |     // 1: }
            ( (uint16_t)0u << 4 ) |     // 0: } watchdog check, must be 101
            ( (uint16_t)1u << 3 ) |     // 1: }
            ( (uint16_t)RequiredPrescaler << 0 );	// prescaler divide by ??? (bits 2:0)

        EALLOW;
        genericIO_16bitWrite(WDCR_ADDRESS, RequiredData);
        EDIS;

        bEnabledOK = TRUE;
    }

    return bEnabledOK;
}


// ----------------------------------------------------------------------------
/**
 * WATCHDOG_LockWDOverrideBit clears the WDOVERRIDE bit (by writing a 1 to it)
 * to ensure that the watchdog cannot be disabled until a device reset occurs.
 *
 */
// ----------------------------------------------------------------------------
void WATCHDOG_LockWDOverrideBit(void)
{
    uint16_t RequiredData;

    // Setup all bits to write into the SCSR register, and write them
    // (register is protected so need to use EALLOW and EDIS).
    //lint -e{835, 845, 921}
    RequiredData =
        ( (uint16_t)1u << 2 ) |     // 1: watchdog interrupt inactive
        ( (uint16_t)0u << 1 ) |     // 0: watchdog reset enabled, int disabled
        ( (uint16_t)1u << 0 );	    // 1: clears WDOVERRIDE bit by writing a 1

    EALLOW;
    genericIO_16bitWrite(SCSR_ADDRESS, RequiredData);
    EDIS;
}


// ----------------------------------------------------------------------------
/**
 * WATCHDOG_ForceSoftwareReset forces a software reset by not adhering to the
 * rule that the pattern 101 must always be written to the WDCHK bits of the
 * WDCR register.  This method of forcing a reset is suggested in the data
 * sheet. Note that this code is not tested under TDD because it doesn't use
 * the genericIO functions, so gcov will flag this code as having zero coverage.
 *
 */
// ----------------------------------------------------------------------------
void WATCHDOG_ForceSoftwareReset(void)
{
    EALLOW;
    SysCtrlRegs.WDCR = 0x0000u;
    EDIS;
}


// ----------------------------------------------------------------------------
/**
 * WATCHDOG_KickDog resets the watchdog counter (kicks the watchdog).  This is
 * achieved by writing 0x55 followed by 0xAA to the WDKEY register.  We use
 * magic numbers here for the values to write because it doesn't really add
 * anything to use defines.
 *
 * @note
 * This code is not tested with unit tests because it doesn't use the genericIO
 * functions - we're using the bitfields as it will result in more efficient code,
 * and this function will be called frequently.  gcov will therefore flag this
 * code as having zero coverage.
 *
 */
// ----------------------------------------------------------------------------
void WATCHDOG_KickDog(void)
{
    EALLOW;
    SysCtrlRegs.WDKEY = 0x0055u;
    SysCtrlRegs.WDKEY = 0x00AAu;
    EDIS;
}


// ----------------------------------------------------------------------------
/**
 * WATCHDOG_IsEnabledCheck tests the WDDIS bit in the WDCR register to
 * determine whether the watchdog timer is enabled or not.
 *
 * @retval  bool_t  Boolean value = TRUE if watchdog is enabled.
 *
 */
// ----------------------------------------------------------------------------
bool_t WATCHDOG_IsEnabledCheck(void)
{
    bool_t      bWatchdogIsEnabled = TRUE;
    uint16_t    controlRegister;

    // Read from the control register and mask off everything except WDDIS bit.
    // This bit is set to 1 to indicate that the watchdog is disabled.
    controlRegister = genericIO_16bitRead(WDCR_ADDRESS);
    controlRegister &= WDCR_WDDIS_BIT_MASK;

    if (controlRegister != 0u)
    {
        bWatchdogIsEnabled = FALSE;
    }

    return bWatchdogIsEnabled;
}


// ----------------------------------------------------------------------------
/**
 *  WATCHDOG_LastResetWasWatchdog returns the m_b_lastResetWasFromWatchdog flag.
 *
 * @retval  bool_t  Boolean value = TRUE if last reset was from watchdog.
 *
 */
// ----------------------------------------------------------------------------
bool_t WATCHDOG_LastResetWasWatchdog(void)
{
    return m_b_lastResetWasFromWatchdog;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * CheckWatchdogOverrideBit reads from the SCSR to check the status of the
 * WDOVERRIDE bit, and returns TRUE or FALSE depending on whether the bit is
 * set or not.
 *
 * @retval	bool_t	Boolean value - TRUE if override bit is set, FALSE if not.
 *
 */
// ----------------------------------------------------------------------------
static bool_t CheckWatchdogOverrideBit(void)
{
    uint16_t status;
    bool_t OverrideBitSet = TRUE;

    status = genericIO_16bitRead(SCSR_ADDRESS);

    // If the WDOVERRIDE bit is cleared, then bit is not set, so clear flag.
    if (( status & SCSR_WDOVERRIDE_BIT_MASK ) == 0u)
    {
        OverrideBitSet = FALSE;
    }

    return OverrideBitSet;
}


// ----------------------------------------------------------------------------
/**
 * CheckPreviousResetType checks the WDFLAG bit in the control register, and
 * sets the m_b_lastResetWasFromWatchdog to the correct state depending on the
 * bit.
 *
 * @param   controlRegister     The watchdog control register.
 *
 */
// ----------------------------------------------------------------------------
static void CheckPreviousResetType(const uint16_t controlRegister)
{
    // If the WDFLAG bit is set, then the last reset came from the watchdog.
    if ( (controlRegister & WDCR_WDFLAG_BIT_MASK) != 0u)
    {
        m_b_lastResetWasFromWatchdog = TRUE;
    }
    else
    {
        m_b_lastResetWasFromWatchdog = FALSE;
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

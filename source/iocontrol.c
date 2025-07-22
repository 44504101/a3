/*
 * iocontrol.c
 *
 *  Created on: 2022Äê6ÔÂ13ÈÕ
 *      Author: LeiGe
 */

#include "common_data_types.h"
#include "iocontrol.h"
#include "DSP28335_device.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here

#define SPI_WP_LO           (GpioDataRegs.GPBCLEAR.bit.GPIO50  = 1u)    ///< SPI Write protection enabled.
#define SPI_WP_HI           (GpioDataRegs.GPBSET.bit.GPIO50    = 1u)    ///< SPI Write protection disabled.
#define FLASH1_WP_LO        (GpioDataRegs.GPBCLEAR.bit.GPIO51  = 1u)    ///< FLASH1 Write protection enabled.
#define FLASH1_WP_HI        (GpioDataRegs.GPBSET.bit.GPIO51    = 1u)    ///< FLASH1 Write protection disabled.
#define FLASH2_WP_LO        (GpioDataRegs.GPBCLEAR.bit.GPIO52  = 1u)    ///< FLASH2 Write protection enabled.
#define FLASH2_WP_HI        (GpioDataRegs.GPBSET.bit.GPIO52    = 1u)    ///< FLASH2 Write protection disabled.
#define FLASH_RESET_LO      (GpioDataRegs.GPBCLEAR.bit.GPIO58  = 1u)    ///< FLASH_RESET_LO flash reset asserted.
#define FLASH_RESET_HI      (GpioDataRegs.GPBSET.bit.GPIO58    = 1u)    ///< FLASH_RESET_LO flash reset de-asserted.
#define LOW_OIL_LO          (GpioDataRegs.GPACLEAR.bit.GPIO1   = 1u)    ///< Oil level detection enabled.
#define LOW_OIL_HI          (GpioDataRegs.GPASET.bit.GPIO1     = 1u)    ///< Oil level detection disabled.
#define FLASH1_RDBY_IO      (GpioDataRegs.GPBDAT.bit.GPIO53)            ///< Flash1 not busy if asserted.
#define FLASH2_RDBY_IO      (GpioDataRegs.GPBDAT.bit.GPIO59)            ///< Flash2 not busy if asserted.
#define LOW_OIL_IO          (GpioDataRegs.GPADAT.bit.GPIO27)            ///< Low oil level detected if asserted.
#define LOW_VOLTAGE_IO      (GpioDataRegs.GPADAT.bit.GPIO9)             ///< Low 5V voltage detected if de-asserted.
#define OIL_SENSOR_DET_IO   (GpioDataRegs.GPADAT.bit.GPIO11)            ///< Oil level detection enabled if asserted.
#define LED_ON              (GpioDataRegs.GPBSET.bit.GPIO62     = 1u)   ///< LED on.
#define LED_OFF             (GpioDataRegs.GPACLEAR.bit.GPIO7    = 1u)   ///< LED off.
#define LED_TOGGLE          (GpioDataRegs.GPATOGGLE.bit.GPIO7   = 1u)   ///< LED toggle.
#define HSB100_RS485_RX     (GpioDataRegs.GPACLEAR.bit.GPIO10   = 1u)   ///< HSB100 RS485 interface in Rx mode.
#define HSB100_RS485_TX     (GpioDataRegs.GPASET.bit.GPIO10     = 1u)   ///< HSB100 RS485 interface in Tx mode.
#define HSB100_nRESET_LOW   (GpioDataRegs.GPACLEAR.bit.GPIO8    = 1u)   ///< Reset the HSB100 modem.
#define HSB100_nRESET_HIGH  (GpioDataRegs.GPASET.bit.GPIO8      = 1u)   ///< Release the reset signal of the HSB100 modem.


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module


// ----------------------------------------------------------------------------
// Variables which only have scope within this module


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_SPIWriteProtectEnable drives the !W pin on the M95M01 low.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_SPIWriteProtectEnable(void)
{
    SPI_WP_LO;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_SPIWriteProtectDisable drives the !W pin on the M95M01 high.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_SPIWriteProtectDisable(void)
{
    SPI_WP_HI;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_Flash1WriteProtectEnable drives the !WP pin on the flash low.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_Flash1WriteProtectEnable(void)
{
    FLASH1_WP_LO;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_Flash1WriteProtectDisable drives the !WP pin on the flash high.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_Flash1WriteProtectDisable(void)
{
    FLASH1_WP_HI;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_Flash2WriteProtectEnable drives the !WP pin on the flash low.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_Flash2WriteProtectEnable(void)
{
    FLASH2_WP_LO;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_Flash2WriteProtectDisable drives the !WP pin on the flash high.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_Flash2WriteProtectDisable(void)
{
    FLASH2_WP_HI;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_FlashHoldInReset holds the flash chips in reset by driving the
 * !RESET line low.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_FlashHoldInReset(void)
{
    FLASH_RESET_LO;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_FlashReleaseFromReset releases the flash chips from reset by#
 * driving the !RESET line high.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_FlashReleaseFromReset(void)
{
    FLASH_RESET_HI;
}

// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_LowOilSensorEnable drives the pin low to enable the sensor (there
 * is an inverter on the board, so we're actually sourcing current through the
 * sensor by driving the DSP I/O low).
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_LowOilSensorEnable(void)
{
    LOW_OIL_LO;
}

// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_LowOilSensorDisable drives the pin high to disable the sensor.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_LowOilSensorDisable(void)
{
    LOW_OIL_HI;
}

// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_RS485_HSB100ModemTxModeSet sets the HSB100 RS485 interface driver in Tx mode.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_RS485_HSB100ModemTxModeSet(void)
{
    HSB100_RS485_TX;
}

// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_RS485_HSB100ModemTxModeSet sets the HSB100 RS485 interface driver in Rx mode.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_RS485_HSB100ModemRxModeSet(void)
{
    HSB100_RS485_RX;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_HSB100ModemReset resets the HSB100 modem.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_HSB100ModemReset(void)
{
    HSB100_nRESET_LOW;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_HSB100ModemResetSignalRelease releases the HSB100 modem reset signal.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_HSB100ModemResetSignalRelease(void)
{
    HSB100_nRESET_HIGH;
}

// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_Flash1ChecksForBusy reads the RD/!BY line to check for the flash
 * ready / busy.  If the pin is low then we're currently busy.
 *
 * @retval  bool_t      TRUE if flash is busy, FALSE if flash is not busy.
 *
*/
// ----------------------------------------------------------------------------
bool_t IOCONTROL_Flash1CheckForBusy(void)
{
    bool_t  bFlashIsBusy = FALSE;

    if (FLASH1_RDBY_IO == 0u)
    {
        bFlashIsBusy = TRUE;
    }

    return bFlashIsBusy;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_Flash2ChecksForBusy reads the RD/!BY line to check for the flash
 * ready / busy.  If the pin is low then we're currently busy.
 *
 * @retval  bool_t      TRUE if flash is busy, FALSE if flash is not busy.
 *
*/
// ----------------------------------------------------------------------------
bool_t IOCONTROL_Flash2CheckForBusy(void)
{
    bool_t  bFlashIsBusy = FALSE;

    if (FLASH2_RDBY_IO == 0u)
    {
        bFlashIsBusy = TRUE;
    }

    return bFlashIsBusy;
}




// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_LowOilSensorCheckForMade()
 * Reads the LOW_OIL pin to determine if the
 * low oil sensor is made (LOW_OIL_LEVEL) or not(HIGH_OIL_LEVEL).
 * The function checks that the oil sensor is actually
 * enabled, if not the function returns a
 * OIL_SENSOR_DISABLED status
 *
 * @retval  oilLevel    Oil sensor value:  High, Low or disabled
  *
 */
// ----------------------------------------------------------------------------
EoilLevel_t IOCONTROL_LowOilSensorCheckForMade(void)
{
    EoilLevel_t oilLevel = OIL_SENSOR_DISABLED;

    if (1u ==  OIL_SENSOR_DET_IO ) // Sensor enabled
    {
        if ( 1u == LOW_OIL_IO )
        {
            oilLevel = LOW_OIL_LEVEL;
        }
        else
        {
            oilLevel = HIGH_OIL_LEVEL;
        }
    }

    return oilLevel;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_LowVoltageIndicatorCheckForLowVoltage reads the LBO pin - if the
 * pin is low then the input voltage is too low.
 *
 * @retval  bool_t      TRUE if voltage is too low, FALSE if voltage is OK.
 *
*/
// ----------------------------------------------------------------------------
bool_t IOCONTROL_LowVoltageIndicatorCheckForLowVoltage(void)
{
    bool_t  bVoltageIsTooLow = FALSE;

    if (LOW_VOLTAGE_IO == 0u)
    {
        bVoltageIsTooLow = TRUE;
    }

    return bVoltageIsTooLow;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_LedOn turns the led on.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_LedOn(void)
{
    LED_ON;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_LedOff turns the led off.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_LedOff(void)
{
    LED_OFF;
}


// ----------------------------------------------------------------------------
/**
 *
 * IOCONTROL_LedToggle toggles the led.
 *
*/
// ----------------------------------------------------------------------------
void IOCONTROL_LedToggle(void)
{
    LED_TOGGLE;
}

/*
 * iocontrol.h
 *
 *  Created on: 2022Äê6ÔÂ13ÈÕ
 *      Author: LeiGe
 */

#ifndef HEADER_IOCONTROL_H_
#define HEADER_IOCONTROL_H_

/// Enumerated values for the different possible state of the oil level sensor.
typedef enum
{
    LOW_OIL_LEVEL,              ///< Low oil level detected.
    HIGH_OIL_LEVEL,             ///< High oil level detected.
    OIL_SENSOR_DISABLED         ///< Oil level sensor disabled.
} EoilLevel_t;

void        IOCONTROL_SPIWriteProtectEnable(void);
void        IOCONTROL_SPIWriteProtectDisable(void);
void        IOCONTROL_FlashReleaseFromReset(void);
void        IOCONTROL_Flash1WriteProtectDisable(void);
void        IOCONTROL_Flash2WriteProtectDisable(void);
#endif /* HEADER_IOCONTROL_H_ */

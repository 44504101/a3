// ----------------------------------------------------------------------------
/**
 * @file    	pwm.h
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		04 Mar 2014
 * @brief		Header file for pwm.c.
 * @note		Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2014.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
*/
// ----------------------------------------------------------------------------
#ifndef PWM_H_
#define PWM_H_

void 	PWM_Initialise(void);
void 	PWM_DisableAll(void);
void 	PWM_FrameEnable(void);
void 	PWM_FrameDisable(void);

#endif /* PWM_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


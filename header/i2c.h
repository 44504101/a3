/*
 * i2c.h
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

#ifndef HEADER_I2C_H_
#define HEADER_I2C_H_

/// Enumerated values for I2C operation status.
typedef enum
{
	I2C_STP_NOT_READY = 0,                  ///< Stop bit not ready.
	I2C_BUS_BUSY = 1,                       ///< Bus is busy.
	I2C_NO_ACK_RECEIVED_FROM_SLAVE = 2,     ///< No acknowledgement received from slave.
	I2C_COMPLETED_OK = 3,                   ///< Operation completed OK.
	I2C_ACKPOLL_TIMEOUT_EXCEEDED = 4,       ///< Acknowledgement polling has exceeded timeout.
	I2C_UNKNOWN_ERROR = 5                   ///< Some unknown error.
} EI2CStatus_t;

bool_t				I2C_Open(const uint32_t iSysClk_Hz, const uint32_t iDataRate);
void 				I2C_Close(void);


extern EI2CStatus_t	(*I2C_Read)(const uint16_t SlaveAddress,
                                const uint16_t DeviceAddress,
                                uint16_t DataCount,
                                uint8_t * const pData);


extern EI2CStatus_t	(*I2C_Write)(const uint16_t SlaveAddress,
                                 const uint16_t DeviceAddress,
                                 uint16_t DataCount,
                                 const uint8_t * const pData);


extern EI2CStatus_t	(*I2C_AckPoll)(uint16_t SlaveAddress, uint16_t MaxTimeout);

void                I2C_AckPollTimeoutFlagSet(void);



#endif /* HEADER_I2C_H_ */

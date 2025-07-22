/*
 * spi.h
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

#ifndef HEADER_SPI_H_
#define HEADER_SPI_H_

void        SPI_Open(uint16_t RequiredNumberOfBits);
void        SPI_Close(void);
bool_t      SPI_BaudRateSet(const uint32_t iLspClk_Hz, const uint32_t iBaudRate);
uint16_t    SPI_Read(const uint16_t DummyWord);
void        SPI_Write(uint16_t DataToWrite);
void 		SPI_EEPROMActiveSet(void);
void 		SPI_EEPROMInactiveSet(void);
void 		SPI_RTCActiveSet(void);
void		SPI_RTCInactiveSet(void);
uint16_t    SPI_NumberOfDataBitsGet(void);
uint16_t    SPI_ReceivedBitMaskGet(void);

#endif /* HEADER_SPI_H_ */

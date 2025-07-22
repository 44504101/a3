// ----------------------------------------------------------------------------
/**
 * @file    common/platform/sci.h
 * @author  Simon Haworth (SHaworth@slb.com)
 * @brief   Header file for sci.c
 * @note	Please refer to the .c file for a detailed functional description.
 * @attention
 * (c) Copyright Schlumberger Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Schlumberger Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 */
// ----------------------------------------------------------------------------
#ifndef SCI_H_
#define SCI_H_

#ifdef UNIT_TEST_BUILD
#define interrupt           //lint !e9051 Re-use of C keyword is deliberate.
#endif

typedef void(*pTriggerTimerFunction)(void);

/// Number of interrupt routine counters for the serial driver.
#define SCI_MAX_IRQ_COUNTERS    6u

/// Enumerated values for the different serial port modules.
typedef enum
{
	SCI_A                   = 0,        ///< Serial port A.
	SCI_B                   = 1,        ///< Serial port B.
	SCI_C                   = 2,        ///< Serial port C.
	SCI_NUMBER_OF_PORTS     = 3         ///< The number of serial ports.
} ESCIModule_t;


void 		    SCI_Open(const ESCIModule_t module);

void 	        SCI_TimerFunctionAssign(const ESCIModule_t module,
     	                                const pTriggerTimerFunction p_triggerTimerDo);

void 		    SCI_Close(const ESCIModule_t module);

bool_t 		    SCI_BaudRateSet(const ESCIModule_t module,
       		                    const uint32_t iLspClk_Hz,
       		                    const uint32_t iBaudRate);

void 		    SCI_RxBufferInitialise(const ESCIModule_t module,
     		                           uint8_t * const p_receiveBuffer,
     		                           const uint16_t maxRxLength);

void            SCI_RxTriggerInitialise(const ESCIModule_t module,
                                        const bool_t b_matchRequired,
                                        const uint8_t matchCharacter,
                                        void * const p_receiveSemaphore);

uint16_t 	    SCI_RxBufferNumberOfCharsGet(const ESCIModule_t module);

void            SCI_TxTriggerInitialise(const ESCIModule_t module,
                                        void * const p_transmitSemaphore);

void 		    SCI_TxStart(const ESCIModule_t module,
     		                const uint8_t * const p_transmitBuffer,
     		                const uint16_t lengthOfMessageToTransmit);

bool_t		    SCI_TxDoneCheck(const ESCIModule_t module);

interrupt void  SCI_RxInterruptA_ISR(void);
interrupt void  SCI_TxInterruptA_ISR(void);
interrupt void  SCI_RxInterruptB_ISR(void);
interrupt void  SCI_TxInterruptB_ISR(void);
interrupt void  SCI_RxInterruptC_ISR(void);
interrupt void  SCI_TxInterruptC_ISR(void);

uint16_t        SCI_ModuleInterruptCountGet(const uint16_t index);

const char_t*   SCI_ModuleInterruptStringGet(const uint16_t index);

#endif /* SCI_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

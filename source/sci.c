// ----------------------------------------------------------------------------
/**
 * @file    	common/platform/sci.c
 * @author		Simon Haworth (SHaworth@slb.com)
 * @date		24 Apr 2012
 * @brief		SCI (serial port) driver for TI's 28335 DSP.
 * @details
 * Functions are provided to open and close one of the available SCI ports
 * (SCI-A, SCI-B or SCI-C), setting up the baud rate, number of bits, parity and
 * multiprocessor addressing. Functions are also provided to read and write to
 * the receive and transmit registers of the SCI port(s), along with the
 * interrupt handlers for rx, rx error, tx and tx error.  The device registers
 * are accessed as 16 bit reads \ writes, using the genericIO functions - this
 * allows the code to be tested using TDD, as the IO functions can be mocked out
 * (whereas direct access using a pointer to a volatile cannot).
 *
 * The convention for writing the individual bits comes from a series of
 * articles on embeddedgurus.com by Nigel Jones, in which he makes a reasoned
 * case for not using bit fields or defines for the individual bits, to reduce
 * the likelihood of errors (please refer to Configuring Hardware parts 1-3).
 * As part of this implementation, we disable lint warnings for zero being given
 * as an argument to ?? (835) and also for the right side of the | operator
 * being zero (845).  We also perform some fairly heavy duty casting, for
 * compliance with MISRA rule 10.5, to ensure that the data is always 16 bits
 * following a shift, i.e. it is implementation independent.
 *
 * @warning
 * The GPIO multiplexers need to be set-up so that the SCI pins are mux'ed
 * through to the correct IO pins - this will need to be taken care of in a
 * separate module, so all of the muxes are setup at the same time (which also
 * allows for different pin mappings for different DSP's, while still using
 * this common code).
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
#include "sci.h"
#include "genericIO.h"
#include "testpoints.h"
#include "testpointoffsets.h"

#ifdef FREE_RTOS_USED
#include "FreeRTOS.h"
#include "semphr.h"
#endif

// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define SCI_A_BASE_ADDRESS	0x00007050u		///< Base address for SCI-A
#define SCI_B_BASE_ADDRESS	0x00007750u		///< Base address for SCI-B
#define SCI_C_BASE_ADDRESS	0x00007770u		///< Base address for SCI-C

#define	SCICCR_OFFSET		0x0000u			///< Offset from base for SCICCR
#define SCICTL1_OFFSET		0x0001u			///< Offset from base for SCICTL1
#define SCIHBAUD_OFFSET		0x0002u			///< Offset from base for SCIHBAUD
#define SCILBAUD_OFFSET		0x0003u			///< Offset from base for SCILBAUD
#define SCICTL2_OFFSET		0x0004u			///< Offset from base for SCICTL2

#if 0
// These aren't used, and Lint complains. Keep them here for completeness.
#define SCIRXST_OFFSET		0x0005u			///< Offset from base for SCIRXST
#define SCIRXBUF_OFFSET		0x0007u			///< Offset from base for SCIRXBUF
#define SCITXBUF_OFFSET		0x0009u			///< Offset from base for SCITXBUF
#endif

#define SCIFFTX_OFFSET		0x000Au			///< Offset from base for SCIFFTX
#define SCIFFRX_OFFSET		0x000Bu			///< Offset from base for SCIFFRX
#define SCIFFCT_OFFSET		0x000Cu			///< Offset from base for SCIFFCT
#define SCIPRI_OFFSET		0x000Fu			///< Offset from base for SCIPRI

#if 0
// These aren't used, and Lint complains. Keep them here for completeness.
#define SCIRXST_RXERROR_BIT_MASK    0x0080u ///< RXERROR is bit 7
#define SCICTL1_SWRESET_BIT_MASK    0x0020u ///< SWRESET is bit 5
#define SCIFFRX_RXFFST_BIT_MASK     0x1F00u ///< number of words in fifo bits 12:8
#define SCIFFRX_RXFFST_BIT_SHIFT    8u      ///< shift down by 8 bits
#endif

#define SCIRXBUF_ERROR_BIT_MASK		0xC000u	///< Bits 15 and 14
#define SCIFFTX_TXFFST_BIT_MASK		0x1F00u ///< number of words in fifo bits 12:8
#define SCIFFTX_TXFFST_BIT_SHIFT	8u		///< shift down by 8 bits
#define SCICTL2_TXEMPTY_BIT_MASK	0x0040u	///< TXEMPTY is bit 6

#define SCI_TX_FIFO_DEPTH	(uint16_t)16u  ///< hardware specific FIFO depth

#define SCI_A_RX_IRQ_COUNT_INDEX    0u
#define SCI_A_TX_IRQ_COUNT_INDEX    1u
#define SCI_B_RX_IRQ_COUNT_INDEX    2u
#define SCI_B_TX_IRQ_COUNT_INDEX    3u
#define SCI_C_RX_IRQ_COUNT_INDEX    4u
#define SCI_C_TX_IRQ_COUNT_INDEX    5u


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static inline void  RxInterruptHandler(const ESCIModule_t module,
                                       volatile struct SCI_REGS * const p_sciRegs,
                                       const uint16_t pieAckGroup);

static inline void  RxIntPutCharInBufferAndCheckIt(const ESCIModule_t module,
                                                   const uint16_t dataWord);

static inline void  TxInterruptHandler(const ESCIModule_t module,
                                       volatile struct SCI_REGS * const p_sciRegs,
                                       const uint16_t pieAckGroup);

static void         ResetAllSCIRegisters(const uint32_t iBaseAddress);
static uint32_t     SetupSCIBaseAddress(const ESCIModule_t module);
static uint16_t     GetNumberOfCharsInTxFifo(const uint32_t baseAddress);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/// Structure to hold serial port related variables.
typedef struct
{
    uint8_t*                p_rxBuffer;          ///< Pointer to receive buffer.
    uint16_t                rxOffset;            ///< Current offset into buffer.
    uint16_t                rxMaxLength;         ///< Maximum length of receive buffer.
    pTriggerTimerFunction   p_timerTrigger;      ///< Pointer to the function triggering sci related timer.
    bool_t                  b_matchRequired;     ///< Flag to say character match required.
    uint8_t                 matchCharacter;      ///< 'Character' to match.
    uint16_t                matchCounter;        ///< Match counter, pseudo-binary semaphore.
    void *                  p_receiveSemaphore;  ///< Pointer to receive semaphore.

    const uint8_t*          p_txBuffer;          ///< Pointer to transmit buffer.
    uint16_t                txOffset;            ///< Current offset into buffer.
    uint16_t                txMessageLength;     ///< Length of message to send.
    void *                  p_transmitSemaphore; ///< Pointer to transmit semaphore.
} serialPortVars_t;


/// Local array of structures for the serial port modules, SCI A through SCI C.
//lint -e{956} Doesn't need to be volatile.
static serialPortVars_t m_serialPorts[SCI_NUMBER_OF_PORTS];

/// Local counters for all interrupts in this module.
//lint -e{956} Doesn't need to be volatile.
static uint16_t         m_moduleIrqCounters[SCI_MAX_IRQ_COUNTERS]
                                                = {0u, 0u, 0u, 0u, 0u, 0u};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * SCI_Open opens a serial port on the 28335.
 * Enables the transmitter and receiver.  Sets up standard 8N1, with transmit
 * and receive interrupts enabled.  The receive fifo is set to a trigger level
 * of 1, because all the SLB comms protocols are not fixed length.
 * Also initialises all the serial port variables for the required port.
 *
 * @param	Module					Enumerated type for which serial port to open.
 *
 */
// ----------------------------------------------------------------------------
void SCI_Open(const ESCIModule_t module)
{
    uint32_t    baseAddress;
    uint16_t    requiredData;

    if (module < SCI_NUMBER_OF_PORTS)
    {
        baseAddress = SetupSCIBaseAddress(module);

        // Initialise all the SCI registers to zero (just in case)
        ResetAllSCIRegisters(baseAddress);

        // Reset all serial port variables - note that we do this before
        // initialising the module, just in case we get any interrupts (and if
        // the variables weren't initialised, bad stuff could happen).
        m_serialPorts[module].p_rxBuffer          = NULL;
        m_serialPorts[module].rxOffset            = 0u;
        m_serialPorts[module].rxMaxLength         = 0u;
        m_serialPorts[module].p_timerTrigger      = NULL;
        m_serialPorts[module].b_matchRequired     = FALSE;
        m_serialPorts[module].matchCharacter      = 0x00u;
        m_serialPorts[module].matchCounter        = 0u;
        m_serialPorts[module].p_receiveSemaphore  = NULL;
        m_serialPorts[module].p_txBuffer          = NULL;
        m_serialPorts[module].txOffset            = 0u;
        m_serialPorts[module].txMessageLength     = 0u;
        m_serialPorts[module].p_transmitSemaphore = NULL;

        // Setup all bits to write into the SCICCR register, and write them
        //lint -e{835, 845, 921}
        requiredData =
            ( (uint16_t)0u << 7 ) |	    // 0: one stop bit
            ( (uint16_t)0u << 6 ) |	    // 0: odd parity
            ( (uint16_t)0u << 5 ) |     // 0: parity disabled
            ( (uint16_t)0u << 4 ) |     // 0: loop back mode disabled
            ( (uint16_t)0u << 3 ) |     // 0: idle-line mode protocol selected
            ( (uint16_t)7u << 0 );	    // 111: 8 bit data (bits 2:0)
        genericIO_16bitWrite(( baseAddress + SCICCR_OFFSET ), requiredData); //lint !e835

        // Setup all bits to write into the SCICTL1 register, and write them
        //lint -e{835, 845, 921}
        requiredData =
            ( (uint16_t)0u << 7 ) |	    // 0: reserved bit, writes have no effect
            ( (uint16_t)1u << 6 ) |	    // 1: receiver error interrupt enabled
            ( (uint16_t)1u << 5 ) |     // 1: initialise the SCI state machine and operating flags
            ( (uint16_t)0u << 4 ) |     // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 3 ) |     // 0: transmit wake-up feature not selected
            ( (uint16_t)0u << 2 ) |     // 0: sleep mode disabled
            ( (uint16_t)1u << 1 ) |     // 1: transmitter enabled
            ( (uint16_t)1u << 0 );      // 1: allow received characters into RXREG
        genericIO_16bitWrite(( baseAddress + SCICTL1_OFFSET ), requiredData);

        // Setup all bits to write into the SCICTL2 register, and write them
        //lint -e{835, 845, 921}
        requiredData =
            ( (uint16_t)0u << 7 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 6 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 5 ) |     // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 4 ) |     // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 3 ) |     // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 2 ) |     // 0: reserved bit, writes have no effect
            ( (uint16_t)1u << 1 ) |     // 1: RXRDY\BRKDT interrupt enabled
            ( (uint16_t)1u << 0 );	    // 1: TXRDY interrupt enabled
        genericIO_16bitWrite(( baseAddress + SCICTL2_OFFSET ), requiredData);

        // Setup all bits to write into the SCIFFTX register, and write them
        //lint -e{835, 845, 921}
        requiredData =
            ( (uint16_t)1u << 15 ) |    // 1: SCI FIFO can resume RX or TX
            ( (uint16_t)1u << 14 ) |    // 1: SCI FIFO is enabled
            ( (uint16_t)1u << 13 ) |    // 1: SCI FIFO transmit pointer enabled
            ( (uint16_t)0u << 12 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 11 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 10 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 9 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 8 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 7 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)1u << 6 ) |	    // 1: clear any outstanding TXFFINT
            ( (uint16_t)0u << 5 ) | 	// 0: TX FIFO interrupt based on match disabled
            ( (uint16_t)0u << 0 );	    // 00000: TX FIFO interrupt depth = 0 (bits 4:0)
        genericIO_16bitWrite(( baseAddress + SCIFFTX_OFFSET ), requiredData);

        // Setup all bits to write into the SCIFFRX register, and write them
        //lint -e{835, 845, 921}
        requiredData =
            ( (uint16_t)0u << 15 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)1u << 14 ) |    // 1: clear any outstanding RXFFOVF
            ( (uint16_t)1u << 13 ) |    // 1: SCI FIFO receive pointer enabled
            ( (uint16_t)0u << 12 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 11 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 10 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 9 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 8 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)0u << 7 ) |	    // 0: read only bit, writes have no effect
            ( (uint16_t)1u << 6 ) |	    // 1: clear any outstanding RXFFINT
            ( (uint16_t)1u << 5 ) |     // 1: RX FIFO interrupt based on match enabled
            ( (uint16_t)1u << 0 );	    // 00001: RX FIFO interrupt depth = 1 (bits 4:0)
        genericIO_16bitWrite(( baseAddress + SCIFFRX_OFFSET ), requiredData);

        // Setup all bits to write into the SCIFFCT register, and write them
        //lint -e{835, 845, 921}
        requiredData =
            ( (uint16_t)0u << 15 ) |    // 0: read only bit, writes have no effect
            ( (uint16_t)1u << 14 ) |    // 1: clear any ABD flag
            ( (uint16_t)0u << 13 ) |    // 0: disable auto baud detection
            ( (uint16_t)0u << 12 ) |    // 0: reserved, writes have no effect
            ( (uint16_t)0u << 11 ) |    // 0: reserved, writes have no effect
            ( (uint16_t)0u << 10 ) |    // 0: reserved, writes have no effect
            ( (uint16_t)0u << 9 ) |	    // 0: reserved, writes have no effect
            ( (uint16_t)0u << 8 ) |	    // 0: reserved, writes have no effect
            ( (uint16_t)0u << 0 );	    // 00000000: FIFO transfer delay = 0 (bits 7:0)
        genericIO_16bitWrite(( baseAddress + SCIFFCT_OFFSET ), requiredData);

        // Setup all bits to write into the SCIPRI register, and write them
        //lint -e{835, 845, 921}
        requiredData =
            ( (uint16_t)0u << 7 ) |	    // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 6 ) |	    // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 5 ) |	    // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 3 ) |	    // 00: immediate stop on suspend (bits 4:3)
            ( (uint16_t)0u << 2 ) |	    // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 1 ) |	    // 0: reserved bit, writes have no effect
            ( (uint16_t)0u << 0 );	    // 0: reserved bit, writes have no effect
        genericIO_16bitWrite(( baseAddress + SCIPRI_OFFSET ), requiredData);
    }
}


// ----------------------------------------------------------------------------
/**
 * SCI_TimerFunctionAssign assigns the timer trigger function pointer to an
 * sci module.
 *
 * @param	Module					Enumerated type for which serial port to open.
 * @param	pTimerTriggerFunction	Pointer to the function triggering the interrupt related timer.
 *
 */
// ----------------------------------------------------------------------------
void SCI_TimerFunctionAssign(const ESCIModule_t module,
                             const pTriggerTimerFunction p_triggerTimerDo)
{
    if (module < SCI_NUMBER_OF_PORTS)
    {
        m_serialPorts[module].p_timerTrigger = p_triggerTimerDo;
    }
}

// ----------------------------------------------------------------------------
/**
 * SCI_Close closes (disables) a serial port on the 28335.
 *
 * @param	Module	Enumerated type for which serial port to close.
 *
 */
// ----------------------------------------------------------------------------
void SCI_Close(const ESCIModule_t module)
{
    uint32_t    baseAddress;

    baseAddress = SetupSCIBaseAddress(module);
    ResetAllSCIRegisters(baseAddress);
}


// ----------------------------------------------------------------------------
/**
 * SCI_BaudRateSet sets up the baud rate generator for a serial port.
 *
 * @warning
 * This does not check for non-exact baud rate values, it is the responsibIlity
 * of the programmer to ensure that your LSPCLK will give you the baud rate you
 * require (or near	enough to work).
 *
 * @param	Module		Enumerated type for which serial port to open.
 * @param	iLspClk_Hz	LSPCLK which is fed into the serial port, in Hz.
 * @param	iBaudRate	Required baud rate, in bits per second.
 * @retval	bool_t		TRUE if baud rate OK, FALSE if value out of range.
 *
 */
// ----------------------------------------------------------------------------
bool_t SCI_BaudRateSet(const ESCIModule_t module,
                       const uint32_t iLspClk_Hz,
                       const uint32_t iBaudRate)
{
    uint32_t    iResult;
    bool_t      b_validBaudRate = TRUE;
    uint16_t    iData;
    uint32_t    baseAddress;

    // Baud rate divider = (LSPCLK / (BAUD RATE * 8)) - 1
    // (taken from table 2.5 in SPRUFZ5A - SCI reference manual).
    iResult = ( iLspClk_Hz / ( iBaudRate * 8u ) ) - 1u;

    // If result overflows 16 bits then we can't set the baud rate.
    if (iResult > 65535u)
    {
        b_validBaudRate = FALSE;
    }
    else
    {
        baseAddress = SetupSCIBaseAddress(module);

        // Write LSB of baud rate - mask off everything except bottom 8 bits.
        //lint -e{921} Cast to uint16_t as the write function uses 16 bit.
        iData = (uint16_t)( iResult & 0x000000FFu );
        genericIO_16bitWrite(( baseAddress + SCILBAUD_OFFSET ), iData);

        // Write MSB of baud rate - shift down and mask off.
        //lint -e{921} Cast to uint16_t as the write function uses 16 bit.
        iData = (uint16_t)( ( iResult >> 8u ) & 0x000000FFu );
        genericIO_16bitWrite(( baseAddress + SCIHBAUD_OFFSET ), iData);
    }

    return b_validBaudRate;
}


// ----------------------------------------------------------------------------
/**
 * SCI_RxBufferInitialise sets up the receive variables for a particular
 * buffer - stores a pointer to the buffer to write in, and zeros the offset.
 * Note that we don't actually erase the buffer, only set the offset to zero
 * (as we always write a NULL after any received character there is no
 * requirement to erase as well).
 *
 * @param	Module		Enumerated type for which SCI port to initialise.
 * @param	pBuffer		Pointer to start of buffer to put received data in.
 * @param	MaxRxLength	Size of receive buffer.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{952} pBuffer can't be declared const.
void SCI_RxBufferInitialise(const ESCIModule_t module,
                            uint8_t * const p_receiveBuffer,
                            const uint16_t maxRxLength)
{
    if (module < SCI_NUMBER_OF_PORTS)
    {
        m_serialPorts[module].rxOffset    = 0u;
        m_serialPorts[module].p_rxBuffer  = p_receiveBuffer;
        m_serialPorts[module].rxMaxLength = maxRxLength;
    }
}


// ----------------------------------------------------------------------------
/**
 * SCI_RxTriggerInitialise sets up the receive trigger variables for a
 * particular port, to allow either a flag to be set or a semaphore to be
 * posted when a particular character is received.
 *
 * Note that although the semaphore is RTOS specific behaviour, we store it
 * anyway, to avoid having conditional compilation in too many places.  The
 * semaphore will not be used unless the code is built with OpenRTOS present.
 *
 * @param   module              Enumerated type for which SCI module to use.
 * @param   b_matchRequired     Flag to say whether matching is required.
 * @param   matchCharacter      The character to match.
 * @param   p_receiveSemaphore  Pointer to the counting semaphore to post.
 *
 */
// ----------------------------------------------------------------------------
void SCI_RxTriggerInitialise(const ESCIModule_t module,
                             const bool_t b_matchRequired,
                             const uint8_t matchCharacter,
                             void * const p_receiveSemaphore)
{
    if (module < SCI_NUMBER_OF_PORTS)
    {
        m_serialPorts[module].b_matchRequired    = b_matchRequired;
        m_serialPorts[module].matchCounter       = 0u;
        m_serialPorts[module].matchCharacter     = matchCharacter;
        m_serialPorts[module].p_receiveSemaphore = p_receiveSemaphore;
    }
}


// ----------------------------------------------------------------------------
/**
 * SCI_RxBufferNumberOfCharsGet returns the number of characters which are
 * currently in the receive buffer.
 *
 * @param	Module		Enumerated type for which serial port buffer to query.
 * @retval	uint16_t	Number of received characters.
 *
 */
// ----------------------------------------------------------------------------
uint16_t SCI_RxBufferNumberOfCharsGet(const ESCIModule_t module)
{
    uint16_t    numberOfCharacters = 0u;

    if (module < SCI_NUMBER_OF_PORTS)
    {
        numberOfCharacters = m_serialPorts[module].rxOffset;
    }

    return numberOfCharacters;
}


// ----------------------------------------------------------------------------
/**
 * SCI_TxTriggerInitialise sets up the transmit trigger variables for a
 * particular port, to allow a semaphore to be posted when the transmission
 * has finished.
 *
 * Note that although the semaphore is RTOS specific behaviour, we store it
 * anyway, to avoid having conditional compilation in too many places.  The
 * semaphore will not be used unless the code is built with OpenRTOS present.
 *
 * @param   module              Enumerated type for which SCI module to use.
 * @param   p_transmitSemaphore Pointer to the counting semaphore to post.
 *
 */
// ----------------------------------------------------------------------------
void SCI_TxTriggerInitialise(const ESCIModule_t module,
                             void * const p_transmitSemaphore)
{
    if (module < SCI_NUMBER_OF_PORTS)
    {
        m_serialPorts[module].p_transmitSemaphore = p_transmitSemaphore;
    }
}


// ----------------------------------------------------------------------------
/**
 * SCI_TxStart initialises the message length and offset for the message in
 * the transmit buffer, and enables the transmit interrupt for the appropriate
 * serial port.
 *
 * @param	Module		Enumerated type for which serial port to use.
 * @param	pBuffer		Pointer to buffer containing message to transmit.
 * @param	Length		Length of message to transmit.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{952} pBuffer can't be declared const.
void SCI_TxStart(const ESCIModule_t module,
                 const uint8_t* const p_transmitBuffer,
                 const uint16_t lengthOfMessageToTransmit)
{
    // Only setup if there is actually something to transmit.
    if ( (lengthOfMessageToTransmit != 0u) && (module < SCI_NUMBER_OF_PORTS) )
    {
        m_serialPorts[module].p_txBuffer       = p_transmitBuffer;
        m_serialPorts[module].txMessageLength  = lengthOfMessageToTransmit;
        m_serialPorts[module].txOffset         = 0u;

        // Setup length, offset and enable interrupt for correct serial port.
        // Note that the default is to do nothing - we don't want to enable a port
        // unless we're sure!
        switch (module)
        {
            case SCI_A:
                SciaRegs.SCIFFTX.bit.TXFFIENA = 1u;
                break;

            case SCI_B:
                ScibRegs.SCIFFTX.bit.TXFFIENA = 1u;
                break;

            case SCI_C:
                ScicRegs.SCIFFTX.bit.TXFFIENA = 1u;
                break;

            case SCI_NUMBER_OF_PORTS:
            default:
                // Default case does nothing - there are only 3 x serial ports.
                break;
        }
    }
}


// ----------------------------------------------------------------------------
/**
 * SCI_TxDoneCheck tests to see if the transmitter has finished transmitting.
 *
 * @param	Module		Enumerated type for which serial port to use.
 * @retval	bool_t		TRUE if transmit has finished, FALSE if not finished.
 *
 */
// ----------------------------------------------------------------------------
bool_t SCI_TxDoneCheck(const ESCIModule_t module)
{
    bool_t      b_transmitDone = FALSE;
    uint16_t    charsInFifo;
    uint32_t    baseAddress;
    uint16_t    status;

    // If offset is equal to message length then transmit might have finished
    // - there may still be a number of characters in the FIFO so we need to
    // make sure all of these have been transmitted before returning TRUE.
    if ( (module < SCI_NUMBER_OF_PORTS)
            && (m_serialPorts[module].txOffset == m_serialPorts[module].txMessageLength))
    {
        baseAddress = SetupSCIBaseAddress(module);
        charsInFifo = GetNumberOfCharsInTxFifo(baseAddress);

        // If the FIFO is empty we still need to make sure that the last
        // character has actually been shifted out of the shift register.
        if (charsInFifo == 0u)
        {
            status = genericIO_16bitRead(( baseAddress + SCICTL2_OFFSET ));
            status &= SCICTL2_TXEMPTY_BIT_MASK;

            // If the TXEMPTY bit is set then the transmit buffer and shift
            // registers are both empty, so we've definitely finished.
            if (status != 0u)
            {
                b_transmitDone = TRUE;
            }
        }
    }

    return b_transmitDone;
}


// ----------------------------------------------------------------------------
/**
 * SCI_RxInterruptA_ISR is the interrupt service routine for the receiver of
 * serial port A.
 *
 * @note
 * This function cannot be declared as static, because the function which
 * initialises the vector table for the interrupts needs to know where it is.
 *
 */
// ----------------------------------------------------------------------------
interrupt void SCI_RxInterruptA_ISR(void)
{
    TESTPOINTS_Set(TP_OFFSET_SCI_RXINTA);

#ifndef BUILD_FOR_DSP_B
    // For DSP A only (because it does not contain an RTOS)
    // Allow higher priority interrupts to be serviced from within this ISR.
    EINT;
#endif

    ++m_moduleIrqCounters[SCI_A_RX_IRQ_COUNT_INDEX];

    RxInterruptHandler(SCI_A, &SciaRegs, PIEACK_GROUP9);

    TESTPOINTS_Clear(TP_OFFSET_SCI_RXINTA);
}


// ----------------------------------------------------------------------------
/**
 * SCI_TxInterruptA_ISR is the interrupt service routine for the transmitter of
 * serial port A.
 *
 * @note
 * This function cannot be declared as static, because the function which
 * initialises the vector table for the interrupts needs to know where it is.
 *
 */
// ----------------------------------------------------------------------------
interrupt void SCI_TxInterruptA_ISR(void)
{
    TESTPOINTS_Set(TP_OFFSET_SCI_TXINTA);

#ifndef BUILD_FOR_DSP_B
    // For DSP A only (because it does not contain an RTOS)
    // Allow higher priority interrupts to be serviced from within this ISR.
    EINT;
#endif

    ++m_moduleIrqCounters[SCI_A_TX_IRQ_COUNT_INDEX];

    TxInterruptHandler(SCI_A, &SciaRegs, PIEACK_GROUP9);

    TESTPOINTS_Clear(TP_OFFSET_SCI_TXINTA);
}


// ----------------------------------------------------------------------------
/**
 * SCI_RxInterruptB_ISR is the interrupt service routine for the receiver of
 * serial port B.
 *
 * @note
 * This function cannot be declared as static, because the function which
 * initialises the vector table for the interrupts needs to know where it is.
 *
 */
// ----------------------------------------------------------------------------
interrupt void SCI_RxInterruptB_ISR(void)
{
#ifndef BUILD_FOR_DSP_B
    // For DSP A only (because it does not contain an RTOS)
    // Allow higher priority interrupts to be serviced from within this ISR.
    EINT;
#endif

    ++m_moduleIrqCounters[SCI_B_RX_IRQ_COUNT_INDEX];

    RxInterruptHandler(SCI_B, &ScibRegs, PIEACK_GROUP9);
}


// ----------------------------------------------------------------------------
/**
 * SCI_TxInterruptB_ISR is the interrupt service routine for the transmitter of
 * serial port B.
 *
 * @note
 * This function cannot be declared as static, because the function which
 * initialises the vector table for the interrupts needs to know where it is.
 *
 */
// ----------------------------------------------------------------------------
interrupt void SCI_TxInterruptB_ISR(void)
{
#ifndef BUILD_FOR_DSP_B
    // For DSP A only (because it does not contain an RTOS)
    // Allow higher priority interrupts to be serviced from within this ISR.
    EINT;
#endif

    ++m_moduleIrqCounters[SCI_B_TX_IRQ_COUNT_INDEX];

    TxInterruptHandler(SCI_B, &ScibRegs, PIEACK_GROUP9);
}


// ----------------------------------------------------------------------------
/**
 * SCI_RxInterruptC_ISR is the interrupt service routine for the receiver of
 * serial port C.
 *
 * @note
 * This function cannot be declared as static, because the function which
 * initialises the vector table for the interrupts needs to know where it is.
 *
 */
// ----------------------------------------------------------------------------
interrupt void SCI_RxInterruptC_ISR(void)
{
#ifndef BUILD_FOR_DSP_B
    // For DSP A only (because it does not contain an RTOS)
    // Allow higher priority interrupts to be serviced from within this ISR.
    EINT;
#endif

    ++m_moduleIrqCounters[SCI_C_RX_IRQ_COUNT_INDEX];

    RxInterruptHandler(SCI_C, &ScicRegs, PIEACK_GROUP8);
}


// ----------------------------------------------------------------------------
/**
 * SCI_TxInterruptC_ISR is the interrupt service routine for the transmitter of
 * serial port C.
 *
 * @note
 * This function cannot be declared as static, because the function which
 * initialises the vector table for the interrupts needs to know where it is.
 *
 */
// ----------------------------------------------------------------------------
interrupt void SCI_TxInterruptC_ISR(void)
{
#ifndef BUILD_FOR_DSP_B
    // For DSP A only (because it does not contain an RTOS)
    // Allow higher priority interrupts to be serviced from within this ISR.
    EINT;
#endif

    ++m_moduleIrqCounters[SCI_C_TX_IRQ_COUNT_INDEX];

    TxInterruptHandler(SCI_C, &ScicRegs, PIEACK_GROUP8);
}


// ----------------------------------------------------------------------------
/**
 * SCI_ModuleInterruptCountGet returns the interrupt counter for the
 * required interrupt for this module.
 *
 * @param   index       Index relating to the required interrupt.
 * @retval  uint16_t    Interrupt counter
 *
 */
// ----------------------------------------------------------------------------
uint16_t SCI_ModuleInterruptCountGet(const uint16_t index)
{
    uint16_t    valueToReturn = 0xFFFFu;

    if (index < SCI_MAX_IRQ_COUNTERS)
    {
        valueToReturn = m_moduleIrqCounters[index];
    }

    return valueToReturn;
}


// ----------------------------------------------------------------------------
/**
 * SCI_ModuleInterruptStringGet returns the string for the required interrupt
 * for this module, so we know which interrupt is which.
 *
 * @param   index       Index relating to the required interrupt.
 * @retval  char_t*     Pointer to the start of the string.
 *
 */
// ----------------------------------------------------------------------------
const char_t* SCI_ModuleInterruptStringGet(const uint16_t index)
{
    static const char_t	strings[][9] = {"SCI A RX",
                                        "SCI A TX",
                                        "SCI B RX",
                                        "SCI B TX",
                                        "SCI C RX",
                                        "SCI C TX"};

    const char_t *  p_stringToReturn = NULL;

    if (index < SCI_MAX_IRQ_COUNTERS)
    {
        p_stringToReturn = &strings[index][0u];
    }

    return p_stringToReturn;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * RxInterruptHandler is the receive interrupt handler which is called from
 * all of the receive interrupts.
 *
 * @note
 * This function is declared as inline, but will only be compiled inline if
 * the appropriate compiler optimisation options are set.
 *
 * @warning
 * Not all of this function can be tested using unit tests, so review the code!
 *
 * @param   module      Enumerated value for the SCI module to use.
 * @param   p_sciRegs   Volatile pointer to the serial port structure to use.
 *
 */
// ----------------------------------------------------------------------------
static inline void RxInterruptHandler(const ESCIModule_t module,
                                      volatile struct SCI_REGS * const p_sciRegs,
                                      const uint16_t pieAckGroup)
{
    uint16_t    receivedWords;
    uint16_t    dataFromReceiver;
    uint16_t    readCounter;

    /*
     * Acknowledge the interrupt to allow other interrupts from this group to
     * be serviced.
     */
    PieCtrlRegs.PIEACK.all = pieAckGroup;

    /*
     * If rx error bit is set then reset the SCI by toggling the SW_RESET bit
     * (this could be one of 4 errors - BRKDT, FE, OE or PE).
     */
    if (p_sciRegs->SCIRXST.bit.RXERROR != 0u)
    {
        p_sciRegs->SCICTL1.bit.SWRESET = 0u;
        p_sciRegs->SCICTL1.bit.SWRESET = 1u;
    }
    /*
     * Otherwise read the receive buffer the required number of times to empty
     * the receive fifo.
     */
    else
    {
        // Get number of received words from FIFO receive register.
        receivedWords = p_sciRegs->SCIFFRX.bit.RXFFST;

        /*
         * Loop around reading from the receive buffer.
         * Put the character into the receive buffer and check to see if it
         * matches with the match character.  The RxIntPutCharInBufferAndCheckIt
         * will also post a semaphore and perform a context switch if needed.
         */
        for (readCounter = 0u; readCounter < receivedWords; readCounter++)
        {
            dataFromReceiver = p_sciRegs->SCIRXBUF.all;

            RxIntPutCharInBufferAndCheckIt(module, dataFromReceiver);
        }
    }

    /* Trigger the inter-char timeout timer, if it is configured. */
    if (m_serialPorts[module].p_timerTrigger != NULL)
    {
       m_serialPorts[module].p_timerTrigger();
    }

    /* Clear RXFFINT bit so we can service the next interrupt from the FIFO. */
    p_sciRegs->SCIFFRX.bit.RXFFINTCLR = 1u;
}


// ----------------------------------------------------------------------------
/**
 * RxIntPutCharInBufferAndCheckIt is called from the receive interrupt handler
 * above, and puts a received character in the receive buffer, and also checks
 * to see whether the character matches the match character.
 *
 * @note
 * This function is declared as inline, but will only be compiled inline if
 * the appropriate compiler optimisation options are set.
 *
 * @note
 * If using FreeRTOS, this function will post a semaphore and may perform
 * a context switch if the task which 'owns' the semaphore is a higher
 * priority than the one which is currently running.
 *
 * @param   module      Enumerated value for the SCI module to use.
 * @param   dataWord    The data word read from the receiver.
 *
 */
// ----------------------------------------------------------------------------
static inline void RxIntPutCharInBufferAndCheckIt(const ESCIModule_t module,
                                                  const uint16_t dataWord)
{
    uint8_t receivedData;

#ifdef FREE_RTOS_USED
    BaseType_t  higherPriorityTaskWoken;
#endif

    /*
     * Note that the top bits in the data word read from the receive buffer
     * tell us if we've had any FIFO receive error.
     * If either FIFO error bit is set then just ignore the new word,
     * otherwise store the next received word in the receiver buffer
     * plus a null character after the received word.
     */
    if ( (0u == (dataWord & SCIRXBUF_ERROR_BIT_MASK))
            && (m_serialPorts[module].p_rxBuffer != NULL) )
    {
        //lint -e{921} Cast to uint8_t to remove top bits.
        receivedData = (uint8_t)(dataWord & 0x00FFu);

        m_serialPorts[module].p_rxBuffer[m_serialPorts[module].rxOffset]
            = receivedData;

        m_serialPorts[module].p_rxBuffer[m_serialPorts[module].rxOffset + 1u]
            = 0x00u;

        /*
         * Increment the offset if we haven't reached the end of the
         * buffer - uses -2 to make sure we leave room for the '\0'.
         */
        if (m_serialPorts[module].rxOffset
                < (m_serialPorts[module].rxMaxLength - 2u) )
        {
            m_serialPorts[module].rxOffset++;
        }

        if (m_serialPorts[module].b_matchRequired
                && (m_serialPorts[module].matchCharacter == receivedData) )
        {
            m_serialPorts[module].matchCounter++;
        }

#ifdef FREE_RTOS_USED
        /*
         * If running FreeRTOS then we either post a semaphore for every
         * character (if b_matchRequired is false) or post a semaphore
         * when the received character matches the match character.
         * If no semaphore handle then don't post the semaphore.
         */
        if ( (m_serialPorts[module].matchCharacter == receivedData
                || !m_serialPorts[module].b_matchRequired)
                && (NULL != m_serialPorts[module].p_receiveSemaphore) )
        {
            (void)xSemaphoreGiveFromISR(m_serialPorts[module].p_receiveSemaphore,
                                        &higherPriorityTaskWoken);

            /*
             * A higher priority task may need to be unblocked,
             * so force a context switch to ensure that the interrupt
             * returns directly to the unblocked (higher priority) task.
             */
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
#endif
    }
}


// ----------------------------------------------------------------------------
/**
 * TxInterruptHandler is the transmit interrupt handler which is called from
 * all of the transmit interrupts.
 *
 * @note
 * This function is declared as inline, but will only be compiled inline if
 * the appropriate compiler optimisation options are set.
 *
 * @warning
 * Not all of this function can be tested using unit tests, so review the code!
 *
 * @param   module      Enumerated value for the SCI module to use.
 * @param   p_sciRegs   Volatile pointer to the serial port structure to use.
 *
 */
// ----------------------------------------------------------------------------
static inline void TxInterruptHandler(const ESCIModule_t module,
                                      volatile struct SCI_REGS * const p_sciRegs,
                                      const uint16_t pieAckGroup)
{
    uint16_t    availableSpace;
    uint16_t    writeCounter;

#ifdef FREE_RTOS_USED
    BaseType_t  higherPriorityTaskWoken;
#endif

    // Acknowledge the interrupt to allow other interrupts from this group to
    // be serviced.
    PieCtrlRegs.PIEACK.all = pieAckGroup;

    // Get number of characters which are already in the transmit FIFO, and
    // calculate the space left which we can use.
    //lint -e{921} Cast top uint16_t in the depth macro.
    availableSpace = SCI_TX_FIFO_DEPTH - (uint16_t)p_sciRegs->SCIFFTX.bit.TXFFST;

    // If there is some space in the transmit FIFO then use it.
    for (writeCounter = 0u; writeCounter < availableSpace; writeCounter++)
    {
        // If we've still got another character to transmit, then sent it.
        if (m_serialPorts[module].txOffset < m_serialPorts[module].txMessageLength)
        {
            p_sciRegs->SCITXBUF
                = m_serialPorts[module].p_txBuffer[m_serialPorts[module].txOffset];

            m_serialPorts[module].txOffset++;
        }
        // Otherwise the message is finished, so jump out.
        else
        {
            break;
        }
    }

    // Clear TXFFINT bit so we can service the next interrupt from the FIFO.
    p_sciRegs->SCIFFTX.bit.TXFFINTCLR = 1u;

    // Disable the transmit FIFO interrupt.
    if (m_serialPorts[module].txOffset == m_serialPorts[module].txMessageLength)
    {
        p_sciRegs->SCIFFTX.bit.TXFFIENA = 0u;

#ifdef FREE_RTOS_USED
        /*
         * If running FreeRTOS then post a semaphore when transmission finished.
         * If no semaphore handle then don't post the semaphore.
         */
        if (NULL != m_serialPorts[module].p_transmitSemaphore)
        {
            (void)xSemaphoreGiveFromISR(m_serialPorts[module].p_transmitSemaphore,
                                        &higherPriorityTaskWoken);

            /*
             * A higher priority task may need to be unblocked,
             * so force a context switch to ensure that the interrupt
             * returns directly to the unblocked (higher priority) task.
             */
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
#endif

        /* Trigger the end of transmission callback, if it has been configured. */
        if (m_serialPorts[module].p_timerTrigger != NULL)
        {
            m_serialPorts[module].p_timerTrigger();
        }
    }
}


// ----------------------------------------------------------------------------
/**
 * ResetAllSCIRegisters sets all the SCI control registers for a serial port to
 * zero.
 *
 * @param	iBaseAddress	Base address of the serial port to write to.
 *
 */
// ----------------------------------------------------------------------------
static void ResetAllSCIRegisters(const uint32_t iBaseAddress)
{
    genericIO_16bitWrite(( iBaseAddress + SCICTL1_OFFSET ), 0u);
    genericIO_16bitWrite(( iBaseAddress + SCICCR_OFFSET ), 0u);	//lint !e835
    genericIO_16bitWrite(( iBaseAddress + SCICTL2_OFFSET ), 0u);
    genericIO_16bitWrite(( iBaseAddress + SCIFFTX_OFFSET ), 0u);
    genericIO_16bitWrite(( iBaseAddress + SCIFFRX_OFFSET ), 0u);
    genericIO_16bitWrite(( iBaseAddress + SCIFFCT_OFFSET ), 0u);
    genericIO_16bitWrite(( iBaseAddress + SCIPRI_OFFSET ), 0u);
}


// ----------------------------------------------------------------------------
/**
 * SetupSCIBaseAddress sets up the base address for a serial port.
 *
 * @param	Module		Enumerated type for which serial port to use.
 * @retval	uint32_t	Base address for the appropriate serial port.
 *
 */
// ----------------------------------------------------------------------------
static uint32_t SetupSCIBaseAddress(const ESCIModule_t module)
{
    uint32_t address;

    switch (module)
    {
        case SCI_A:
            address = SCI_A_BASE_ADDRESS;
            break;

        case SCI_B:
            address = SCI_B_BASE_ADDRESS;
            break;

        case SCI_C:
            address = SCI_C_BASE_ADDRESS;
            break;

        case SCI_NUMBER_OF_PORTS:
        default:
            address = SCI_A_BASE_ADDRESS;
            break;
    }

    return address;
}


// ----------------------------------------------------------------------------
/**
 * GetNumberOfCharsInTxFIFO reads from the transmit FIFO to determine the
 * number of characters which are currently in the FIFO (still to be
 * transmitted).
 *
 * @param	BaseAddress		Base Address of serial port.
 * @retval	uint16_t		Number of characters still in the transmit FIFO.
 *
 */
// ----------------------------------------------------------------------------
static uint16_t GetNumberOfCharsInTxFifo(const uint32_t baseAddress)
{
    uint16_t    charactersInFifo;

    // Read TX FIFO register, clear bits apart from the TXFFST bits (which hold
    // the number of characters still in the FIFO) and shift these bits down
    // to the bottom of the word.
    charactersInFifo = genericIO_16bitRead(baseAddress + SCIFFTX_OFFSET);
    charactersInFifo &= SCIFFTX_TXFFST_BIT_MASK;
    charactersInFifo = charactersInFifo >> SCIFFTX_TXFFST_BIT_SHIFT;

    return charactersInFifo;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

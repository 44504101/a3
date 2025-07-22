/*
 * xintfconfig.c
 *
 *  Created on: 2022Äê6ÔÂ14ÈÕ
 *      Author: LeiGe
 */
// ----------------------------------------------------------------------------
// Include section
// Add all #includes here

#include "common_data_types.h"
#include "xintfconfig.h"
#include "DSP28335_device.h"


// ----------------------------------------------------------------------------
// Defines section
// Add all #defines here


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module


// ----------------------------------------------------------------------------
// Variables which only have scope within this module


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * @note
 * XINTFCONFIG_Initialise sets up the XINTF.  Zone 7 is connected to the flash.
 * All XINTF registers are EALLOW protected.
 *
*/
// ----------------------------------------------------------------------------
void XINTFCONFIG_Initialise(void)
{
    uint32_t    RequiredData32;
    uint16_t    RequiredData16;

    EALLOW;

    // Setup all bits to write into the XTIMING0 register, and write them.
    // Note that all the X???? timings can be scaled by a factor of 2 if the
    // X2TIMING bit (bit 22) is set.
    //lint -e{835, 845, 921}
    RequiredData32 =
        ( (uint32_t)0u << 31) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 30) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 29) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 28) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 27) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 26) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 25) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 24) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 23) | //   0: Reserved bit - write has no effect
        ( (uint32_t)1u << 22) | //   1: Scaling factor of 2:1 (doubled)
        ( (uint32_t)0u << 21) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 20) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 19) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 18) | //   0: Reserved bit - write has no effect
        ( (uint32_t)3u << 16) | //  11: Data bus width is 16 bits
        ( (uint32_t)1u << 15) | //   1: XREADY input is asynchronous
        ( (uint32_t)0u << 14) | //   0: Ignore the use of XREADY signal
        ( (uint32_t)3u << 12) | //  11: XRDLEAD is 3 \ 6 XTIMCLK cycles
        ( (uint32_t)7u << 9) |  // 111: XRDACTIVE is 7 \ 14 XTIMCLK cycles
        ( (uint32_t)3u << 7) |  //  11: XRDTRAIL is 3 \ 6 XTIMCLK cycles
        ( (uint32_t)3u << 5) |  //  11: XWRLEAD is 3 \ 6 XTIMCLK cycles
        ( (uint32_t)7u << 2) |  // 111: XWRACTIVE is 7 \ 14 XTIMCLK cycles
        ( (uint32_t)3u << 0);   //  11: XWRTRAIL is 3 \ 6 XTIMCLK cycles
    XintfRegs.XTIMING7.all = RequiredData32;

    // Setup all bits to write into the XINTCNF2 register, and write them.
    //lint -e{835, 845, 921}
    RequiredData32 =
        ( (uint32_t)0u << 31) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 30) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 29) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 28) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 27) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 26) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 25) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 24) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 23) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 22) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 21) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 20) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 19) | //   0: Reserved bit - write has no effect
        ( (uint32_t)1u << 16) | // 001: XTIMCLK = SYSCLKOUT / 2
        ( (uint32_t)0u << 15) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 14) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 13) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 12) | //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 11) | //   0: Read only bit - write has no effect
        ( (uint32_t)0u << 10) | //   0: Read only bit - write has no effect
        ( (uint32_t)0u << 9) |  //   0: Read only bit - write has no effect
        ( (uint32_t)0u << 8) |  //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 6) |  //  00: Read only bits - write has no effect
        ( (uint32_t)0u << 5) |  //   0: Reserved bit - write has no effect
        ( (uint32_t)0u << 4) |  //   0: Reserved bit - write has no effect
        ( (uint32_t)1u << 3) |  //   1: XCLKOUT is disabled
        ( (uint32_t)1u << 2) |  //   1: XCLKOUT = XTIMCLK / 2
        ( (uint32_t)0u << 0);   //  00: Write buffer depth is zero
    XintfRegs.XINTCNF2.all = RequiredData32;

    // Setup all bits to write into the XBANK register, and write them.
    //lint -e{835, 845, 921}
    RequiredData16 =
        ( (uint16_t)0u << 15) | //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 14) | //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 13) | //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 12) | //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 11) | //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 10) | //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 9) |  //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 8) |  //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 7) |  //   0: Reserved bit - write has no effect
        ( (uint16_t)0u << 6) |  //   0: Reserved bit - write has no effect
        ( (uint16_t)7u << 3) |  // 111: 7 XTIMCLK cycles between bank accesses
        ( (uint16_t)7u << 0);   // 111: Bank switching enabled for zone 7
    XintfRegs.XBANK.all = RequiredData16;

    EDIS;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * XINTFCONFIG_XclkoutEnable enables the XCLKOUT signal by clearing the CLKOFF
 * bit in XINTCNF2.  All XINTF registers are EALLOW protected.
 *
*/
// ----------------------------------------------------------------------------
void XINTFCONFIG_XclkoutEnable(void)
{
    EALLOW;
    XintfRegs.XINTCNF2.bit.CLKOFF = 0u;
    EDIS;
}


// ----------------------------------------------------------------------------
/**
 * @note
 * XINTFCONFIG_XclkoutDisable disables the XCLKOUT signal by setting the CLKOFF
 * bit in XINTCNF2.  All XINTF registers are EALLOW protected.
 *
*/
// ----------------------------------------------------------------------------
void XINTFCONFIG_XclkoutDisable(void)
{
    EALLOW;
    XintfRegs.XINTCNF2.bit.CLKOFF = 1u;
    EDIS;
}




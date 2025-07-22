/*
//###########################################################################
//
// FILE:	F28335_lnk_bootloader.cmd
//
// TITLE:	Linker Command File for use with SDRM bootloader and 28335 DSP.
//
// NOTES:   This is a minimal configuration, where only flash sector A
//          and RAM M0,M1 and L4 are used.
//          The linker file for the promloader should NOT use these sections.
//          The linker file for the application should NOT use flash sector A.
//
//###########################################################################
*/

/* Define the memory block start/length for the F28335  
   PAGE 0 will be used to organize program sections
   PAGE 1 will be used to organize data sections

    Notes: 
          Memory blocks on F28335 are uniform (ie same
          physical memory) in both PAGE 0 and PAGE 1.  
          That is the same memory region should not be
          defined for both PAGE 0 and PAGE 1.
          Doing so will result in corruption of program 
          and/or data. 
          
          L0/L1/L2 and L3 memory blocks are mirrored - that is
          they can be accessed in high memory or low memory.
          For simplicity only one instance is used in this
          linker file. 
          
          Contiguous SARAM memory blocks can be combined 
          if required to create a larger memory block. 
 */

MEMORY
{
PAGE 0:    /* Program Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE1 for data allocation */

   RAMM1       : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */
   RAML0      : origin = 0x008002, length = 0x000FFE
   RAML1      : origin = 0x009000, length = 0x001000
   RAML2      : origin = 0x00A000, length = 0x001000
   RAML3      : origin = 0x00B000, length = 0x001000
   APP_START   : origin = 0x300000, length = 0x000002     /* Branch to application always goes here - not used in this file but for reference. */
   FLASHA      : origin = 0x338000, length = 0x007F7F     /* on-chip FLASH */
   FLASHACRC   : origin = 0x33FF7F, length = 0x000001     /* reserved location for the CRC of FLASHA */
   CSM_RSVD    : origin = 0x33FF80, length = 0x000076     /* Part of FLASHA.  Program with all 0x0000 when CSM is in use. */
   BEGIN       : origin = 0x33FFF6, length = 0x000002     /* Part of FLASHA.  Used for "boot to Flash" bootloader mode. */
   CSM_PWL     : origin = 0x33FFF8, length = 0x000008     /* Part of FLASHA.  CSM password locations in FLASHA */
   OTP         : origin = 0x380400, length = 0x000400     /* on-chip OTP */
   ADC_CAL     : origin = 0x380080, length = 0x000009     /* ADC_cal function in Reserved memory */
   
   ROM         : origin = 0x3FF27C, length = 0x000D44     /* Boot ROM */        
   RESET       : origin = 0x3FFFC0, length = 0x000002     /* part of boot ROM  */
   VECTORS     : origin = 0x3FFFC2, length = 0x00003E     /* part of boot ROM  */

PAGE 1 :   /* Data Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE0 for program allocation */
           /* Registers remain on PAGE1                                                  */
   
   BOOT_RSVD   : origin = 0x000000, length = 0x000050     /* Part of M0, BOOT rom will use this for stack */
   RAMM0       : origin = 0x000050, length = 0x0003B0     /* on-chip RAM block M0 */
   RAML4       : origin = 0x00C000, length = 0x003000     /* on-chip RAM block L4 */
   //RAML5      : origin = 0x00D000, length = 0x001000
   //RAML6      : origin = 0x00E000, length = 0x001000
}

/* Allocate sections to memory blocks.
   Note:
         codestart user defined section in DSP28_CodeStartBranch.asm used to redirect code 
                   execution when booting to flash
         ramfuncs  user defined section to store functions that will be copied from Flash into RAM
*/ 
 
SECTIONS
{
 
   /* Allocate program areas: */
   .cinit              : > FLASHA      PAGE = 0
   .pinit              : > FLASHA,     PAGE = 0
   .text               : > FLASHA      PAGE = 0
   codestart           : > BEGIN       PAGE = 0 /* DSP reset vectors to here */
   Flash28_API:
	{
		-l Flash28335_API_V210.lib(.econst)
		-l Flash28335_API_V210.lib(.text)
	}
			LOAD = FLASHA,
			RUN = RAML0,
			LOAD_START(_Flash28_API_LoadStart),
			LOAD_END(_Flash28_API_LoadEnd),
			RUN_START(_Flash28_API_RunStart),
			PAGE = 0
	/*----------------------------------------------*/
   ramfuncs            : LOAD = FLASHA,
                         RUN = RAMM1, 
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         PAGE = 0

   csmpasswds          : > CSM_PWL     PAGE = 0
   csm_rsvd            : > CSM_RSVD    PAGE = 0
   
   /* Allocate uninitalized data sections: */
   /* Note - section for .esysmem is not used - no dynamic memory allocation */
   .stack              : > RAMM0       PAGE = 1
   .ebss               : > RAML4       PAGE = 1
   .esysmem            : > RAML4       PAGE = 1

   /* Initalized sections go in Flash */
   /* For SDFlash to program these, they must be allocated to page 0 */
   .econst             : > FLASHA      PAGE = 0
   .switch             : > FLASHA      PAGE = 0
   .crcTable		   : > FLASHA      PAGE = 0
   .bootloaderCRC      : > FLASHACRC   PAGE = 0
   
   /* .reset is a standard section used by the compiler.  It contains the */ 
   /* the address of the start of _c_int00 for C Code.   /*
   /* When using the boot ROM this section and the CPU vector */
   /* table is not needed.  Thus the default type is set here to DSECT  */
   .reset              : > RESET,      PAGE = 0, TYPE = DSECT
   vectors             : > VECTORS     PAGE = 0, TYPE = DSECT
   
   /* Allocate ADC_cal function (pre-programmed by factory into TI reserved memory) */
   .adc_cal     : load = ADC_CAL,   PAGE = 0, TYPE = NOLOAD
}


/*
//===========================================================================
// End of file.
//===========================================================================
*/


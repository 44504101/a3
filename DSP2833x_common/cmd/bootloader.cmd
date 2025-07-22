/*
	Linker file for Xceed ACQ \ MTC application code when running from flash
	but without any bootloader - standalone mode.

	The bootloader resides in FLASHA, so the application code must NOT use this area.
        The application start ALWAYS vectors to BEGIN, at address 0x3FFF6.
	The memory map is almost identical to the bootloader version, to make sure
	the code performance is the same (the only difference is where the application start is).
 */
// this is bootloader

MEMORY
{
PAGE 0:    /* Program Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE1 for data allocation */

   ZONE0       : origin = 0x004000, length = 0x001000     /* XINTF zone 0 */
   RAMM0       : origin = 0x000050, length = 0x0003B0     /* on-chip RAM block M0 */
   ZONE6       : origin = 0x100000, length = 0x100000     /* XINTF zone 6 */
   ZONE7A      : origin = 0x200000, length = 0x00FC00     /* XINTF zone 7 - program space */
   FLASHMEM    : origin = 0x338000, length = 0x007F80     /* on-chip FLASHA */
   CSM_RSVD    : origin = 0x33FF80, length = 0x000076     /* Part of FLASHA.  Program with all 0x0000 when CSM is in use. */
   BEGIN       : origin = 0x33FFF6, length = 0x000002     /* Part of FLASHA.  Used for "boot to Flash" bootloader mode. */
   CSM_PWL     : origin = 0x33FFF8, length = 0x000008     /* Part of FLASHA.  CSM password locations in FLASHA */
   OTP         : origin = 0x380400, length = 0x000400     /* on-chip OTP */
   ADC_CAL     : origin = 0x380080, length = 0x000009     /* ADC_cal function in Reserved memory */
   
   IQTABLES    : origin = 0x3FE000, length = 0x000b50     /* IQ Math Tables in Boot ROM */
   IQTABLES2   : origin = 0x3FEB50, length = 0x00008c     /* IQ Math Tables in Boot ROM */  
   FPUTABLES   : origin = 0x3FEBDC, length = 0x0006A0     /* FPU Tables in Boot ROM */
   ROM         : origin = 0x3FF27C, length = 0x000D44     /* Boot ROM */        
   RESET       : origin = 0x3FFFC0, length = 0x000002     /* part of boot ROM  */
   VECTORS     : origin = 0x3FFFC2, length = 0x00003E     /* part of boot ROM  */

PAGE 1 :   /* Data Memory */
           /* Memory (RAM/FLASH/OTP) blocks can be moved to PAGE0 for program allocation */
           /* Registers remain on PAGE1                                                  */
   
   BOOT_RSVD   : origin = 0x000000, length = 0x000050     /* Part of M0, BOOT rom will use this for stack */
   RAMM1       : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */
   RAML0123456 : origin = 0x008000, length = 0x007500     /* on-chip RAM block L0 to L7 0x500 bytes */
   RAML7       : origin = 0x00F500, length = 0x000B00     /* on-chip RAM block L7 -0x500 byres */
   ZONE7B      : origin = 0x20FC00, length = 0x000400     /* XINTF zone 7 - data space */
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
   .cinit              : > FLASHMEM      PAGE = 0
   .pinit              : > FLASHMEM      PAGE = 0
   .text               : > FLASHMEM      PAGE = 0
   codestart           : > BEGIN         PAGE = 0
   ramfuncs            : LOAD = FLASHMEM,
                         RUN = RAMM0,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         PAGE = 0
Flash28_API:
				{
					-l Flash28335_API_V210.lib(.econst)
					-l Flash28335_API_V210.lib(.text)
				} LOAD = FLASHMEM,
				RUN = RAMM0,
				LOAD_START(_Flash28_API_LoadStart),
				LOAD_END(_Flash28_API_LoadEnd),
				RUN_START(_Flash28_API_RunStart),
				PAGE = 0
   csmpasswds          : > CSM_PWL     PAGE = 0
   csm_rsvd            : > CSM_RSVD    PAGE = 0
   
   /* Allocate uninitalized data sections: */
   .stack              : > RAML0123456       	 PAGE = 1
   .ebss               : > RAML0123456   PAGE = 1
   .esysmem            : > RAML0123456       	 PAGE = 1

   /* Initialized sections go in Flash */
   /* For SDFlash to program these, they must be allocated to page 0 */
   .econst             : > FLASHMEM      PAGE = 0
   .switch             : > FLASHMEM      PAGE = 0

   /* Vibration 68 taps FIR filter coefficient and sample buffers*/
   vibration_coefficients    ALIGN(0x100)    > FLASHMEM    PAGE = 0
   vibration_Xsamples        ALIGN(0x100)    > RAML7       PAGE = 1
   vibration_Ysamples        ALIGN(0x100)    > RAML7       PAGE = 1
   vibration_Zsamples        ALIGN(0x100)    > RAML7       PAGE = 1

   /* .reset is a standard section used by the compiler.  It contains the */ 
   /* the address of the start of _c_int00 for C Code.   /*
   /* When using the boot ROM this section and the CPU vector */
   /* table is not needed.  Thus the default type is set here to  */
   /* DSECT  */ 
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


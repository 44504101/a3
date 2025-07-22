/**
 * \file
 * Describes a generic interface into the hardware for those parts
 * of the hardware which are relevant to the Loader.
 * 
 * @author Scott DiPasquale
 * @date 26 August 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */

#include "common_data_types.h"
#include "tool_specific_programming.h"
#include "prom_hardware.h"
#include "tool_specific_config.h"
#include "dsp_crc.h"
#include "genericIO.h"
#include "utils.h"
#include "Flash2833x_API_Library.h"
//Partition numbers
#define BOOT_PARTITION			0 		//boot-loader's partition number
#define APPLICATION_PARTITION	1 		//application's partition number
#define PARAMETER_PARTITION		2		//parameter's partition number
#define CONFIG_PARTITION		3		//configuration's partition number
#define UNDEFINED_PARTITION		0xFF 	//undefined partition number (initialize to this)


static bool_t erasePartition(Uint16 partition);
static bool_t CheckForValidPartitionAndSetupParameters(void);
static bool_t SetupPartitionParameters(Uint16 PartitionNumber, PartitionParameters_t* pParameters);


static PartitionParameters_t mPartitionParameters = {UNDEFINED_PARTITION, FALSE, FALSE, 0u, 0u, 0u, 0u, {0u, 0u, 0u, 0}, 0u};

// Setup flags for allowing the bootloader to be programmed, and allowing the
// flash to be written incrementally.  These are variables to allow them to be
// altered via the debug port, but use #defines for the initial state to also
// allow them to be changed using different build options (rather than having
// separate header files).
static bool_t mbAllowBootloaderProgramming = ALLOW_BOOTLOADER_PROGRAMMING;
static bool_t mbAllowIncrementalFlashWrite = ALLOW_INCREMENTAL_FLASH_WRITE;


/**
 * Writes the program data to the flash memory.  If mbAllowIncrementalFlashWrite is FALSE,
 * this copies the data into a temporary buffer prepared by PromHardware_PartitionPrepare().
 * If mbAllowIncrementalFlashWrite is TRUE, data is still copied into the temporary buffer
 * but is then programmed into the flash (which has been previously erased with
 * PromHardware_PartitionPrepare()).
 * 
 * @param pData					Array of data to write to the ROM
 * @param LengthInBytes 		The length of the data array, in bytes.
 * @param StartAddressInFlash	The start address of the flash into which the data will be written.
 * @return bool_t				TRUE if the write encountered no errors, else FALSE.
 *
 */
bool_t PromHardware_ProgramMemoryWrite(Uint8* pData, Uint32 LengthInBytes, Uint32 StartAddressInFlash)
{
	Uint32 			BufferAddress;
	Uint32 			wordLen = LengthInBytes >> 1; //divide by 2
	Uint16 			workingData;
	Uint32 			i = 0; //counter
	bool_t 			bRomCanBeWritten = FALSE;
	FlashStatus_t	FlashStatus;

	if (CheckForValidPartitionAndSetupParameters() == TRUE)
	{
		if ( (StartAddressInFlash >= mPartitionParameters.TargetStartAddress) && ( (StartAddressInFlash+wordLen) < mPartitionParameters.TargetEndAddress) )
		{
			// Generate address in buffer to copy data into - if an incremental flash write,
			// it is likely that this buffer is quite small so we always start at the beginning
			// of the buffer, otherwise the buffer is large enough to hold an entire image
			// so setup the address to be offset by the correct amount.
			if (mbAllowIncrementalFlashWrite == TRUE)
			{
				BufferAddress = BUFFER_BASE_ADDRESS;
			}
			else
			{
			    BufferAddress = BUFFER_BASE_ADDRESS + (StartAddressInFlash - mPartitionParameters.TargetStartAddress);
			}

			// Check to make sure new data can fit into the buffer.
			if ( (BufferAddress + wordLen) <= ( (Uint32)BUFFER_BASE_ADDRESS + (Uint32)BUFFER_LENGTH) )
			{
				bRomCanBeWritten = TRUE;
			}
		}
	}

	if (bRomCanBeWritten == TRUE)
	{
		// Reformat incoming data and write them into RAM buffer.
		// We suppress the Lint warning for 'BufferAddress may not have been initialised'
        // - there is no path through the code where we can get here without it being set.
		for(i = 0; i < wordLen; i++)
		{
			workingData = utils_toUint16(&pData[i*2], DOWNLOAD_ENDIANESS);
			genericIO_16bitWrite( (BufferAddress + i), workingData);		//lint !e644
		}
		// Copy from RAM buffer into flash, always starting at BUFFER_BASE_ADDRESS.
		// (In incremental write mode, the data is always written into the start of
		// the buffer and then copied pass by pass, rather than waiting until the end).
		if (mbAllowIncrementalFlashWrite == TRUE)
		{
			bRomCanBeWritten = ToolSpecificProgramming_SafeFlashProgram((void*)StartAddressInFlash,
			                                                            (Uint16*)BUFFER_BASE_ADDRESS,
			                                                            wordLen,
																		&FlashStatus);
		}
	}

	return bRomCanBeWritten;
}


/**
 * Gets whether the indicated partition is a valid partition in the current
 * target.
 * Note the suppression of the Lint warning for 'Boolean within if always
 * evaluates to true' and 'Constant value Boolean' - this depends on the
 * contents of tool_specific_config.h and just happens to be the case for the
 * test file when we need to test all possible options.  
 * Either way, it's a safe warning to ignore.
 *
 * @param 	partition 	The partition number to query for validity.
 * @return  bool_t		Whether the indicated partition is a valid partition.
 */
bool_t PromHardware_isValidPartition(Uint16 partition)
{
    bool_t retval = FALSE;

    switch (partition)
    {
    	// Boot partition might be valid - either depends on programming flag
    	// or conditional compilation, which overrides the flag.
		case BOOT_PARTITION:
#ifdef BOOTLOADER_PROGRAMMING
			mbAllowBootloaderProgramming = TRUE;
#endif   //BOOTLOADER_PROGRAMMING
			retval = mbAllowBootloaderProgramming;
			break;

		// Application partition is always valid.
		case APPLICATION_PARTITION:
			retval = TRUE;
			break;

		// Parameter partition is only valid if length is non-zero.
		case PARAMETER_PARTITION:
			if (PARAMETER_LENGTH != 0)		//lint !e774 !e506 Always evaluates to TRUE.
			{
				retval = TRUE;
			}
			break;

		// Config partition is only valid if length is non-zero.
		case CONFIG_PARTITION:
			if (CONFIG_LENGTH != 0)			//lint !e774 !e506 Always evaluates to TRUE.
			{
				retval = TRUE;
			}
			break;

		default:
			break;
    }

    return retval;
}


/**
 * Prepares the specified partition for overwriting.  This corresponds to
 * opcode39(0, partition#).  If isWriteIncremental() would return TRUE, this should
 * allocate a buffer to hold the data destined for ROM and initialize the buffer to
 * an erased status (all 0xff for Flash, for instance); if it would return FALSE,
 * then this should erase the relevant partition in ROM.
 *
 * @param partition The partition to prepare.
 * @return 0 if the preparation did not encounter an error, else error code
 *
 */
Uint16 PromHardware_PartitionPrepare(Uint16 partition)
{
	Uint32 i;
    Uint16 retval = 0u;

    mPartitionParameters.PartitionNumber = partition;
	mPartitionParameters.bPartitionProgrammed = FALSE;
	mPartitionParameters.bPartitionPrepared = FALSE;

	if (CheckForValidPartitionAndSetupParameters() == TRUE)
	{
		// Assume partition can be prepared...
		mPartitionParameters.bPartitionPrepared = TRUE;

		// If not writing to the flash itself then fill RAM with 0xFFFF to make
		// it look like flash and set the partition prepared flag.
		if (mbAllowIncrementalFlashWrite == FALSE)
		{
			for (i=0; i< mPartitionParameters.PartitionLength; i++)
			{
				genericIO_16bitWrite( (BUFFER_BASE_ADDRESS + i), 0xFFFF);
			}
		}
		// Otherwise erase the appropriate partition - note that the erase may
		// take some time, and the code will wait in the erase function until
		// completion.
		else
		{
			if (erasePartition(partition) == FALSE)
			{
				retval = mPartitionParameters.FlashStatus.FlashStatusCode;
				mPartitionParameters.bPartitionPrepared = FALSE;
			}
		}
    }
	else
	{
		retval = 0xFFFFu;
	}

    return retval;
}


/**
 * Gets whether the partition specified in PromHardware_PartitionPrepare() has been prepared.
 * This either just returns the partition prepared flag (which will have been set
 * elsewhere if we're not erasing flash as part of the partition preparation),
 * or checks the flash to see if the partition is actually blank.
 *
 * @return Whether the partition has been successfully prepared.
 */
bool_t PromHardware_isPartitionPrepared(void)
{
	return mPartitionParameters.bPartitionPrepared;
}


/**
 * Validates the given CRC against the calculated CRC of the partition that was
 * specified in PromHardware_PartitionPrepare().  The common loader protocol specifies that this
 * is done after downloading the partition data.  If isWriteIncremental() would
 * return FALSE, then this calculates the CRC from the data in the temporary buffer;
 * otherwise it takes it directly from the freshly programmed partition.
 *
 * @param crc The CRC against which to validate the partition
 * @return TRUE if the CRCs match, else FALSE
 */
bool_t PromHardware_PartitionCRCValidate(Uint16 crc)
{
	Uint16  new_crc;

	// Compute CRC from buffer.
	if(mbAllowIncrementalFlashWrite == FALSE)
	{
		if (CheckForValidPartitionAndSetupParameters() == FALSE)
		{
			return FALSE;
		}

		new_crc = 0;
		new_crc = crc_calcRunningCRC(new_crc,(const Uint16*)BUFFER_BASE_ADDRESS,mPartitionParameters.PartitionLength,WORD_CRC_CALC);
		new_crc = crc_calcFinalCRC(new_crc, WORD_CRC_CALC);
		return new_crc==crc;
	}
	// Compute CRC from flash itself.
	else
	{
		// If CRC calculation fails (invalid partition,normally) then return false.
		if (PromHardware_PartitionCRCCalculate(mPartitionParameters.PartitionNumber, &new_crc) == FALSE)
		{
			return FALSE;
		}else{
		    return TRUE;
		}
		//return new_crc==crc;
	}
}


/**
 * Directs the hardware to begin programming the partition specified in
 * PromHardware_PartitionPrepare().
 * Depending on the target, this function might not return until programming has
 * completed.
 *
 * @return 0 if programming begins without an error, else the Flash Error triggering.
 * Other error should be
 *  returned if the partition specified in PromHardware_PartitionPrepare() is not a valid
 *  partition; however, this is only a recommended idea because the download process
 *  should have halted when PromHardware_PartitionPrepare() or PromHardware_isPartitionPrepared() returned
 *  FALSE.
 *  1 for non-flash errors
 *  2 for crc errors
 *  3 for flash errors
 */
Uint16 PromHardware_PartitionProgram(void)
{
	Uint16  crc; 		//computed crc of the new program
	bool_t  bProgrammedOK = FALSE;

    // If NOT doing incremental flash write we need to erase the appropriate partition
	// and then copy the code from the RAM buffer into the flash.
	// (If doing incremental flash write, the flash will already have been written).
    if (mbAllowIncrementalFlashWrite == FALSE)
    {
        if (erasePartition(mPartitionParameters.PartitionNumber) == FALSE)
        {
            return 1;
        }

		if (CheckForValidPartitionAndSetupParameters() == FALSE)
		{
			return 1;
		}

		// Copy from RAM buffer into flash, starting at BUFFER_BASE_ADDRESS.
		bProgrammedOK = ToolSpecificProgramming_SafeFlashProgram((void*)mPartitionParameters.TargetStartAddress,
																	(Uint16*)BUFFER_BASE_ADDRESS,
																	mPartitionParameters.PartitionLength,
																	&mPartitionParameters.FlashStatus);
		if (bProgrammedOK == FALSE)
		{
			return mPartitionParameters.FlashStatus.FlashStatusCode;
		}
	}

    // Once we get to here we can be in either write 'mode', so calculate the checksum and write it.
	//get the new crc
	crc = 0; //initialize it to zero, standard procedure
	if(PromHardware_PartitionCRCCalculate(mPartitionParameters.PartitionNumber,&crc) == FALSE)
	{
		return 2; //failed to calculated the new crc
	}

	bProgrammedOK = ToolSpecificProgramming_SafeFlashProgram((void*)mPartitionParameters.CRCAddress,
																&crc, (Uint32)1, &mPartitionParameters.FlashStatus);
	if(bProgrammedOK == FALSE)
	{
		return mPartitionParameters.FlashStatus.FlashStatusCode;
	}
    mPartitionParameters.bPartitionProgrammed = TRUE;
    mPartitionParameters.bPartitionPrepared = FALSE;
	return 0; //no error
}


/**
 * Gets whether the prepared partition has been fully transferred to the ROM.
 * This should NOT check the data in ROM against the temporary buffer (if applicable),
 * since PromHardware_PartitionCRCCalculate() is for that purpose.
 *
 * @return TRUE if the ROM device has been programmed with the new application, else
 *  FALSE.  FALSE signifies either that programming has not begun, or that
 *  programming is currently in progress (as obtained asking the ROM device).
 */
bool_t PromHardware_isPartitionProgrammed( void )
{
	return mPartitionParameters.bPartitionProgrammed;
}


/**
 * Calculates the CRC of the given partition.  This CRC will ALWAYS come from the
 * partition that is present in the ROM (as opposed to a temporary buffer).
 * Note that the CRC of any partition can be calculated here - not just limited
 * to the ones which we're allowed to write to, so this function doesn't use
 * CheckForValidPartitionAndSetupParameters() to setup the address and length.
 *
 * @param partition The partition on which to calculate the CRC.
 * @param crc The calculated CRC value will be placed here.
 * @return TRUE if the calculation proceeded without error, else FALSE.  Example
 *  FALSE condition: invalid partition number.
 */
bool_t PromHardware_PartitionCRCCalculate(Uint16 partition, Uint16* crc)
{
	PartitionParameters_t TempParameters;

	if (SetupPartitionParameters(partition, &TempParameters) == FALSE)
	{
		return FALSE;
	}

	if (crc != NULL)
	{
		(*crc) = 0; //initialize CRC to known value;
		(*crc) = crc_calcRunningCRC(*crc,(const Uint16*)TempParameters.TargetStartAddress,
										TempParameters.PartitionLength, WORD_CRC_CALC);
		(*crc) = crc_calcFinalCRC(*crc, WORD_CRC_CALC);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
 * Gets the expected-CRC value that is stored in the partition.
 * Note that the CRC of any partition can be fetched here - not just limited
 * to the ones which we're allowed to write to, so this function doesn't use
 * CheckForValidPartitionAndSetupParameters() to setup the address and length.
 *
 * @param partition The partition from which to get the expected CRC.
 * @param expectedCRC The retrieved CRC value will be placed here
 * @return TRUE if the retrieval proceeded without error, else FALSE.  Example
 *  FALSE condition: invalid partition number
 */
bool_t PromHardware_PartitionCRCGetExpected(Uint16 partition, Uint16* expectedCRC)
{
	if(partition == BOOT_PARTITION)
	{
		*expectedCRC = genericIO_16bitRead(BOOTLOADER_CRC_ADDRESS);
	}
	else if (partition == APPLICATION_PARTITION)
	{
		*expectedCRC = genericIO_16bitRead(APPLICATION_CRC_ADDRESS);
	}
	else if (partition == PARAMETER_PARTITION)
	{
		*expectedCRC = genericIO_16bitRead(PARAMETER_CRC_ADDRESS);
	}
	else if (partition == CONFIG_PARTITION)
	{
		*expectedCRC = genericIO_16bitRead(CONFIG_CRC_ADDRESS);
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}


/**
 * Reads data related to program memory.
 * if( !mbAllowIncrementalFlashWrite && PromHardware_isPartitionPrepared() && !PromHardware_isPartitionProgrammed() ),
 * then the data returned is from the temporary application buffer; otherwise it is
 * read directly from the ROM.
 *
 * @param data Buffer into which to place the requested data
 * @param length The amount of data to place into the buffer
 * @param address The program ROM address at which to begin reading
 * @return TRUE if the read proceeded without error, else FALSE.  FALSE is returned
 *  if any part of the address space to be read (address+length) falls outside the
 *  valid program ROM area.
 */
bool_t PromHardware_ProgramMemoryRead(Uint8* pData, Uint32 LengthInBytes, Uint32 Address)
{
	Uint32 new_address; 					// For address translation
	Uint32 wordLen = LengthInBytes >> 1; 	// Converted length, since each location is 2bytes (16bits)
	Uint32 ii; 								// Loop counter
	Uint16 workingData;
	bool_t bDataReadAllowed = FALSE;
	new_address = Address;

	if (mPartitionParameters.PartitionNumber != UNDEFINED_PARTITION)
	{
		// If partition is valid under the current configuration then read from it,
		// otherwise not allowed - note that this means that the boot partition
		// cannot be read unless mbAllowBootloaderProgramming is TRUE.
		if (CheckForValidPartitionAndSetupParameters() == TRUE)
		{
			if ( (Address >= mPartitionParameters.TargetStartAddress)
					&& ( (Address+wordLen) < mPartitionParameters.TargetEndAddress) )
			{
				if ( (mbAllowIncrementalFlashWrite == FALSE)
						&& (PromHardware_isPartitionPrepared() == TRUE)
						&& (PromHardware_isPartitionProgrammed() == FALSE) )
				{
					new_address = BUFFER_BASE_ADDRESS + (Address - mPartitionParameters.TargetStartAddress);
				}

				bDataReadAllowed = TRUE;
			}
		}
	}
	else
	{
		bDataReadAllowed = TRUE;		// Treat as direct memory access if partition undefined.
	}

	if (bDataReadAllowed == TRUE)
	{
		for (ii=0; ii < wordLen; ii++)
		{
			workingData = genericIO_16bitRead(new_address + ii);
			utils_to2Bytes(&pData[ii<<1], workingData, UPLOAD_ENDIANESS);
		}
	}

	return bDataReadAllowed;
}


/*
 * Set flag to allow \ disallow programming of the bootloader.
 * Use this carefully!!
 *
 * @param	Allow	Allow bootloader programming - TRUE or FALSE.
 *
 */
void PromHardware_AllowBootloaderProgrammingFlagSet(bool_t Allow)
{
	mbAllowBootloaderProgramming = Allow;
}

/*
 * Set flag to allow \ disallow writing of flash in incremental
 * chunks (i.e. not using the double buffering if your hardware
 * won't support it).
 * Use this carefully!!
 *
 * @param	Allow	Allow incremental flash writing - TRUE or FALSE.
 */
void PromHardware_AllowIncrementalFlashWriteFlagSet(bool_t Allow)
{
	mbAllowIncrementalFlashWrite = Allow;
}


/*
 * Get pointer to partition parameter structure, so unit tests can
 * either manipulate the variables or check the values.
 *
 */
const PartitionParameters_t* PromHardware_PartitionParameterPointerGet_TDD(void)
{
	return &mPartitionParameters;
}


/**
 * Erase the sectors specified in the given array.
 * Any error will be in mParitionParameters.FlashStatus.FlashStatusCode (zero if OK).
 *
 * @param 	partition	Partition number to erase.
 * @retval	bool_t		TRUE if erased OK, FALSE if some error.
 *
 */
static bool_t erasePartition(Uint16 partition)
{
	bool_t  				bErasedOK = FALSE;
	PartitionParameters_t	TempParameters;

	if (SetupPartitionParameters(partition, &TempParameters) == TRUE)
	{
#ifndef	DEBUG_FLASH_ERASE_NOT_REQUIRED
        bErasedOK = ToolSpecificProgramming_SafeFlashErase(TempParameters.SectorMask, &mPartitionParameters.FlashStatus);
#else
		bErasedOK = TRUE;
#endif
	}

	return bErasedOK;
}


/**
 * Checks to see whether a particular partition is valid (i.e. allowed to write to it
 * and supported on the target platform), and if so, sets up the length, start
 * address and CRC address of the partition.
 *
 * @warning
 * Note that the partition number must have been initialised before this function can be called.
 *
 * @retval	bool_t		TRUE if partition validated, FALSE if some error.
 */
static bool_t CheckForValidPartitionAndSetupParameters(void)
{
	bool_t	bPartitionIsOK;

	bPartitionIsOK = PromHardware_isValidPartition(mPartitionParameters.PartitionNumber);

	if (bPartitionIsOK == TRUE)
	{
		(void)SetupPartitionParameters(mPartitionParameters.PartitionNumber, &mPartitionParameters);
	}

	return bPartitionIsOK;
}


/**
 * Sets up various parameters for a particular partition - length, address etc.
 *
 * @param	PartitionNumber		Partition number to setup parameters for.
 * @param	pParameters			Pointer to structure to put parameters in.
 * @retval	bool_t				TRUE if valid partition number, FALSE if not.
 */
static bool_t SetupPartitionParameters(Uint16 PartitionNumber, PartitionParameters_t* pParameters)
{
	bool_t bPartitionIsOK = TRUE;

	if (pParameters != NULL)
	{
		switch (PartitionNumber)
		{
			case BOOT_PARTITION:
				pParameters->PartitionLength = (Uint32)BOOTLOADER_LENGTH;
				pParameters->TargetStartAddress = (Uint32)BOOTLOADER_START_ADDRESS;
				pParameters->TargetEndAddress = (Uint32)BOOTLOADER_END_ADDRESS;
				pParameters->CRCAddress = (Uint32)BOOTLOADER_CRC_ADDRESS;
				pParameters->SectorMask = BOOT_SECTOR_MASK;
				break;

			case APPLICATION_PARTITION:
				pParameters->PartitionLength = (Uint32)APPLICATION_LENGTH;
				pParameters->TargetStartAddress = (Uint32)APPLICATION_START_ADDRESS;
				pParameters->TargetEndAddress = (Uint32)APPLICATION_END_ADDRESS;
				pParameters->CRCAddress = (Uint32)APPLICATION_CRC_ADDRESS;
				pParameters->SectorMask = APPLICATION_SECTOR_MASK;
				break;

			case PARAMETER_PARTITION:
				pParameters->PartitionLength = (Uint32)PARAMETER_LENGTH;
				pParameters->TargetStartAddress = (Uint32)PARAMETER_START_ADDRESS;
				pParameters->TargetEndAddress = (Uint32)PARAMETER_END_ADDRESS;
				pParameters->CRCAddress = (Uint32)PARAMETER_CRC_ADDRESS;
				pParameters->SectorMask = PARAMETER_SECTOR_MASK;
				break;

			case CONFIG_PARTITION:
				pParameters->PartitionLength = (Uint32)CONFIG_LENGTH;
				pParameters->TargetStartAddress = (Uint32)CONFIG_START_ADDRESS;
				pParameters->TargetEndAddress = (Uint32)CONFIG_END_ADDRESS;
				pParameters->CRCAddress = (Uint32)CONFIG_CRC_ADDRESS;
				pParameters->SectorMask = CONFIG_SECTOR_MASK;
				break;

			default:
				bPartitionIsOK = FALSE;
				break;
		}
	}

	return bPartitionIsOK;
}

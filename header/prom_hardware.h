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

#ifndef PROM_HARDWARE_H
#define PROM_HARDWARE_H

typedef struct PartitionParameters
{
	uint16_t		PartitionNumber;
	bool_t 			bPartitionPrepared;				// Whether the partition has been prepared.
	bool_t 			bPartitionProgrammed;			// Whether the partition has been programmed.
	uint32_t		PartitionLength;
	uint32_t		TargetStartAddress;
	uint32_t		TargetEndAddress;
	uint32_t		CRCAddress;
	FlashStatus_t	FlashStatus;
	uint16_t		SectorMask;
} PartitionParameters_t;

bool_t PromHardware_ProgramMemoryWrite(Uint8* pData, Uint32 LengthInBytes, Uint32 StartAddressInFlash);
bool_t PromHardware_isValidPartition( Uint16 partition );
Uint16 PromHardware_PartitionPrepare( Uint16 partition ) ;
bool_t PromHardware_isPartitionPrepared( void ) ;
bool_t PromHardware_PartitionCRCValidate( Uint16 crc ) ;
Uint16 PromHardware_PartitionProgram( void ) ;
bool_t PromHardware_isPartitionProgrammed( void ) ;
bool_t PromHardware_PartitionCRCCalculate( Uint16 partition, Uint16 * crc ) ;
bool_t PromHardware_PartitionCRCGetExpected( Uint16 partition, Uint16 * expectedCRC ) ;
bool_t PromHardware_ProgramMemoryRead(Uint8* pData, Uint32 LengthInBytes, Uint32 Address);
void   PromHardware_AllowBootloaderProgrammingFlagSet(bool_t Allow);
void   PromHardware_AllowIncrementalFlashWriteFlagSet(bool_t Allow);

// Functions for TDD \ unit test use only.
const PartitionParameters_t* PromHardware_PartitionParameterPointerGet_TDD(void);

#endif   // PROM_HARDWARE_H

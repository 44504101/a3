/**
 * \file
 * Definition of opcode 39 (protect/unprotect EEPROM)
 * 
 * @author Scott DiPasquale
 * @date 13 September 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */
 
#ifndef OPCODE039_H
#define OPCODE039_H

typedef enum ProgrammingStatus
{
    PROGRAMMING_NOT_BEGUN,
    PROGRAMMING_IN_PROGRESS,
    PROGRAMMING_INVALID_CRC,
    PROGRAMMING_FAILED,
    PROGRAMMING_SUCCEEDED
}EProgrammingStatus_t ;

/**
 * Subfield of opcode39 which directs the loader to either prepare a temporary
 * partition of prepare the ROM for writing
 */
#define OPCODE39_UNPROTECT 0

/**
 * Subfield of opcode39 which is used poll the loader for its status after sending
 * OPCODE39_UNPROTECT or OPCODE39_CHECKSUM
 */
#define OPCODE39_PROTECT 1

/**
 * Subfield of opcode39 which is used to calcuate the checksum of the partition
 * to be programmed.  If the checksum is good, the programming begins.
 */
#define OPCODE39_CHECKSUM 2

/**
 * Executes opcode 39
 * 
 * @param loaderState The current state of the program
 * @param message Pointer to the received message
 * @param timer Pointer to the current Timer running in the Loader
 */
void opcode39_execute(ELoaderState_t* loaderState, LoaderMessage_t* message, Timer_t* timer);

// Functions for TDD \ unit test use only - don't need to support this in both .c files.
EProgrammingStatus_t 	opcode39_ProgrammingStatusGet_TDD(void);
void					opcode39_ProgrammingStatusSet_TDD(EProgrammingStatus_t Status);
Uint16					opcode39_FlashErrorCodeGet_TDD(void);
void					opcode39_FlashErrorCodeSet_TDD(Uint16 ErrorCode);

#endif   // OPCODE039_H

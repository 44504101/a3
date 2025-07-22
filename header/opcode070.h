/**
 * \file
 * Interface for opcode 70 (reset subsystem)
 * 
 * @author Scott DiPasquale
 * @date 2 September 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 *
 */
 
#ifndef OPCODE070_H
#define OPCODE070_H

/**
 * Executes opcode 70 (reset subsystem)
 * 
 * @param loaderState The current state of the program
 * @param message Pointer to the received message
 */
void opcode70_execute(ELoaderState_t* loaderState, LoaderMessage_t* message);


#endif   // OPCODE070_H

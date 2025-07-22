/**
 * \file
 * Declaration of opcode 1 (jump to application)
 * 
 * @author Scott DiPasquale
 * @date 7 September 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */

#ifndef OPCODE001_H
#define OPCODE001_H

/**
 * Executes opcode 1
 * 
 * @param loaderState Pointer to an indicator of the current program state
 * @param message Pointer to the received message
 */
void opcode1_execute(ELoaderState_t* loaderState, LoaderMessage_t* message);


#endif   // OPCODE001_H

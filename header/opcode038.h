/**
 * \file
 * Declaration of opcode 38 ( upload program )
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
 
#ifndef OPCODE038_H
#define OPCODE038_H

/**
 * Executes opcode 38
 * 
 * @param loaderState Pointer to the current state of the program
 * @param message Pointer to the current SSB message
 * @param timer Pointer to the current Timer running in the Loader
 */
void opcode38_execute(ELoaderState_t* loaderState, LoaderMessage_t* message, Timer_t* timer);

#endif   // OPCODE038_H

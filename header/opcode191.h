/**
 * \file
 * Definition of opcode 191 (compute program CRC)
 * 
 * @author Scott DiPasquale
 * @date 14 September 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */
 
#ifndef OPCODE191_H
#define OPCODE191_H

/**
 * Executes opcode 191 ( compute program CRC )
 * 
 * @param loaderState Pointer to the current state of the program
 * @param message Pointer to the current SSB message
 * @param timer Pointer to the current Timer running in the Loader
 */
void opcode191_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
                        Timer_t* timer) ;

#endif   // OPCODE191_H


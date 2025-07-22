/**
 * \file
 * Definition of opcode 37 (download)
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
 
#ifndef OPCODE037_H
#define OPCODE037_H
#include "loader_state.h"
#include "timer.h"
#include "comm.h"
/**
 * Executes opcode 37 (download)
 * 
 * @param loaderState The current state of the program
 * @param message Pointer to the received message
 * @param timer Pointer to the current Timer running in the Loader
 */
void opcode37_execute(ELoaderState_t* loaderState, LoaderMessage_t* message, Timer_t* timer);

#endif   // OPCODE037_H

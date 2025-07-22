/**
 * \file
 * Interface for opcode 0 (Activate Loader)
 * 
 * @author Scott DiPasquale
 * @date 1 September 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */
 
#ifndef OPCODE000_H
#define OPCODE000_H

/**
 * Executes opcode 0 ( Active Loader )
 * 
 * @param loaderState The current state of the loader
 * @param timer Pointer to the current Timer running in the Loader
 */
void opcode0_execute( ELoaderState_t* loaderState, Timer_t* timer);


#endif   // OPCODE000_H

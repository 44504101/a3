/**
 * \file
 * Definition of opcode 8 ( stop acquisition )
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
 
#ifndef OPCODE008_H
#define OPCODE008_H

/**
 * Executes opcode 8.
 * Returns all 0's, since the response isn't important in the Loader
 * 
 */
void opcode8_execute(void);

#endif   // OPCODE008_H

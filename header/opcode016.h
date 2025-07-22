/**
 * \file
 * Definition of opcode 16 ( request recording status )
 * 
 * @author Tom Skerry
 * @date 5 October 2017
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2017.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */
 
#ifndef OPCODE016_H
#define OPCODE016_H

/**
 * Executes opcode 16.
 * Returns 0 for acquisition status, since the bootloader has no concept of acquisition
 * 
 */
void opcode16_execute(void);

#endif   // OPCODE016_H

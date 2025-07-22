/**
 * \file
 * Contains various definitions and types to describe the current state of the 
 * loader
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
 
#ifndef LOADER_STATE_H
#define LOADER_STATE_H

/**
 * Typedef'ed enumeration describing the possible states of the Loader
 */
typedef enum
{
    LOADER_WAITING,             // initial waiting state
    LOADER_ACTIVATED,           // Waiting for download or upload to start
    LOADER_DOWNLOADING,         // Downloading, and/or waiting for upload to begin
    LOADER_UPLOADING,           // Uploading data
    LOADER_PROGRAMMING,         // Currently writing to ROM
    LOADER_DONE_PROGRAMMING,    // Program operation has finished
    LOADER_PREPARING_SCRATCH,   // Double-buffer scratch space is being prepared
    LOADER_SCRATCH_PREPARED     // The scratch space has been prepared
} ELoaderState_t;

#endif   // LOADER_STATE_H

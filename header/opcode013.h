/**
 * \file
 * Definition of opcode 13 (format memory)
 * 
 */
 
#ifndef OPCODE013_H
#define OPCODE013_H

#include "common_data_types.h"
#include "loader_state.h"
#include "timer.h"
#include "comm.h"
/**
 * Executes opcode 13.
 * 
 */
void opcode13_execute(ELoaderState_t * loaderState, LoaderMessage_t * message, Timer_t* timer);

#endif   // OPCODE013_H

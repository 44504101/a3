/*
 * opcode046.h
 *
 *  Created on: 2022Äê5ÔÂ27ÈÕ
 *      Author: l
 */

#ifndef HEADER_OPCODE046_H_
#define HEADER_OPCODE046_H_

/// Enumerated the fast dump state machine.
typedef enum
{
	FAST_DUMP_INITIAL 		= 0,			///< Initial state.
	FAST_DUMP_FIRST_FRAME 	= 1,			///< Send the first frame.
	FAST_DUMP_EVEN_FRAME	= 2,			///< Send an even frame.
	FAST_DUMP_ODD_FRAME		= 3,			///< Send an odd frame.
	FAST_DUMP_LAST_FRAME	= 4,			///< Send the last frame.
	FAST_DUMP_END			= 5				///< Last step.
}sendFrameState_t;

typedef enum
{
    SSB_TX_BAUD_RATE_57600      = 0u,
    SSB_TX_BAUD_RATE_115200     = 1u,
    SSB_TX_BAUD_RATE_921600     = 2u,
    SSB_TX_BAUD_RATE_1843200    = 3u,
    SSB_TX_BAUD_RATE_3686400    = 4u,
    SSB_TX_BAUD_RATE_UNKNOWN    = 5u
}Ebaud_rate_t;

void opcode46_execute(ELoaderState_t* loaderState, LoaderMessage_t* message,
        Timer_t* timer);


#endif /* HEADER_OPCODE046_H_ */

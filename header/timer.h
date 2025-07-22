/*
	Timer.h
	Keep track of time in the milli-seconds range.
*/
#ifndef _timer_h
#define _timer_h

#include "common_data_types.h"

typedef struct Timer
{
	Uint32 start;
	Uint32 timeout;
} Timer_t;


//Get Current Timer Value
Uint32 Timer_GetRawTime(void);

//Get Stopwatch Value (milliseconds since last ResetStopWatch)
Uint32 Timer_StopWatchGet(Uint32 clk);

//Reset Stopwatch to Zero
void Timer_StopWatchSet(Uint32* pClk);

//Wait for x milliseconds
void Timer_Wait(Uint32 x);

//return a timer object, ready to use
Timer_t Timer_TimerMake(Uint32 period);

//Set Timeout time
void Timer_TimerSet(Timer_t* pTimer, Uint32 period);

//reset timer to the same period
void Timer_TimerReset(Timer_t* pTimer);

//Get amount of time remaining before timing out
Uint32 Timer_TimerRemainGet(Timer_t* pTimer);

//Check if timer has expired, true if the timer has expired
bool_t Timer_TimerExpiredCheck(Timer_t* pTimer);

#endif


/*
	Timer.c File
*/

#include "common_data_types.h"
#include "timer.h"
#include "tool_specific_hardware.h"

// Note - the underlying timer for this module, which is dealt with
// by the tool specific code, is (probably) a 32 bit timer.
// This means the timer will roll over after ~49 days at 1mS rate.

Uint32 Timer_GetRawTime(void)
{
	return ToolSpecificHardware_TimerRawTimeGet();
}

Uint32 Timer_StopWatchGet(Uint32 StopWatch)
{
	return ToolSpecificHardware_TimerRawTimeGet() - StopWatch;
}

void Timer_StopWatchSet(Uint32* StopWatch)
{
	(*StopWatch) = ToolSpecificHardware_TimerRawTimeGet();
}

void Timer_Wait(Uint32 x)
{
	Uint32 start;

	start = ToolSpecificHardware_TimerRawTimeGet();

	//wait until time has elapsed
	while ( (ToolSpecificHardware_TimerRawTimeGet() - start) < x)
	{
	    ;
	}
}

// Return a timer object, ready to use.
// Note that this isn't returning a pointer to the timer,
// it's returning the timer structure itself, so, although
// legal, the behaviour of this function will be platform \ compiler dependent.
Timer_t Timer_TimerMake(Uint32 period)
{
	Timer_t timer;
	timer.start = ToolSpecificHardware_TimerRawTimeGet();
	timer.timeout = period;
	return timer;
}

//Set Timeout time
void Timer_TimerSet(Timer_t* pTimer, Uint32 period)
{
	pTimer->start = ToolSpecificHardware_TimerRawTimeGet();
	pTimer->timeout = period;
}

//Reset timer
void Timer_TimerReset(Timer_t* pTimer)
{
	pTimer->start = ToolSpecificHardware_TimerRawTimeGet();
}

//Get amount of time remaining before timing out
Uint32 Timer_TimerRemainGet(Timer_t* pTimer)
{
	if(Timer_TimerExpiredCheck(pTimer))
	{
		return 0;
	}
	else
	{
		return pTimer->timeout- (ToolSpecificHardware_TimerRawTimeGet() - pTimer->start);
	}
}

//Check if timer has expired, true if the timer has expired
bool_t Timer_TimerExpiredCheck(Timer_t* pTimer)
{
	Uint32	RawTime;

	RawTime = ToolSpecificHardware_TimerRawTimeGet();

	if ( (RawTime - pTimer->start) >= pTimer->timeout)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


#include "common_data_types.h"
#include "opcode008.h"
#include "timer.h"
#include "comm.h"

// Opcode 8 (stop acquisition) is supposed to return the time at which acquisition was stopped.
// In here, we just return 0000 - we use the NUL character to force hex zero into the buffer.
void opcode8_execute(void)
{
    loader_MessageSend(LOADER_OK, 4, "\0\0\0\0");	//lint !e840 Use of nul character in string literal
}


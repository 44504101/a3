#include "common_data_types.h"
#include "opcode016.h"
#include "timer.h"
#include "comm.h"

// Opcode 16 (Request Recording Status) is used to recover the acquisition status
// In the bootloader we have no acquisition task so we return hex zero (null char)
void opcode16_execute(void)
{
    loader_MessageSend(LOADER_OK, 1, "\0");	//lint !e840 Use of nul character in string literal
}


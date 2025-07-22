/**
 * @file        opcode013.c
*/
#include "opcode013.h"
#include "rsapi.h"
#include "rspartition.h"
#include "tool_specific_config.h"

// Opcode 13 (Format memory) is used to recover the acquisition status
void opcode13_execute(ELoaderState_t * loaderState, LoaderMessage_t * message, Timer_t* timer)
{

    rs_error_t                  format_status;
    uint8_t              m_partition_format_progress = 0u;
    uint8_t partition_index = message->dataPtr[0];

    format_status = rspartition_format_partition(partition_index,&m_partition_format_progress);

    if(format_status == RS_ERR_NO_ERROR){
        (void)rspartition_bisection_search_do(partition_index);
        loader_MessageSend(LOADER_OK, 0, ""); //lint !e840 Use of nul character in string literal
    }else{
        loader_MessageSend(LOADER_CANNOT_FORMAT, 0, ""); //lint !e840 Use of nul character in string literal
    }
    Timer_TimerSet(timer, LOADERMODE_TIMEOUT);
    Timer_TimerReset(timer);
}


/******************************************************************
 *
 * Motor_BootLoader Main
 *
 * Author : leig
 *
 ******************************************************************/

#include "common_data_types.h"
#include "timer.h"
#include "tool_specific_config.h"
#include "tool_specific_hardware.h"
#include "loader_state.h"
#include "self_test.h"
#include "comm.h"
#include "rsapi.h"
#include "opcode000.h"
#include "opcode001.h"
#include "opcode002.h"
#include "opcode008.h"
#include "opcode013.h"
#include "opcode016.h"
#include "opcode021.h"
#include "opcode037.h"
#include "opcode038.h"
#include "opcode039.h"
#include "opcode046.h"
#include "opcode070.h"
#include "opcode191.h"
#include "opcode204.h"
#include "opcode205.h"
#include "opcode206.h"
#include "opcode207.h"
#include "opcode208.h"
#include "opcode217.h"
#include "opcode219.h"
#include "opcode221.h"

#ifdef COMM_DEBUG
#include "debug.h"
#endif

// Function prototypes:
static void     common_main(uint32_t initialTimeout, ELoaderState_t initialState);
static void     common_timeoutOperation(ELoaderState_t loaderState);

// Static variable to allow code to boot regardless of whether the CRC is good or not.
static bool_t   mbBootIfBadCRCFound = JUMP_TO_APP_WITH_BAD_CRC;

#ifdef UNIT_TEST_BUILD
void PseudoBootloaderMainLoop(void)
#else
void main(void)
#endif
{
    uint32_t    Timeout;

    ToolSpecificHardware_Initialise();
    SelfTest_TestExecute();
//    rsapi_recording_system_init();     //初始化记录系统
//#ifdef COMM_DEBUG
//    Debug_Initialise();
//#endif

    if (SelfTest_isApplicationImageValid() == TRUE)
    {
        Timeout = WAITMODE_TIMEOUT;
//        ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - application CRC is OK.  Waiting for timeout...\r");
    }
    else if (mbBootIfBadCRCFound == TRUE)
    {
        Timeout = WAITMODE_TIMEOUT;
//        ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - CRC is bad but allowing boot.  Waiting for timeout...\r");
    }
    else
    {
        Timeout = BAD_APP_CRC_TIMEOUT;
//        ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - CRC is bad.  Waiting for timeout...\r");
    }

#ifdef INFINITE_BOOT_MODE
    // Set maximum length timeout if using infinite boot mode - this will take forever to boot!
    Timeout = 0xFFFFFFFFu;
#endif

    common_main(Timeout, LOADER_WAITING);
}


static void common_main(uint32_t initialTimeout, ELoaderState_t initialState)
{
    bool_t              done = FALSE;
    LoaderMessage_t*    messagePtr;
    ELoaderState_t      loaderState = initialState;
    Timer_t             loaderTimer;

    // Start the timer going with the initial timeout given.
    Timer_TimerSet(&loaderTimer, initialTimeout );
    Timer_TimerReset(&loaderTimer);

    // Listen for messages until a timeout occurs.
    while (done == FALSE)
    {
        messagePtr = (LoaderMessage_t*)loader_waitForMessage(&loaderTimer);

        if (NULL == messagePtr)
        {
            if (Timer_TimerExpiredCheck(&loaderTimer))
            {
                done = TRUE;
//                ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - timed out waiting for message.\r");
            }
            else
            {
//                ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - invalid message received.\r");
            }
        }
        else
        {
            // Read the opcode number and execute the proper opcode.
            // The opcodes should reset the timer and maybe set it to a different value.
            switch (messagePtr->opcode)
            {
                case 0:
                    opcode0_execute(&loaderState, &loaderTimer);
                    break;

                case 1:
                    opcode1_execute(&loaderState, messagePtr);
                    break;

                case 2:
                    //lint -fallthrough intentional fallthrough
                case 201:
                    opcode2_execute(&loaderState, &loaderTimer);
                    break;

                case 21:
                    opcode21_execute(&loaderState, &loaderTimer);
                    break;

                case 13:
                    opcode13_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 211:
                    //lint -fallthrough reboot=reset
                case 70:
                    opcode70_execute(&loaderState, messagePtr);
                    break;

                case 37:
                    opcode37_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 38:
                    opcode38_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 39:
                    opcode39_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 46:
                    opcode46_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 191:
                    opcode191_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 204:
                    opcode204_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 205:
                    opcode205_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 206:
                    opcode206_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 207:
                    opcode207_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 208:
                    opcode208_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 217:
                    opcode217_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 219:
                    opcode219_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 221:
                    opcode221_execute(&loaderState, messagePtr, &loaderTimer);
                    break;

                case 8:
                    opcode8_execute();
                    break;

                case 16:
                    opcode16_execute();
                    break;

                // Special case when using the debug port - this is (currently)
                // an opcode which isn't allocated, so we use this to avoid
                // transmitting an invalid opcode message (which the default
                // case would otherwise do).
                case 255:
                    break;

                default:
                    // got an invalid opcode, say so
                    loader_MessageSend(LOADER_INVALID_OPCODE, 0, "");
                    break;
            }
        }
    }

    // Do a timeout operation, which is based on the program state as well as
    // the particular program in use (PROMloader or bootloader).
    common_timeoutOperation(loaderState);
}

static void common_timeoutOperation(ELoaderState_t loaderState)
{
    if (LOADER_WAITING == loaderState)
    {
        // No attempt was made to activate the loader in order to download
        // a new application, so start the application resident in ROM,
        // if CRC is valid OR ( CRC is invalid AND jump to app on a bad CRC ).
        if ( (SelfTest_isApplicationImageValid() == TRUE) || (mbBootIfBadCRCFound == TRUE) )
        {
//            ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - no load attempted, booting application...\r");
            ToolSpecificHardware_TimerDisableAndReset();
            ToolSpecificHardware_ApplicationExecute((void*)APPLICATION_START_ADDRESS);
        }
        else
        {
//            ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - application CRC bad, rebooting the tool...\r");
            //ToolSpecificHardware_ApplicationExecute((void*)APPLICATION_START_ADDRESS);   //雷戈添加跳转程序
            ToolSpecificHardware_CPUReset();
        }
    }
    else
    {
        // An attempt to load another application began, but timed out.
        // Per the common loader spec, reboot the tool.
//        ToolSpecificHardware_DebugMessageSend("MAIN: Bootloader - timed out, rebooting the tool...\r");
        //ToolSpecificHardware_ApplicationExecute((void*)APPLICATION_START_ADDRESS);   //雷戈添加跳转程序
        ToolSpecificHardware_CPUReset();
    }

    // This function might return, but any code executed after it will be
    // meaningless, since a hard reset or jump-to-app is being done.
}

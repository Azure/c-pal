// Copyright (C) Microsoft Corporation. All rights reserved.

// This is a child process (Linux version) that:
// 1. Initializes process_watchdog with a short timeout
// 2. Sleeps longer than the timeout
// 3. Gets terminated by the watchdog when timeout fires
// The parent process (process_watchdog_int.c) launches this and verifies timing.

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <unistd.h>

#include "c_logging/logger.h"

#include "c_pal/process_watchdog.h"

#include "process_watchdog_int_common.h"

int main(int argc, char* argv[])
{
    int result;

    if (logger_init() != 0)
    {
        (void)printf("logger_init failed\n");
        result = CHILD_EXIT_CODE_INIT_FAILED;
    }
    else
    {
        if (argc < 2)
        {
            LogError("Usage: process_watchdog_int_child <timeout_ms>");
            result = CHILD_EXIT_CODE_INIT_FAILED;
        }
        else
        {
            uint32_t timeout_ms = (uint32_t)atoi(argv[1]);

            LogInfo("Child process starting with timeout_ms=%" PRIu32, timeout_ms);

            if (process_watchdog_init(timeout_ms) != 0)
            {
                LogError("process_watchdog_init failed");
                result = CHILD_EXIT_CODE_INIT_FAILED;
            }
            else
            {
                LogInfo("Watchdog initialized, waiting for timeout...");

                // Sleep longer than the timeout - the watchdog should terminate us
                uint32_t sleep_time_ms = timeout_ms * 2;
                usleep(sleep_time_ms * 1000);

                // If we reach here, the watchdog did NOT terminate the process
                LogError("Watchdog did not terminate process after timeout!");

                process_watchdog_deinit();
                result = CHILD_EXIT_CODE_SURVIVED_TIMEOUT;
            }
        }

        logger_deinit();
    }

    return result;
}

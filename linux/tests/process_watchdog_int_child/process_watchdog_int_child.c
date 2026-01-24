// Copyright (C) Microsoft Corporation. All rights reserved.

// This is a child process (Linux version) that:
// 1. Initializes process_watchdog with the specified timeout
// 2. Sleeps for the specified duration
// 3. If sleep < timeout: completes successfully before watchdog fires
// 4. If sleep >= timeout: gets terminated by the watchdog
// The parent process (process_watchdog_int.c) launches this and verifies timing.

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "c_logging/logger.h"

#include "c_pal/threadapi.h"

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
        if (argc < 3 || argv[1] == NULL || argv[2] == NULL)
        {
            LogError("Usage: process_watchdog_int_child <timeout_ms> <sleep_ms>");
            result = CHILD_EXIT_CODE_INIT_FAILED;
        }
        else
        {
            uint32_t timeout_ms = (uint32_t)atoi(argv[1]);
            uint32_t sleep_time_ms = (uint32_t)atoi(argv[2]);

            if (timeout_ms == 0 || sleep_time_ms == 0)
            {
                LogError("timeout_ms and sleep_ms must be non-zero");
                result = CHILD_EXIT_CODE_INIT_FAILED;
            }
            else
            {
                LogInfo("Child process starting with timeout_ms=%" PRIu32 "" ", sleep_ms=%" PRIu32 "", timeout_ms, sleep_time_ms);

                if (process_watchdog_init(timeout_ms) != 0)
                {
                    LogError("process_watchdog_init failed");
                    result = CHILD_EXIT_CODE_INIT_FAILED;
                }
                else
                {
                    LogInfo("Watchdog initialized, sleeping for %" PRIu32 "" " ms...", sleep_time_ms);

                    ThreadAPI_Sleep(sleep_time_ms);

                    // If we reach here, the watchdog did NOT terminate the process
                    if (sleep_time_ms < timeout_ms)
                    {
                        // Expected: child completed before watchdog timeout
                        LogInfo("Child completed successfully before watchdog timeout");
                        process_watchdog_deinit();
                        result = CHILD_EXIT_CODE_SUCCESS;
                    }
                    else
                    {
                        // Unexpected: watchdog should have terminated us
                        LogError("Watchdog did not terminate process after timeout!");
                        process_watchdog_deinit();
                        result = CHILD_EXIT_CODE_SURVIVED_TIMEOUT;
                    }
                }
            }
        }

        logger_deinit();
    }

    return result;
}

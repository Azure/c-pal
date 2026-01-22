// Copyright (C) Microsoft Corporation. All rights reserved.

// Integration test for process_watchdog module (Windows version).
// This test verifies that the watchdog correctly terminates a process after timeout.
//
// Test strategy:
// 1. Launch a child process that initializes watchdog with a short timeout
// 2. The child sleeps longer than the timeout
// 3. The watchdog should terminate the child
// 4. Verify that the child was terminated within the expected time window

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "windows.h"

#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/timer.h"

// Timeout for the child process watchdog (3 seconds)
#define WATCHDOG_TIMEOUT_MS 3000

// Tolerance for timing verification
// Allow variance for process startup, VLD initialization/shutdown, and scheduling
#define TOLERANCE_MS 5000

// Maximum time to wait for child (should be much more than timeout + tolerance)
#define MAX_WAIT_MS 10000

// Child exit code when it survives the timeout (indicates failure)
#define CHILD_EXIT_CODE_SURVIVED_TIMEOUT 2

static int launch_and_wait_for_child(const char* exe_path, uint32_t timeout_ms, double* out_elapsed_ms)
{
    int result;
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    char cmd_line[512];

    (void)snprintf(cmd_line, sizeof(cmd_line), "\"%s\" %" PRIu32, exe_path, timeout_ms);

    LogInfo("Launching child process: %s", cmd_line);

    double start_ms = timer_global_get_elapsed_ms();

    if (!CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        LogError("CreateProcessA failed with error %" PRIu32, GetLastError());
        result = -1;
    }
    else
    {
        // Wait for child to terminate
        DWORD wait_result = WaitForSingleObject(pi.hProcess, MAX_WAIT_MS);

        double end_ms = timer_global_get_elapsed_ms();
        *out_elapsed_ms = end_ms - start_ms;

        if (wait_result == WAIT_TIMEOUT)
        {
            LogError("Child process did not terminate within %d ms", MAX_WAIT_MS);
            TerminateProcess(pi.hProcess, 99);
            result = -1;
        }
        else if (wait_result == WAIT_FAILED)
        {
            LogError("WaitForSingleObject failed with error %" PRIu32, GetLastError());
            result = -1;
        }
        else
        {
            DWORD exit_code;
            if (!GetExitCodeProcess(pi.hProcess, &exit_code))
            {
                LogError("GetExitCodeProcess failed with error %" PRIu32, GetLastError());
                result = -1;
            }
            else
            {
                LogInfo("Child process terminated with exit code %" PRIu32, exit_code);
                result = (int)exit_code;
            }
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return result;
}

static void get_child_exe_path(char* buffer, size_t buffer_size)
{
    // Get the directory of the current executable
    char module_path[MAX_PATH];
    GetModuleFileNameA(NULL, module_path, MAX_PATH);

    // Find the last backslash
    char* last_slash = strrchr(module_path, '\\');
    if (last_slash != NULL)
    {
        *last_slash = '\0';
    }

    // Construct the child executable path (child is in sibling folder)
    (void)snprintf(buffer, buffer_size, "%s\\..\\process_watchdog_int_child\\process_watchdog_int_child.exe", module_path);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(function_init)
{
}

TEST_FUNCTION_CLEANUP(function_cleanup)
{
}

// Test that process_watchdog terminates a process after timeout
TEST_FUNCTION(process_watchdog_terminates_on_timeout)
{
    // arrange
    char child_exe_path[1024];
    get_child_exe_path(child_exe_path, sizeof(child_exe_path));
    LogInfo("Child executable path: %s", child_exe_path);

    double elapsed_ms = 0;

    // act - launch child and wait for termination
    int exit_code = launch_and_wait_for_child(child_exe_path, WATCHDOG_TIMEOUT_MS, &elapsed_ms);

    LogInfo("Child process elapsed time: %.0f ms", elapsed_ms);
    LogInfo("Child process exit code: %d", exit_code);

    // assert - verify timing
    // The process should terminate after the timeout but within tolerance
    ASSERT_IS_TRUE(elapsed_ms >= WATCHDOG_TIMEOUT_MS,
        "Process terminated too early: %.0f ms < %d ms", elapsed_ms, WATCHDOG_TIMEOUT_MS);
    ASSERT_IS_TRUE(elapsed_ms <= WATCHDOG_TIMEOUT_MS + TOLERANCE_MS,
        "Process terminated too late: %.0f ms > %d ms", elapsed_ms, WATCHDOG_TIMEOUT_MS + TOLERANCE_MS);

    // The child should NOT have exited cleanly (it was terminated by watchdog)
    // On Windows, LogCriticalAndTerminate calls TerminateProcess which can give various exit codes
    ASSERT_ARE_NOT_EQUAL(int, CHILD_EXIT_CODE_SURVIVED_TIMEOUT, exit_code,
        "Child process survived the timeout - watchdog did not terminate it");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

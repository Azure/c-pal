// Copyright (C) Microsoft Corporation. All rights reserved.

// Integration test for process_watchdog module (Windows version).
// This test verifies that the watchdog correctly terminates a process after timeout
// and also verifies that the process completes successfully when it finishes before timeout.
//
// Test strategy for timeout test:
// 1. Launch a child process that initializes watchdog with a short timeout
// 2. The child sleeps longer than the timeout
// 3. The watchdog should terminate the child
// 4. Verify that the child was terminated within the expected time window
//
// Test strategy for no-timeout test:
// 1. Launch a child process that initializes watchdog with a long timeout
// 2. The child sleeps for a short duration (less than timeout)
// 3. The child should complete successfully before watchdog fires
// 4. Verify that the child exited with success code

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "windows.h"

#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/timer.h"

// Shared exit codes between parent and child
#include "process_watchdog_int_common.h"

// Timeout for the child process watchdog (3 seconds)
#define WATCHDOG_TIMEOUT_MS 3000

// Tolerance for timing verification
// Allow variance for process startup, VLD initialization/shutdown, and scheduling
#define TOLERANCE_MS 5000

// Maximum time to wait for child (computed from timeout + tolerance)
#define MAX_WAIT_MS (WATCHDOG_TIMEOUT_MS + TOLERANCE_MS)

// For no-timeout test: short sleep, long timeout
#define SHORT_SLEEP_MS 1000
#define LONG_TIMEOUT_MS 10000

static int launch_and_wait_for_child(const char* exe_path, uint32_t timeout_ms, uint32_t sleep_ms, double* out_elapsed_ms)
{
    int result;
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    char cmd_line[512];

    int snprintf_result = snprintf(cmd_line, sizeof(cmd_line), "\"%s\" %" PRIu32 " %" PRIu32, exe_path, timeout_ms, sleep_ms);
    ASSERT_IS_TRUE(snprintf_result > 0 && (size_t)snprintf_result < sizeof(cmd_line));

    LogInfo("Launching child process: %s", cmd_line);

    double start_ms = timer_global_get_elapsed_ms();

    if (!CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        LogError("CreateProcessA failed with error %" PRIu32, GetLastError());
        result = -1;
    }
    else
    {
        // WaitForSingleObject: waits until the process handle is signaled (process terminates) or timeout
        DWORD wait_result = WaitForSingleObject(pi.hProcess, MAX_WAIT_MS);

        double end_ms = timer_global_get_elapsed_ms();
        *out_elapsed_ms = end_ms - start_ms;

        // WAIT_TIMEOUT: the wait timed out before the process terminated
        if (wait_result == WAIT_TIMEOUT)
        {
            LogError("Child process did not terminate within %d ms", MAX_WAIT_MS);
            // TerminateProcess: forcefully terminates the child process
            TerminateProcess(pi.hProcess, 99);
            result = -1;
        }
        // WAIT_FAILED: the wait function failed
        else if (wait_result == WAIT_FAILED)
        {
            LogError("WaitForSingleObject failed with error %" PRIu32, GetLastError());
            result = -1;
        }
        // WAIT_OBJECT_0: the process handle was signaled (process terminated)
        else
        {
            DWORD exit_code;
            // GetExitCodeProcess: retrieves the exit code of the terminated process
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

// Constructs the path to the child test executable.
// The child executable is built in the parent directory of this test:
//   test_exe:   BUILD_DIR/process_watchdog_int_exe_<project>/process_watchdog_int_exe_<project>.exe
//   child_exe:  BUILD_DIR/process_watchdog_int_child.exe
// This function uses GetModuleFileNameA to get the current executable path,
// then looks for the child executable in the parent directory.
static void get_child_exe_path(char* buffer, size_t buffer_size)
{
    // GetModuleFileNameA: retrieves the full path of the current executable (NULL = current process)
    char module_path[MAX_PATH];
    DWORD path_len = GetModuleFileNameA(NULL, module_path, MAX_PATH);
    ASSERT_IS_TRUE(path_len > 0 && path_len < MAX_PATH);

    // strrchr: find the last backslash to extract the directory portion
    char* last_slash = strrchr(module_path, '\\');
    ASSERT_IS_NOT_NULL(last_slash);
    *last_slash = '\0';

    // Go up one level to BUILD_DIR where the child executable is located
    int snprintf_result = snprintf(buffer, buffer_size, "%s\\..\\process_watchdog_int_child.exe", module_path);
    ASSERT_IS_TRUE(snprintf_result > 0 && (size_t)snprintf_result < buffer_size);
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
    // Sleep for twice the timeout to ensure watchdog triggers
    uint32_t sleep_ms = WATCHDOG_TIMEOUT_MS * 2;

    // act - launch child and wait for termination
    int exit_code = launch_and_wait_for_child(child_exe_path, WATCHDOG_TIMEOUT_MS, sleep_ms, &elapsed_ms);

    LogInfo("Child process elapsed time: %.0f ms", elapsed_ms);
    LogInfo("Child process exit code: %d", exit_code);

    // assert - verify timing
    // The process should terminate after the timeout but within tolerance
    ASSERT_IS_TRUE(elapsed_ms >= WATCHDOG_TIMEOUT_MS,
        "Process terminated too early: %.0f ms < %d ms", elapsed_ms, WATCHDOG_TIMEOUT_MS);
    ASSERT_IS_TRUE(elapsed_ms <= WATCHDOG_TIMEOUT_MS + TOLERANCE_MS,
        "Process terminated too late: %.0f ms > %d ms", elapsed_ms, WATCHDOG_TIMEOUT_MS + TOLERANCE_MS);

    // The child should NOT have exited cleanly (it was terminated by watchdog)
    ASSERT_ARE_NOT_EQUAL(int, CHILD_EXIT_CODE_SURVIVED_TIMEOUT, exit_code,
        "Child process survived the timeout - watchdog did not terminate it");
}

// Test that process_watchdog does NOT terminate a process that completes before timeout
TEST_FUNCTION(process_watchdog_does_not_terminate_before_timeout)
{
    // arrange
    char child_exe_path[1024];
    get_child_exe_path(child_exe_path, sizeof(child_exe_path));
    LogInfo("Child executable path: %s", child_exe_path);

    double elapsed_ms = 0;

    // act - launch child with long timeout but short sleep
    // Child should complete successfully before watchdog fires
    int exit_code = launch_and_wait_for_child(child_exe_path, LONG_TIMEOUT_MS, SHORT_SLEEP_MS, &elapsed_ms);

    LogInfo("Child process elapsed time: %.0f ms", elapsed_ms);
    LogInfo("Child process exit code: %d", exit_code);

    // assert - verify timing
    // The process should complete around SHORT_SLEEP_MS (with some tolerance for startup)
    ASSERT_IS_TRUE(elapsed_ms >= SHORT_SLEEP_MS,
        "Process completed too early: %.0f ms < %d ms", elapsed_ms, SHORT_SLEEP_MS);
    ASSERT_IS_TRUE(elapsed_ms < LONG_TIMEOUT_MS,
        "Process took too long: %.0f ms >= %d ms (timeout)", elapsed_ms, LONG_TIMEOUT_MS);

    // The child should have exited successfully (not terminated by watchdog)
    ASSERT_ARE_EQUAL(int, CHILD_EXIT_CODE_SUCCESS, exit_code,
        "Child process did not complete successfully, exit_code=%d", exit_code);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

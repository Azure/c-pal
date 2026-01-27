// Copyright (C) Microsoft Corporation. All rights reserved.

// Integration test for process_watchdog module (Linux version).
// This test verifies that the watchdog correctly terminates a process after timeout
// and also verifies that the process completes successfully when it finishes before timeout.

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <spawn.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/timer.h"

// Shared exit codes between parent and child
#include "process_watchdog_int_common.h"

// Timeout for the child process watchdog (3 seconds)
#define WATCHDOG_TIMEOUT_MS 3000

// Tolerance for timing verification
// Allow variance for process startup and scheduling
#define TOLERANCE_MS 5000

// Maximum time to wait for child (computed from timeout + tolerance)
#define MAX_WAIT_MS (WATCHDOG_TIMEOUT_MS + TOLERANCE_MS)

// For no-timeout test: short sleep, long timeout
#define SHORT_SLEEP_MS 1000
#define LONG_TIMEOUT_MS 10000

static int launch_and_wait_for_child(const char* exe_path, uint32_t timeout_ms, uint32_t sleep_ms, double* out_elapsed_ms)
{
    int result;

    // Build argv array for posix_spawn
    char timeout_str[32];
    char sleep_str[32];
    int snprintf_result = snprintf(timeout_str, sizeof(timeout_str), "%" PRIu32 "", timeout_ms);
    if (snprintf_result < 0 || (size_t)snprintf_result >= sizeof(timeout_str))
    {
        LogError("snprintf failed for timeout_ms");
        result = -1;
    }
    else
    {
        snprintf_result = snprintf(sleep_str, sizeof(sleep_str), "%" PRIu32 "", sleep_ms);
        if (snprintf_result < 0 || (size_t)snprintf_result >= sizeof(sleep_str))
        {
            LogError("snprintf failed for sleep_ms");
            result = -1;
        }
        else
        {
            // posix_spawn argv: program name, timeout_ms, sleep_ms, NULL terminator
            char* argv[] = { (char*)exe_path, timeout_str, sleep_str, NULL };

            LogInfo("Launching child process: %s timeout=%" PRIu32 "" " sleep=%" PRIu32 "", exe_path, timeout_ms, sleep_ms);

            double start_ms = timer_global_get_elapsed_ms();

            // posix_spawn creates a new process without fork(), avoiding inherited state issues
            pid_t pid;
            int spawn_result = posix_spawn(&pid, exe_path, NULL, NULL, argv, NULL);
            if (spawn_result != 0)
            {
                LogError("posix_spawn failed with error %d: %s", spawn_result, strerror(spawn_result));
                result = -1;
            }
            else
            {
                // Wait for child process
                int status;
                pid_t wait_result = waitpid(pid, &status, 0);

                double end_ms = timer_global_get_elapsed_ms();
                *out_elapsed_ms = end_ms - start_ms;

                if (wait_result < 0)
                {
                    LogError("waitpid failed with error: %s", strerror(errno));
                    result = -1;
                }
                // WIFEXITED: returns true if child terminated normally (via exit() or return from main)
                else if (WIFEXITED(status))
                {
                    // WEXITSTATUS: extracts the exit code passed to exit() or returned from main
                    result = WEXITSTATUS(status);
                    LogInfo("Child process exited with status %d", result);
                }
                // WIFSIGNALED: returns true if child was terminated by a signal
                else if (WIFSIGNALED(status))
                {
                    // WTERMSIG: extracts the signal number that caused termination
                    int sig = WTERMSIG(status);
                    LogInfo("Child process was terminated by signal %d", sig);
                    // Return signal number + 128 to indicate signal termination (standard shell convention)
                    result = 128 + sig;
                }
                else
                {
                    LogError("Child process terminated abnormally");
                    result = -1;
                }
            }
        }
    }

    return result;
}

// Constructs the path to the child test executable.
// The child executable is built in a sibling directory of this test:
//   test_exe:   BUILD_DIR/process_watchdog_int_exe_<project>/process_watchdog_int_exe_<project>
//   child_exe:  BUILD_DIR/process_watchdog_int_child/process_watchdog_int_child
// This function reads /proc/self/exe to get the current executable path,
// then looks for the child executable in its sibling directory.
static void get_child_exe_path(char* buffer, size_t buffer_size)
{
    // readlink: reads the symbolic link /proc/self/exe which points to the current executable
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    ASSERT_IS_TRUE(len > 0);
    exe_path[len] = '\0';
    // dirname: extracts the directory portion of the path (e.g., BUILD_DIR/process_watchdog_int_exe_<project>)
    char* dir = dirname(exe_path);
    ASSERT_IS_NOT_NULL(dir);
    // Go up one level to BUILD_DIR, then into child's directory
    int snprintf_result = snprintf(buffer, buffer_size, "%s/../process_watchdog_int_child/process_watchdog_int_child", dir);
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
// Test strategy:
// 1. Launch a child process that initializes watchdog with a short timeout (WATCHDOG_TIMEOUT_MS)
// 2. The child sleeps longer than the timeout (sleep_ms = timeout * 2)
// 3. The watchdog should terminate the child
// 4. Verify that the child was terminated within the expected time window
TEST_FUNCTION(process_watchdog_terminates_on_timeout)
{
    // arrange
    char child_exe_path[1024];
    get_child_exe_path(child_exe_path, sizeof(child_exe_path));
    LogInfo("Child executable path: %s", child_exe_path);

    double elapsed_ms = 0;
    // 1. Launch child with short timeout, 2. child sleeps longer than timeout
    uint32_t sleep_ms = WATCHDOG_TIMEOUT_MS * 2;

    // act - 3. launch child and wait for watchdog to terminate it
    int exit_code = launch_and_wait_for_child(child_exe_path, WATCHDOG_TIMEOUT_MS, sleep_ms, &elapsed_ms);

    LogInfo("Child process elapsed time: %.0f ms", elapsed_ms);
    LogInfo("Child process exit code: %d", exit_code);

    // assert - 4. verify timing and termination
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
// Test strategy:
// 1. Launch a child process that initializes watchdog with a long timeout (LONG_TIMEOUT_MS)
// 2. The child sleeps for a short duration (SHORT_SLEEP_MS, less than timeout)
// 3. The child should complete successfully before watchdog fires
// 4. Verify that the child exited with success code
TEST_FUNCTION(process_watchdog_does_not_terminate_before_timeout)
{
    // arrange
    char child_exe_path[1024];
    get_child_exe_path(child_exe_path, sizeof(child_exe_path));
    LogInfo("Child executable path: %s", child_exe_path);

    double elapsed_ms = 0;

    // act - 1. launch child with long timeout, 2. child sleeps for short duration
    // 3. child should complete successfully before watchdog fires
    int exit_code = launch_and_wait_for_child(child_exe_path, LONG_TIMEOUT_MS, SHORT_SLEEP_MS, &elapsed_ms);

    LogInfo("Child process elapsed time: %.0f ms", elapsed_ms);
    LogInfo("Child process exit code: %d", exit_code);

    // assert - 4. verify timing and successful completion
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

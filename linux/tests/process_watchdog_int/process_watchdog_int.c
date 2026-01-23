// Copyright (C) Microsoft Corporation. All rights reserved.

// Integration test for process_watchdog module (Linux version).
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

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

static int launch_and_wait_for_child(const char* exe_path, uint32_t timeout_ms, double* out_elapsed_ms)
{
    int result;

    LogInfo("Launching child process: %s %" PRIu32 "", exe_path, timeout_ms);

    double start_ms = timer_global_get_elapsed_ms();

    pid_t pid = fork();
    if (pid < 0)
    {
        LogErrorNo("fork failed");
        result = -1;
    }
    else if (pid == 0)
    {
        // Child process - exec the test program
        // Note: Cannot use ASSERT macros in forked child as they would affect child process, not the test
        char timeout_str[32];
        int snprintf_result = snprintf(timeout_str, sizeof(timeout_str), "%" PRIu32, timeout_ms);
        if (snprintf_result < 0 || (size_t)snprintf_result >= sizeof(timeout_str))
        {
            // Using printf because logger_init may not have been called in child process
            (void)printf("snprintf failed for timeout_ms\n");
            _exit(MU_FAILURE);
        }
        else
        {
            // execl only returns on error (-1), on success it never returns
            if (execl(exe_path, exe_path, timeout_str, NULL) == -1)
            {
                // Using printf because logger_init may not have been called in child process
                (void)printf("execl failed: %s\n", strerror(errno));
            }
            _exit(MU_FAILURE);
        }
    }
    else
    {
        // Parent process - wait for child
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

    return result;
}

// Constructs the path to the child test executable.
// The child executable is built in the same directory as this test (via CMake configuration):
//   test_exe:   BUILD_DIR/process_watchdog_int_exe_<project>/process_watchdog_int_exe_<project>
//   child_exe:  BUILD_DIR/process_watchdog_int_exe_<project>/process_watchdog_int_child
// This function reads /proc/self/exe to get the current executable path,
// then looks for the child executable in the same directory.
static void get_child_exe_path(char* buffer, size_t buffer_size)
{
    // readlink: reads the symbolic link /proc/self/exe which points to the current executable
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    ASSERT_IS_TRUE(len > 0);
    exe_path[len] = '\0';
    // dirname: extracts the directory portion of the path
    char* dir = dirname(exe_path);
    ASSERT_IS_NOT_NULL(dir);
    // Child exe is in the same directory
    int snprintf_result = snprintf(buffer, buffer_size, "%s/process_watchdog_int_child", dir);
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
    ASSERT_ARE_NOT_EQUAL(int, CHILD_EXIT_CODE_SURVIVED_TIMEOUT, exit_code,
        "Child process survived the timeout - watchdog did not terminate it");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

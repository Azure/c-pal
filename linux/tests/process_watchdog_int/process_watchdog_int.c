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

#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/timer.h"

// Timeout for the child process watchdog (3 seconds)
#define WATCHDOG_TIMEOUT_MS 3000

// Tolerance for timing verification
// Allow variance for process startup and scheduling
#define TOLERANCE_MS 5000

// Maximum time to wait for child (should be much more than timeout + tolerance)
#define MAX_WAIT_MS 10000

// Child exit code when it survives the timeout (indicates failure)
#define CHILD_EXIT_CODE_SURVIVED_TIMEOUT 2

static int launch_and_wait_for_child(const char* exe_path, uint32_t timeout_ms, double* out_elapsed_ms)
{
    int result;

    LogInfo("Launching child process: %s %" PRIu32, exe_path, timeout_ms);

    double start_ms = timer_global_get_elapsed_ms();

    pid_t pid = fork();
    if (pid < 0)
    {
        LogError("fork failed with error: %s", strerror(errno));
        result = -1;
    }
    else if (pid == 0)
    {
        // Child process - exec the test program
        char timeout_str[32];
        (void)snprintf(timeout_str, sizeof(timeout_str), "%" PRIu32, timeout_ms);
        execl(exe_path, exe_path, timeout_str, NULL);

        // If execl returns, it failed
        (void)fprintf(stderr, "execl failed: %s\n", strerror(errno));
        _exit(127);
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
        else if (WIFEXITED(status))
        {
            result = WEXITSTATUS(status);
            LogInfo("Child process exited with status %d", result);
        }
        else if (WIFSIGNALED(status))
        {
            int sig = WTERMSIG(status);
            LogInfo("Child process was terminated by signal %d", sig);
            // Return signal number + 128 to indicate signal termination
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

static void get_child_exe_path(char* buffer, size_t buffer_size)
{
    // Get the directory of the current executable
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0)
    {
        exe_path[len] = '\0';
        char* dir = dirname(exe_path);
        (void)snprintf(buffer, buffer_size, "%s/../process_watchdog_int_child/process_watchdog_int_child", dir);
    }
    else
    {
        // Fallback: assume child is in current directory
        (void)snprintf(buffer, buffer_size, "./process_watchdog_int_child");
    }
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
    // On Linux, it may exit via abort() or similar
    ASSERT_ARE_NOT_EQUAL(int, CHILD_EXIT_CODE_SURVIVED_TIMEOUT, exit_code,
        "Child process survived the timeout - watchdog did not terminate it");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

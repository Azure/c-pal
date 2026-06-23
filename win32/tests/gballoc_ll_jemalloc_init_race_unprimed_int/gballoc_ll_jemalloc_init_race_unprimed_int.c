// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Demonstration of the defect fixed by SRS_GBALLOC_LL_JEMALLOC_02_011 (jemalloc init-time priming).
//
// jemalloc initializes lazily on the process's first allocation, and that one-time initialization is
// not safe to run concurrently. WITHOUT priming, many threads making the first ever allocation at the
// same time race jemalloc's lazy init and corrupt its global state (crash or deadlock).
//
// The failure can only be hunted ONCE per process: after the first allocation succeeds jemalloc is
// initialized and the race window is gone. So this test HUNTS the failure by respawning fresh child
// processes (each making the first allocation a 16-thread race) until one crashes or hangs, bounded to
// ~9 minutes. Reproducing the failure fails this test, which CTest inverts to a pass via the WILL_FAIL
// property (see CMakeLists.txt). The companion gballoc_ll_jemalloc_init_race_primed_int proves the
// priming fix removes the race.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>     // for getenv
#include <inttypes.h>

#include <windows.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_pal/gballoc_ll.h"
#include "c_pal/timed_test_suite.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

#define RACE_THREAD_COUNT   16
#define RACE_ALLOC_COUNT    256

// when set in the environment, this process is a spawned child that runs a single race and exits
#define CHILD_ENV_NAME      "GBALLOC_LL_JEMALLOC_RACE_CHILD"
// a child either crashes/passes in well under a second; waiting longer than this means it deadlocked
#define CHILD_WAIT_MS       15000
// total hunt budget, kept comfortably under the 10 minute timed-test backstop
#define HUNT_BUDGET_MS      540000.0

typedef struct RACE_CONTEXT_TAG
{
    volatile_atomic int32_t* ready_count;
    volatile_atomic int32_t* start_gate;
    int result;
} RACE_CONTEXT;

static int race_first_allocation(void* arg)
{
    RACE_CONTEXT* context = (RACE_CONTEXT*)arg;

    (void)interlocked_increment(context->ready_count);
    while (interlocked_add(context->start_gate, 0) == 0)
    {
    }

    context->result = 0;
    for (uint32_t i = 0; i < RACE_ALLOC_COUNT; i++)
    {
        void* ptr = gballoc_ll_malloc(8 + (i * 13) % 1024);
        if (ptr == NULL)
        {
            context->result = MU_FAILURE;
            break;
        }
        gballoc_ll_free(ptr);
    }
    return context->result;
}

// Runs exactly one simultaneous-start race of the process's first jemalloc allocation. A crash or a
// deadlock here is the defect being demonstrated and takes the whole (child) process down with it.
static void run_one_first_allocation_race(void)
{
    volatile_atomic int32_t ready_count;
    volatile_atomic int32_t start_gate;
    (void)interlocked_exchange(&ready_count, 0);
    (void)interlocked_exchange(&start_gate, 0);

    RACE_CONTEXT contexts[RACE_THREAD_COUNT];
    THREAD_HANDLE threads[RACE_THREAD_COUNT];
    for (uint32_t index = 0; index < RACE_THREAD_COUNT; index++)
    {
        contexts[index].ready_count = &ready_count;
        contexts[index].start_gate = &start_gate;
        contexts[index].result = MU_FAILURE;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[index], race_first_allocation, &contexts[index]));
    }

    while (interlocked_add(&ready_count, 0) != RACE_THREAD_COUNT)
    {
    }

    (void)interlocked_exchange(&start_gate, 1);

    for (uint32_t index = 0; index < RACE_THREAD_COUNT; index++)
    {
        int dont_care;
        (void)ThreadAPI_Join(threads[index], &dont_care);
    }
}

// spawns one child that runs a single first-allocation race; returns true if that child crashed or hung
static bool child_reproduced_the_race(const char* exe_path)
{
    bool reproduced = false;

    STARTUPINFOA startup_info;
    PROCESS_INFORMATION process_info;
    (void)memset(&startup_info, 0, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    (void)memset(&process_info, 0, sizeof(process_info));

    // the child inherits this process's environment, which now carries CHILD_ENV_NAME
    if (!CreateProcessA(exe_path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info))
    {
        LogError("CreateProcessA(%s) failed, GetLastError()=%lu", exe_path, GetLastError());
    }
    else
    {
        DWORD wait_result = WaitForSingleObject(process_info.hProcess, CHILD_WAIT_MS);
        if (wait_result == WAIT_TIMEOUT)
        {
            // the child deadlocked racing jemalloc's lazy init
            (void)TerminateProcess(process_info.hProcess, 1);
            (void)WaitForSingleObject(process_info.hProcess, INFINITE);
            reproduced = true;
        }
        else
        {
            DWORD exit_code = 0;
            (void)GetExitCodeProcess(process_info.hProcess, &exit_code);
            // a non-zero exit means the child crashed racing jemalloc's lazy init
            reproduced = (exit_code != 0);
        }
        (void)CloseHandle(process_info.hThread);
        (void)CloseHandle(process_info.hProcess);
    }
    return reproduced;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TIMED_TEST_SUITE_INITIALIZE(TestClassInitialize, TIMED_TEST_DEFAULT_TIMEOUT_MS)
{
    // deliberately NO priming here: a child must hit jemalloc's lazy init concurrently
}

TIMED_TEST_SUITE_CLEANUP(TestClassCleanup)
{
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(unprimed_jemalloc_first_allocation_race_is_hunted_until_it_fails)
{
    if (getenv(CHILD_ENV_NAME) != NULL)
    {
        // CHILD: run a single race. A crash/hang takes this process down (that is the signal the parent
        // hunts for). If it somehow survives, exit cleanly so the parent keeps hunting.
        run_one_first_allocation_race();
        (void)TerminateProcess(GetCurrentProcess(), 0);
    }
    else
    {
        ///arrange
        char exe_path[MAX_PATH];
        DWORD path_len = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));
        ASSERT_IS_TRUE(path_len > 0 && path_len < sizeof(exe_path));

        // children inherit this; the parent already passed its own child check above
        ASSERT_IS_TRUE(SetEnvironmentVariableA(CHILD_ENV_NAME, "1"));

        ///act
        // respawn fresh child processes (each gets one shot at jemalloc's first-init race) until one fails
        bool reproduced = false;
        uint32_t attempts = 0;
        double start_ms = timer_global_get_elapsed_ms();
        while (!reproduced)
        {
            attempts++;
            reproduced = child_reproduced_the_race(exe_path);

            if (timer_global_get_elapsed_ms() - start_ms >= HUNT_BUDGET_MS)
            {
                break;
            }
        }

        ///assert
        // reproducing the crash is the expected outcome; failing here is what the WILL_FAIL registration inverts to a pass
        if (reproduced)
        {
            ASSERT_FAIL("reproduced the unprimed jemalloc first-allocation race after %" PRIu32 " attempt(s)", attempts);
        }
        LogError("did NOT reproduce the unprimed jemalloc first-allocation race within the hunt budget after %" PRIu32 " attempt(s)", attempts);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

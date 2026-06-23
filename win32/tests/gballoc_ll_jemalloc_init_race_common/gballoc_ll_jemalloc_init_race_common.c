// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>     // for getenv
#include <inttypes.h>

#include <windows.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_pal/gballoc_ll.h"

#include "gballoc_ll_jemalloc_init_race_common.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

#define RACE_THREAD_COUNT   16
#define RACE_ALLOC_COUNT    256

// when set in the environment, this process is a spawned child that runs a single race and exits
#define CHILD_ENV_NAME      "GBALLOC_LL_JEMALLOC_RACE_CHILD"
// a child either crashes/passes in well under a second; waiting longer than this means it deadlocked
#define CHILD_WAIT_MS       15000
// total time the parent spends spawning children, hardcoded to 3 minutes
#define SPAWN_BUDGET_MS     180000.0

typedef struct RACE_CONTEXT_TAG
{
    volatile_atomic int32_t* ready_count;
    volatile_atomic int32_t* start_gate;
    int result;
} RACE_CONTEXT;

static int race_first_allocation(void* arg)
{
    RACE_CONTEXT* context = (RACE_CONTEXT*)arg;

    // announce readiness, then spin until released so every thread allocates at the same instant
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

// runs exactly one simultaneous-start race of the process's first jemalloc allocation; a crash or a
// deadlock here takes the whole (child) process down
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

// spawns one child that runs a single race; returns true if that child crashed, hung, or returned non-zero
static bool child_failed(const char* exe_path)
{
    bool failed;

    STARTUPINFOA startup_info;
    PROCESS_INFORMATION process_info;
    (void)memset(&startup_info, 0, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    (void)memset(&process_info, 0, sizeof(process_info));

    // the child inherits this process's environment, which carries CHILD_ENV_NAME
    if (!CreateProcessA(exe_path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info))
    {
        LogError("CreateProcessA(%s) failed, GetLastError()=%lu", exe_path, GetLastError());
        failed = true;
    }
    else
    {
        DWORD wait_result = WaitForSingleObject(process_info.hProcess, CHILD_WAIT_MS);
        if (wait_result == WAIT_TIMEOUT)
        {
            // the child deadlocked
            (void)TerminateProcess(process_info.hProcess, 1);
            (void)WaitForSingleObject(process_info.hProcess, INFINITE);
            failed = true;
        }
        else
        {
            DWORD exit_code = 0;
            (void)GetExitCodeProcess(process_info.hProcess, &exit_code);
            // a non-zero exit means the child crashed or its race saw a NULL allocation
            failed = (exit_code != 0);
        }
        (void)CloseHandle(process_info.hThread);
        (void)CloseHandle(process_info.hProcess);
    }
    return failed;
}

void gballoc_ll_jemalloc_init_race_run(bool prime)
{
    if (getenv(CHILD_ENV_NAME) != NULL)
    {
        // CHILD: optionally prime jemalloc single-threaded, then run one race. A crash/hang takes this
        // process down; otherwise exit cleanly so the parent keeps spawning.
        if (prime)
        {
            (void)gballoc_ll_init(NULL);
        }
        run_one_first_allocation_race();
        (void)TerminateProcess(GetCurrentProcess(), 0);
    }
    else
    {
        // PARENT: spawn fresh child processes for the whole budget, stopping early if one fails
        char exe_path[MAX_PATH];
        DWORD path_len = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));
        ASSERT_IS_TRUE(path_len > 0 && path_len < sizeof(exe_path));

        // children inherit this; the parent already passed its own child check above
        ASSERT_IS_TRUE(SetEnvironmentVariableA(CHILD_ENV_NAME, "1"));

        bool any_child_failed = false;
        uint32_t attempts = 0;
        double start_ms = timer_global_get_elapsed_ms();
        while (timer_global_get_elapsed_ms() - start_ms < SPAWN_BUDGET_MS)
        {
            attempts++;
            if (child_failed(exe_path))
            {
                any_child_failed = true;
                break;
            }
        }

        // A failing child fails this test. The primed test expects no failure and passes; the unprimed
        // test is registered WILL_FAIL, so the failure it reliably finds is inverted to a pass.
        if (any_child_failed)
        {
            ASSERT_FAIL("a spawned child crashed, hung, or returned non-zero after %" PRIu32 " attempt(s)", attempts);
        }
        LogInfo("all %" PRIu32 " spawned child process(es) finished and returned 0 within the spawn budget", attempts);
    }
}

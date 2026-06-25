// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Regression test for jemalloc's concurrent first-allocation (lazy init) safety, WITHOUT priming.
//
// History: gballoc_ll_init used to "prime" jemalloc (force its one-time init single-threaded) because
// jemalloc 5.3.0's Windows TSD allocated a per-thread wrapper on first access, so several threads
// making the process's first ever allocation at once raced jemalloc's lazy init and corrupted its
// global state (crash/hang). jemalloc 5.3.1 fixed that root cause (commit 3a0d9cda, native MSVC
// __declspec(thread) TSD - no first-access allocation), so the priming workaround was removed.
//
// This test guards against a regression of that bug. Because the race is on the process's FIRST ever
// jemalloc allocation, it can only be exercised once per process, so the parent spawns many fresh
// child processes; each child races the unprimed first allocation across several threads. A regression
// would corrupt jemalloc's state and show up as a child that crashes, hangs, or returns non-zero. The
// parent (run under VLD) fails if any child fails; with a correct jemalloc every child completes
// cleanly. NOTE: jemalloc allocates from VirtualAlloc/mmap, not the CRT heap VLD tracks, so the
// authoritative regression signal here is corruption/crash/hang rather than a CRT leak report.

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

#include "c_pal/gballoc_ll.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

#define RACE_THREAD_COUNT   16
#define RACE_ALLOC_COUNT    256

// when set in the environment, this process is a spawned child that runs one race and exits
#define CHILD_ENV_NAME      "GBALLOC_LL_JEMALLOC_RACE_CHILD"
// a healthy child finishes in well under a second; waiting longer than this means jemalloc deadlocked
#define CHILD_WAIT_MS       15000
// number of fresh child processes the parent spawns to probe the first-allocation race
#define SPAWN_COUNT         64

typedef struct RACE_CONTEXT_TAG
{
    volatile_atomic int32_t* ready_count;
    volatile_atomic int32_t* start_gate;
    int result;
} RACE_CONTEXT;

static int race_first_allocation(void* arg)
{
    RACE_CONTEXT* context = arg;

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
// deadlock here takes the whole (child) process down. Returns 0 if every thread allocated/freed
// successfully, non-zero if any allocation came back NULL (a corrupted init that did not crash).
static int run_one_first_allocation_race(void)
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

    int overall_result = 0;
    for (uint32_t index = 0; index < RACE_THREAD_COUNT; index++)
    {
        int thread_result;
        (void)ThreadAPI_Join(threads[index], &thread_result);
        if (contexts[index].result != 0)
        {
            overall_result = MU_FAILURE;
        }
    }
    return overall_result;
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
        LogLastError("CreateProcessA(%s) failed", exe_path);
        failed = true;
    }
    else
    {
        DWORD wait_result = WaitForSingleObject(process_info.hProcess, CHILD_WAIT_MS);
        if (wait_result == WAIT_FAILED)
        {
            LogLastError("WaitForSingleObject failed while waiting on the child");
            failed = true;
        }
        else if (wait_result == WAIT_TIMEOUT)
        {
            // the child deadlocked
            if (!TerminateProcess(process_info.hProcess, 1))
            {
                LogLastError("TerminateProcess failed on the deadlocked child");
            }
            if (WaitForSingleObject(process_info.hProcess, INFINITE) == WAIT_FAILED)
            {
                LogLastError("WaitForSingleObject failed after terminating the child");
            }
            failed = true;
        }
        else
        {
            DWORD exit_code = 0;
            if (!GetExitCodeProcess(process_info.hProcess, &exit_code))
            {
                LogLastError("GetExitCodeProcess failed");
            }
            // a non-zero exit means the child crashed or its race saw a NULL allocation
            failed = (exit_code != 0);
        }
        if (!CloseHandle(process_info.hThread))
        {
            LogLastError("CloseHandle on the child thread handle failed");
        }
        if (!CloseHandle(process_info.hProcess))
        {
            LogLastError("CloseHandle on the child process handle failed");
        }
    }
    return failed;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_FUNCTION(gballoc_ll_unprimed_concurrent_first_allocation_is_safe)
{
    if (getenv(CHILD_ENV_NAME) != NULL)
    {
        // CHILD: race the process's first jemalloc allocation across several threads WITHOUT priming,
        // then exit with the race result. A jemalloc init regression crashes or hangs this process, or
        // (if it corrupts without crashing) makes an allocation return NULL and exits non-zero.
        int race_result = run_one_first_allocation_race();
        if (!TerminateProcess(GetCurrentProcess(), (UINT)(race_result == 0 ? 0 : 1)))
        {
            LogLastError("TerminateProcess on the current process failed");
        }
    }
    else
    {
        // PARENT: spawn fresh child processes that each run the unprimed first-allocation race and fail
        // if any of them crashes, hangs, or returns non-zero.
        char exe_path[MAX_PATH];
        DWORD path_len = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));
        ASSERT_IS_TRUE(path_len > 0 && path_len < sizeof(exe_path));

        // children inherit this; the parent already passed its own child check above
        ASSERT_IS_TRUE(SetEnvironmentVariableA(CHILD_ENV_NAME, "1"));

        for (uint32_t attempt = 1; attempt <= SPAWN_COUNT; attempt++)
        {
            if (child_failed(exe_path))
            {
                ASSERT_FAIL("unprimed first-allocation race child #%" PRIu32 " of %d crashed, hung, or returned non-zero - jemalloc concurrent init may have regressed", attempt, SPAWN_COUNT);
            }
        }

        LogInfo("all %d unprimed first-allocation race children completed cleanly", SPAWN_COUNT);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

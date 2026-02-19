// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>
#include <inttypes.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/job_object_helper.h"
#include "c_pal/timer.h"


#define MAX_REPEATED_CALLS 10

/* CPU burn infrastructure for multi-threaded saturation tests.
   Single-threaded CPU burn is useless on multi-core machines - a single
   thread only uses 1/N of total capacity and never triggers the cap.
   We must saturate all cores to observe the JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP. */
static volatile LONG cpu_burn_active = 0;

static DWORD WINAPI cpu_burn_thread_func(LPVOID param)
{
    (void)param;
    while (InterlockedCompareExchange(&cpu_burn_active, 1, 1) == 1)
    {
        /* Busy spin to consume CPU */
        volatile int x = 0;
        for (int i = 0; i < 100000; i++)
        {
            x += i;
        }
    }
    return 0;
}

/* Helper: saturate all CPU cores for burn_duration_ms milliseconds and return
   the measured CPU usage as a percentage of total available capacity. */
static double measure_cpu_percent_under_saturation(DWORD burn_duration_ms)
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    DWORD num_cpus = sys_info.dwNumberOfProcessors;

    InterlockedExchange(&cpu_burn_active, 1);

    HANDLE* threads = (HANDLE*)malloc(num_cpus * sizeof(HANDLE));
    ASSERT_IS_NOT_NULL(threads);

    for (DWORD i = 0; i < num_cpus; i++)
    {
        threads[i] = CreateThread(NULL, 0, cpu_burn_thread_func, NULL, 0, NULL);
        ASSERT_IS_NOT_NULL(threads[i]);
    }

    /* Record process CPU time at start */
    FILETIME creation, exit_time, kernel_start, user_start;
    BOOL times_ok = GetProcessTimes(GetCurrentProcess(), &creation, &exit_time, &kernel_start, &user_start);
    ASSERT_IS_TRUE(times_ok, "GetProcessTimes failed at start");

    double start_ms = timer_global_get_elapsed_ms();

    Sleep(burn_duration_ms);

    double end_ms = timer_global_get_elapsed_ms();

    /* Record process CPU time at end */
    FILETIME kernel_end, user_end;
    times_ok = GetProcessTimes(GetCurrentProcess(), &creation, &exit_time, &kernel_end, &user_end);
    ASSERT_IS_TRUE(times_ok, "GetProcessTimes failed at end");

    /* Stop burn threads */
    InterlockedExchange(&cpu_burn_active, 0);
    (void)WaitForMultipleObjects(num_cpus, threads, TRUE, 10000);
    for (DWORD i = 0; i < num_cpus; i++)
    {
        (void)CloseHandle(threads[i]);
    }
    free(threads);

    /* Calculate actual CPU usage as percentage of total available */
    ULARGE_INTEGER k_start, k_end, u_start, u_end;
    k_start.LowPart = kernel_start.dwLowDateTime;
    k_start.HighPart = kernel_start.dwHighDateTime;
    k_end.LowPart = kernel_end.dwLowDateTime;
    k_end.HighPart = kernel_end.dwHighDateTime;
    u_start.LowPart = user_start.dwLowDateTime;
    u_start.HighPart = user_start.dwHighDateTime;
    u_end.LowPart = user_end.dwLowDateTime;
    u_end.HighPart = user_end.dwHighDateTime;

    uint64_t cpu_time_100ns = (k_end.QuadPart - k_start.QuadPart) + (u_end.QuadPart - u_start.QuadPart);
    double time_ms = end_ms - start_ms;
    uint64_t time_100ns = (uint64_t)(time_ms * 10000.0);
    uint64_t total_possible_100ns = time_100ns * num_cpus;

    double cpu_percent = (double)cpu_time_100ns / (double)total_possible_100ns * 100.0;

    LogInfo("CPU burn results: cpu_time=%" PRIu64 " (100ns ticks), time=%.0f ms, num_cpus=%lu, cpu_percent=%.1f%%",
        cpu_time_100ns, time_ms, (unsigned long)num_cpus, cpu_percent);

    return cpu_percent;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(init)
{
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    job_object_helper_deinit_for_test();
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_002: [ If the process-level singleton job object has already been created with the same percent_cpu and percent_physical_memory values, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_004: [ On success, job_object_helper_set_job_limits_to_current_process shall store the THANDLE(JOB_OBJECT_HELPER) and the percent_cpu and percent_physical_memory values in static variables for the singleton pattern. ]*/
TEST_FUNCTION(test_job_object_helper_repeated_calls_return_same_singleton_and_no_cpu_compounding)
{
    /* This test calls job_object_helper_set_job_limits_to_current_process
       2, 5, and 9 times and verifies after each checkpoint:
       (a) Every returned THANDLE points to the same underlying object.
       (b) The CPU cap is still ~50%, not compounded (50%^2=25%, 50%^5~3%, 50%^9~0.2%). */

    ///arrange
    THANDLE(JOB_OBJECT_HELPER) handles[MAX_REPEATED_CALLS] = { NULL };

    /* checkpoints: after 2, 5, and 9 calls */
    static const int checkpoints[] = { 2, 5, 9 };
    static const int num_checkpoints = sizeof(checkpoints) / sizeof(checkpoints[0]);
    int calls_so_far = 0;

    for (int cp = 0; cp < num_checkpoints; cp++)
    {
        int target = checkpoints[cp];

        /* Issue calls up to the current checkpoint */
        for (int i = calls_so_far; i < target; i++)
        {
            THANDLE(JOB_OBJECT_HELPER) temp = job_object_helper_set_job_limits_to_current_process("", 50, 1);
            ASSERT_IS_NOT_NULL(temp, "Call %d should succeed", i + 1);
            THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&handles[i], temp);
            THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&temp, NULL);
        }
        calls_so_far = target;

        /* Verify all returned handles point to the same underlying object */
        for (int i = 1; i < target; i++)
        {
            ASSERT_ARE_EQUAL(void_ptr, (const void*)handles[0], (const void*)handles[i],
                "After %d calls, call %d should return the same singleton", target, i + 1);
        }
        LogInfo("After %d calls: singleton verified (all handles are the same pointer)", target);

        ///act
        /* Saturate CPU and measure - cap should still be ~50%, not compounded */
        double cpu_percent = measure_cpu_percent_under_saturation(3000);

        ///assert
        /* CPU should be around 50% (the cap), NOT compounded down.
           If compounding occurred: 50%^2=25%, 50%^5~3.1%, 50%^9~0.2% - all far below 30%.
           Use 30% as lower bound and 70% as upper bound. */
        ASSERT_IS_TRUE(cpu_percent > 30.0,
            "CPU usage %.1f%% is too low after %d calls - possible compounding detected (expected ~50%%)", cpu_percent, target);
        ASSERT_IS_TRUE(cpu_percent < 70.0,
            "CPU usage %.1f%% is too high after %d calls - cap may not be enforced (expected ~50%%)", cpu_percent, target);

        LogInfo("PASS: CPU usage %.1f%% after %d calls is within expected range [30%%, 70%%] (no compounding)", cpu_percent, target);
    }

    ///cleanup
    for (int i = 0; i < MAX_REPEATED_CALLS; i++)
    {
        THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&handles[i], NULL);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

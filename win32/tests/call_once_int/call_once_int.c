// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "windows.h"

#include "azure_macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "azure_c_pal/timer.h"

#include "azure_c_pal/call_once.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

static volatile_atomic int32_t g_stateSleep = CALL_ONCE_NOT_CALLED;
static volatile_atomic int32_t threadThatSleeps_executions = 0;

static DWORD WINAPI sleepThread(
    _In_ LPVOID lpParameter
)
{
    (void)lpParameter;
    while (call_once_begin(&g_stateSleep) == CALL_ONCE_PROCEED)
    {
        (void)interlocked_increment(&threadThatSleeps_executions);
        Sleep(4000);
        call_once_end(&g_stateSleep, true);
    }
    return 0;
}

#define N_THREADS_FOR_CHAOS 16
#define N_AT_LEAST_TIME_MS 2000 /*ms*/
static volatile_atomic int32_t n_threads_that_failed = 0;
static double startTime;
static volatile_atomic int32_t chaosThread_executions = 0;
static volatile_atomic int32_t g_stateChaos = CALL_ONCE_NOT_CALLED;

static DWORD WINAPI chaosThread(
    _In_ LPVOID lpParameter
)
{
    (void)lpParameter;
    bool shouldIncrementThreadsThatFailed = true;
    while (call_once_begin(&g_stateChaos) != CALL_ONCE_ALREADY_CALLED)
    {
        if (
            (timer_global_get_elapsed_ms() - startTime <= N_AT_LEAST_TIME_MS) ||
            (interlocked_add(&n_threads_that_failed, 0) != N_THREADS_FOR_CHAOS)
            )
        {
            /*continue to fail*/
            call_once_end(&g_stateChaos, false);

            if (shouldIncrementThreadsThatFailed)
            {
                (void)interlocked_increment(&n_threads_that_failed);
                shouldIncrementThreadsThatFailed = false;
            }
        }
        else
        {
            /*now that enough time has passed... and at least every thread has failed at least once... end of the threads*/
            call_once_end(&g_stateChaos, true);
            (void)interlocked_increment(&chaosThread_executions);
        }
    }
    return 0;
}

BEGIN_TEST_SUITE(call_once_inttests)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/*Tests_SRS_CALL_ONCE_02_003: [ If interlocked_compare_exchange returns 1 then call_once_begin shall call InterlockedHL_WaitForNotValue(state, 1, INFINITE) and call again interlocked_compare_exchange(state, 1, 0). ]*/
TEST_FUNCTION(call_once_will_wake_a_waiting_thread)
{
    /*this test spawns 2 threads*/
    /*both threads are trying to compete over the same call_once_sequence*/
    /*the sequence has a sleep(4000); */
    /*it is assumed at this time that the first thread will get to its sleep*/
    /*the second thread will be stucked in a "waitfornotvalue"*/

    /*the first thread will exit succesfully, thus unblocking the second thread*/

    ///arrange
    HANDLE threads[2];

    ///act
    threads[0] = CreateThread(NULL, 0, sleepThread, NULL, 0, NULL);
    ASSERT_IS_NOT_NULL(threads[0]);

    threads[1] = CreateThread(NULL, 0, sleepThread, NULL, 0, NULL);
    ASSERT_IS_NOT_NULL(threads[1]);

    DWORD dw = WaitForMultipleObjects(2, threads, TRUE, INFINITE);
    ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + 2 - 1));

    ///assert
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&threadThatSleeps_executions, 0));

    (void)CloseHandle(threads[0]);
    (void)CloseHandle(threads[1]);
}

TEST_FUNCTION(call_once_chaos_knight)
{
    /*this test spawns 10 threads*/
    /*all threads are trying to init. for the first "n" seconds all threads will fail to initialize. Then, when all threads have tried at least once (and failed), one thread will succeed
    and all threads will exit*/

    ///arrange
    size_t i;
    HANDLE threads[N_THREADS_FOR_CHAOS];
    startTime = timer_global_get_elapsed_ms();

    for (i = 0; i < N_THREADS_FOR_CHAOS; i++)
    {
        threads[i] = CreateThread(NULL, 0, chaosThread, NULL, 0, NULL);
        ASSERT_IS_NOT_NULL(threads[0]);
    }

    DWORD dw = WaitForMultipleObjects(N_THREADS_FOR_CHAOS, threads, TRUE, INFINITE);
    ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + N_THREADS_FOR_CHAOS - 1));

    ///assert
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&chaosThread_executions, 0));

    for (i = 0; i < N_THREADS_FOR_CHAOS; i++)
    {
        (void)CloseHandle(threads[i]);
    }
}


END_TEST_SUITE(call_once_inttests)

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
#include <ctime>
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#endif

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/threadapi.h"

#include "c_pal/call_once.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

static call_once_t g_stateSleep = CALL_ONCE_NOT_CALLED;
static call_once_t threadThatSleeps_executions = 0;

static int sleepThread(
    void* lpParameter
)
{
    (void)lpParameter;
    while (call_once_begin(&g_stateSleep) == CALL_ONCE_PROCEED)
    {
        (void)interlocked_increment(&threadThatSleeps_executions);
        ThreadAPI_Sleep(4000);
        call_once_end(&g_stateSleep, true);
    }
    return 0;
}

#define N_THREADS_FOR_CHAOS 16
#define N_AT_LEAST_TIME_MS 3000 /*ms*/
static call_once_t n_threads_that_failed = 0;
static time_t startTime;
static call_once_t chaosThread_executions = 0;
static call_once_t g_stateChaos = CALL_ONCE_NOT_CALLED;

static int chaosThread(
    void* lpParameter
)
{
    (void)lpParameter;
    bool shouldIncrementThreadsThatFailed = true;
    while (call_once_begin(&g_stateChaos) != CALL_ONCE_ALREADY_CALLED)
    {
        double elapsed = difftime(time(NULL), startTime);
        if (
            (elapsed * 1000 <= N_AT_LEAST_TIME_MS) ||
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

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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

/*Tests_SRS_CALL_ONCE_02_003: [ If interlocked_compare_exchange returns 1 then call_once_begin shall call wait_on_address(state) with timeout UINT32_MAX and call again interlocked_compare_exchange(state, 1, 0). ]*/
TEST_FUNCTION(call_once_will_wake_a_waiting_thread)
{
    /*this test spawns 2 threads*/
    /*both threads are trying to compete over the same call_once_sequence*/
    /*the sequence has a sleep(4000); */
    /*it is assumed at this time that the first thread will get to its sleep*/
    /*the second thread will be stucked in a "waitfornotvalue"*/

    /*the first thread will exit succesfully, thus unblocking the second thread*/

    ///arrange
    THREAD_HANDLE threads[2];

    ///act
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[0], sleepThread, NULL));
    ASSERT_IS_NOT_NULL(threads[0]);

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[1], sleepThread, NULL));
    ASSERT_IS_NOT_NULL(threads[1]);

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[0], NULL));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[1], NULL));

    ///assert
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&threadThatSleeps_executions, 0));
}

TEST_FUNCTION(call_once_chaos_knight)
{
    /*this test spawns 10 threads*/
    /*all threads are trying to init. for the first "n" seconds all threads will fail to initialize. Then, when all threads have tried at least once (and failed), one thread will succeed
    and all threads will exit*/

    ///arrange
    size_t i;
    THREAD_HANDLE threads[N_THREADS_FOR_CHAOS];
    startTime = time(NULL);

    for (i = 0; i < N_THREADS_FOR_CHAOS; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[i], chaosThread, NULL));
        ASSERT_IS_NOT_NULL(threads[i]);
    }

    for (i = 0; i < N_THREADS_FOR_CHAOS; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[i], NULL));
    }

    ///assert
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&chaosThread_executions, 0));

}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright (c) Microsoft. All rights reserved.

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include "c_logging/xlogging.h"

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h" // IWYU pragma: keep

#include "c_pal/gballoc_hl.h"
#include "c_pal/threadpool.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"
#include "c_pal/execution_engine.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    time_t seed = time(NULL);
    LogInfo("Test using random seed = %u", (unsigned int)seed);
    srand((unsigned int)seed);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);

    gballoc_hl_deinit();
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

static void threadpool_task_wait_20_millisec(void* parameter)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)parameter;
    ThreadAPI_Sleep(20);
    (void)interlocked_increment(thread_counter);
}

static void threadpool_task_wait_random(void* parameter)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)parameter;

    // Sleep anywhere between 0 and 750 milliseconds
    unsigned int sleepy_time = rand() % 750;
    ThreadAPI_Sleep(sleepy_time);

    (void)interlocked_increment(thread_counter);
}

TEST_FUNCTION(one_work_item_schedule_works)
{
    // assert
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    volatile_atomic int32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    while (interlocked_add(&thread_counter, 0) != 1);

    // cleanup
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(scheduling_20_work_items)
{
    // assert
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    uint32_t num_threads = 20;
    volatile_atomic int32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item", num_threads);
    for (size_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));
    }

    // assert
    while (interlocked_add(&thread_counter, 0) != num_threads);

    // cleanup
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_chaos_knight)
{
    // assert
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    uint32_t num_threads = 100;
    volatile_atomic int32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item", num_threads);
    for (size_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_random, (void*)&thread_counter));
        // Sleep between 0 and 150 milliseconds
        unsigned int sleepy_time = rand() % 150;
        ThreadAPI_Sleep(sleepy_time);
    }

    // assert
    while (interlocked_add(&thread_counter, 0) != num_threads);

    // cleanup
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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
#include "c_pal/sync.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

static void on_threadpool_open_complete(void* context, THREADPOOL_OPEN_RESULT open_result)
{
    (void)context;
    (void)open_result;
}

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
    wake_by_address_single(thread_counter);
}

static void threadpool_task_wait_60_millisec(void* parameter)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)parameter;
    ThreadAPI_Sleep(60);
    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static void threadpool_task_wait_random(void* parameter)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)parameter;

    // Sleep anywhere between 0 and 250 milliseconds
    unsigned int sleepy_time = rand() % 250;
    ThreadAPI_Sleep(sleepy_time);

    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

TEST_FUNCTION(one_work_item_schedule_works)
{
    // assert
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    volatile_atomic int32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_threadpool_open_complete, NULL));

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    LogInfo("Waiting for task to complete");
    do
    {
        (void)wait_on_address(&thread_counter, 1, UINT32_MAX);
    } while (thread_counter != 1);

    ASSERT_ARE_EQUAL(int32_t, thread_counter, 1, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

#define N_WORK_ITEMS 30

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items))
{
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    uint32_t num_threads = N_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_threadpool_open_complete, NULL));

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item", num_threads);
    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_60_millisec, (void*)&thread_counter));
    }

    // assert
    do
    {
        wait_on_address(&thread_counter, 1, UINT32_MAX);
    } while (thread_counter != num_threads);

    //ThreadAPI_Sleep(120*1000);
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_with_pool_threads))
{
    // assert
    EXECUTION_ENGINE_PARAMETERS_LINUX params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    uint32_t num_threads = N_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_threadpool_open_complete, NULL));

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item with 20 millisecons work", num_threads);
    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));
    }

    do
    {
        wait_on_address(&thread_counter, 1, UINT32_MAX);
    } while (thread_counter != num_threads);

    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

#define N_CHAOS_WORK_ITEMS 50

TEST_FUNCTION(threadpool_chaos_knight)
{
    // assert
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    uint32_t num_threads = N_CHAOS_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_threadpool_open_complete, NULL));

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work items", num_threads);
    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_random, (void*)&thread_counter));
        // Sleep between 0 and 250 milliseconds
        unsigned int sleepy_time = rand() % 250;
        ThreadAPI_Sleep(sleepy_time);
    }

    // assert
    do
    {
        wait_on_address(&thread_counter, 1, UINT32_MAX);
    } while (thread_counter != num_threads);
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

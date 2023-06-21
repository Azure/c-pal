// Copyright (c) Microsoft. All rights reserved.

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "c_logging/logger.h"

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h" // IWYU pragma: keep

#include "c_pal/timer.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/threadpool.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"
#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"

typedef struct WAIT_WORK_CONTEXT_TAG
{
    volatile_atomic int32_t call_count;
    volatile_atomic int32_t wait_event;
} WAIT_WORK_CONTEXT;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    time_t seed = time(NULL);
    LogInfo("Test using random seed = %u", (unsigned int)seed);
    srand((unsigned int)seed);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
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

static void work_function(void* context)
{
    volatile_atomic int32_t* call_count = (volatile_atomic int32_t*)context;
    (void)interlocked_increment(call_count);
    wake_by_address_single(call_count);
}

static void wait_work_function(void* context)
{
    WAIT_WORK_CONTEXT* wait_work_context = (WAIT_WORK_CONTEXT*)context;

    int32_t current_value = interlocked_add(&wait_work_context->wait_event, 0);
    ASSERT_IS_TRUE(wait_on_address(&wait_work_context->wait_event, current_value, 2000) == WAIT_ON_ADDRESS_TIMEOUT);
    (void)interlocked_increment(&wait_work_context->call_count);
    wake_by_address_single(&wait_work_context->call_count);
}

static void wait_for_equal(volatile_atomic int32_t* value, int32_t expected, uint32_t timeout)
{
    double start_time = timer_global_get_elapsed_ms();
    double current_time = timer_global_get_elapsed_ms();
    do
    {
        if (current_time - start_time >= timeout)
        {
            ASSERT_FAIL("Timeout waiting for value");
        }

        int32_t current_value = interlocked_add(value, 0);
        if (current_value == expected)
        {
            break;
        }
        (void)wait_on_address(value, current_value, timeout - (uint32_t)(current_time - start_time));
    } while (1);
}

TEST_FUNCTION(one_work_item_schedule_works)
{
    // assert
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

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
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define N_WORK_ITEMS 30

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items))
{
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    uint32_t num_threads = N_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

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
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_with_pool_threads))
{
    // assert
    //EXECUTION_ENGINE_PARAMETERS_LINUX params = {0};
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    uint32_t num_threads = N_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

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
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define N_CHAOS_WORK_ITEMS 50

TEST_FUNCTION(threadpool_chaos_knight)
{
    // assert
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    uint32_t num_threads = N_CHAOS_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

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
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_start_timer_works_runs_once)
{
    // assert
    // create an execution engine
    volatile_atomic int32_t call_count;
    EXECUTION_ENGINE_PARAMETERS_LINUX params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // NOTE: this test runs with retries because there are possible timing issues with thread scheduling
    // We are making sure the worker doesn't start before the delay time and does run once after the delay time
    // First check could fail in theory

    bool need_to_retry = true;
    do
    {
        (void)interlocked_exchange(&call_count, 0);

        LogInfo("Starting timer");

        // act (start a timer to start delayed and then execute once)
        TIMER_INSTANCE_HANDLE timer;
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 2000, 0, work_function, (void*)&call_count, &timer));

        // assert

        // Timer starts after 2 seconds, wait a bit and it should not yet have run
        ThreadAPI_Sleep(500);
        if (interlocked_add(&call_count, 0) != 0)
        {
            LogWarning("Timer ran after sleeping 500ms, we just got unlucky, try test again");
        }
        else
        {
            LogInfo("Waiting for timer to execute after short delay of no execution");

            // Should eventually run once (wait up to 2.5 seconds, but it should run in 1.5 seconds)
            wait_for_equal(&call_count, 1, 5000);
            LogInfo("Timer completed, make sure it doesn't run again");

            // And should not run again
            ThreadAPI_Sleep(5000);
            ASSERT_ARE_EQUAL(uint32_t, 1, interlocked_add(&call_count, 0));
            LogInfo("Done waiting for timer");

            need_to_retry = false;
        }

        threadpool_timer_destroy(timer);
    } while (need_to_retry);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(restart_timer_works_runs_once)
{
    // assert
    // create an execution engine
    volatile_atomic int32_t call_count;
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // NOTE: this test runs with retries because there are possible timing issues with thread scheduling
    // We are making sure the worker doesn't start before the delay time and does run once after the delay time
    // First check could fail in theory

    bool need_to_retry = true;
    do
    {
        (void)interlocked_exchange(&call_count, 0);

        LogInfo("Starting timer");

        // start a timer to start delayed after 4 seconds (which would fail test)
        TIMER_INSTANCE_HANDLE timer;
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 4000, 0, work_function, (void*)&call_count, &timer));

        // act (restart timer to start delayed instead after 2 seconds)
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(timer, 2000, 0));

        // assert

        // Timer starts after 2 seconds, wait a bit and it should not yet have run
        ThreadAPI_Sleep(500);
        if (interlocked_add(&call_count, 0) != 0)
        {
            LogWarning("Timer ran after sleeping 500ms, we just got unlucky, try test again");
        }
        else
        {
            LogInfo("Waiting for timer to execute after short delay of no execution");

            // Should eventually run once (wait up to 2.5 seconds, but it should run in 1.5 seconds)
            wait_for_equal(&call_count, 1, 2000);
            LogInfo("Timer completed, make sure it doesn't run again");

            // And should not run again
            ThreadAPI_Sleep(5000);
            ASSERT_ARE_EQUAL(uint32_t, 1, interlocked_add(&call_count, 0));
            LogInfo("Done waiting for timer");

            need_to_retry = false;
        }

        threadpool_timer_destroy(timer);
    } while (need_to_retry);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_start_timer_works_runs_periodically)
{
    // assert
    // create an execution engine
    volatile_atomic int32_t call_count;
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange(&call_count, 0);

    // act (start a timer to start delayed and then execute every 500ms)
    LogInfo("Starting timer");
    TIMER_INSTANCE_HANDLE timer;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&call_count, &timer));

    // assert

    // Timer should run 4 times in about 2.1 seconds
    wait_for_equal(&call_count, 4, 3000);
    LogInfo("Timer completed 4 times");

    // cleanup
    threadpool_timer_destroy(timer);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(timer_cancel_restart_works_runs_periodically)
{
    // assert
    // create an execution engine
    volatile_atomic int32_t call_count;
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange(&call_count, 0);

    // start a timer to start delayed and then execute every 500ms
    LogInfo("Starting timer");
    TIMER_INSTANCE_HANDLE timer;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&call_count, &timer));

    // Timer should run 4 times in about 2.1 seconds
    wait_for_equal(&call_count, 4, 3000);
    LogInfo("Timer completed 4 times");

    // act
    LogInfo("Cancel then restart timer");
    threadpool_timer_cancel(timer);
    (void)interlocked_exchange(&call_count, 0);
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(timer, 100, 1000));

    // assert

    // Timer should run 2 more times in about 2.1 seconds
    wait_for_equal(&call_count, 2, 3000);
    LogInfo("Timer completed 2 more times");

    // cleanup
    threadpool_timer_destroy(timer);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(stop_timer_waits_for_ongoing_execution)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    volatile_atomic int32_t call_count;
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange(&wait_work_context.call_count, 0);
    (void)interlocked_exchange(&wait_work_context.wait_event, 0);

    // schedule one timer that waits
    LogInfo("Starting timer");
    TIMER_INSTANCE_HANDLE timer;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 0, 5000, wait_work_function, (void*)&wait_work_context, &timer));

    // act
    ThreadAPI_Sleep(500);

    // call stop
    LogInfo("Timer should be running and waiting, now stop timer");
    threadpool_timer_destroy(timer);

    LogInfo("Timer stopped");

    // set the event, that would trigger a WAIT_OBJECT_0 if stop would not wait for all items
    interlocked_increment(&wait_work_context.wait_event);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(cancel_timer_waits_for_ongoing_execution)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    volatile_atomic int32_t call_count;
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange(&wait_work_context.call_count, 0);
    (void)interlocked_exchange(&wait_work_context.wait_event, 0);

    // schedule one timer that waits
    LogInfo("Starting timer");
    TIMER_INSTANCE_HANDLE timer;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 0, 5000, wait_work_function, (void*)&wait_work_context, &timer));

    // act
    ThreadAPI_Sleep(500);

    // call cancel
    LogInfo("Timer should be running and waiting, now cancel timer");
    threadpool_timer_cancel(timer);

    LogInfo("Timer canceled");

    // set the event, that would trigger a WAIT_OBJECT_0 if stop would not wait for all items
    interlocked_increment(&wait_work_context.wait_event);

    // cleanup
    threadpool_timer_destroy(timer);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

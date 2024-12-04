// Copyright (c) Microsoft. All rights reserved.

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

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
#include "c_pal/interlocked_hl.h"
#include "c_pal/execution_engine.h"
#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"

#define XTEST_FUNCTION(A) void A(void)

static volatile_atomic int64_t g_call_count;

//diff: LONG to int64_t, wait_event need to change type
// typedef struct WAIT_WORK_CONTEXT_TAG
// {
//     volatile LONG call_count;
//     HANDLE wait_event; ? should this be changed?
// } WAIT_WORK_CONTEXT;

typedef struct WAIT_WORK_CONTEXT_TAG
{
    volatile_atomic int64_t call_count;
    volatile_atomic int32_t wait_event;
} WAIT_WORK_CONTEXT;

typedef struct WRAP_DATA_TAG
{
    volatile_atomic int32_t* counter;
    char mem[10];
} WRAP_DATA;

#define TEST_TIMEOUT_VALUE      60000   // 60 seconds
//diff: added this
#define WAIT_WORK_FUNCTION_SLEEP_IN_MS 300

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    time_t seed = time(NULL);
    LogInfo("Test using random seed = %u", (unsigned int)seed);
    srand((unsigned int)seed);
    //diff: if this is needed
    //logger_set_config((LOGGER_CONFIG) { .log_sinks = NULL, .log_sink_count = 0 });
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    (void)interlocked_exchange_64(&g_call_count, 0);
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


static void threadpool_long_task(void* context)
{
    WRAP_DATA* data = context;
    ASSERT_ARE_EQUAL(int, 0, strcmp(data->mem, "READY"));
    strcpy(data->mem, "DONE");
    (void)interlocked_increment(data->counter);
    wake_by_address_single(data->counter);
    free(data);
}

static void threadpool_long_task_v2(void* context)
{
    WRAP_DATA* data = context;
    ASSERT_ARE_EQUAL(int, 0, strcmp(data->mem, "READY"));
    ThreadAPI_Sleep(1);
    (void)interlocked_increment(data->counter);
    wake_by_address_single(data->counter);
}

//diff: win32, need to make anywhere call work_function to use g_call_count;
// static void work_function(void* context)
// {
//     volatile LONG64* call_count = (volatile LONG64*)context;
//     (void)InterlockedIncrement64(call_count);
//     WakeByAddressSingle((PVOID)call_count);
// }

static void work_function(void* context)
{
    (void)interlocked_increment_64(&g_call_count);
    wake_by_address_single_64(&g_call_count);
}

//diff:wait for object need to be changed, should wait for infinite time?
// static void wait_work_function(void* context)
// {
//     WAIT_WORK_CONTEXT* wait_work_context = (WAIT_WORK_CONTEXT*)context;
//     ASSERT_IS_TRUE(WaitForSingleObject(wait_work_context->wait_event, UINT_MAX) == WAIT_OBJECT_0);
//     (void)InterlockedIncrement(&wait_work_context->call_count);
//     WakeByAddressSingle((PVOID)&wait_work_context->call_count);
// }

static void wait_work_function(void* context)
{
    WAIT_WORK_CONTEXT* wait_work_context = (WAIT_WORK_CONTEXT*)context;

    int32_t current_value = interlocked_add(&wait_work_context->wait_event, 0);
    ASSERT_IS_TRUE(wait_on_address(&wait_work_context->wait_event, current_value, 2000) == WAIT_ON_ADDRESS_TIMEOUT); //todo: should this line be changed?
    (void)interlocked_increment_64(&wait_work_context->call_count);
    wake_by_address_single_64(&wait_work_context->call_count);
}

#define TIMER_STATE_VALUES \
    TIMER_STATE_NONE, \
    TIMER_STATE_STARTING, \
    TIMER_STATE_STARTED, \
    TIMER_STATE_CANCELING, \
    TIMER_STATE_STOPPING

#define TEST_ACTION_VALUES \
    TEST_ACTION_CLEANUP_TIMER, \
    TEST_ACTION_SCHEDULE_WORK, \
    TEST_ACTION_START_TIMER, \
    TEST_ACTION_CANCEL_TIMER, \
    TEST_ACTION_RESTART_TIMER, \
    TEST_ACTION_SCHEDULE_WORK_ITEM

MU_DEFINE_ENUM(TEST_ACTION, TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS(TEST_ACTION, TEST_ACTION_VALUES)

MU_DEFINE_ENUM(TIMER_STATE, TIMER_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(TIMER_STATE, TIMER_STATE_VALUES)

static void wait_for_greater_or_equal(volatile_atomic int64_t* value, int64_t expected, uint64_t timeout)
{
    double start_time = timer_global_get_elapsed_ms();
    double current_time = timer_global_get_elapsed_ms();
    do
    {
        if (current_time - start_time >= timeout)
        {
            ASSERT_FAIL("Timeout waiting for value");
        }

        int64_t current_value = interlocked_add_64(value, 0);
        if (current_value >= expected)
        {
            break;
        }
        (void)wait_on_address_64(value, current_value, timeout - (uint64_t)(current_time - start_time));
    } while (1);
}

static void wait_for_equal(volatile_atomic int64_t* value, int64_t expected, uint64_t timeout)
{
    double start_time = timer_global_get_elapsed_ms();
    double current_time = timer_global_get_elapsed_ms();
    do
    {
        if (current_time - start_time >= timeout)
        {
            ASSERT_FAIL("Timeout waiting for value");
        }

        int64_t current_value = interlocked_add_64(value, 0);
        if (current_value == expected)
        {
            break;
        }
        (void)wait_on_address_64(value, current_value, timeout - (uint64_t)(current_time - start_time));
    } while (1);
}

TEST_FUNCTION(one_work_item_schedule_works)
{
    // assert
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = 1;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    LogInfo("Waiting for task to complete");
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_work_item_schedule_work_item)
{
    // assert
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = 1;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    LogInfo("Creating work item");
    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);
    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));

    // assert
    LogInfo("Waiting for task to complete");
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_destroy_work_item(threadpool, threadpool_work_item);
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_work_item_schedule_works_1)
{
    // assert
    // create an execution engine
    volatile_atomic int64_t call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange_64(&call_count, 0);

    // act (schedule one work item)
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));

    // assert
    wait_for_equal(&call_count, 1, UINT64_MAX);
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}
#if 0
TEST_FUNCTION(threadpool_owns_execution_engine_reference_and_can_schedule_work)
{
    // arrange
    // create an execution engine
    volatile_atomic int64_t call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // this is safe because the threadpool has a reference
    execution_engine_dec_ref(execution_engine);

    (void)interlocked_exchange_64(&call_count, 0);

    // act (schedule one work item)
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));

    // assert
    wait_for_equal(&call_count, 1, UINT64_MAX);
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}
#endif
#define N_WORK_ITEMS 30

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items))
{
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = N_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    // Create Work Items
    LogInfo("Creating work item once");
    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_60_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item", num_threads);
    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
    }

    LogInfo("Scheduled threads waiting for threads to complete");

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_destroy_work_item(threadpool, threadpool_work_item);
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work))
{
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = N_WORK_ITEMS;
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

    LogInfo("Scheduled threads waiting for threads to complete");

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_with_pool_threads))
{
    // assert
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = N_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    // Create Work Items
    LogInfo("Creating work item once");
    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item with 20 millisecons work", num_threads);
    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
    }

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_destroy_work_item(threadpool, threadpool_work_item);
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_with_pool_threads))
{
    // assert
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = N_WORK_ITEMS;
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

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
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
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = N_CHAOS_WORK_ITEMS;
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
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_chaos_knight_v2)
{
    // assert
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = N_CHAOS_WORK_ITEMS;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    // Create Work Items
    LogInfo("Creating work item once");
    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_random, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work items", num_threads);
    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
        // Sleep between 0 and 250 milliseconds
        unsigned int sleepy_time = rand() % 250;
        ThreadAPI_Sleep(sleepy_time);
    }

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_destroy_work_item(threadpool, threadpool_work_item);
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define WRAP_TEST_WORK_ITEMS 10000

TEST_FUNCTION(threadpool_force_wrap_around)
{
    // arrange
    const uint32_t num_threads = WRAP_TEST_WORK_ITEMS;

    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = num_threads;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    volatile_atomic int32_t thread_counter;
    interlocked_exchange(&thread_counter, 0);

    for (uint32_t index = 0; index < num_threads; index++)
    {
        WRAP_DATA* data = malloc(sizeof(WRAP_DATA));
        data->counter = &thread_counter;
        strcpy(data->mem, "READY");
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_long_task, data));
    }

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_force_wrap_around_v2)
{
    // arrange
    const uint32_t num_threads = WRAP_TEST_WORK_ITEMS;
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = num_threads;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    volatile_atomic int32_t thread_counter;
    interlocked_exchange(&thread_counter, 0);

    WRAP_DATA* data = malloc(sizeof(WRAP_DATA));
    data->counter = &thread_counter;
    strcpy(data->mem, "READY");
    // Create Work Items
    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_long_task_v2, data);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
    }

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup

    threadpool_destroy_work_item(threadpool, threadpool_work_item);
    free(data);
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_start_timer_works_runs_once)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS params;
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
        LogInfo("Starting timer");

        // act (start a timer to start delayed and then execute once)
        THANDLE(TIMER) timer = NULL;
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 2000, 0, work_function, NULL, &timer));

        // assert

        // Timer starts after 2 seconds, wait a bit and it should not yet have run
        ThreadAPI_Sleep(500);
        if (interlocked_add_64(&g_call_count, 0) != 0)
        {
            LogWarning("Timer ran after sleeping 500ms, we just got unlucky, try test again");
        }
        else
        {
            LogInfo("Waiting for timer to execute after short delay of no execution");

            // Should eventually run once (wait up to 2.5 seconds, but it should run in 1.5 seconds)
            wait_for_equal(&g_call_count, 1, 5000);
            LogInfo("Timer completed, make sure it doesn't run again");

            // And should not run again
            ThreadAPI_Sleep(5000);
            ASSERT_ARE_EQUAL(uint64_t, 1, interlocked_add_64(&g_call_count, 0));
            LogInfo("Done waiting for timer");

            need_to_retry = false;
        }

        THANDLE_ASSIGN(TIMER)(&timer, NULL);
    } while (need_to_retry);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(restart_timer_works_runs_once)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS params;
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
        LogInfo("Starting timer");

        // start a timer to start delayed after 4 seconds (which would fail test)
        THANDLE(TIMER) timer = NULL;
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 4000, 0, work_function, (void*)&g_call_count, &timer));

        // act (restart timer to start delayed instead after 2 seconds)
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(timer, 2000, 0));

        // assert

        // Timer starts after 2 seconds, wait a bit and it should not yet have run
        ThreadAPI_Sleep(500);
        if (interlocked_add_64(&g_call_count, 0) != 0)
        {
            LogWarning("Timer ran after sleeping 500ms, we just got unlucky, try test again");
        }
        else
        {
            LogInfo("Waiting for timer to execute after short delay of no execution");

            // Should eventually run once (wait up to 2.5 seconds, but it should run in 1.5 seconds)
            wait_for_equal(&g_call_count, 1, 2000);
            LogInfo("Timer completed, make sure it doesn't run again");

            // And should not run again
            ThreadAPI_Sleep(5000);
            ASSERT_ARE_EQUAL(uint32_t, 1, interlocked_add_64(&g_call_count, 0));
            LogInfo("Done waiting for timer");

            need_to_retry = false;
        }

        THANDLE_ASSIGN(TIMER)(&timer, NULL);
    } while (need_to_retry);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_start_timer_works_runs_periodically)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // act (start a timer to start delayed and then execute every 500ms)
    LogInfo("Starting timer");
    THANDLE(TIMER) timer = NULL;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&g_call_count, &timer));

    // assert

    // Timer should run 4 times in about 2.1 seconds
    wait_for_equal(&g_call_count, 4, 3000);
    LogInfo("Timer completed 4 times");

    // cleanup
    THANDLE_ASSIGN(TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(timer_cancel_restart_works_runs_periodically)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // start a timer to start delayed and then execute every 500ms
    LogInfo("Starting timer");
    THANDLE(TIMER) timer = NULL;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&g_call_count, &timer));

    // Timer should run 4 times in about 2.1 seconds
    wait_for_equal(&g_call_count, 4, 3000);
    LogInfo("Timer completed 4 times");

    // act
    LogInfo("Cancel then restart timer");
    threadpool_timer_cancel(timer);
    (void)interlocked_exchange_64(&g_call_count, 0);
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(timer, 100, 1000));

    // assert

    // Timer should run 2 more times in about 2.1 seconds
    wait_for_equal(&g_call_count, 2, 3000);
    LogInfo("Timer completed 2 more times");

    // cleanup
    THANDLE_ASSIGN(TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(stop_timer_waits_for_ongoing_execution)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange_64(&wait_work_context.call_count, 0);
    (void)interlocked_exchange(&wait_work_context.wait_event, 0);

    // schedule one timer that waits
    LogInfo("Starting timer");
    THANDLE(TIMER) timer = NULL;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 0, 5000, wait_work_function, (void*)&wait_work_context, &timer));

    // act
    ThreadAPI_Sleep(500);

    // call stop
    LogInfo("Timer should be running and waiting, now stop timer");
    THANDLE_ASSIGN(TIMER)(&timer, NULL);

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
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange_64(&wait_work_context.call_count, 0);
    (void)interlocked_exchange(&wait_work_context.wait_event, 0);

    // schedule one timer that waits
    LogInfo("Starting timer");
    THANDLE(TIMER) timer = NULL;
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
    THANDLE_ASSIGN(TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(schedule_after_close_works)
{
    // assert
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = 1;
    volatile_atomic int32_t thread_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));

    // assert
    LogInfo("Waiting for task to complete");
    do
    {
        (void)InterlockedHL_WaitForValue(&thread_counter, num_threads, TEST_TIMEOUT_VALUE);
    } while (interlocked_add(&thread_counter, 0) != num_threads);

    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");
    threadpool_destroy_work_item(threadpool, threadpool_work_item);
    threadpool_close(threadpool);

    (void)interlocked_exchange(&thread_counter, 0);

    // Reopen the threadpool
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));

    // assert
    LogInfo("Waiting for task to complete");
    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_destroy_work_item(threadpool, threadpool_work_item);
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(schedule_after_close_works_v2)
{
    // assert
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_threads = 1;
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
        (void)InterlockedHL_WaitForValue(&thread_counter, num_threads, TEST_TIMEOUT_VALUE);
    } while (interlocked_add(&thread_counter, 0) != num_threads);

    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");
    threadpool_close(threadpool);

    (void)interlocked_exchange(&thread_counter, 0);

    // Reopen the threadpool
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    LogInfo("Waiting for task to complete");
    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    threadpool_close(threadpool);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

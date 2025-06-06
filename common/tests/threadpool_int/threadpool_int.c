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
#include "c_pal/srw_lock.h"
#include "c_pal/sync.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/execution_engine.h"
#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"

static volatile_atomic int64_t g_call_count;

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

#define WAIT_WORK_FUNCTION_SLEEP_IN_MS 300

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

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

#define WRAP_TEST_WORK_ITEMS 10000

static void threadpool_long_task(void* context)
{
    WRAP_DATA* data = context;
    ASSERT_ARE_EQUAL(int, 0, strcmp(data->mem, "READY"));
    strcpy(data->mem, "DONE");
    ThreadAPI_Sleep(1);
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

static void work_function(void* context)
{
    volatile_atomic int64_t* call_count = (volatile_atomic int64_t*)context;
    (void)interlocked_increment_64(call_count);
    wake_by_address_all_64((void*)call_count);
}

static void wait_work_function(void* context)
{
    WAIT_WORK_CONTEXT* wait_work_context = (WAIT_WORK_CONTEXT*)context;

    int32_t current_value = interlocked_add(&wait_work_context->wait_event, 0);
    ASSERT_IS_TRUE(wait_on_address(&wait_work_context->wait_event, current_value, 2000) == WAIT_ON_ADDRESS_TIMEOUT); //todo: should this line be changed?
    (void)interlocked_increment_64(&wait_work_context->call_count);
    wake_by_address_single_64(&wait_work_context->call_count);
}

#define THREADPOOL_TIMER_STATE_VALUES \
    THREADPOOL_TIMER_STATE_NONE, \
    THREADPOOL_TIMER_STATE_STARTING, \
    THREADPOOL_TIMER_STATE_STARTED, \
    THREADPOOL_TIMER_STATE_CANCELING, \
    THREADPOOL_TIMER_STATE_STOPPING

#define TEST_ACTION_VALUES \
    TEST_ACTION_SCHEDULE_WORK, \
    TEST_ACTION_SCHEDULE_WORK_ITEM, \
    TEST_ACTION_START_THREADPOOL_TIMER, \
    TEST_ACTION_CANCEL_THREADPOOL_TIMER, \
    TEST_ACTION_RESTART_THREADPOOL_TIMER, \
    TEST_ACTION_DISPOSE_THREADPOOL_TIMER

MU_DEFINE_ENUM_WITHOUT_INVALID(TEST_ACTION, TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TEST_ACTION, TEST_ACTION_VALUES)

MU_DEFINE_ENUM(THREADPOOL_TIMER_STATE, THREADPOOL_TIMER_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(THREADPOOL_TIMER_STATE, THREADPOOL_TIMER_STATE_VALUES)

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
        (void)wait_on_address_64(value, current_value, (uint32_t)(timeout - (current_time - start_time)));
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

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    LogInfo("Waiting for task to complete");
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, interlocked_add(&thread_counter, 0), num_threads, "Thread counter has timed out");

    // cleanup
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

    LogInfo("Creating work item");
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);
    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));

    // assert
    LogInfo("Waiting for task to complete");
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, interlocked_add(&thread_counter, 0), num_threads, "Thread counter has timed out");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_owns_execution_engine_reference_and_can_schedule_work)
{
    // arrange
    // create an execution engine
    volatile_atomic int32_t thread_counter = 0;

    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // this is safe because the threadpool has a reference
    execution_engine_dec_ref(execution_engine);

    // act (schedule one work item)
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, 1, UINT32_MAX));
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

#define N_WORK_ITEMS 30

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items))
{
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_work_items = N_WORK_ITEMS;
    volatile_atomic int32_t execution_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create Work Items
    LogInfo("Creating work item once");
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_60_millisec, (void*)&execution_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item", num_work_items);
    for (uint32_t index = 0; index < num_work_items; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
    }

    LogInfo("Scheduled threads waiting for threads to complete");

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&execution_counter, num_work_items, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, execution_counter, num_work_items);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work))
{
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    const uint32_t num_work_items = N_WORK_ITEMS;
    volatile_atomic int32_t execution_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item", num_work_items);
    for (uint32_t index = 0; index < num_work_items; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_60_millisec, (void*)&execution_counter));
    }

    LogInfo("Scheduled, waiting for work items to complete");

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&execution_counter, num_work_items, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, execution_counter, num_work_items);

    // cleanup
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
    const uint32_t num_work_items = N_WORK_ITEMS;
    volatile_atomic int32_t execution_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create Work Items
    LogInfo("Creating work item once");
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&execution_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item with 20 millisecons work", num_work_items);
    for (uint32_t index = 0; index < num_work_items; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
    }

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&execution_counter, num_work_items, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, interlocked_add(&execution_counter, 0), num_work_items);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
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
    const uint32_t num_work_items = N_WORK_ITEMS;
    volatile_atomic int32_t execution_counter = 0;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create double the amount of threads that is the max
    LogInfo("Scheduling %" PRIu32 " work item with 20 millisecons work", num_work_items);
    for (uint32_t index = 0; index < num_work_items; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&execution_counter));
    }

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&execution_counter, num_work_items, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, interlocked_add(&execution_counter, 0), num_work_items);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define N_THREADPOOL_TIMERS 100

TEST_FUNCTION(MU_C3(starting_, N_THREADPOOL_TIMERS, _timer_start_runs_once))
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    THANDLE(THREADPOOL) threadpool_1 = NULL;
    THANDLE_INITIALIZE(THREADPOOL)(&threadpool_1, threadpool);

    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool_1, 200, 200, work_function, (void*)&g_call_count);
    ASSERT_IS_NOT_NULL(timer_instance);

    ThreadAPI_Sleep(5000);
    LogInfo("Waiting for timer to execute after short delay of no execution");

    ThreadAPI_Sleep(5000);
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);

    THANDLE_ASSIGN(THREADPOOL)(&threadpool_1, NULL);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(MU_C3(starting_, N_THREADPOOL_TIMERS, _start_timers_work_and_run_periodically))
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 16, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // act (start a timer to start delayed and then execute every 500ms)
    LogInfo("Starting " MU_TOSTRING(N_THREADPOOL_TIMERS) " timers");
    THANDLE(THREADPOOL_TIMER)* timers = malloc(N_THREADPOOL_TIMERS * sizeof(THANDLE(THREADPOOL_TIMER)));
    for (uint32_t i = 0; i < N_THREADPOOL_TIMERS; i++)
    {
        THANDLE(THREADPOOL_TIMER) timer_temp = threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&g_call_count);

        THANDLE_INITIALIZE_MOVE(THREADPOOL_TIMER)((void *) & timers[i], &timer_temp);
        ASSERT_IS_NOT_NULL(timers[i]);
    }

    // assert
    LogInfo("Waiting for " MU_TOSTRING(N_THREADPOOL_TIMERS) " timers to run at least 2 times each");

    // Every timer should execute at least twice in less than 1 second
    // Wait up to 2 seconds

    wait_for_greater_or_equal(&g_call_count, (2 * N_THREADPOOL_TIMERS), 2000);

    LogInfo("Timers completed, stopping all timers");

    for (uint32_t i = 0; i < N_THREADPOOL_TIMERS; i++)
    {
        THANDLE_ASSIGN(THREADPOOL_TIMER)((void*)&timers[i], NULL);
    }

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // cleanup
    free((void*)timers);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(close_while_items_are_scheduled_still_executes_all_items)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 1, 1 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange(&wait_work_context.wait_event, 0);

    (void)interlocked_exchange_64(&wait_work_context.call_count, 0);

    // schedule one item that waits
    LogInfo("Scheduling 2 work items");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, wait_work_function, (void*)&wait_work_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&g_call_count));

    ThreadAPI_Sleep(WAIT_WORK_FUNCTION_SLEEP_IN_MS);

    (void)interlocked_increment(&wait_work_context.wait_event);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 1, UINT32_MAX));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&wait_work_context.call_count, 1, UINT32_MAX));

    // cleanup
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

    // Create Work Items
    LogInfo("Creating work item once");
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_random, (void*)&thread_counter);
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
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_force_wrap_around)
{
    // arrange
    const uint32_t num_threads = WRAP_TEST_WORK_ITEMS;

    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    volatile_atomic int32_t thread_counter;
    (void)interlocked_exchange(&thread_counter, 0);

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
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_force_wrap_around_v2)
{
    // arrange
    const uint32_t num_threads = WRAP_TEST_WORK_ITEMS;
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 1;
    params.max_thread_count = 16;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    volatile_atomic int32_t thread_counter;
    (void)interlocked_exchange(&thread_counter, 0);

    WRAP_DATA* data = malloc(sizeof(WRAP_DATA));
    ASSERT_IS_NOT_NULL(data);

    data->counter = &thread_counter;
    (void)strcpy(data->mem, "READY");
    // Create Work Items
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_long_task_v2, data);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    for (uint32_t index = 0; index < num_threads; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
    }

    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    free(data);
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
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
        THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, 2000, 0, work_function, (void*)&g_call_count);
        ASSERT_IS_NOT_NULL(timer);

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
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 1, 5000));
            LogInfo("Timer completed, make sure it doesn't run again");

            // And should not run again
            ThreadAPI_Sleep(5000);
            ASSERT_ARE_EQUAL(uint64_t, 1, interlocked_add_64(&g_call_count, 0));
            LogInfo("Done waiting for timer");

            need_to_retry = false;
        }

        THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
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
        THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, 4000, 0, work_function, (void*)&g_call_count);
        ASSERT_IS_NOT_NULL(timer);

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
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 1, 2000));
            LogInfo("Timer completed, make sure it doesn't run again");

            // And should not run again
            ThreadAPI_Sleep(5000);
            ASSERT_ARE_EQUAL(uint32_t, 1, interlocked_add_64(&g_call_count, 0));
            LogInfo("Done waiting for timer");

            need_to_retry = false;
        }

        THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
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
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&g_call_count);

    // assert

    // Timer should run 4 times in about 2.1 seconds
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 4, 3000));
    LogInfo("Timer completed 4 times");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
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
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&g_call_count);
    ASSERT_IS_NOT_NULL(timer);

    // Timer should run 4 times in about 3 seconds
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 4, 3000));
    LogInfo("Timer completed 4 times");

    // act
    LogInfo("Cancel then restart timer");
    threadpool_timer_cancel(timer);
    (void)interlocked_exchange_64(&g_call_count, 0);
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(timer, 100, 1000));

    // assert

    // Timer should run 2 more times in about 3 seconds
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 2, 3000));
    LogInfo("Timer completed 2 more times");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
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
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, 0, 5000, wait_work_function, (void*)&wait_work_context);
    ASSERT_IS_NOT_NULL(timer);

    // act
    ThreadAPI_Sleep(500);

    // call stop
    LogInfo("Timer should be running and waiting, now stop timer");
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);

    LogInfo("Timer stopped");

    // set the event, that would trigger a WAIT_OBJECT_0 if stop would not wait for all items
    (void)interlocked_increment(&wait_work_context.wait_event);

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
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, 0, 5000, wait_work_function, (void*)&wait_work_context);
    ASSERT_IS_NOT_NULL(timer);

    // act
    ThreadAPI_Sleep(500);

    // call cancel
    LogInfo("Timer should be running and waiting, now cancel timer");
    threadpool_timer_cancel(timer);

    LogInfo("Timer canceled");

    // set the event, that would trigger a WAIT_OBJECT_0 if stop would not wait for all items
    (void)interlocked_increment(&wait_work_context.wait_event);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(schedule_work_two_times)
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

    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));

    // assert
    LogInfo("Waiting for task to complete");
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));

    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);

    (void)interlocked_exchange(&thread_counter, 0);

    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item_2 = threadpool_create_work_item(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter);
    ASSERT_IS_NOT_NULL(threadpool_work_item_2);

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item_2));

    // assert
    LogInfo("Waiting for task to complete");
    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&threadpool_work_item_2, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(schedule_work_two_times_v2)
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

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    LogInfo("Waiting for task to complete");
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));

    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    (void)interlocked_exchange(&thread_counter, 0);

    // Create 1 thread pool
    LogInfo("Scheduling work item");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, threadpool_task_wait_20_millisec, (void*)&thread_counter));

    // assert
    LogInfo("Waiting for task to complete");
    // assert
    ASSERT_ARE_EQUAL(int, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&thread_counter, num_threads, UINT32_MAX));
    ASSERT_ARE_EQUAL(int32_t, thread_counter, num_threads, "Thread counter has timed out");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define CHAOS_THREAD_COUNT 4
#define TEST_RUN_TIME 10000 //ms

typedef struct CHAOS_TEST_THREADPOOL_TIMER_DATA_TAG
{
    volatile_atomic int32_t state; // THREADPOOL_TIMER_STATE
    THANDLE(THREADPOOL_TIMER) timer;
} CHAOS_TEST_THREADPOOL_TIMER_DATA;

#define MAX_THREADPOOL_TIMER_COUNT 10

typedef struct CHAOS_TEST_DATA_TAG
{
    volatile_atomic int64_t expected_call_count;
    volatile_atomic int64_t executed_work_functions;
    volatile_atomic int64_t executed_timer_functions;
    volatile_atomic int32_t chaos_test_done;
    THANDLE(THREADPOOL) threadpool;

    volatile_atomic int64_t timers_starting;
    CHAOS_TEST_THREADPOOL_TIMER_DATA timers[MAX_THREADPOOL_TIMER_COUNT];
    THANDLE(THREADPOOL_WORK_ITEM) work_item_context;
} CHAOS_TEST_DATA;

#define THREADPOOL_TIMER_START_DELAY_MIN 0
#define THREADPOOL_TIMER_START_DELAY_MAX 100

#define THREADPOOL_TIMER_PERIOD_MIN 50
#define THREADPOOL_TIMER_PERIOD_MAX 400

static int chaos_thread_func(void* context)
{
    CHAOS_TEST_DATA* chaos_test_data = context;

    while (interlocked_add(&chaos_test_data->chaos_test_done, 0) == 0)
    {
        int which_action = rand() % 2;
        switch (which_action)
        {
        case 0:
            //perform a schedule item
            if (threadpool_schedule_work(chaos_test_data->threadpool, work_function, (void*)&chaos_test_data->executed_work_functions) == 0)
            {
                (void)interlocked_increment_64(&chaos_test_data->expected_call_count);
                wake_by_address_all_64(&chaos_test_data->expected_call_count);
            }
            break;
        case 1:
            // perform a schedule work item
            if (threadpool_schedule_work_item(chaos_test_data->threadpool, chaos_test_data->work_item_context) == 0)
            {
                (void)interlocked_increment_64(&chaos_test_data->expected_call_count);
                wake_by_address_all_64(&chaos_test_data->expected_call_count);
            }
            break;
        }
    }

    return 0;
}

static void chaos_cleanup_all_timers(CHAOS_TEST_DATA* chaos_test_data)
{
    for (uint32_t i = 0; i < MAX_THREADPOOL_TIMER_COUNT; i++)
    {
        if (interlocked_compare_exchange(&chaos_test_data->timers[i].state, THREADPOOL_TIMER_STATE_STOPPING, THREADPOOL_TIMER_STATE_STARTED) == THREADPOOL_TIMER_STATE_STARTED)
        {
            LogInfo("Cleaning up timer %d", i);
            THANDLE_ASSIGN(THREADPOOL_TIMER)(&chaos_test_data->timers[i].timer, NULL);
            (void)interlocked_exchange(&chaos_test_data->timers[i].state, THREADPOOL_TIMER_STATE_NONE);
        }
    }

    LogInfo("All timers cleaned up");
}

// do not schedule more than this many work items to be pending
// this is to avoid the test from running forever under valgrind/helgrind
#define MAX_CHAOS_PENDING_WORK_ITEMS 5000

// for how long should we run the chaos test
#define CHAOS_TEST_RUN_TIME_IN_MS 5000 //ms

static int chaos_thread_with_timers_no_lock_func(void* context)
{
    CHAOS_TEST_DATA* chaos_test_data = context;

    while (interlocked_add(&chaos_test_data->chaos_test_done, 0) == 0)
    {
        TEST_ACTION which_action = (TEST_ACTION)((uint64_t)rand() * (MU_COUNT_ARG(TEST_ACTION_VALUES)) / ((uint32_t)RAND_MAX + 1));
        switch (which_action)
        {
        default:
            ASSERT_FAIL("unexpected action type=%" PRI_MU_ENUM "", MU_ENUM_VALUE(TEST_ACTION, which_action));
            break;
        case TEST_ACTION_DISPOSE_THREADPOOL_TIMER:
            // Cancel a timer
            {
                int which_timer_slot = rand() % MAX_THREADPOOL_TIMER_COUNT;
                if (interlocked_compare_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_STOPPING, THREADPOOL_TIMER_STATE_STARTED) == THREADPOOL_TIMER_STATE_STARTED)
                {
                    THANDLE_ASSIGN(THREADPOOL_TIMER)(&chaos_test_data->timers[which_timer_slot].timer, NULL);
                    (void)interlocked_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_NONE);
                }
                break;
            }
           
        case TEST_ACTION_START_THREADPOOL_TIMER:
            // Start a timer
        {
            int which_timer_slot = rand() % MAX_THREADPOOL_TIMER_COUNT;
            if (interlocked_compare_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_STARTING, THREADPOOL_TIMER_STATE_NONE) == THREADPOOL_TIMER_STATE_NONE)
            {
                uint32_t timer_start_delay = rand() % THREADPOOL_TIMER_START_DELAY_MAX;
                uint32_t timer_period = rand() % THREADPOOL_TIMER_PERIOD_MAX;
                THANDLE(THREADPOOL_TIMER) temp_timer = threadpool_timer_start(chaos_test_data->threadpool, timer_start_delay, timer_period, threadpool_task_wait_20_millisec, (void*)&chaos_test_data->executed_timer_functions);
                if (temp_timer == NULL)
                {
                    (void)interlocked_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_NONE);
                }
                else
                {
                    THANDLE_INITIALIZE_MOVE(THREADPOOL_TIMER)(&chaos_test_data->timers[which_timer_slot].timer, &temp_timer);
                    (void)interlocked_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_STARTED);
                }
            }
            break;
        }
        case TEST_ACTION_CANCEL_THREADPOOL_TIMER:
            // Cancel a timer
        {
            // Synchronize with close
            int which_timer_slot = rand() % MAX_THREADPOOL_TIMER_COUNT;
            if (interlocked_compare_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_CANCELING, THREADPOOL_TIMER_STATE_STARTED) == THREADPOOL_TIMER_STATE_STARTED)
            {
                threadpool_timer_cancel(chaos_test_data->timers[which_timer_slot].timer);
                (void)interlocked_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_STARTED);
            }
            break;
        }
        case TEST_ACTION_RESTART_THREADPOOL_TIMER:
            // Restart a timer
        {
            // Synchronize with close
            int which_timer_slot = rand() % MAX_THREADPOOL_TIMER_COUNT;
            if (interlocked_compare_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_STARTING, THREADPOOL_TIMER_STATE_STARTED) == THREADPOOL_TIMER_STATE_STARTED)
            {
                uint32_t timer_start_delay = rand() % THREADPOOL_TIMER_START_DELAY_MAX;
                uint32_t timer_period = rand() % THREADPOOL_TIMER_PERIOD_MAX;
                ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(chaos_test_data->timers[which_timer_slot].timer, timer_start_delay, timer_period));
                (void)interlocked_exchange(&chaos_test_data->timers[which_timer_slot].state, THREADPOOL_TIMER_STATE_STARTED);
            }
            break;
        }
        case TEST_ACTION_SCHEDULE_WORK:
        {
            int64_t expected_call_count = interlocked_add_64(&chaos_test_data->expected_call_count, 0);
            int64_t executed_work_functions = interlocked_add_64(&chaos_test_data->executed_work_functions, 0);

            if (expected_call_count - executed_work_functions < MAX_CHAOS_PENDING_WORK_ITEMS)
            {
                // perform a schedule item
                if (threadpool_schedule_work(chaos_test_data->threadpool, work_function, (void*)&chaos_test_data->executed_work_functions) == 0)
                {
                    (void)interlocked_increment_64(&chaos_test_data->expected_call_count);
                }
            }
            else
            {
                // we already scheduled too many work items, don't schedule more
            }
            break;
        }
        case TEST_ACTION_SCHEDULE_WORK_ITEM:
        {
            int64_t expected_call_count = interlocked_add_64(&chaos_test_data->expected_call_count, 0);
            int64_t executed_work_functions = interlocked_add_64(&chaos_test_data->executed_work_functions, 0);

            if (expected_call_count - executed_work_functions < MAX_CHAOS_PENDING_WORK_ITEMS)
            {
                // perform a schedule work item
                if (threadpool_schedule_work_item(chaos_test_data->threadpool, chaos_test_data->work_item_context) == 0)
                {
                    (void)interlocked_increment_64(&chaos_test_data->expected_call_count);
                }
                else
                {
                    //don't care if it failed
                }
            }
            else
            {
                // we already scheduled too many work items, don't schedule more
            }
            break;
        }
        }
    }

    return 0;
}

TEST_FUNCTION(chaos_knight_test)
{
    // start a number of threads and each of them will do a random action on the threadpool
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 16, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    THREAD_HANDLE thread_handles[CHAOS_THREAD_COUNT];
    size_t i;
    CHAOS_TEST_DATA chaos_test_data;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&chaos_test_data.threadpool, &threadpool);

    (void)interlocked_exchange_64(&chaos_test_data.expected_call_count, 0);
    (void)interlocked_exchange_64(&chaos_test_data.executed_work_functions, 0);
    (void)interlocked_exchange(&chaos_test_data.chaos_test_done, 0);

    for (i = 0; i < MAX_THREADPOOL_TIMER_COUNT; i++)
    {
        THANDLE_INITIALIZE(THREADPOOL_TIMER)(&chaos_test_data.timers[i].timer, NULL);
        (void)interlocked_exchange(&chaos_test_data.timers[i].state, THREADPOOL_TIMER_STATE_NONE);
    }

    // Create the Work Item Context once
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(chaos_test_data.threadpool, work_function, (void*)&chaos_test_data.executed_work_functions);
    THANDLE_INITIALIZE_MOVE(THREADPOOL_WORK_ITEM)(&chaos_test_data.work_item_context, &work_item);

    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread_handles[i], chaos_thread_func, &chaos_test_data));
        ASSERT_IS_NOT_NULL(thread_handles[i], "thread %zu failed to start", i);
    }

    (void)interlocked_exchange(&chaos_test_data.chaos_test_done, 1);
    // wait for all threads to complete
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread_handles[i], &dont_care));
    }
    LogInfo("executed_work_functions=%" PRIu64 ", expected_call_count=%" PRIu64 "", chaos_test_data.executed_work_functions, chaos_test_data.expected_call_count);
    // assert that all scheduled items were executed
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&chaos_test_data.executed_work_functions, chaos_test_data.expected_call_count, UINT32_MAX));

    LogInfo("Chaos test executed %" PRId64 " work items", interlocked_add_64(&chaos_test_data.executed_work_functions, 0));

    // call close
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&chaos_test_data.work_item_context, NULL);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&chaos_test_data.threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(chaos_knight_test_with_timers_no_lock)
{
    // start a number of threads and each of them will do a random action on the threadpool
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);
    THREAD_HANDLE thread_handles[CHAOS_THREAD_COUNT];
    size_t i;
    CHAOS_TEST_DATA chaos_test_data;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&chaos_test_data.threadpool, &threadpool);

    (void)interlocked_exchange_64(&chaos_test_data.expected_call_count, 0);
    (void)interlocked_exchange_64(&chaos_test_data.executed_work_functions, 0);
    (void)interlocked_exchange_64(&chaos_test_data.executed_timer_functions, 0);
    (void)interlocked_exchange_64(&chaos_test_data.timers_starting, 0);
    (void)interlocked_exchange(&chaos_test_data.chaos_test_done, 0);
    wake_by_address_all_64(&chaos_test_data.executed_work_functions);

    for (i = 0; i < MAX_THREADPOOL_TIMER_COUNT; i++)
    {
        THANDLE_INITIALIZE(THREADPOOL_TIMER)(&chaos_test_data.timers[i].timer, NULL);
        (void)interlocked_exchange(&chaos_test_data.timers[i].state, THREADPOOL_TIMER_STATE_NONE);
    }

    LogInfo("Starting test");

    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(chaos_test_data.threadpool, work_function, (void*)&chaos_test_data.executed_work_functions);
    ASSERT_IS_NOT_NULL(work_item);
    THANDLE_INITIALIZE_MOVE(THREADPOOL_WORK_ITEM)(&chaos_test_data.work_item_context, &work_item);
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread_handles[i], chaos_thread_with_timers_no_lock_func, &chaos_test_data));
    }

    LogInfo("Threads created, waiting");

    // wait for some time
    ThreadAPI_Sleep(CHAOS_TEST_RUN_TIME_IN_MS);

    LogInfo("Stopping test");

    (void)interlocked_exchange(&chaos_test_data.chaos_test_done, 1);

    // wait for all threads to complete
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        int dont_care;
        LogInfo("Stopping thread %zu", i);
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread_handles[i], &dont_care));
    }

    // assert that all scheduled items were executed
    int64_t expected_call_count = interlocked_add_64(&chaos_test_data.expected_call_count, 0);
    LogInfo("Waiting for execute_work_functions to be %" PRId64 ", now it is: %" PRId64 "",
        expected_call_count, interlocked_add_64(&chaos_test_data.executed_work_functions, 0));

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&chaos_test_data.executed_work_functions, expected_call_count, UINT32_MAX));

    LogInfo("Chaos test executed %" PRId64 " work items, %" PRId64 " timers",
        interlocked_add_64(&chaos_test_data.executed_work_functions, 0), interlocked_add_64(&chaos_test_data.executed_timer_functions, 0));

    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&chaos_test_data.work_item_context, NULL);
    chaos_cleanup_all_timers(&chaos_test_data);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&chaos_test_data.threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_work_item_schedule_works_v2)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // act (schedule one work item)
    LogInfo("Create Work Item Context");
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, work_function, (void*)&g_call_count);
    ASSERT_IS_NOT_NULL(work_item);
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, work_item));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 1, UINT32_MAX));
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_owns_execution_engine_reference_and_can_schedule_work_v2)
{
    // arrange
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // this is safe because the threadpool has a reference
    execution_engine_dec_ref(execution_engine);

    // act (schedule one work item)
    LogInfo("Create Work Item Context");
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, work_function, (void*)&g_call_count);
    ASSERT_IS_NOT_NULL(work_item);
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&g_call_count));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 1, UINT32_MAX));
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_works_v2))
{
    // assert
    // create an execution engine
    size_t i;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    LogInfo("Scheduling work %" PRIu32 " times", (uint32_t)N_WORK_ITEMS);
    // act (schedule work items)
    LogInfo("Create Work Item Context");
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, work_function, (void*)&g_call_count);
    ASSERT_IS_NOT_NULL(work_item);
    for (i = 0; i < N_WORK_ITEMS; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, work_item));
    }

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, N_WORK_ITEMS, UINT32_MAX));
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(close_while_items_are_scheduled_still_executes_all_items_v2)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 1, 1 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)interlocked_exchange(&wait_work_context.wait_event, 0);

    (void)interlocked_exchange_64(&wait_work_context.call_count, 0);

    // schedule one item that waits
    LogInfo("Create Work Item Context");
    THANDLE(THREADPOOL_WORK_ITEM) wait_work_item_context = threadpool_create_work_item(threadpool, wait_work_function, (void*)&wait_work_context);
    ASSERT_IS_NOT_NULL(wait_work_item_context);
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, work_function, (void*)&g_call_count);
    ASSERT_IS_NOT_NULL(work_item);
    LogInfo("Scheduling 2 work items");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, wait_work_item_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, work_item));

    ThreadAPI_Sleep(WAIT_WORK_FUNCTION_SLEEP_IN_MS);

    (void)interlocked_increment(&wait_work_context.wait_event);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&g_call_count, 1, UINT32_MAX));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&wait_work_context.call_count, 1, UINT32_MAX));

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&wait_work_item_context, NULL);
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

typedef struct TIMER_STARTS_A_TIMER_CONTEXT_TAG
{
    volatile_atomic int32_t timer_started_from_a_timer_function_executed;
    THANDLE(THREADPOOL) threadpool;
    THANDLE(THREADPOOL_TIMER) second_timer;
} TIMER_STARTS_AT_TIMER_CONTEXT;

static void complete_timer_starts_a_timer_test_work_function(void* context)
{
    (void)context;
    TIMER_STARTS_AT_TIMER_CONTEXT* timer_starts_a_timer_context = context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&timer_starts_a_timer_context->timer_started_from_a_timer_function_executed, 1));
}

static void timer_work_function_that_starts_another_timer(void* context)
{
    TIMER_STARTS_AT_TIMER_CONTEXT* timer_starts_a_timer_context = context;
    THANDLE(THREADPOOL_TIMER) second_timer_instance = threadpool_timer_start(timer_starts_a_timer_context->threadpool, 200, 0, complete_timer_starts_a_timer_test_work_function, (void*)timer_starts_a_timer_context);
    THANDLE_INITIALIZE_MOVE(THREADPOOL_TIMER)(&timer_starts_a_timer_context->second_timer, &second_timer_instance);
}

TEST_FUNCTION(starting_a_timer_from_a_timer_callback_works)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 1, 1 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    TIMER_STARTS_AT_TIMER_CONTEXT timer_starts_a_timer_context;
    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&timer_starts_a_timer_context.threadpool, &threadpool);
    (void)interlocked_exchange(&timer_starts_a_timer_context.timer_started_from_a_timer_function_executed, 0);

    THANDLE(THREADPOOL_TIMER) first_timer = threadpool_timer_start(timer_starts_a_timer_context.threadpool, 200, 0, timer_work_function_that_starts_another_timer, (void*)&timer_starts_a_timer_context);
    ASSERT_IS_NOT_NULL(first_timer);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&timer_starts_a_timer_context.timer_started_from_a_timer_function_executed, 1, UINT32_MAX));

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&first_timer, NULL);
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_starts_a_timer_context.second_timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&timer_starts_a_timer_context.threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

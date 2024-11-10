// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "windows.h"

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/timer.h"
#include "c_pal/threadpool.h"
#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/sync.h"
#include "c_pal/interlocked_hl.h"

#include "c_pal/execution_engine_win32.h"

#define XTEST_FUNCTION(A) void A(void)

static void work_function(void* context)
{
    volatile LONG64* call_count = (volatile LONG64*)context;
    (void)InterlockedIncrement64(call_count);
    WakeByAddressSingle((PVOID)call_count);
}

typedef struct WAIT_WORK_CONTEXT_TAG
{
    volatile LONG call_count;
    HANDLE wait_event;
} WAIT_WORK_CONTEXT;

#define WAIT_WORK_FUNCTION_SLEEP_IN_MS 3000

static void wait_work_function(void* context)
{
    WAIT_WORK_CONTEXT* wait_work_context = (WAIT_WORK_CONTEXT*)context;
    ASSERT_IS_TRUE(WaitForSingleObject(wait_work_context->wait_event, 2000) == WAIT_TIMEOUT);
    (void)InterlockedIncrement(&wait_work_context->call_count);
    WakeByAddressSingle((PVOID)&wait_work_context->call_count);
}

typedef struct CLOSE_WORK_CONTEXT_TAG
{
    volatile LONG call_count;
    THANDLE(THREADPOOL) threadpool;
} CLOSE_WORK_CONTEXT;

typedef struct OPEN_WORK_CONTEXT_TAG
{
    volatile LONG call_count;
    THANDLE(THREADPOOL) threadpool;
} OPEN_WORK_CONTEXT;

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
    TEST_ACTION_DESTROY_TIMER, \
    TEST_ACTION_CANCEL_TIMER, \
    TEST_ACTION_RESTART_TIMER, \
    TEST_ACTION_SCHEDULE_WORK_ITEM

MU_DEFINE_ENUM(TEST_ACTION, TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS(TEST_ACTION, TEST_ACTION_VALUES)

MU_DEFINE_ENUM(TIMER_STATE, TIMER_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(TIMER_STATE, TIMER_STATE_VALUES)

static void wait_for_greater_or_equal(volatile LONG* value, LONG expected, DWORD timeout)
{
    double start_time = timer_global_get_elapsed_ms();
    double current_time = timer_global_get_elapsed_ms();
    do
    {
        if (current_time - start_time >= timeout)
        {
            ASSERT_FAIL("Timeout waiting for value");
        }

        LONG current_value = InterlockedAdd(value, 0);
        if (current_value >= expected)
        {
            break;
        }
        (void)WaitOnAddress(value, &current_value, sizeof(current_value), timeout - (DWORD)(current_time - start_time));
    } while (1);
}

static void wait_for_equal(volatile LONG* value, LONG expected, DWORD timeout)
{
    double start_time = timer_global_get_elapsed_ms();
    double current_time = timer_global_get_elapsed_ms();
    do
    {
        if (current_time - start_time >= timeout)
        {
            ASSERT_FAIL("Timeout waiting for value");
        }

        LONG current_value = InterlockedAdd(value, 0);
        if (current_value == expected)
        {
            break;
        }
        (void)WaitOnAddress(value, &current_value, sizeof(current_value), timeout - (DWORD)(current_time - start_time));
    } while (1);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    // Turn off logging so it doesn't put unneccesary
    // sleeps in the code.
    logger_set_config((LOGGER_CONFIG) { .log_sinks = NULL, .log_sink_count = 0 });
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

TEST_FUNCTION(one_work_item_schedule_works)
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)InterlockedExchange(&call_count, 0);

    // act (schedule one work item)
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));

    // assert
    wait_for_equal(&call_count, 1, INFINITE);
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_owns_execution_engine_reference_and_can_schedule_work)
{
    // arrange
    // create an execution engine
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // this is safe because the threadpool has a reference
    execution_engine_dec_ref(execution_engine);

    (void)InterlockedExchange(&call_count, 0);

    // act (schedule one work item)
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));

    // assert
    wait_for_equal(&call_count, 1, INFINITE);
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

#define N_WORK_ITEMS 100

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_works))
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    size_t i;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)InterlockedExchange(&call_count, 0);

    LogInfo("Scheduling work " MU_TOSTRING(N_WORK_TIMES) " times");
    // act (schedule work items)
    for (i = 0; i < N_WORK_ITEMS; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));
    }

    // assert
    wait_for_equal(&call_count, N_WORK_ITEMS, INFINITE);
    LogInfo("Work completed");

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_start_timer_works_runs_once)
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
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
        (void)InterlockedExchange(&call_count, 0);

        LogInfo("Starting timer");

        // act (start a timer to start delayed and then execute once)
        TIMER_INSTANCE_HANDLE timer;
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 2000, 0, work_function, (void*)&call_count, &timer));

        // assert

        // Timer starts after 2 seconds, wait a bit and it should not yet have run
        Sleep(500);
        if (InterlockedAdd(&call_count, 0) != 0)
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
            Sleep(5000);
            ASSERT_ARE_EQUAL(uint32_t, 1, (uint32_t)InterlockedAdd(&call_count, 0));
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
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
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
        (void)InterlockedExchange(&call_count, 0);

        LogInfo("Starting timer");

        // start a timer to start delayed after 4 seconds (which would fail test)
        TIMER_INSTANCE_HANDLE timer;
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 4000, 0, work_function, (void*)&call_count, &timer));

        // act (restart timer to start delayed instead after 2 seconds)
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(timer, 2000, 0));

        // assert

        // Timer starts after 2 seconds, wait a bit and it should not yet have run
        Sleep(500);
        if (InterlockedAdd(&call_count, 0) != 0)
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
            Sleep(5000);
            ASSERT_ARE_EQUAL(uint32_t, 1, (uint32_t)InterlockedAdd(&call_count, 0));
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
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)InterlockedExchange(&call_count, 0);

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
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)InterlockedExchange(&call_count, 0);

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
    (void)InterlockedExchange(&call_count, 0);
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
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    wait_work_context.wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(wait_work_context.wait_event);

    (void)InterlockedExchange(&wait_work_context.call_count, 0);

    // schedule one timer that waits
    LogInfo("Starting timer");
    TIMER_INSTANCE_HANDLE timer;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 0, 5000, wait_work_function, (void*)&wait_work_context, &timer));

    // act

    Sleep(WAIT_WORK_FUNCTION_SLEEP_IN_MS);

    // call stop
    LogInfo("Timer should be running and waiting, now stop timer");
    threadpool_timer_destroy(timer);

    LogInfo("Timer stopped");

    // set the event, that would trigger a WAIT_OBJECT_0 if stop would not wait for all items
    SetEvent(wait_work_context.wait_event);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(cancel_timer_waits_for_ongoing_execution)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    wait_work_context.wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(wait_work_context.wait_event);

    (void)InterlockedExchange(&wait_work_context.call_count, 0);

    // schedule one timer that waits
    LogInfo("Starting timer");
    TIMER_INSTANCE_HANDLE timer;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 0, 5000, wait_work_function, (void*)&wait_work_context, &timer));

    // act

    Sleep(WAIT_WORK_FUNCTION_SLEEP_IN_MS);

    // call cancel
    LogInfo("Timer should be running and waiting, now cancel timer");
    threadpool_timer_cancel(timer);

    LogInfo("Timer canceled");

    // set the event, that would trigger a WAIT_OBJECT_0 if stop would not wait for all items
    SetEvent(wait_work_context.wait_event);

    // cleanup
    threadpool_timer_destroy(timer);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define N_TIMERS 100

TEST_FUNCTION(MU_C3(starting_, N_TIMERS, _timer_start_runs_once))
{
    // assert
    // create an execution engine
    volatile uint32_t call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    THANDLE(THREADPOOL) threadpool_1 = NULL;
    THANDLE_INITIALIZE(THREADPOOL)(&threadpool_1, threadpool);

    TIMER_INSTANCE_HANDLE timers[N_TIMERS];
    for (uint32_t index = 0; index < 1; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool_1, 200, 200, work_function, (void*)&call_count, &timers[index]));

    }

    Sleep(5000);
    LogInfo("Waiting for timer to execute after short delay of no execution");

    for (uint32_t index = 0; index < 1; index++)
    {
        threadpool_timer_destroy(timers[index]);
    }

    THANDLE_ASSIGN(THREADPOOL)(&threadpool_1, NULL);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}


TEST_FUNCTION(MU_C3(starting_, N_TIMERS, _start_timers_work_and_run_periodically))
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)InterlockedExchange(&call_count, 0);

    // act (start a timer to start delayed and then execute every 500ms)
    LogInfo("Starting " MU_TOSTRING(N_TIMERS) " timers");
    TIMER_INSTANCE_HANDLE timers[N_TIMERS];
    for (uint32_t i = 0; i < N_TIMERS; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, 100, 500, work_function, (void*)&call_count, &timers[i]));
    }

    // assert

    LogInfo("Waiting for " MU_TOSTRING(N_TIMERS) " timers to run at least 2 times each");

    // Every timer should execute at least twice in less than 1 second
    // Wait up to 2 seconds
    wait_for_greater_or_equal(&call_count, (2 * N_TIMERS), 2000);

    LogInfo("Timers completed, stopping all timers");

    for (uint32_t i = 0; i < N_TIMERS; i++)
    {
        threadpool_timer_destroy(timers[i]);
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
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

    wait_work_context.wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(wait_work_context.wait_event);

    (void)InterlockedExchange(&wait_work_context.call_count, 0);

    // schedule one item that waits
    LogInfo("Scheduling 2 work items");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, wait_work_function, (void*)&wait_work_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&wait_work_context.call_count));

    Sleep(WAIT_WORK_FUNCTION_SLEEP_IN_MS);
    // call close
    LogInfo("Closing threadpool");

    // set the event, that would trigger a WAIT_OBJECT_0 if close would not wait for all items
    SetEvent(wait_work_context.wait_event);

    // assert
    wait_for_equal(&wait_work_context.call_count, 2, INFINITE);

    // cleanup
    (void)CloseHandle(wait_work_context.wait_event);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define CHAOS_THREAD_COUNT 4
#define TEST_RUN_TIME 10000 //ms

typedef struct CHAOS_TEST_TIMER_DATA_TAG
{
    volatile LONG state; // TIMER_STATE
    TIMER_INSTANCE_HANDLE timer;
} CHAOS_TEST_TIMER_DATA;

#define MAX_TIMER_COUNT 10

typedef struct CHAOS_TEST_DATA_TAG
{
    volatile LONG64 expected_call_count;
    volatile LONG64 executed_work_functions;
    volatile LONG64 executed_timer_functions;
    volatile LONG chaos_test_done;
    THANDLE(THREADPOOL) threadpool;

    volatile LONG can_start_timers;
    volatile LONG can_schedule_works;
    volatile LONG timers_starting;
    CHAOS_TEST_TIMER_DATA timers[MAX_TIMER_COUNT];
    THREADPOOL_WORK_ITEM_HANDLE work_item_context;
} CHAOS_TEST_DATA;

#define TIMER_START_DELAY_MIN 0
#define TIMER_START_DELAY_MAX 100

#define TIMER_PERIOD_MIN 50
#define TIMER_PERIOD_MAX 400

static void chaos_delay()
{
    // Earlier, the threadpool_open and threadpool_close would start and stop threadpool threads whereby there would be some limit on number of times work would be scheduled
    // on Windows Threadpool using threadpool_schedule_work and threadpool_schedule_work_item because both functions would test for threadpool open condition and return if threadpool is not open.
    // With threadpool_open and threadpool_close now only there for backward compatibility and doing nothing, the chaos test for optimized threadpool_schedule_work_item
    // would create a very large number of work items compared to the previous version of threadpool_schedule_work. The tests would fail because expected_call_count would be very high than the
    // executed_work_functions count because Windows Threadpool is unable to schedule that many work items. Adding Sleep(1) would significantly slow the tests. So adding following malloc and free
    // would ensure delay less than 1 milliseconds, enough to allow Windows Threadpool to handle this chaos test scenarios for threadpool_schedule_work_item. For threadpool_schedule_work there are
    // already extra operations than threadpool_schedule_work_item like malloc and free (in its callback) that would create more delay than this function. Hence threadpool_schedule_work would pass in
    // chaos test even after threadpool_open and threadpool_close have reduced functionality.
    for (int i = 0; i < CHAOS_THREAD_COUNT * CHAOS_THREAD_COUNT; i++)
    {
        // The loop and randomized data count around malloc avoids compiler optimizaton where consecutive malloc and free are not removed by the compiler optimization.
        int random_count = (rand() * (i+1) * 4) / (RAND_MAX + 1);
        CHAOS_TEST_DATA* temp = (CHAOS_TEST_DATA*)malloc(random_count * sizeof(CHAOS_TEST_DATA));
        free(temp);
    }
}

static DWORD WINAPI chaos_thread_func(LPVOID lpThreadParameter)
{
    CHAOS_TEST_DATA* chaos_test_data = (CHAOS_TEST_DATA*)lpThreadParameter;

    while (InterlockedAdd(&chaos_test_data->chaos_test_done, 0) == 0)
    {
        int which_action = rand() * 2 / (RAND_MAX + 1);
        switch (which_action)
        {
            case 0:
                // perform a schedule item
                if (threadpool_schedule_work(chaos_test_data->threadpool, work_function, (void*)&chaos_test_data->executed_work_functions) == 0)
                {
                    (void)InterlockedIncrement64(&chaos_test_data->expected_call_count);
                }
                break;
            case 1:
                // perform a schedule work item
                if (threadpool_schedule_work_item(chaos_test_data->threadpool, chaos_test_data->work_item_context) == 0)
                {
                    (void)InterlockedIncrement64(&chaos_test_data->expected_call_count);
                }
                break;
        }
    }

    return 0;
}

static void chaos_cleanup_all_timers(CHAOS_TEST_DATA* chaos_test_data)
{
    for (uint32_t i = 0; i < MAX_TIMER_COUNT; i++)
    {
        if (InterlockedCompareExchange(&chaos_test_data->timers[i].state, TIMER_STATE_STOPPING, TIMER_STATE_STARTED) == TIMER_STATE_STARTED)
        {
            threadpool_timer_destroy(chaos_test_data->timers[i].timer);
            chaos_test_data->timers[i].timer = NULL;
            InterlockedExchange(&chaos_test_data->timers[i].state, TIMER_STATE_NONE);
        }
    }
}

static DWORD WINAPI chaos_thread_with_timers_no_lock_func(LPVOID lpThreadParameter)
{
    CHAOS_TEST_DATA* chaos_test_data = lpThreadParameter;

    while (InterlockedAdd(&chaos_test_data->chaos_test_done, 0) == 0)
    {
        int which_action = rand() * (MU_ENUM_VALUE_COUNT(TEST_ACTION_VALUES) - 2) / RAND_MAX + 1;
        switch (which_action)
        {
        default:
            ASSERT_FAIL("unexpected action type=%" PRI_MU_ENUM "", MU_ENUM_VALUE(TEST_ACTION, which_action));
            break;
        case TEST_ACTION_CLEANUP_TIMER:
            // First prevent new timers, because we need to clean them all up (lock)
            if (InterlockedCompareExchange(&chaos_test_data->can_start_timers, 0, 1) == 1 && InterlockedCompareExchange(&chaos_test_data->can_schedule_works, 0, 1) == 1)
            {
                // Wait for any threads that had been starting timers to complete
                wait_for_equal(&chaos_test_data->timers_starting, 0, INFINITE);

                wait_for_equal((void*)(&chaos_test_data->executed_work_functions), (LONG)(chaos_test_data->expected_call_count), INFINITE);
                // Cleanup all timers
                chaos_cleanup_all_timers(chaos_test_data);

                // Now back to normal
                (void)InterlockedExchange(&chaos_test_data->can_start_timers, 1);
                (void)InterlockedExchange(&chaos_test_data->can_schedule_works, 1);
            }
            break;
        case TEST_ACTION_SCHEDULE_WORK:
            // perform a schedule item
            if (InterlockedAdd(&chaos_test_data->can_schedule_works, 0) != 0)
            {
                if (threadpool_schedule_work(chaos_test_data->threadpool, work_function, (void*)&chaos_test_data->executed_work_functions) == 0)
                {
                    (void)InterlockedIncrement64(&chaos_test_data->expected_call_count);
                }
            }
            break;
        case TEST_ACTION_START_TIMER:
            // Start a timer
        {
            // Synchronize with close
            (void)InterlockedIncrement(&chaos_test_data->timers_starting);
            if (InterlockedAdd(&chaos_test_data->can_start_timers, 0) != 0)
            {
                int which_timer_slot = rand() * MAX_TIMER_COUNT / (RAND_MAX + 1);
                if (InterlockedCompareExchange(&chaos_test_data->timers[which_timer_slot].state, TIMER_STATE_STARTING, TIMER_STATE_NONE) == TIMER_STATE_NONE)
                {
                    uint32_t timer_start_delay = TIMER_START_DELAY_MIN + rand() * (TIMER_START_DELAY_MAX - TIMER_START_DELAY_MIN) / (RAND_MAX + 1);
                    uint32_t timer_period = TIMER_PERIOD_MIN + rand() * (TIMER_PERIOD_MAX - TIMER_PERIOD_MIN) / (RAND_MAX + 1);
                    if (threadpool_timer_start(chaos_test_data->threadpool, timer_start_delay, timer_period, work_function, (void*)&chaos_test_data->executed_timer_functions, &chaos_test_data->timers[which_timer_slot].timer) == 0)
                    {
                        InterlockedExchange(&chaos_test_data->timers[which_timer_slot].state, TIMER_STATE_STARTED);
                    }
                    else
                    {
                        InterlockedExchange(&chaos_test_data->timers[which_timer_slot].state, TIMER_STATE_NONE);
                    }
                }
            }
            (void)InterlockedDecrement(&chaos_test_data->timers_starting);
            WakeByAddressSingle((PVOID)&chaos_test_data->timers_starting);
        }
        break;
        case TEST_ACTION_DESTROY_TIMER:
            // Stop a timer
        {
            // Synchronize with close
            (void)InterlockedIncrement(&chaos_test_data->timers_starting);
            if (InterlockedAdd(&chaos_test_data->can_start_timers, 0) != 0)
            {
                int which_timer_slot = rand() * MAX_TIMER_COUNT / (RAND_MAX + 1);
                if (InterlockedCompareExchange(&chaos_test_data->timers[which_timer_slot].state, TIMER_STATE_STOPPING, TIMER_STATE_STARTED) == TIMER_STATE_STARTED)
                {
                    threadpool_timer_destroy(chaos_test_data->timers[which_timer_slot].timer);
                    chaos_test_data->timers[which_timer_slot].timer = NULL;
                    InterlockedExchange(&chaos_test_data->timers[which_timer_slot].state, TIMER_STATE_NONE);
                }
            }
            (void)InterlockedDecrement(&chaos_test_data->timers_starting);
            WakeByAddressSingle((PVOID)&chaos_test_data->timers_starting);
        }
        break;
        case TEST_ACTION_CANCEL_TIMER:
            // Cancel a timer
        {
            // Synchronize with close
            (void)InterlockedIncrement(&chaos_test_data->timers_starting);
            if (InterlockedAdd(&chaos_test_data->can_start_timers, 0) != 0)
            {
                int which_timer_slot = rand() * MAX_TIMER_COUNT / (RAND_MAX + 1);
                if (InterlockedAdd(&chaos_test_data->timers[which_timer_slot].state, 0) == TIMER_STATE_STARTED)
                {
                    threadpool_timer_cancel(chaos_test_data->timers[which_timer_slot].timer);
                }
            }
            (void)InterlockedDecrement(&chaos_test_data->timers_starting);
            WakeByAddressSingle((PVOID)&chaos_test_data->timers_starting);
        }
        break;
        case TEST_ACTION_RESTART_TIMER:
            // Restart a timer
            {
                // Synchronize with close
                (void)InterlockedIncrement(&chaos_test_data->timers_starting);
                if (InterlockedAdd(&chaos_test_data->can_start_timers, 0) != 0)
                {
                    int which_timer_slot = rand() * MAX_TIMER_COUNT / (RAND_MAX + 1);
                    if (InterlockedCompareExchange(&chaos_test_data->timers[which_timer_slot].state, TIMER_STATE_STARTING, TIMER_STATE_STARTED) == TIMER_STATE_STARTED)
                    {
                        uint32_t timer_start_delay = TIMER_START_DELAY_MIN + rand() * (TIMER_START_DELAY_MAX - TIMER_START_DELAY_MIN) / (RAND_MAX + 1);
                        uint32_t timer_period = TIMER_PERIOD_MIN + rand() * (TIMER_PERIOD_MAX - TIMER_PERIOD_MIN) / (RAND_MAX + 1);
                        ASSERT_ARE_EQUAL(int, 0, threadpool_timer_restart(chaos_test_data->timers[which_timer_slot].timer, timer_start_delay, timer_period));
                        (void)InterlockedExchange(&chaos_test_data->timers[which_timer_slot].state, TIMER_STATE_STARTED);
                    }
                }
                (void)InterlockedDecrement(&chaos_test_data->timers_starting);
                WakeByAddressSingle((PVOID)&chaos_test_data->timers_starting);
            }
            break;
        case 7:
            //// perform a schedule work item
            //if (threadpool_schedule_work_item(chaos_test_data->threadpool, chaos_test_data->work_item_context) == 0)
            //{
            //    (void)InterlockedIncrement64(&chaos_test_data->expected_call_count);
            //}
            break;
        }
    }

    return 0;
}

TEST_FUNCTION(chaos_knight_test)
{
    // start a number of threads and each of them will do a random action on the threadpool
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    HANDLE thread_handles[CHAOS_THREAD_COUNT];
    size_t i;
    CHAOS_TEST_DATA chaos_test_data;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&chaos_test_data.threadpool, &threadpool);

    (void)InterlockedExchange64(&chaos_test_data.expected_call_count, 0);
    (void)InterlockedExchange64(&chaos_test_data.executed_work_functions, 0);
    (void)InterlockedExchange(&chaos_test_data.chaos_test_done, 0);

    for (i = 0; i < MAX_TIMER_COUNT; i++)
    {
        chaos_test_data.timers[i].timer = NULL;
        InterlockedExchange(&chaos_test_data.timers[i].state, TIMER_STATE_NONE);
    }

    // Create the Work Item Context once
    chaos_test_data.work_item_context = threadpool_create_work_item(chaos_test_data.threadpool, work_function, (void*)&chaos_test_data.executed_work_functions);

    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        thread_handles[i] = CreateThread(NULL, 0, chaos_thread_func, &chaos_test_data, 0, NULL);
        ASSERT_IS_NOT_NULL(thread_handles[i], "thread %zu failed to start", i);
    }

    // wait for some time
    Sleep(10000);

    (void)InterlockedExchange(&chaos_test_data.chaos_test_done, 1);

    // wait for all threads to complete
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        ASSERT_IS_TRUE(WaitForSingleObject(thread_handles[i], INFINITE) == WAIT_OBJECT_0);
    }

    // assert that all scheduled items were executed

    InterlockedHL_WaitForValue64(&chaos_test_data.executed_work_functions, chaos_test_data.expected_call_count, UINT32_MAX);

    LogInfo("Chaos test executed %" PRIu64 " work items",
        InterlockedAdd64(&chaos_test_data.executed_work_functions, 0));

    // call close
    threadpool_destroy_work_item(chaos_test_data.threadpool, chaos_test_data.work_item_context);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&chaos_test_data.threadpool, NULL);

    execution_engine_dec_ref(execution_engine);
}

//test used for detect race condition between timer_restart/timer_cancel and timer destory, failed due to the race condition for the current code, will uncomment after the fix
TEST_FUNCTION(chaos_knight_test_with_timers_no_lock)
{
    // start a number of threads and each of them will do a random action on the threadpool
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);
    HANDLE thread_handles[CHAOS_THREAD_COUNT];
    size_t i;
    CHAOS_TEST_DATA chaos_test_data;

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&chaos_test_data.threadpool, &threadpool);

    (void)InterlockedExchange64(&chaos_test_data.expected_call_count, 0);
    (void)InterlockedExchange64(&chaos_test_data.executed_work_functions, 0);
    (void)InterlockedExchange64(&chaos_test_data.executed_timer_functions, 0);
    (void)InterlockedExchange(&chaos_test_data.timers_starting, 0);
    (void)InterlockedExchange(&chaos_test_data.chaos_test_done, 0);
    (void)InterlockedExchange(&chaos_test_data.can_start_timers, 1);

    for (i = 0; i < MAX_TIMER_COUNT; i++)
    {
        chaos_test_data.timers[i].timer = NULL;
        (void)InterlockedExchange(&chaos_test_data.timers[i].state, TIMER_STATE_NONE);
    }

    chaos_test_data.work_item_context = threadpool_create_work_item(chaos_test_data.threadpool, work_function, (void*)&chaos_test_data.executed_work_functions);
    ASSERT_IS_NOT_NULL(chaos_test_data.work_item_context);
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        thread_handles[i] = CreateThread(NULL, 0, chaos_thread_with_timers_no_lock_func, &chaos_test_data, 0, NULL);
        ASSERT_IS_NOT_NULL(thread_handles[i], "thread %zu failed to start", i);
    }

    // wait for some time
    Sleep(TEST_RUN_TIME);

    (void)InterlockedExchange(&chaos_test_data.chaos_test_done, 1);

    //will change to use a future API: InterlockedHL_waitforvalue_64 later, task:https://msazure.visualstudio.com/One/_workitems/edit/30197585
    while (InterlockedAdd64(&chaos_test_data.expected_call_count, 0) != InterlockedAdd64(&chaos_test_data.executed_work_functions, 0))
    {
        Sleep(1);
    }

    // wait for all threads to complete
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        ASSERT_IS_TRUE(WaitForSingleObject(thread_handles[i], INFINITE) == WAIT_OBJECT_0);
    }

    // assert that all scheduled items were executed
    InterlockedHL_WaitForValue64(&chaos_test_data.executed_work_functions, chaos_test_data.expected_call_count, UINT32_MAX);

    LogInfo("Chaos test executed %" PRIu64 " work items, %" PRIu64 " timers",
        InterlockedAdd64(&chaos_test_data.executed_work_functions, 0), InterlockedAdd64(&chaos_test_data.executed_timer_functions, 0));

    // call close
    chaos_cleanup_all_timers(&chaos_test_data);
    threadpool_destroy_work_item(chaos_test_data.threadpool, chaos_test_data.work_item_context);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&chaos_test_data.threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(one_work_item_schedule_works_v2)
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)InterlockedExchange(&call_count, 0);

    // act (schedule one work item)
    LogInfo("Create Work Item Context");
    THREADPOOL_WORK_ITEM_HANDLE work_item_context = threadpool_create_work_item(threadpool, work_function, (void*)&call_count);
    ASSERT_IS_NOT_NULL(work_item_context);
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, work_item_context));

    // assert
    wait_for_equal(&call_count, 1, INFINITE);
    LogInfo("Work completed");

    // cleanup
    threadpool_destroy_work_item(threadpool, work_item_context);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(threadpool_owns_execution_engine_reference_and_can_schedule_work_v2)
{
    // arrange
    // create an execution engine
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // this is safe because the threadpool has a reference
    execution_engine_dec_ref(execution_engine);

    (void)InterlockedExchange(&call_count, 0);

    // act (schedule one work item)
    LogInfo("Create Work Item Context");
    THREADPOOL_WORK_ITEM_HANDLE work_item_context = threadpool_create_work_item(threadpool, work_function, (void*)&call_count);
    ASSERT_IS_NOT_NULL(work_item_context);
    LogInfo("Scheduling work");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));

    // assert
    wait_for_equal(&call_count, 1, INFINITE);
    LogInfo("Work completed");

    // cleanup
    threadpool_destroy_work_item(threadpool, work_item_context);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_works_v2))
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    size_t i;
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    (void)InterlockedExchange(&call_count, 0);

    LogInfo("Scheduling work " MU_TOSTRING(N_WORK_TIMES) " times");
    // act (schedule work items)
    LogInfo("Create Work Item Context");
    THREADPOOL_WORK_ITEM_HANDLE work_item_context = threadpool_create_work_item(threadpool, work_function, (void*)&call_count);
    ASSERT_IS_NOT_NULL(work_item_context);
    for (i = 0; i < N_WORK_ITEMS; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, work_item_context));
    }

    // assert
    wait_for_equal(&call_count, N_WORK_ITEMS, INFINITE);
    LogInfo("Work completed");

    // cleanup
    threadpool_destroy_work_item(threadpool, work_item_context);
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

    wait_work_context.wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(wait_work_context.wait_event);

    (void)InterlockedExchange(&wait_work_context.call_count, 0);

    // schedule one item that waits
    LogInfo("Create Work Item Context");
    THREADPOOL_WORK_ITEM_HANDLE wait_work_item_context = threadpool_create_work_item(threadpool, wait_work_function, (void*)&wait_work_context);
    ASSERT_IS_NOT_NULL(wait_work_item_context);
    THREADPOOL_WORK_ITEM_HANDLE work_item_context = threadpool_create_work_item(threadpool, work_function, (void*)&wait_work_context.call_count);
    ASSERT_IS_NOT_NULL(work_item_context);
    LogInfo("Scheduling 2 work items");
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, wait_work_item_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, work_item_context));

    Sleep(WAIT_WORK_FUNCTION_SLEEP_IN_MS);
    // call close
    LogInfo("Closing threadpool");

    // set the event, that would trigger a WAIT_OBJECT_0 if close would not wait for all items
    SetEvent(wait_work_context.wait_event);

    // assert
    wait_for_equal(&wait_work_context.call_count, 2, INFINITE);

    // cleanup
    (void)CloseHandle(wait_work_context.wait_event);
    threadpool_destroy_work_item(threadpool, wait_work_item_context);
    threadpool_destroy_work_item(threadpool, work_item_context);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

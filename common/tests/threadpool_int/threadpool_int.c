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

static volatile_atomic int32_t g_start;
static volatile_atomic int64_t g_call_count;
static volatile_atomic int32_t g_schedule_calls_count;

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
#define XTEST_FUNCTION(A) void A(void)
//diff: added this
#define WAIT_WORK_FUNCTION_SLEEP_IN_MS 300

#define THREAD_COUNT 5
#define WORK_ITEM_COUNT 32

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

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
    (void) context;
    (void)interlocked_increment_64(&g_call_count);
    wake_by_address_single_64(&g_call_count);
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
    TEST_ACTION_CLEANUP_THREADPOOL_TIMER, \
    TEST_ACTION_SCHEDULE_WORK, \
    TEST_ACTION_START_THREADPOOL_TIMER, \
    TEST_ACTION_CANCEL_THREADPOOL_TIMER, \
    TEST_ACTION_RESTART_THREADPOOL_TIMER, \
    TEST_ACTION_SCHEDULE_WORK_ITEM

MU_DEFINE_ENUM(TEST_ACTION, TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS(TEST_ACTION, TEST_ACTION_VALUES)

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
        (void)wait_on_address_64(value, current_value, (uint32_t)(timeout - (current_time - start_time)));
    } while (1);
}

static int schedule_work(void* context)
{
    THANDLE(THREADPOOL) threadpool = context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&g_start, 1, UINT32_MAX));

    for(size_t i = 0; i < WORK_ITEM_COUNT; i++)
    {
        (void)interlocked_increment(&g_schedule_calls_count);
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, NULL));
    }
    return 0;
}

TEST_FUNCTION(schedule_work_from_multiple_threads)
{
    // arrange
    LogError("STARTING TEST");

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    THREAD_HANDLE thread_handles[THREAD_COUNT];

    (void)interlocked_exchange(&g_start, 0);
    (void)interlocked_exchange_64(&g_call_count, 0);
    (void)interlocked_exchange(&g_schedule_calls_count, 0);

    // act
    for(size_t i = 0; i < THREAD_COUNT; i++)
    {
        ThreadAPI_Create(&thread_handles[i], schedule_work, (void*)threadpool);
    }

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&g_start, 1));

    LogError("JOINING THREADS");
    for(size_t i = 0; i < THREAD_COUNT; i++)
    {
        int thread_result;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread_handles[i], &thread_result));
        ASSERT_ARE_EQUAL(int, 0, thread_result);
    }
    LogError("JOINED THREADS");

    INTERLOCKED_HL_RESULT wait_res = InterlockedHL_WaitForValue64(&g_call_count, THREAD_COUNT*WORK_ITEM_COUNT, 15000);

    LogError("Schedule Calls: %" PRId32 " CALL COUNT %" PRId64 "", interlocked_add(&g_schedule_calls_count,0), interlocked_add_64(&g_call_count,0));

    dump(threadpool);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, wait_res);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

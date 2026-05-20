// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>


#include "testrunnerswitcher.h"

#include "c_pal/threadapi.h"

#include "c_pal/interlocked_hl.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/timer.h"
#include "c_pal/execution_engine.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"
#include "c_pal/gballoc_hl.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

volatile_atomic int32_t globalValue = 10;
volatile_atomic int64_t globalValue64 = 10;
/*
Tests:
InterlockedHL_WaitForValue
InterlockedHL_DecrementAndWake
*/
static int decrement_and_wake_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&globalValue, 9, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_decrement_and_wake_operates_successfully)
{
    // + have a helper thread which waits for decremented value
    // + main thread creates helper thread and decrements value
    // + helper thread wakes up when the global address value is decremented by 1
    // arrange
    globalValue = 10;
    THREAD_HANDLE helper_thread_handle;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread_handle, decrement_and_wake_helper_thread_function, NULL));

    // act and assert
    // sleep so that the helper thread can go into wait mode
    ThreadAPI_Sleep(5000);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_DecrementAndWake(&globalValue));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 9, globalValue);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread_handle, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForValue64
InterlockedHL_DecrementAndWake64
*/
static int decrement_and_wake_helper_thread_function_64(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&globalValue64, 9, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_decrement_and_wake_operates_successfully_64)
{
    // + have a helper thread which waits for decremented value
    // + main thread creates helper thread and decrements value
    // + helper thread wakes up when the global address value is decremented by 1
    // arrange
    globalValue64 = 10;
    THREAD_HANDLE helper_thread_handle;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread_handle, decrement_and_wake_helper_thread_function_64, NULL));

    // act and assert
    // sleep so that the helper thread can go into wait mode
    ThreadAPI_Sleep(5000);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_DecrementAndWake64(&globalValue64));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 9, globalValue64);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread_handle, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForValue
InterlockedHL_SetAndWakeAll
*/
static int set_and_wake_all_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&globalValue, 15, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_set_and_wake_all_operates_successfully)
{
    // + create 10 helper threads which wait on a value
    // + main thread creates helper threads and sets value for the helper threads to wake up
    // + main thread joins on the helper threads and ensures that all threads wake up and terminate
    // arrange
    globalValue = 10;

    enum { NUMBER_OF_HELPER_THREADS = 10 };
    THREAD_HANDLE helper_threads[NUMBER_OF_HELPER_THREADS];

    for (uint32_t i = 0; i < NUMBER_OF_HELPER_THREADS; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_threads[i], set_and_wake_all_helper_thread_function, NULL));
    }

    // wait time so that all helper threads go into wait mode
    ThreadAPI_Sleep(5000);

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&globalValue, 15));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 15, globalValue);

    // cleanup
    for (uint32_t i = 0; i < NUMBER_OF_HELPER_THREADS; ++i)
    {
        int return_code;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_threads[i], &return_code));
        ASSERT_ARE_EQUAL(int, 0, return_code);
    }
}

/*
Tests:
InterlockedHL_WaitForValue64
InterlockedHL_SetAndWakeAll64
*/
static int set_and_wake_all_helper_thread_function_64(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&globalValue64, 15, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_set_and_wake_all_operates_successfully_64)
{
    // + create 10 helper threads which wait on a value
    // + main thread creates helper threads and sets value for the helper threads to wake up
    // + main thread joins on the helper threads and ensures that all threads wake up and terminate
    // arrange
    globalValue64 = INT64_MAX;

    enum { NUMBER_OF_HELPER_THREADS = 10 };
    THREAD_HANDLE helper_threads[NUMBER_OF_HELPER_THREADS];

    for (uint32_t i = 0; i < NUMBER_OF_HELPER_THREADS; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_threads[i], set_and_wake_all_helper_thread_function_64, NULL));
    }

    // wait time so that all helper threads go into wait mode
    ThreadAPI_Sleep(5000);

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll64(&globalValue64, 15));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 15, globalValue64);

    // cleanup
    for (uint32_t i = 0; i < NUMBER_OF_HELPER_THREADS; ++i)
    {
        int return_code;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_threads[i], &return_code));
        ASSERT_ARE_EQUAL(int, 0, return_code);
    }
}

/*
Tests:
InterlockedHL_WaitForValue
InterlockedHL_SetAndWake
*/
static int set_and_wake_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&globalValue, 20, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_set_and_wake_operates_successfully)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets a value and wakes up the helper thread
    // + helper thread return from wait as the value it is waiting on is same
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue = 10;

    THREAD_HANDLE helper_thread;

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread, set_and_wake_helper_thread_function, NULL));

    // wait time so that all helper thread goes into wait mode
    ThreadAPI_Sleep(5000);

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&globalValue, 20));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 20, globalValue);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForValue64
InterlockedHL_SetAndWake64
*/
static int set_and_wake_helper_thread_function_64(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&globalValue64, 20, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_set_and_wake_operates_successfully_64)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets a value and wakes up the helper thread
    // + helper thread return from wait as the value it is waiting on is same
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue64 = 10;

    THREAD_HANDLE helper_thread;

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread, set_and_wake_helper_thread_function_64, NULL));

    // wait time so that all helper thread goes into wait mode
    ThreadAPI_Sleep(5000);

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake64(&globalValue64, 20));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 20, globalValue64);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForNotValue
InterlockedHL_SetAndWake
*/
static int wait_for_not_value_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForNotValue(&globalValue, 25, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_wait_for_not_value_operates_successfully)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets a different value and wakes up the helper thread
    // + helper thread returns from wait as the value changed
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue = 25;

    THREAD_HANDLE helper_thread;

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread, wait_for_not_value_helper_thread_function, NULL));

    // wait time so that all helper thread goes into wait mode as the value is 25
    ThreadAPI_Sleep(5000);

    // act and assert
    //set and wake up helper thread
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&globalValue, 30));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 30, globalValue);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForNotValue64
InterlockedHL_SetAndWake64
*/
static int wait_for_not_value_helper_thread_function_64(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForNotValue64(&globalValue64, 25, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_wait_for_not_value_operates_successfully_64)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets a different value and wakes up the helper thread
    // + helper thread returns from wait as the value changed
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue64 = 25;

    THREAD_HANDLE helper_thread;

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread, wait_for_not_value_helper_thread_function_64, NULL));

    // wait time so that all helper thread goes into wait mode as the value is 25
    ThreadAPI_Sleep(5000);

    // act and assert
    //set and wake up helper thread
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake64(&globalValue64, 30));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 30, globalValue64);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}
/*
Tests:
InterlockedHL_WaitForNotValue
*/
TEST_FUNCTION(interlocked_hl_wait_for_not_value_times_out_returns_time_out)
{
    // + ensure that when the InterlockedHL_WaitForNotValue call times out it returns the correct time out value
    globalValue = 25;

    // act and assert
    // Wait a second for the value to time out and make sure it returns the correct value
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, InterlockedHL_WaitForNotValue(&globalValue, 25, 1000));

    // cleanup
}

/*
Tests:
InterlockedHL_WaitForNotValue64
*/
TEST_FUNCTION(interlocked_hl_wait_for_not_value_64)
{
    volatile_atomic int64_t localvalue = 0;

    (void)interlocked_exchange_64(&localvalue, 25);

    // act and assert
    // Wait a second for the value to time out and make sure it returns the correct value
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, InterlockedHL_WaitForNotValue64(&localvalue, 25, 1000));

    (void)interlocked_exchange_64(&localvalue, 100);

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForNotValue64(&localvalue, 25, 1000));
    // cleanup
}

/*
Tests:
InterlockedHL_WaitForValue
*/
TEST_FUNCTION(interlocked_hl_wait_for_value_times_out_returns_time_out)
{
    // + ensure that when the InterlockedHL_WaitForValue call times out it returns the correct time out value
    globalValue = 25;

    // act and assert
    // Wait a second for the value to time out and make sure it returns the correct value
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, InterlockedHL_WaitForValue(&globalValue, 1, 1000));

    // cleanup
}

/*
Tests:
InterlockedHL_WaitForValue64
*/
TEST_FUNCTION(interlocked_hl_wait_for_value_timest)
{
    volatile_atomic int64_t localvalue = 0;

    (void)interlocked_exchange_64(&localvalue, 25);

    // act and assert
    // Wait a second for the value to time out and make sure it returns the correct value
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, InterlockedHL_WaitForValue64(&localvalue, 1, 1000));

    (void)interlocked_exchange_64(&localvalue, 1);

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue64(&localvalue, 1, 1000));

    // cleanup
}

/*
Tests:
InterlockedHL_WaitForValue timeout with non-matching value returns TIMEOUT and does not hang
*/
TEST_FUNCTION(interlocked_hl_wait_for_value_timeout_does_not_hang)
{
    ///arrange
    volatile_atomic int32_t localvalue = 0;
    (void)interlocked_exchange(&localvalue, 42);

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForValue(&localvalue, 99, 500);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}

/*
Tests:
InterlockedHL_WaitForValue64 timeout with non-matching value returns TIMEOUT and does not hang
*/
TEST_FUNCTION(interlocked_hl_wait_for_value_64_timeout_does_not_hang)
{
    ///arrange
    volatile_atomic int64_t localvalue = 0;
    (void)interlocked_exchange_64(&localvalue, 42);

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForValue64(&localvalue, 99, 500);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}

/*
Tests:
InterlockedHL_WaitForNotValue timeout with matching value returns TIMEOUT and does not hang
*/
TEST_FUNCTION(interlocked_hl_wait_for_not_value_timeout_does_not_hang)
{
    ///arrange
    volatile_atomic int32_t localvalue = 0;
    (void)interlocked_exchange(&localvalue, 42);

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForNotValue(&localvalue, 42, 500);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}

/*
Tests:
InterlockedHL_WaitForNotValue64 timeout with matching value returns TIMEOUT and does not hang
*/
TEST_FUNCTION(interlocked_hl_wait_for_not_value_64_timeout_does_not_hang)
{
    ///arrange
    volatile_atomic int64_t localvalue = 0;
    (void)interlocked_exchange_64(&localvalue, 42);

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForNotValue64(&localvalue, 42, 500);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
}

/*
Tests:
InterlockedHL_WaitForValue with spurious wakeups returns TIMEOUT within bounded time.
A helper thread sends wake_by_address_single every 50ms without changing the value.
Without the elapsed-time tracking fix, each spurious wakeup would reset the full timeout,
causing the total wait to be N * timeout_ms instead of ~timeout_ms.
*/

typedef struct SPURIOUS_WAKEUP_CONTEXT_TAG
{
    volatile_atomic int32_t value;
    volatile_atomic int32_t should_stop;
} SPURIOUS_WAKEUP_CONTEXT;

static int spurious_wakeup_thread_func(void* context)
{
    SPURIOUS_WAKEUP_CONTEXT* ctx = (SPURIOUS_WAKEUP_CONTEXT*)context;
    while (interlocked_add(&ctx->should_stop, 0) == 0)
    {
        wake_by_address_single(&ctx->value);
        ThreadAPI_Sleep(50);
    }
    return 0;
}

TEST_FUNCTION(interlocked_hl_wait_for_value_with_spurious_wakeups_returns_timeout_in_bounded_time)
{
    ///arrange
    SPURIOUS_WAKEUP_CONTEXT ctx;
    (void)interlocked_exchange(&ctx.value, 42);
    (void)interlocked_exchange(&ctx.should_stop, 0);

    THREAD_HANDLE wakeup_thread;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&wakeup_thread, spurious_wakeup_thread_func, &ctx));

    // let the wakeup thread start sending spurious wakeups
    ThreadAPI_Sleep(100);

    double start_ms = timer_global_get_elapsed_ms();

    ///act
    // timeout = 1000ms, value will never match (42 != 99)
    // helper thread sends spurious wakeups every 50ms
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForValue(&ctx.value, 99, 1000);

    double elapsed_ms = timer_global_get_elapsed_ms() - start_ms;

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_TIMEOUT, result);
    // with proper timeout accounting, elapsed should be ~1000ms
    // without the fix, ~20 spurious wakeups would each reset the full 1000ms timeout -> ~20000ms
    // 5000ms (5x) is a generous upper bound to avoid flakiness
    ASSERT_IS_TRUE(elapsed_ms < 5000.0);

    ///cleanup
    (void)interlocked_exchange(&ctx.should_stop, 1);
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(wakeup_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_Add64WithCeiling
*/
TEST_FUNCTION(interlocked_hl_add64_with_ceiling_operates_successfully)
{
    // + tests the trivial test case of adding a value to 64bit integer
    // arrange
    volatile_atomic int64_t addend = 55;
    int64_t original_addend = 1;
    const int64_t CEILING = 100;
    int64_t value = 20;

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_Add64WithCeiling(&addend, CEILING, value, &original_addend));
    ASSERT_ARE_EQUAL(int64_t, 55, original_addend);
    ASSERT_ARE_EQUAL(int64_t, 75, addend);
}

/*
Tests:
InterlockedHL_CompareExchangeIf
*/
static bool helper_int32_compare_function(int32_t target, int32_t exchange)
{
    //always returns true
    (void)target;
    (void)exchange;
    return true;
}

TEST_FUNCTION(interlocked_hl_compare_exchange_if_operates_successfully)
{
    // + tests the trivial test case of successfully exchanging a value into target
    // arrange
    volatile_atomic int32_t target = 60;
    int32_t original_target = 1;
    int32_t exchange = 88;

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_CompareExchangeIf(&target, exchange, helper_int32_compare_function, &original_target));
    ASSERT_ARE_EQUAL(int32_t, 60, original_target);
    ASSERT_ARE_EQUAL(int32_t, 88, target);
}

/*
Tests:
InterlockedHL_CompareExchange64If
*/
static bool helper_int64_compare_function(int64_t target, int64_t exchange)
{
    //always returns true
    (void)target;
    (void)exchange;
    return true;
}

TEST_FUNCTION(interlocked_hl_compare_exchange_64_if_operates_successfully)
{
    // + tests the trivial test case of successfully exchanging a value into target
    // arrange
    volatile_atomic int64_t target = 120;
    int64_t original_target = 1;
    int64_t exchange = 97;

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_CompareExchange64If(&target, exchange, helper_int64_compare_function, &original_target));
    ASSERT_ARE_EQUAL(int64_t, 120, original_target);
    ASSERT_ARE_EQUAL(int64_t, 97, target);
}

/*
Tests:
Maximum-noise reproduction of helgrind false positive from zrpc build 164315648.

The zrpc tcp_io_client_sends_1_byte_with_threading test creates a complex threading
environment: 2 UV loop collections (each with background threads), TCP socket operations,
and 8+ InterlockedHL_WaitForValue/SetAndWake pairs across different variables. The actual
race is between v2_test_helper_execute_on_loop_sync (WaitForValue on the test thread) and
internal_v2_test_helper_execute_sync_callback (SetAndWake on a UV loop thread).

This generates massive helgrind shadow state causing false race reports on the C11 atomics
in interlocked_add (WaitForValue) and interlocked_exchange (SetAndWake).

This test replicates that noise level by:
1. Running 8 noise threads doing rapid interlocked operations on 4 variables each (32 total)
2. Using a real threadpool with 8 workers
3. Running 4 concurrent WaitForValue/SetAndWake channels per iteration (200 iterations)
4. Total: 800 WaitForValue/SetAndWake pairs while 8 noise threads saturate helgrind
*/

#define NOISE_THREAD_COUNT 8
#define NOISE_VARS_PER_THREAD 4
#define HELGRIND_REPRO_ITERATIONS 200

typedef struct NOISE_CONTEXT_TAG
{
    volatile_atomic int32_t counters[NOISE_VARS_PER_THREAD];
    volatile_atomic int32_t should_stop;
} NOISE_CONTEXT;

static int noise_thread_func(void* context)
{
    NOISE_CONTEXT* ctx = (NOISE_CONTEXT*)context;
    while (interlocked_add(&ctx->should_stop, 0) == 0)
    {
        for (int i = 0; i < NOISE_VARS_PER_THREAD; i++)
        {
            (void)interlocked_increment(&ctx->counters[i]);
            (void)interlocked_compare_exchange(&ctx->counters[i], 0, 100);
            (void)interlocked_exchange(&ctx->counters[i], interlocked_add(&ctx->counters[i], 0) + 1);
        }
    }
    return 0;
}

typedef struct MULTI_SIGNAL_CONTEXT_TAG
{
    volatile_atomic int32_t signals[4];
} MULTI_SIGNAL_CONTEXT;

static void set_signal_0_callback(void* context)
{
    MULTI_SIGNAL_CONTEXT* ctx = (MULTI_SIGNAL_CONTEXT*)context;
    (void)InterlockedHL_SetAndWake(&ctx->signals[0], 1);
}

static void set_signal_1_callback(void* context)
{
    MULTI_SIGNAL_CONTEXT* ctx = (MULTI_SIGNAL_CONTEXT*)context;
    (void)InterlockedHL_SetAndWake(&ctx->signals[1], 1);
}

static void set_signal_2_callback(void* context)
{
    MULTI_SIGNAL_CONTEXT* ctx = (MULTI_SIGNAL_CONTEXT*)context;
    (void)InterlockedHL_SetAndWake(&ctx->signals[2], 1);
}

static void set_signal_3_callback(void* context)
{
    MULTI_SIGNAL_CONTEXT* ctx = (MULTI_SIGNAL_CONTEXT*)context;
    (void)InterlockedHL_SetAndWake(&ctx->signals[3], 1);
}

TEST_FUNCTION(interlocked_hl_helgrind_max_noise_repro)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS params;
    params.min_thread_count = 8;
    params.max_thread_count = 8;

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);

    // Start 8 noise generators doing rapid interlocked operations on 32 variables
    // to saturate helgrind's shadow state tracking — matching the noise level
    // created by zrpc's UV loop threads, TCP I/O, and THANDLE ref-counting
    NOISE_CONTEXT noise_contexts[NOISE_THREAD_COUNT];
    THREAD_HANDLE noise_threads[NOISE_THREAD_COUNT];
    for (uint32_t i = 0; i < NOISE_THREAD_COUNT; i++)
    {
        for (int j = 0; j < NOISE_VARS_PER_THREAD; j++)
        {
            (void)interlocked_exchange(&noise_contexts[i].counters[j], 0);
        }
        (void)interlocked_exchange(&noise_contexts[i].should_stop, 0);
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK,
            ThreadAPI_Create(&noise_threads[i], noise_thread_func, &noise_contexts[i]));
    }

    // Let noise build up helgrind shadow state before starting the actual test
    ThreadAPI_Sleep(50);

    ///act
    // Run many iterations with 4 concurrent WaitForValue/SetAndWake pairs per iteration.
    // This matches zrpc's pattern where the test thread calls WaitForValue on multiple
    // signal variables (server_open_complete, client_open_complete, send_complete,
    // data_received) while UV loop threads call SetAndWake from callbacks.
    for (uint32_t iter = 0; iter < HELGRIND_REPRO_ITERATIONS; iter++)
    {
        MULTI_SIGNAL_CONTEXT ctx;
        (void)interlocked_exchange(&ctx.signals[0], 0);
        (void)interlocked_exchange(&ctx.signals[1], 0);
        (void)interlocked_exchange(&ctx.signals[2], 0);
        (void)interlocked_exchange(&ctx.signals[3], 0);

        // Schedule all 4 concurrently on threadpool workers
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, set_signal_0_callback, &ctx));
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, set_signal_1_callback, &ctx));
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, set_signal_2_callback, &ctx));
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, set_signal_3_callback, &ctx));

        // Main thread waits for all 4 — each WaitForValue races with a SetAndWake
        // on a different threadpool worker, same pattern as zrpc
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK,
            InterlockedHL_WaitForValue(&ctx.signals[0], 1, UINT32_MAX));
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK,
            InterlockedHL_WaitForValue(&ctx.signals[1], 1, UINT32_MAX));
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK,
            InterlockedHL_WaitForValue(&ctx.signals[2], 1, UINT32_MAX));
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK,
            InterlockedHL_WaitForValue(&ctx.signals[3], 1, UINT32_MAX));
    }

    ///assert
    // All 800 WaitForValue/SetAndWake pairs succeeded

    ///cleanup
    for (uint32_t i = 0; i < NOISE_THREAD_COUNT; i++)
    {
        (void)interlocked_exchange(&noise_contexts[i].should_stop, 1);
        int return_code;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK,
            ThreadAPI_Join(noise_threads[i], &return_code));
        ASSERT_ARE_EQUAL(int, 0, return_code);
    }

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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
InterlockedHL_WaitForNotValue64

Regression test for a Linux-only bug where InterlockedHL_WaitForNotValue64 can
have a lost-wakeup if a concurrent writer changes only the upper 32 bits of the
64-bit value. The function reads the full 64-bit value via interlocked_add_64,
then if it equals value_to_wait, calls wait_on_address_64. On Linux, the futex
syscall used by wait_on_address_64 only compares the lower 32 bits, so when the
upper 32 bits change between the read and the syscall (and the lower 32 bits
remain equal to compare_value's lower 32 bits), the kernel sleeps instead of
returning immediately.

The multiplexer integration tests hit this race because SUBSTREAM_IDs are
64-bit values whose lower 32 bits are an index. The first substream has
index = 0, so the lower 32 bits of its SUBSTREAM_ID match the initial sentinel
value 0, even though the upper 32 bits are non-zero.

This test deterministically reproduces the race by:
  1. Reading the value (mimicking InterlockedHL_WaitForNotValue64's first step)
     and confirming it equals value_to_wait.
  2. Changing the upper 32 bits of the value (simulating a writer that wins the
     race).
  3. Calling wait_on_address_64 with the originally-read value as compare_value
     (mimicking InterlockedHL_WaitForNotValue64's second step).
  4. Asserting that wait_on_address_64 returns OK promptly per the
     wait_on_address contract (SRS_SYNC_43_002).
*/
TEST_FUNCTION(interlocked_hl_wait_for_not_value_64_with_only_upper_32_bits_change)
{
    // arrange
    volatile_atomic int64_t value;
    (void)interlocked_exchange_64(&value, 0);
    int64_t value_to_wait = 0;
    int32_t timeout_ms = 5000;
    double tolerance_factor = 0.1;

    // Step 1 of InterlockedHL_WaitForNotValue64: read value with interlocked_add_64.
    int64_t current_value = interlocked_add_64(&value, 0);
    ASSERT_ARE_EQUAL(int64_t, value_to_wait, current_value, "test setup error: current_value must equal value_to_wait so that wait_on_address_64 is reached");

    // Simulate the race window: a concurrent writer changes ONLY the upper 32 bits
    // of the value before the (about-to-execute) wait_on_address_64 syscall.
    // The lower 32 bits remain equal to current_value's lower 32 bits.
    int64_t new_value = 0x100000000LL;
    (void)interlocked_exchange_64(&value, new_value);

    // act
    // Step 2 of InterlockedHL_WaitForNotValue64: call wait_on_address_64 with the
    // original current_value as compare_value. Per SRS_SYNC_43_002 this must return
    // immediately because *value (0x100000000) != current_value (0).
    double start_time = timer_global_get_elapsed_ms();
    WAIT_ON_ADDRESS_RESULT wait_result = wait_on_address_64(&value, current_value, timeout_ms);
    double time_elapsed = timer_global_get_elapsed_ms() - start_time;

    // Step 3 of InterlockedHL_WaitForNotValue64: re-read and translate to INTERLOCKED_HL_RESULT.
    INTERLOCKED_HL_RESULT hl_result;
    int64_t reread_value = interlocked_add_64(&value, 0);
    if (reread_value != value_to_wait)
    {
        hl_result = INTERLOCKED_HL_OK;
    }
    else if (wait_result == WAIT_ON_ADDRESS_TIMEOUT)
    {
        hl_result = INTERLOCKED_HL_TIMEOUT;
    }
    else
    {
        hl_result = INTERLOCKED_HL_ERROR;
    }

    // assert
    // The final hl_result will be INTERLOCKED_HL_OK even when the bug is present
    // (because the value was changed, so the post-wait re-read sees it). The bug
    // shows up in the time_elapsed check: the function should return promptly,
    // not after the full 5 second timeout.
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, wait_result,
        "wait_on_address_64 must return WAIT_ON_ADDRESS_OK when *value (0x%" PRIx64 ") != compare_value (0x%" PRIx64 "). It returned after %lf ms.",
        (uint64_t)new_value, (uint64_t)current_value, time_elapsed);
    ASSERT_IS_TRUE(time_elapsed < timeout_ms * tolerance_factor,
        "InterlockedHL_WaitForNotValue64 (simulated) took too long: %lf ms (max expected %lf ms). The lost-wakeup bug likely caused wait_on_address_64 to sleep until timeout.",
        time_elapsed, timeout_ms * tolerance_factor);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, hl_result);
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

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

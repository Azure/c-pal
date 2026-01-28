//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <inttypes.h>

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/interlocked.h"
#include "c_pal/threadapi.h"
#include "c_pal/sync.h"
#include "c_pal/timer.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);

static int increment_on_odd_values(void* address)
{
    volatile_atomic int32_t* ptr = (volatile_atomic int32_t*)address;
    while (interlocked_add(ptr, 0) < 98)
    {
        int32_t value = interlocked_add(ptr, 0);
        while (value % 2 != 1)
        {
            ASSERT_IS_TRUE(wait_on_address(ptr, value, UINT32_MAX));
            value = interlocked_add(ptr, 0);
        }
        (void)interlocked_increment(ptr);
        wake_by_address_all(ptr);
    }

    return 0;
}

static int increment_on_odd_values_64(void* address)
{
    volatile_atomic int64_t* ptr = (volatile_atomic int64_t*)address;
    while (interlocked_add_64(ptr, 0) < 98)
    {
        int64_t value = interlocked_add_64(ptr, 0);
        while (value % 2 != 1)
        {
            ASSERT_IS_TRUE(wait_on_address_64(ptr, value, UINT32_MAX));
            value = interlocked_add_64(ptr, 0);
        }
        (void)interlocked_increment_64(ptr);
        wake_by_address_all_64(ptr);
    }

    return 0;
}

static int increment_on_even_values(void* address)
{
    volatile_atomic int32_t* ptr = (volatile_atomic int32_t*)address;
    while(interlocked_add(ptr, 0) < 99)
    {
        int value = interlocked_add(ptr, 0);
        while(value % 2 != 0)
        {
            ASSERT_IS_TRUE(wait_on_address(ptr, value, UINT32_MAX));
            value = interlocked_add(ptr, 0);
        }
        (void)interlocked_increment(ptr);
        wake_by_address_all(ptr);
    }

    return 0;
}

static int increment_on_even_values_64(void* address)
{
    volatile_atomic int64_t* ptr = (volatile_atomic int64_t*)address;
    while (interlocked_add_64(ptr, 0) < 99)
    {
        int64_t value = interlocked_add_64(ptr, 0);
        while (value % 2 != 0)
        {
            ASSERT_IS_TRUE(wait_on_address_64(ptr, value, UINT32_MAX));
            value = interlocked_add_64(ptr, 0);
        }
        (void)interlocked_increment_64(ptr);
        wake_by_address_all_64(ptr);
    }

    return 0;
}

static volatile_atomic int32_t create_count;
static volatile_atomic int32_t woken_threads;
static int increment_on_wake_up(void* address)
{
    volatile_atomic int32_t* ptr = (volatile_atomic int32_t*)address;
    int32_t value = interlocked_add(ptr, 0);
    (void)interlocked_increment(&create_count);
    wake_by_address_single(&create_count);

    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, wait_on_address(ptr, value, UINT32_MAX));
    (void)interlocked_increment(&woken_threads);

    return 0;
}

static volatile_atomic int64_t create_count_64;
static volatile_atomic int64_t woken_threads_64;
static int increment_on_wake_up_64(void* address)
{
    volatile_atomic int64_t* ptr = (volatile_atomic int64_t*)address;
    int64_t value = interlocked_add_64(ptr, 0);
    (void)interlocked_increment_64(&create_count_64);
    wake_by_address_single_64(&create_count_64);

    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, wait_on_address_64(ptr, value, UINT32_MAX));
    (void)interlocked_increment_64(&woken_threads_64);

    return 0;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(a)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(b)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(c)
{
}

TEST_FUNCTION_CLEANUP(d)
{
}

/* Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.] */
/* Tests_SRS_SYNC_43_007: [ If *address is equal to *compare_address, wait_on_address shall cause the thread to sleep. ] */
/* Tests_SRS_SYNC_43_008: [wait_on_address shall wait indefinitely until it is woken up by a call to wake_by_address_[single/all] if timeout_ms is equal to UINT32_MAX] */
/* Tests_SRS_SYNC_43_003: [ wait_on_address shall wait until another thread in the same process signals at address using wake_by_address_[single/all] and return true. ] */
TEST_FUNCTION(two_threads_increment_alternately)
{
    //arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    THREAD_HANDLE thread1;
    THREAD_HANDLE thread2;

    //act
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread1, increment_on_even_values, (void*)&var));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread2, increment_on_odd_values, (void*)&var));

    //assert
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread1, NULL), "ThreadAPI_Join did not work");
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread2, NULL), "ThreadAPI_Join did not work");
    ASSERT_ARE_EQUAL(int32_t, 99, interlocked_add(&var, 0), "Threads did not increment value expected number of times.");
}

TEST_FUNCTION(two_threads_increment_alternately_64)
{
    //arrange
    volatile_atomic int64_t var;
    (void)interlocked_exchange_64(&var, 0);
    THREAD_HANDLE thread1;
    THREAD_HANDLE thread2;

    //act
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread1, increment_on_even_values_64, (void*)&var));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread2, increment_on_odd_values_64, (void*)&var));

    //assert
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread1, NULL), "ThreadAPI_Join did not work");
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread2, NULL), "ThreadAPI_Join did not work");
    ASSERT_ARE_EQUAL(int64_t, 99, interlocked_add_64(&var, 0), "Threads did not increment value expected number of times.");
}

/* Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.] */
/* Tests_SRS_SYNC_43_007: [ If *address is equal to *compare_address, wait_on_address shall cause the thread to sleep. ] */
/* Tests_SRS_SYNC_43_008: [wait_on_address shall wait indefinitely until it is woken up by a call to wake_by_address_[single/all] if timeout_ms is equal to UINT32_MAX] */
/* Tests_SRS_SYNC_43_003: [ wait_on_address shall wait until another thread in the same process signals at address using wake_by_address_[single/all] and return true. ] */
/* Tests_SRS_SYNC_43_004: [ wake_by_address_all shall cause all the thread(s) waiting on a call to wait_on_address with argument address to continue execution. ] */
/* Tests_SRS_SYNC_43_005: [ wake_by_address_single shall cause one thread waiting on a call to wait_on_address with argument address to continue execution. ] */
TEST_FUNCTION(wake_up_all_threads)
{
    //arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    (void)interlocked_exchange(&create_count, 0);
    THREAD_HANDLE threads[100];

    (void)interlocked_exchange(&woken_threads, 0);

    //act
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[i], increment_on_wake_up, (void*)&var));
    }

    LogInfo("Waiting for threads to spin");
    int32_t current_create_count = interlocked_add(&create_count, 0);
    while (current_create_count < 100)
    {
        ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, wait_on_address(&create_count, current_create_count, UINT32_MAX));
        current_create_count = interlocked_add(&create_count, 0);
    }

    // have a cycle of sleep and wake by address all
    // we want to check that we wake up more than one thread most of the times as it cannot be make predictable when the threads run
    // ideally we'd have all threads woken by one call, but it can so happen that not all threads end up in their
    // wait at the time we call the first wake_by_address_all
    int32_t wake_by_address_all_call_count = 0;
    do
    {
        // sleep 1s
        ThreadAPI_Sleep(1000);
        wake_by_address_all_call_count++;

        // wake all threads
        wake_by_address_all(&var);
    } while (interlocked_add(&woken_threads, 0) < 100);

    // assert
    LogInfo("Joining threads");
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[i], NULL), "ThreadAPI_Join did not work");
    }
}

TEST_FUNCTION(wake_up_all_threads_64)
{
    //arrange
    volatile_atomic int64_t var;
    (void)interlocked_exchange_64(&var, 0);
    (void)interlocked_exchange_64(&create_count_64, 0);
    THREAD_HANDLE threads[100];

    (void)interlocked_exchange_64(&woken_threads_64, 0);

    //act
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[i], increment_on_wake_up_64, (void*)&var));
    }

    LogInfo("Waiting for threads to spin");
    int64_t current_create_count = interlocked_add_64(&create_count_64, 0);
    while (current_create_count < 100)
    {
        ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, wait_on_address_64(&create_count_64, current_create_count, UINT32_MAX));
        current_create_count = interlocked_add_64(&create_count_64, 0);
    }

    // have a cycle of sleep and wake by address all
    // we want to check that we wake up more than one thread most of the times as it cannot be make predictable when the threads run
    // ideally we'd have all threads woken by one call, but it can so happen that not all threads end up in their
    // wait at the time we call the first wake_by_address_all_64
    int32_t wake_by_address_all_call_count = 0;
    do
    {
        // sleep 1s
        ThreadAPI_Sleep(1000);
        wake_by_address_all_call_count++;

        // wake all threads
        wake_by_address_all_64(&var);
    } while (interlocked_add_64(&woken_threads_64, 0) < 100);

    // assert
    LogInfo("Joining threads");
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[i], NULL), "ThreadAPI_Join did not work");
    }
}


/* Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.] */
/* Tests_SRS_SYNC_43_002: [ wait_on_address shall immediately return true if *address is not equal to *compare_address.] */
TEST_FUNCTION(wait_on_address_returns_immediately)
{
    //arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    int value = 1;

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, value, UINT32_MAX);

    //assert
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address should have returned ok");
}

TEST_FUNCTION(wait_on_address_64_returns_immediately)
{
    //arrange
    volatile_atomic int64_t var;
    (void)interlocked_exchange_64(&var, 0);
    int64_t value = 1;

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, value, UINT32_MAX);

    //assert
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address_64 should have returned ok");
}

/* Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.] */
/* Tests_SRS_SYNC_43_002: [ wait_on_address shall immediately return true if *address is not equal to *compare_address.] */
/* Tests_SRS_SYNC_43_009: [ If timeout_ms milliseconds elapse, wait_on_address shall return false. ] */
TEST_FUNCTION(wait_on_address_returns_after_timeout_elapses)
{
    //arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    int value = 0;
    int timeout = 1000;
    double tolerance_factor = 1.5;

    // act
    double start_time = timer_global_get_elapsed_ms();
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, value, timeout);
    double time_elapsed = timer_global_get_elapsed_ms() - start_time;

    //assert
    ASSERT_IS_TRUE(time_elapsed < timeout* tolerance_factor, "Too much time elapsed. Maximum Expected: %lf, Actual: %lf", timeout*tolerance_factor, time_elapsed);
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_TIMEOUT, return_val, "wait_on_address should have returned timeout");
}

TEST_FUNCTION(wait_on_address_64_returns_after_timeout_elapses)
{
    //arrange
    volatile_atomic int64_t var;
    (void)interlocked_exchange_64(&var, 0);
    int64_t value = 0;
    int32_t timeout = 1000;
    double tolerance_factor = 1.5;

    // act
    double start_time = timer_global_get_elapsed_ms();
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, value, timeout);
    double time_elapsed = timer_global_get_elapsed_ms() - start_time;

    //assert
    ASSERT_IS_TRUE(time_elapsed < timeout * tolerance_factor, "Too much time elapsed. Maximum Expected: %lf, Actual: %lf", timeout * tolerance_factor, time_elapsed);
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_TIMEOUT, return_val, "wait_on_address_64 should have returned timeout");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

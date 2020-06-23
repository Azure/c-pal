//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include "testrunnerswitcher.h"
#include "interlocked.h"
#include "threadapi.h"
#include "sync.h"
#include "timer.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

static TEST_MUTEX_HANDLE g_testByTest;

static int increment_on_odd_values(void* address)
{
    volatile_atomic int32_t* ptr = (volatile_atomic int32_t*)address;
    while (interlocked_add(ptr, 0) < 98)
    {
        int32_t value = interlocked_add(ptr, 0);
        while (value % 2 != 1)
        {
            ASSERT_IS_TRUE(wait_on_address(ptr, &value, UINT32_MAX));
            value = interlocked_add(ptr, 0);
        }
        (void)interlocked_increment(ptr);
        wake_by_address_all(ptr);
    }
    ThreadAPI_Exit(0);
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
            ASSERT_IS_TRUE(wait_on_address(ptr, &value, UINT32_MAX));
            value = interlocked_add(ptr, 0);
        }
        (void)interlocked_increment(ptr);
        wake_by_address_all(ptr);
    }
    ThreadAPI_Exit(0);
    return 0;
}

static volatile_atomic int32_t create_count;
static int increment_on_wake_up(void* address)
{
    volatile_atomic int32_t* ptr = (volatile_atomic int32_t*)address;
    int32_t value = interlocked_add(ptr, 0);
    (void)interlocked_increment(&create_count);
    wake_by_address_single(&create_count);
    ASSERT_IS_TRUE(wait_on_address(ptr, &value, UINT32_MAX));
    (void)interlocked_increment(ptr);
    ThreadAPI_Exit(0);
    return 0;
}


BEGIN_TEST_SUITE(sync_int)


TEST_SUITE_INITIALIZE(a)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(b)
{

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(c)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(d)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.]*/
/*Tests_SRS_SYNC_43_007: [ If *address is equal to *compare_address, wait_on_address shall cause the thread to sleep. ]*/
/*Tests_SRS_SYNC_43_008: [wait_on_address shall wait indefinitely until it is woken up by a call to wake_by_address_[single/all] if timeout_ms is equal to UINT32_MAX]*/
/*Tests_SRS_SYNC_43_003: [ wait_on_address shall wait until another thread in the same process signals at address using wake_by_address_[single/all] and return true. ]*/
TEST_FUNCTION(two_threads_increment_alternately)
{
    ///arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    THREAD_HANDLE thread1;
    THREAD_HANDLE thread2;

    ///act
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread1, increment_on_even_values, (void*)&var));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread2, increment_on_odd_values, (void*)&var));

    ///assert
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread1, NULL), "ThreadAPI_Join did not work");
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread2, NULL), "ThreadAPI_Join did not work");
    ASSERT_ARE_EQUAL(int32_t, 99, interlocked_add(&var, 0), "Threads did not increment value expected number of times.");
}

/*Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.]*/
/*Tests_SRS_SYNC_43_007: [ If *address is equal to *compare_address, wait_on_address shall cause the thread to sleep. ]*/
/*Tests_SRS_SYNC_43_008: [wait_on_address shall wait indefinitely until it is woken up by a call to wake_by_address_[single/all] if timeout_ms is equal to UINT32_MAX]*/
/*Tests_SRS_SYNC_43_003: [ wait_on_address shall wait until another thread in the same process signals at address using wake_by_address_[single/all] and return true. ]*/
/*Tests_SRS_SYNC_43_004: [ wake_by_address_all shall cause all the thread(s) waiting on a call to wait_on_address with argument address to continue execution. ]*/
/*Tests_SRS_SYNC_43_005: [ wake_by_address_single shall cause one thread waiting on a call to wait_on_address with argument address to continue execution. ]*/
TEST_FUNCTION(wake_up_all_threads)
{
    ///arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    (void)interlocked_exchange(&create_count, 0);
    THREAD_HANDLE threads[100];

    ///act
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[i], increment_on_wake_up, (void*)&var));
    }

    int current_create_count = interlocked_add(&create_count, 0);
    while (current_create_count < 100)
    {
        ASSERT_IS_TRUE(wait_on_address(&create_count, &current_create_count, UINT32_MAX));
        current_create_count = interlocked_add(&create_count, 0);
    }
    wake_by_address_all(&var);
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[i], NULL), "ThreadAPI_Join did not work");
    }
    
    ///assert
    ASSERT_ARE_EQUAL(int32_t, 100, interlocked_add(&var, 0), "Return value is incorrect");
}


/*Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.]*/
/*Tests_SRS_SYNC_43_002: [ wait_on_address shall immediately return true if *address is not equal to *compare_address.]*/
TEST_FUNCTION(wait_on_address_returns_immediately)
{
    ///arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    int value = 1;

    ///act
    bool return_val = wait_on_address(&var, &value, UINT32_MAX);

    ///assert
    ASSERT_IS_TRUE(return_val, "wait_on_address should have returned true");
}


/*Tests_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.]*/
/*Tests_SRS_SYNC_43_002: [ wait_on_address shall immediately return true if *address is not equal to *compare_address.]*/
/*Tests_SRS_SYNC_43_009: [ If timeout_ms milliseconds elapse, wait_on_address shall return false. ]*/
TEST_FUNCTION(wait_on_address_returns_after_timeout_elapses)
{
    ///arrange
    volatile_atomic int32_t var;
    (void)interlocked_exchange(&var, 0);
    int value = 0;
    int timeout = 1000;
    double tolerance_factor = 1.5;

    /// act
    double start_time = timer_global_get_elapsed_ms();
    bool return_val = wait_on_address(&var, &value, timeout);
    double time_elapsed = timer_global_get_elapsed_ms() - start_time;

    ///assert
    ASSERT_IS_TRUE(time_elapsed < timeout* tolerance_factor, "Too much time elapsed. Maximum Expected: %lf, Actual: %lf", timeout*tolerance_factor, time_elapsed);
    ASSERT_IS_FALSE(return_val, "wait_on_address should have returned false");
}
END_TEST_SUITE(sync_int)

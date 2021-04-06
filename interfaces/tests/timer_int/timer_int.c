//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(a)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(b)
{
    TEST_MUTEX_DESTROY(g_testByTest);
    gballoc_hl_deinit();
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

/* timer_create_new */

/* Tests_SRS_TIMER_01_001: [ timer_create_new shall create a new timer and on success return a non-NULL handle to it. ]*/
TEST_FUNCTION(timer_create_new_creates_a_timer)
{
    ///arrange
    TIMER_HANDLE timer;

    ///act
    timer = timer_create_new();

    ///assert
    ASSERT_IS_NOT_NULL(timer);

    /// cleanup
    timer_destroy(timer);
}

/* timer_destroy */

/* Tests_SRS_TIMER_01_002: [ If timer is NULL, timer_destroy shall return. ]*/
TEST_FUNCTION(timer_destroy_with_NULL_timer_returns)
{
    ///arrange

    ///act
    timer_destroy(NULL);

    ///assert
    /// no explicit assert, no crash at least
}

/* Tests_SRS_TIMER_01_003: [ Otherwise, timer_destroy shall free the memory associated with timer. ]*/
TEST_FUNCTION(timer_destroy_frees_resources)
{
    ///arrange
    TIMER_HANDLE timer = timer_create_new();
    ASSERT_IS_NOT_NULL(timer);

    ///act
    timer_destroy(timer);

    ///assert
    /// no explicit assert, no crash at least
}

/* timer_start */

/* Tests_SRS_TIMER_01_004: [ If timer is NULL, timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(timer_start_with_NULL_timer_fails)
{
    ///arrange

    ///act
    int result = timer_start(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_TIMER_01_005: [ Otherwise, timer_start shall record the start time (used for computing the elapsed time). ]*/
TEST_FUNCTION(timer_start_succeeds)
{
    ///arrange
    TIMER_HANDLE timer = timer_create_new();
    ASSERT_IS_NOT_NULL(timer);

    ///act
    int result = timer_start(timer);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);

    /// cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_01_005: [ Otherwise, timer_start shall record the start time (used for computing the elapsed time). ]*/
TEST_FUNCTION(timer_start_twice_succeeds)
{
    ///arrange
    TIMER_HANDLE timer = timer_create_new();
    ASSERT_IS_NOT_NULL(timer);

    ///act
    int result_1 = timer_start(timer);
    int result_2 = timer_start(timer);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result_1);
    ASSERT_ARE_EQUAL(int, 0, result_2);

    /// cleanup
    timer_destroy(timer);
}

/* timer_get_elapsed */

/* Tests_SRS_TIMER_01_006: [ If timer is NULL, timer_get_elapsed shall return -1. ]*/
TEST_FUNCTION(timer_get_elapsed_with_NULL_timer_fails)
{
    ///arrange

    ///act
    double result = timer_get_elapsed(NULL);

    ///assert
    ASSERT_ARE_EQUAL(double, -1.0, result);
}

/* Tests_SRS_TIMER_01_007: [ Otherwise timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_with_implicit_start_succeeds)
{
    ///arrange
    TIMER_HANDLE timer = timer_create_new();
    ASSERT_IS_NOT_NULL(timer);

    // sleep 1s
    ThreadAPI_Sleep(1000);

    ///act
    double result = timer_get_elapsed(timer);

    ///assert
    ASSERT_IS_TRUE(result > 0.5);
    ASSERT_IS_TRUE(result < 1.5);

    /// cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_01_007: [ Otherwise timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_with_start_succeeds)
{
    ///arrange
    TIMER_HANDLE timer = timer_create_new();
    ASSERT_IS_NOT_NULL(timer);

    // sleep 3s
    ThreadAPI_Sleep(3000);
    // start again
    ASSERT_ARE_EQUAL(int, 0, timer_start(timer));

    ThreadAPI_Sleep(1000);

    ///act
    double result = timer_get_elapsed(timer);

    ///assert
    /// giving it a wide tolerance
    ASSERT_IS_TRUE(result > 0.5);
    ASSERT_IS_TRUE(result < 1.5);

    /// cleanup
    timer_destroy(timer);
}

/* timer_get_elapsed_ms */

/* Tests_SRS_TIMER_01_008: [ if timer is NULL, timer_get_elapsed_ms shall return -1. ]*/
TEST_FUNCTION(timer_get_elapsed_ms_with_NULL_timer_fails)
{
    ///arrange

    ///act
    double result = timer_get_elapsed(NULL);

    ///assert
    ASSERT_ARE_EQUAL(double, -1.0, result);
}

/* Tests_SRS_TIMER_01_009: [ Otherwise timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_ms_with_implicit_start_succeeds)
{
    ///arrange
    TIMER_HANDLE timer = timer_create_new();
    ASSERT_IS_NOT_NULL(timer);

    // sleep 1s
    ThreadAPI_Sleep(2000);

    ///act
    double result = timer_get_elapsed_ms(timer);

    ///assert
    /// giving it a wide tolerance
    ASSERT_IS_TRUE(result > 1500);
    ASSERT_IS_TRUE(result < 2500);

    /// cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_01_009: [ Otherwise timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_ms_with_start_succeeds)
{
    ///arrange
    TIMER_HANDLE timer = timer_create_new();
    ASSERT_IS_NOT_NULL(timer);

    // sleep 3s
    ThreadAPI_Sleep(3000);
    // start again
    ASSERT_ARE_EQUAL(int, 0, timer_start(timer));

    ThreadAPI_Sleep(1000);

    ///act
    double result = timer_get_elapsed_ms(timer);

    ///assert
    /// giving it a wide tolerance
    ASSERT_IS_TRUE(result > 500);
    ASSERT_IS_TRUE(result < 1500);

    /// cleanup
    timer_destroy(timer);
}

/* timer_global_get_elapsed_ms */

/* Tests_SRS_TIMER_01_010: [ timer_global_get_elapsed_ms shall return the elapsed time in milliseconds from a start time in the past. ]*/
TEST_FUNCTION(timer_global_get_elapsed_ms_measures_a_seconds)
{
    ///arrange

    // sleep 1s
    double start = timer_global_get_elapsed_ms();
    ThreadAPI_Sleep(1000);

    ///act
    double end = timer_global_get_elapsed_ms();

    ///assert
    /// giving it a wide tolerance
    ASSERT_IS_TRUE((end - start) > 500);
    ASSERT_IS_TRUE((end - start) < 1500);
}

/* timer_global_get_elapsed_us */

/* Tests_SRS_TIMER_01_011: [ timer_global_get_elapsed_us shall return the elapsed time in microseconds from a start time in the past. ]*/
TEST_FUNCTION(timer_global_get_elapsed_us_measures_a_seconds)
{
    ///arrange

    // sleep 1s
    double start = timer_global_get_elapsed_us();
    ThreadAPI_Sleep(1000);

    ///act
    double end = timer_global_get_elapsed_us();

    ///assert
    /// giving it a wide tolerance
    ASSERT_IS_TRUE((end - start) > 500000);
    ASSERT_IS_TRUE((end - start) < 1500000);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

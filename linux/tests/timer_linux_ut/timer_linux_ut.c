// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stdlib.h>
#include <time.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep


    MOCKABLE_FUNCTION(, int, mocked_clock_gettime, clockid_t, clockid, struct timespec*, tp)


#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/timer.h"

// No idea why iwyu warns about this since we include time.h but...
// IWYU pragma: no_forward_declare timespec

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_clock_gettime, 0, -1);

    REGISTER_UMOCK_ALIAS_TYPE(clockid_t, int);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");
}

TEST_FUNCTION_CLEANUP(function_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* timer_create_new */

static void setup_timer_create_expectations(void)
{
    struct timespec start_time = { 0, 0 };
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&start_time, sizeof(start_time));
}

/* Tests_SRS_TIMER_LINUX_01_001: [ timer_create_new shall allocate memory for a new timer. ]*/
/* Tests_SRS_TIMER_LINUX_01_002: [ timer_create_new shall call clock_gettime with CLOCK_MONOTONIC to obtain the initial timer value. ]*/
TEST_FUNCTION(timer_create_succeeds)
{
    //arrange
    setup_timer_create_expectations();

    //act
    TIMER_HANDLE timer = timer_create_new();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(timer, "timer_create_new failed.");

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_003: [ If any error occurs, timer_create_new shall return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_timer_create_also_fails)
{
    //arrange
    setup_timer_create_expectations();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        TIMER_HANDLE timer = timer_create_new();

        // assert
        ASSERT_IS_NULL(timer, "On failed call %zu", i);
    }
}

/*timer_destroy*/

/* Tests_SRS_TIMER_LINUX_01_004: [ If timer is NULL, timer_destroy shall return. ]*/
TEST_FUNCTION(timer_destroy_returns_if_timer_is_NULL)
{
    //arrange
    //act
    timer_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_TIMER_LINUX_01_005: [ Otherwise, timer_destroy shall free the memory associated with timer. ]*/
TEST_FUNCTION(timer_destroy_frees_handle)
{
    //arrange
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    timer_destroy(timer);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* timer_start*/

/* Tests_SRS_TIMER_LINUX_01_007: [ If timer is NULL, timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_timer_is_NULL_timer_start_fails)
{
    //arrange

    //act
    int result = timer_start(NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_TIMER_LINUX_01_006: [ timer_start shall call clock_gettime with CLOCK_MONOTONIC to obtain the start timer value. ]*/
TEST_FUNCTION(timer_start_succeeds)
{
    //arrange
    struct timespec start_time = { 0, 0 };
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&start_time, sizeof(start_time));

    //act
    int result = timer_start(timer);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_022: [ If any error occurs, timer_start shall return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_timer_start_also_fails)
{
    //arrange
    struct timespec start_time = { 0, 0 };
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&start_time, sizeof(start_time));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        int result = timer_start(timer);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
    }

    //cleanup
    timer_destroy(timer);
}

/*timer_get_elapsed*/

/* Tests_SRS_TIMER_LINUX_01_008: [ If timer is NULL, timer_get_elapsed shall return -1. ]*/
TEST_FUNCTION(timer_get_elapsed_fails_if_timer_is_null)
{
    //arrange
    //act
    double start = timer_get_elapsed(NULL);

    //assert
    ASSERT_ARE_EQUAL(double, -1, start);
}

/* Tests_SRS_TIMER_LINUX_01_009: [ Otherwise timer_get_elapsed shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
/* Tests_SRS_TIMER_LINUX_01_010: [ timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_success)
{
    //arrange
    struct timespec stop_time;
    stop_time.tv_sec = 9;
    stop_time.tv_nsec = 900000000;
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time));
    
    //act
    double elapsed_time = timer_get_elapsed(timer);

    //assert
    ASSERT_IS_TRUE(elapsed_time - 9.9 < 0.0000001);

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_009: [ Otherwise timer_get_elapsed shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
/* Tests_SRS_TIMER_LINUX_01_010: [ timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_success_0_1)
{
    //arrange
    struct timespec stop_time;
    stop_time.tv_sec = 0;
    stop_time.tv_nsec = 100000000;
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time));
    
    //act
    double elapsed_time = timer_get_elapsed(timer);

    //assert
    ASSERT_IS_TRUE(elapsed_time - 0.1 < 0.0000001);

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_009: [ Otherwise timer_get_elapsed shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
/* Tests_SRS_TIMER_LINUX_01_010: [ timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_success_0_2_crossing_second)
{
    //arrange
    struct timespec start_time = { 0, 900000000 };
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&start_time, sizeof(start_time));
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    struct timespec stop_time;
    stop_time.tv_sec = 1;
    stop_time.tv_nsec = 100000000;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time));
    
    //act
    double elapsed_time = timer_get_elapsed(timer);

    //assert
    ASSERT_IS_TRUE(elapsed_time - 0.2 < 0.0000001);

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_020: [ If any error occurs, timer_get_elapsed shall fail and return -1. ]*/
TEST_FUNCTION(when_clock_gettime_fails_timer_get_elapsed_also_fails)
{
    //arrange
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    struct timespec stop_time;
    stop_time.tv_sec = 1;
    stop_time.tv_nsec = 100000000;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time))
        .SetReturn(-1);
    
    //act
    double elapsed_time = timer_get_elapsed(timer);

    //assert
    ASSERT_ARE_EQUAL(double, -1, elapsed_time);

    //cleanup
    timer_destroy(timer);
}

/*timer_get_elapsed_ms*/

/* Tests_SRS_TIMER_LINUX_01_011: [ If timer is NULL, timer_get_elapsed_ms shall return -1. ]*/
TEST_FUNCTION(timer_get_elapsed_ms_fails_if_timer_is_null)
{
    //arrange
    //act
    double elapsed_time_ms = timer_get_elapsed_ms(NULL);

    //assert
    ASSERT_ARE_EQUAL(double, -1, elapsed_time_ms);
}

/* Tests_SRS_TIMER_LINUX_01_012: [ Otherwise timer_get_elapsed_ms shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
/* Tests_SRS_TIMER_LINUX_01_013: [ timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_ms_success)
{
    //arrange
    struct timespec stop_time;
    stop_time.tv_sec = 9;
    stop_time.tv_nsec = 900000000;
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time));
    
    //act
    double elapsed_time = timer_get_elapsed_ms(timer);

    //assert
    ASSERT_ARE_EQUAL(double, 9900, elapsed_time);

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_012: [ Otherwise timer_get_elapsed_ms shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
/* Tests_SRS_TIMER_LINUX_01_013: [ timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_ms_success_100_ms)
{
    //arrange
    struct timespec stop_time;
    stop_time.tv_sec = 0;
    stop_time.tv_nsec = 100000000;
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time));
    
    //act
    double elapsed_time = timer_get_elapsed_ms(timer);

    //assert
    ASSERT_ARE_EQUAL(double, 100, elapsed_time);

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_012: [ Otherwise timer_get_elapsed_ms shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
/* Tests_SRS_TIMER_LINUX_01_013: [ timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
TEST_FUNCTION(timer_get_elapsed_ms_success_200_ms_crossing_second)
{
    //arrange
    struct timespec start_time = { 0, 900000000 };
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&start_time, sizeof(start_time));
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    struct timespec stop_time;
    stop_time.tv_sec = 1;
    stop_time.tv_nsec = 100000000;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time));
    
    //act
    double elapsed_time = timer_get_elapsed_ms(timer);

    //assert
    ASSERT_ARE_EQUAL(double, 200, elapsed_time);

    //cleanup
    timer_destroy(timer);
}

/* Tests_SRS_TIMER_LINUX_01_021: [ If any error occurs, timer_get_elapsed_ms shall fail and return -1. ]*/
TEST_FUNCTION(when_clock_gettime_fails_timer_get_elapsed_ms_also_fails)
{
    //arrange
    setup_timer_create_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    struct timespec stop_time;
    stop_time.tv_sec = 1;
    stop_time.tv_nsec = 100000000;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&stop_time, sizeof(stop_time))
        .SetReturn(-1);
    
    //act
    double elapsed_time = timer_get_elapsed_ms(timer);

    //assert
    ASSERT_ARE_EQUAL(double, -1, elapsed_time);

    //cleanup
    timer_destroy(timer);
}

/* timer_global_get_elapsed_ms */

/* Tests_SRS_TIMER_LINUX_01_014: [ timer_global_get_elapsed_ms shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
/* Tests_SRS_TIMER_LINUX_01_015: [ timer_global_get_elapsed_ms shall return the elapsed time in milliseconds (as returned by clock_gettime). ]*/
TEST_FUNCTION(timer_global_get_elapsed_ms_succeeds)
{
    ///arrange
    struct timespec time_1;
    time_1.tv_sec = 0;
    time_1.tv_nsec = 0;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&time_1, sizeof(time_1));
    struct timespec time_2;
    time_2.tv_sec = 9;
    time_2.tv_nsec = 900000000;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&time_2, sizeof(time_2));

    ///act
    double elapsed1 = timer_global_get_elapsed_ms();
    double elapsed2 = timer_global_get_elapsed_ms();

    ASSERT_IS_TRUE(elapsed1 == 0.0);
    ASSERT_IS_TRUE(elapsed2 == 9900.0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_TIMER_LINUX_01_018: [ If any error occurs, timer_global_get_elapsed_ms shall return -1. ]*/
TEST_FUNCTION(when_clock_gettime_fails_timer_global_get_elapsed_ms_also_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .SetReturn(-1);

    ///act
    double elapsed = timer_global_get_elapsed_ms();

    ASSERT_IS_TRUE(elapsed == -1.0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* timer_global_get_elapsed_us */

/* Tests_SRS_TIMER_LINUX_01_016: [ timer_global_get_elapsed_us shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value.  ]*/
/* Tests_SRS_TIMER_LINUX_01_017: [ timer_global_get_elapsed_us shall return the elapsed time in microseconds (as returned by clock_gettime). ]*/
TEST_FUNCTION(timer_global_get_elapsed_us_succeeds)
{
    ///arrange
    struct timespec time_1;
    time_1.tv_sec = 0;
    time_1.tv_nsec = 0;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&time_1, sizeof(time_1));
    struct timespec time_2;
    time_2.tv_sec = 9;
    time_2.tv_nsec = 900000000;
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .CopyOutArgumentBuffer_tp(&time_2, sizeof(time_2));

    ///act
    double elapsed1 = timer_global_get_elapsed_us();
    double elapsed2 = timer_global_get_elapsed_us();

    ASSERT_IS_TRUE(elapsed1 == 0.0);
    ASSERT_IS_TRUE(elapsed2 == 9900000.0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_TIMER_LINUX_01_019: [ If any error occurs, timer_global_get_elapsed_us shall return -1. ]*/
TEST_FUNCTION(when_clock_gettime_fails_timer_global_get_elapsed_us_also_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG))
        .SetReturn(-1);

    ///act
    double elapsed = timer_global_get_elapsed_ms();

    ASSERT_IS_TRUE(elapsed == -1.0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright(C) Microsoft Corporation.All rights reserved.



#include "timer_win32_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
    MOCKABLE_FUNCTION(, BOOLEAN, mocked_QueryPerformanceCounter, LARGE_INTEGER*, lpPerformanceCount)

    MOCKABLE_FUNCTION(, BOOLEAN, mocked_QueryPerformanceFrequency, LARGE_INTEGER*, lpFrequency)
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_init");

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_stdint_register_types failed");

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_charptr_register_types failed");

    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_bool_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
    global_timer_state_reset();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleanup)
{
}

/* timer_create_new */
TEST_FUNCTION(timer_create_malloc_fails) // no-srs
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    //act
    TIMER_HANDLE timer = timer_create_new();

    //assert
    ASSERT_IS_NULL(timer, "timer_create_new failed.");
}

static void test_timer_create_success_expectations(void)
{
    LARGE_INTEGER frequency, start_time;
    frequency.QuadPart = 10;
    start_time.QuadPart = 1;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceFrequency(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpFrequency(&frequency, sizeof(frequency));
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&start_time, sizeof(start_time));
}

TEST_FUNCTION(timer_create_succeeds) // no-srs
{
    //arrange
    test_timer_create_success_expectations();

    //act
    TIMER_HANDLE timer = timer_create_new();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(timer, "timer_create_new failed.");

    //cleanup
    timer_destroy(timer);
}

/* timer_start*/
TEST_FUNCTION(timer_start_returns_if_timer_is_null) // no-srs
{
    //arrange
    //act
     timer_start(NULL);

    //assert
     ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(timer_start_succeeds) // no-srs
{
    //arrange
    LARGE_INTEGER stop_time;
    stop_time.QuadPart = 100;
    test_timer_create_success_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&stop_time, sizeof(stop_time));

    //act
    timer_start(timer);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    timer_destroy(timer);
}

/*timer_get_elapsed*/
TEST_FUNCTION(timer_get_elapsed_fails_if_timer_is_null) // no-srs
{
    //arrange
    //act
    double start = timer_get_elapsed(NULL);

    //assert
    ASSERT_ARE_EQUAL(double, -1, start);
}

TEST_FUNCTION(timer_get_elapsed_success) // no-srs
{
    //arrange
    LARGE_INTEGER stop_time;
    stop_time.QuadPart = 100;
    test_timer_create_success_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&stop_time, sizeof(stop_time));

    //act
    double elapsed_time = timer_get_elapsed(timer);

    //assert
    ASSERT_ARE_EQUAL(double, 9.9, elapsed_time);

    //cleanup
    timer_destroy(timer);
}

/*timer_get_elapsed_ms*/
TEST_FUNCTION(timer_get_elapsed_ms_fails_if_timer_is_null) // no-srs
{
    //arrange
    //act
    double elapsed_time_ms = timer_get_elapsed_ms(NULL);

    //assert
    ASSERT_ARE_EQUAL(double, -1, elapsed_time_ms);
}

TEST_FUNCTION(timer_get_elapsed_ms_success) // no-srs
{
    //arrange
    LARGE_INTEGER stop_time;
    stop_time.QuadPart = 100;
    test_timer_create_success_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&stop_time, sizeof(stop_time));

    //act
    double elapsed_time_ms = timer_get_elapsed_ms(timer);

    //assert
    ASSERT_ARE_EQUAL(double, 9900, elapsed_time_ms);

    //cleanup
    timer_destroy(timer);
}

/*timer_destroy*/
TEST_FUNCTION(timer_destroy_returns_if_timer_is_NULL) // no-srs
{
    //arrange
    //act
    timer_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(timer_destroy_frees_handle) // no-srs
{
    //arrange
    test_timer_create_success_expectations();
    TIMER_HANDLE timer = timer_create_new();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    timer_destroy(timer);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_TIMER_27_001: [timer_global_get_elapsed_s shall return the elapsed time in seconds from a start time in the past.] */
TEST_FUNCTION(g_timer_get_elapsed_in_seconds_succeeds)
{
    ///arrange
    LARGE_INTEGER pretendFreq;
    pretendFreq.QuadPart = 1000;

    STRICT_EXPECTED_CALL(mocked_QueryPerformanceFrequency(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpFrequency(&pretendFreq, sizeof(pretendFreq));

    LARGE_INTEGER pretendCounter1;
    pretendCounter1.QuadPart = 2000;
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&pretendCounter1, sizeof(pretendCounter1));

    /*note: missing second QueryPerformanceFrequency*/
    LARGE_INTEGER pretendCounter2;
    pretendCounter2.QuadPart = 5000;
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&pretendCounter2, sizeof(pretendCounter2));

    ///act
    double elapsed1 = timer_global_get_elapsed_s();
    double elapsed2 = timer_global_get_elapsed_s();

    ASSERT_IS_TRUE(elapsed1 == 2.0); /* all integer number up to 2^31 are perfectly representable by double*/

    ASSERT_IS_TRUE(elapsed2 == 5.0); /* all integer number up to 2^31 are perfectly representable by double*/

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(g_timer_get_elapsed_in_ms_succeeds) // no-srs
{
    ///arrange
    LARGE_INTEGER pretendFreq;
    pretendFreq.QuadPart = 1000;

    STRICT_EXPECTED_CALL(mocked_QueryPerformanceFrequency(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpFrequency(&pretendFreq, sizeof(pretendFreq));

    LARGE_INTEGER pretendCounter1;
    pretendCounter1.QuadPart = 2000;
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&pretendCounter1, sizeof(pretendCounter1));

    /*note: missing second QueryPerformanceFrequency*/
    LARGE_INTEGER pretendCounter2;
    pretendCounter2.QuadPart = 5000;
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&pretendCounter2, sizeof(pretendCounter2));

    ///act
    double elapsed1 = timer_global_get_elapsed_ms();
    double elapsed2 = timer_global_get_elapsed_ms();

    ASSERT_IS_TRUE(elapsed1 == 2000.0); /* all integer number up to 2^31 are perfectly representable by double*/

    ASSERT_IS_TRUE(elapsed2 == 5000.0); /* all integer number up to 2^31 are perfectly representable by double*/

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(g_timer_get_elapsed_in_us_succeeds) // no-srs
{

    ///arrange
    LARGE_INTEGER pretendFreq;
    pretendFreq.QuadPart = 1000;

    STRICT_EXPECTED_CALL(mocked_QueryPerformanceFrequency(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpFrequency(&pretendFreq, sizeof(pretendFreq));

    LARGE_INTEGER pretendCounter1;
    pretendCounter1.QuadPart = 2000;
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&pretendCounter1, sizeof(pretendCounter1));

    /*note: missing second QueryPerformanceFrequency*/
    LARGE_INTEGER pretendCounter2;
    pretendCounter2.QuadPart = 5000;
    STRICT_EXPECTED_CALL(mocked_QueryPerformanceCounter(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpPerformanceCount(&pretendCounter2, sizeof(pretendCounter2));

    ///act
    double elapsed1 = timer_global_get_elapsed_us();
    double elapsed2 = timer_global_get_elapsed_us();

    ASSERT_IS_TRUE(elapsed1 == 2000000.0); /* all integer number up to 2^31 are perfectly representable by double*/

    ASSERT_IS_TRUE(elapsed2 == 5000000.0); /* all integer number up to 2^31 are perfectly representable by double*/

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

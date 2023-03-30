// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <inttypes.h>


#include "windows.h"
#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "c_pal/execution_engine.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/execution_engine_win32.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(WINAPI, PTP_POOL, mocked_CreateThreadpool, PVOID, reserved)
MOCK_FUNCTION_END((PTP_POOL)real_gballoc_hl_malloc(1))
MOCK_FUNCTION_WITH_CODE(WINAPI, void, mocked_CloseThreadpool, PTP_POOL, ptpp)
    real_gballoc_hl_free(ptpp);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(WINAPI, BOOL, mocked_SetThreadpoolThreadMinimum, PTP_POOL, ptpp, DWORD, cthrdMic)
MOCK_FUNCTION_END(TRUE)
MOCK_FUNCTION_WITH_CODE(WINAPI, void, mocked_SetThreadpoolThreadMaximum, PTP_POOL, ptpp, DWORD, cthrdMost)
MOCK_FUNCTION_END()

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_init failed");

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(PTP_POOL, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PVOID, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, unsigned long);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, int);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* execution_engine_create */

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_011: [ If execution_engine_parameters is NULL, execution_engine_create shall use the defaults DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
TEST_FUNCTION(execution_engine_create_with_NULL_arguments_uses_Defaults)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    PTP_POOL ptp_pool;

    STRICT_EXPECTED_CALL(mocked_CreateThreadpool(NULL))
        .CaptureReturn(&ptp_pool);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolThreadMinimum(IGNORED_ARG, DEFAULT_MIN_THREAD_COUNT))
        .ValidateArgumentValue_ptpp(&ptp_pool);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));

    // act
    execution_engine = execution_engine_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(execution_engine);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_002: [ execution_engine_parameters shall be interpreted as EXECUTION_ENGINE_PARAMETERS_WIN32. ]*/
/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_003: [ execution_engine_create shall call CreateThreadpool to create the Win32 threadpool. ]*/
/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_004: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_012: [ If max_thread_count is 0, execution_engine_create shall not set the maximum thread count. ]*/
TEST_FUNCTION(execution_engine_create_succeeds)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_params_win32 = { 1, 0 };
    PTP_POOL ptp_pool;

    STRICT_EXPECTED_CALL(mocked_CreateThreadpool(NULL))
        .CaptureReturn(&ptp_pool);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolThreadMinimum(IGNORED_ARG, 1))
        .ValidateArgumentValue_ptpp(&ptp_pool);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));

    // act
    execution_engine = execution_engine_create(&execution_engine_params_win32);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(execution_engine);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_005: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
TEST_FUNCTION(execution_engine_create_with_max_thread_count_succeeds)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_params_win32 = { 2, 42 };
    PTP_POOL ptp_pool;

    STRICT_EXPECTED_CALL(mocked_CreateThreadpool(NULL))
        .CaptureReturn(&ptp_pool);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolThreadMinimum(IGNORED_ARG, 2))
        .ValidateArgumentValue_ptpp(&ptp_pool);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolThreadMaximum(IGNORED_ARG, 42))
        .ValidateArgumentValue_ptpp(&ptp_pool);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));

    // act
    execution_engine = execution_engine_create(&execution_engine_params_win32);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(execution_engine);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_013: [ If max_thread_count is non-zero, but less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_with_max_thread_count_less_than_min_thread_count_fails)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_params_win32 = { 2, 1 };

    // act
    execution_engine = execution_engine_create(&execution_engine_params_win32);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_execution_engine_create_fails)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_params_win32 = { 2, 42 };
    size_t i;

    STRICT_EXPECTED_CALL(mocked_CreateThreadpool(NULL))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolThreadMinimum(IGNORED_ARG, 2))
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolThreadMaximum(IGNORED_ARG, 42));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            execution_engine = execution_engine_create(&execution_engine_params_win32);

            // assert
            ASSERT_IS_NULL(execution_engine, "On failed call %zu", i);
        }
    }
}

/* execution_engine_inc_ref */

/* Tests_SRS_EXECUTION_ENGINE_WIN32_03_003: [ If execution_engine is NULL then execution_engine_inc_ref shall return. ]*/
TEST_FUNCTION(execution_engine_inc_ref_returns_if_execution_engine_is_NULL)
{
    // arrange

    // act
    execution_engine_inc_ref(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_WIN32_03_004: [ Otherwise execution_engine_inc_ref shall increment the reference count for execution_engine. ]*/
TEST_FUNCTION(execution_engine_inc_ref_succeeds)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    umock_c_reset_all_calls();

    // act
    execution_engine_inc_ref(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(execution_engine);
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_dec_ref */

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_007: [ If execution_engine is NULL, execution_engine_dec_ref shall return. ]*/
TEST_FUNCTION(execution_engine_dec_ref_with_NULL_execution_engine_returns)
{
    // arrange

    // act
    execution_engine_dec_ref(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/* Tests_SRS_EXECUTION_ENGINE_WIN32_03_002: [ If the refcount is zero execution_engine_dec_ref shall close the threadpool. ]*/
TEST_FUNCTION(execution_engine_dec_ref_frees_resources)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_params_win32 = { 1, 0 };
    PTP_POOL ptp_pool;

    STRICT_EXPECTED_CALL(mocked_CreateThreadpool(NULL))
        .CaptureReturn(&ptp_pool);
    execution_engine = execution_engine_create(&execution_engine_params_win32);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_CloseThreadpool(ptp_pool));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    execution_engine_dec_ref(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_WIN32_03_001: [ Otherwise execution_engine_dec_ref shall decrement the refcount.]*/
TEST_FUNCTION(execution_engine_dec_ref_decrements_ref_count)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_params_win32 = { 1, 0 };
    PTP_POOL ptp_pool;

    STRICT_EXPECTED_CALL(mocked_CreateThreadpool(NULL))
        .CaptureReturn(&ptp_pool);
    execution_engine = execution_engine_create(&execution_engine_params_win32);
    execution_engine_inc_ref(execution_engine);
    umock_c_reset_all_calls();

    // act
    execution_engine_dec_ref(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    execution_engine_dec_ref(execution_engine);

}

/* execution_engine_win32_get_threadpool */

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_009: [ If execution_engine is NULL, execution_engine_win32_get_threadpool shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_win32_get_threadpool_with_NULL_execution_engine_fails)
{
    // arrange
    PTP_POOL result;

    // act
    result = execution_engine_win32_get_threadpool(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_EXECUTION_ENGINE_WIN32_01_010: [ Otherwise, execution_engine_win32_get_threadpool shall return the threadpool handle created in execution_engine_create. ]*/
TEST_FUNCTION(execution_engine_win32_get_threadpool_returns_the_underlying_PTP_POOL)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_params_win32 = { 1, 0 };
    PTP_POOL ptp_pool;
    PTP_POOL result;

    STRICT_EXPECTED_CALL(mocked_CreateThreadpool(NULL))
        .CaptureReturn(&ptp_pool);
    execution_engine = execution_engine_create(&execution_engine_params_win32);
    umock_c_reset_all_calls();

    // act
    result = execution_engine_win32_get_threadpool(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, ptp_pool, result);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

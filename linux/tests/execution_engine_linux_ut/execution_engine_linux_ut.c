// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"

#undef ENABLE_MOCKS

#include "real_interlocked.h"
#include "real_gballoc_hl.h"
#include "../reals/real_interlocked_hl.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"


#define DEFAULT_MIN_THREAD_COUNT 4
#define DEFAULT_MAX_THREAD_COUNT 0
#define MIN_THREAD_COUNT 5
#define MAX_THREAD_COUNT 10

EXECUTION_ENGINE_PARAMETERS_LINUX test_execution_engine_parameter = {MIN_THREAD_COUNT, MAX_THREAD_COUNT};

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
    real_gballoc_ll_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* execution_engine_create_create */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_002: [ If execution_engine_parameters is NULL, execution_engine_create shall use the default DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
TEST_FUNCTION(execution_engine_create_with_NULL_execution_engine_succeeds)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    execution_engine = execution_engine_create(NULL);

    // assert
    const EXECUTION_ENGINE_PARAMETERS_LINUX* result = execution_engine_linux_get_parameters(execution_engine);
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MIN_THREAD_COUNT, result->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MAX_THREAD_COUNT, result->max_thread_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(execution_engine);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_003: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_004: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
TEST_FUNCTION(execution_engine_create_succeeds)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    execution_engine = execution_engine_create(&test_execution_engine_parameter);

    // assert
    const EXECUTION_ENGINE_PARAMETERS_LINUX* result = execution_engine_linux_get_parameters(execution_engine);
    ASSERT_ARE_EQUAL(uint32_t, MIN_THREAD_COUNT, result->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, MAX_THREAD_COUNT, result->max_thread_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(execution_engine);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_005: [ If max_thread_count is non-zero and less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_max_thread_count_nonzero_and_less_than_min_thread_count)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_LINUX test_execution_engine_parameter = {5, 3};

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    execution_engine = execution_engine_create(&test_execution_engine_parameter);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_allocation_fails)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_PARAMETERS_LINUX test_execution_engine_parameter = {5, 3};

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(NULL);

    // act
    execution_engine = execution_engine_create(&test_execution_engine_parameter);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(execution_engine);
}

/* execution_engine_execution_engine_dec_ref */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_007: [ If execution_engine is NULL, execution_engine_dec_ref shall return. ]*/
TEST_FUNCTION(execution_engine_dec_ref_with_NULL_execution_engine_returns)
{
    // arrange

    // act
    execution_engine_dec_ref(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_008: [ Otherwise execution_engine_dec_ref shall decrement the refcount. ]*/
TEST_FUNCTION(execution_engine_dec_ref_decrements_ref_count)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;
    execution_engine = execution_engine_create(&test_execution_engine_parameter);
    execution_engine_inc_ref(execution_engine);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    // act
    execution_engine_dec_ref(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_009: [ If the refcount is zero execution_engine_dec_ref shall free the memory for EXECUTION_ENGINE. ]*/
TEST_FUNCTION(execution_engine_dec_ref_frees_resources)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine;

    execution_engine = execution_engine_create(&test_execution_engine_parameter);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    execution_engine_dec_ref(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* execution_engine_execution_engine_inc_ref */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_010: [ If execution_engine is NULL then execution_engine_inc_ref shall return. ]*/
TEST_FUNCTION(execution_engine_inc_ref_returns_if_execution_engine_is_NULL)
{
    // arrange

    // act
    execution_engine_inc_ref(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_011: [ Otherwise execution_engine_inc_ref shall increment the reference count for execution_engine. ]*/
TEST_FUNCTION(execution_engine_inc_ref_succeeds)
{
    // arrange
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    // act
    execution_engine_inc_ref(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(execution_engine);
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_get_parameters */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_012: [ If execution_engine is NULL, execution_engine_linux_get_parameters shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_get_parameters_with_NULL_execution_engine_fails)
{
    // arrange
    const EXECUTION_ENGINE_PARAMETERS_LINUX* result;

    // act
    result = execution_engine_linux_get_parameters(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_013: [ Otherwise, execution_engine_linux_get_parameters shall return the parameters in EXECUTION_ENGINE. ]*/
TEST_FUNCTION(execution_engine_linux_get_parameters_succeeds)
{
    // arrange
    const EXECUTION_ENGINE_PARAMETERS_LINUX* result;
    EXECUTION_ENGINE_HANDLE execution_engine;
    execution_engine = execution_engine_create(&test_execution_engine_parameter);
    umock_c_reset_all_calls();

    // act
    result = execution_engine_linux_get_parameters(execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, MIN_THREAD_COUNT, result->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, MAX_THREAD_COUNT, result->max_thread_count);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

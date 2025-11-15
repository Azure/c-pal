// Copyright (c) Microsoft. All rights reserved.


#include "gballoc_hl_metrics_wout_init_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

MU_DEFINE_ENUM_STRINGS(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_UMOCK_ALIAS_TYPE(LAZY_INIT_FUNCTION, void*);

    REGISTER_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT);

    REGISTER_LAZY_INIT_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    gballoc_hl_deinit();
}

/* gballoc_hl_deinit */

/* Tests_SRS_GBALLOC_HL_METRICS_01_005: [ If gballoc_hl_deinit is called while not initialized, gballoc_hl_deinit shall return. ]*/
TEST_FUNCTION(gballoc_hl_deinit_when_not_initialized_returns)
{
    // arrange

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // act
    gballoc_hl_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_malloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_008: [ If the module was not initialized, gballoc_hl_malloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_malloc_when_not_initialized_returns_NULL)
{
    // arrange
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    // act
    result = gballoc_hl_malloc(1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_METRICS_02_027: [ If the module was not initialized, gballoc_hl_malloc_2 shall return NULL. ]*/
TEST_FUNCTION(gballoc_malloc_2_when_not_initialized_returns_NULL)
{
    // arrange
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    // act
    result = gballoc_hl_malloc_2(2, 3);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_METRICS_02_008: [ If the module was not initialized, gballoc_hl_malloc_flex shall return NULL. ]*/
TEST_FUNCTION(gballoc_malloc_flex_when_not_initialized_returns_NULL)
{
    // arrange
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    // act
    result = gballoc_hl_malloc_flex(2, 3, 5);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_calloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_011: [ If the module was not initialized, gballoc_hl_calloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_calloc_when_not_initialized_fails)
{
    // arrange
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    // act
    result = gballoc_hl_calloc(1, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_realloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_015: [ If the module was not initialized, gballoc_hl_realloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_realloc_when_not_initialized_fails)
{
    // arrange
    void* result;
    void* ptr;
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    // act
    result = gballoc_hl_realloc(ptr, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_METRICS_02_012: [ If the module was not initialized, gballoc_hl_realloc_2 shall return NULL. ]*/
TEST_FUNCTION(gballoc_realloc_2_when_not_initialized_fails)
{
    // arrange
    void* result;
    void* ptr;
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    // act
    result = gballoc_hl_realloc_2(ptr, 1, 2);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_METRICS_02_017: [ If the module was not initialized, gballoc_hl_realloc_flex shall return NULL. ]*/
TEST_FUNCTION(gballoc_realloc_flex_when_not_initialized_fails)
{
    // arrange
    void* result;
    void* ptr;
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_malloc(42);
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    // act
    result = gballoc_hl_realloc_flex(ptr, 2, 3, 5);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_free */

/* Tests_SRS_GBALLOC_HL_METRICS_01_016: [ If the module was not initialized, gballoc_hl_free shall return. ]*/
TEST_FUNCTION(gballoc_free_when_not_initialized_returns)
{
    // arrange
    void* ptr;
    STRICT_EXPECTED_CALL(gballoc_ll_init(NULL));
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_calloc(3, 4);
    ptr = gballoc_hl_realloc(ptr, 1);
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // act
    gballoc_hl_free(ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_size */

/* Tests_SRS_GBALLOC_HL_METRICS_01_074: [ If the module was not initialized, gballoc_hl_size shall return 0. ]*/
TEST_FUNCTION(gballoc_hl_size_when_not_initialized_returns_0)
{
    // arrange
    void* ptr;
    size_t size;
    STRICT_EXPECTED_CALL(gballoc_ll_init(NULL));
    (void)gballoc_hl_init(NULL, NULL);
    ptr = gballoc_hl_calloc(3, 4);
    gballoc_hl_free(ptr);
    gballoc_hl_deinit();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // act
    size = gballoc_hl_size(ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, 0, size);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

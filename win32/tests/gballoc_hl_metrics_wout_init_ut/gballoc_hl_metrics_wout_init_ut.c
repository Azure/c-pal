// Copyright (c) Microsoft. All rights reserved.

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "azure_c_pal/gballoc_hl.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4189)
#endif

#define ENABLE_MOCKS
#include "azure_c_pal/gballoc_ll.h"
#undef ENABLE_MOCKS

static TEST_MUTEX_HANDLE test_serialize_mutex;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(gballoc_hl_metrics_wout_init_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    int result;

    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_reset_all_calls();
    result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_negative_tests_init");
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/* gballoc_hl_deinit */

/* Tests_SRS_GBALLOC_HL_METRICS_01_005: [ If gballoc_hl_deinit is called while not initialized, gballoc_hl_deinit shall return. ]*/
TEST_FUNCTION(gballoc_hl_deinit_when_not_initialized_returns)
{
    // arrange

    // act
    gballoc_hl_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_malloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_008: [ If the module was not initialized, gballoc_malloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_malloc_when_not_initialized_returns_NULL)
{
    // arrange
    void* result;

    // act
    result = gballoc_hl_malloc(1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_calloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_011: [ If the module was not initialized, gballoc_calloc shall return NULL. ]*/
TEST_FUNCTION(gballoc_calloc_when_not_initialized_fails)
{
    // arrange
    void* result;

    // act
    result = gballoc_hl_calloc(1, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_realloc */

/* Tests_SRS_GBALLOC_HL_METRICS_01_015: [ If the module was not initialized, gballoc_realloc shall return NULL. ]*/
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

    // act
    result = gballoc_hl_realloc(ptr, 1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_free */

/* Tests_SRS_GBALLOC_HL_METRICS_01_016: [ If the module was not initialized, gballoc_free shall return. ]*/
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

    // act
    gballoc_hl_free(ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(gballoc_hl_metrics_wout_init_unittests)

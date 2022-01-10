// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;

static void* TEST_MALLOC_RESULT = (void*)0x1;
static void* TEST_CALLOC_RESULT = (void*)0x2;
static void* TEST_REALLOC_RESULT = (void*)0x3;

#include "umock_c/umock_c.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"

    MOCKABLE_FUNCTION(, void*, mock_mi_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void*, mock_mi_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, mock_mi_realloc, void*, ptr, size_t, size);
    MOCKABLE_FUNCTION(, void, mock_mi_free, void*, ptr);

    MOCKABLE_FUNCTION(, size_t, mock_mi_usable_size, void*, ptr);

#undef ENABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

#include "c_pal/gballoc_ll.h"

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    REGISTER_GLOBAL_MOCK_RETURN(mock_mi_malloc, TEST_MALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_mi_calloc, TEST_CALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_mi_realloc, TEST_REALLOC_RESULT);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_001: [ gballoc_ll_init shall return 0. ]*/
TEST_FUNCTION(gballoc_ll_init_returns_0)
{
    ///arrange
    int result;

    ///act
    result = gballoc_ll_init(NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_001: [ gballoc_ll_init shall return 0. ]*/
TEST_FUNCTION(gballoc_ll_init_with_non_NULL_pointer_returns_0)
{
    ///arrange
    int result;

    ///act
    result = gballoc_ll_init((void*)0x24);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_002: [ gballoc_ll_deinit shall return. ]*/
TEST_FUNCTION(gballoc_ll_deinit_returns)
{
    ///arrange

    ///act
    gballoc_ll_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_003: [ gballoc_ll_malloc shall call mi_malloc and returns what mi_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_calls_mimalloc)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_malloc(1));

    ///act
    void* ptr = gballoc_ll_malloc(1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr, TEST_MALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_nmemb_0_calls_mi_malloc)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_malloc(0));

    ///act
    void* ptr = gballoc_ll_malloc_2(0, 1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr, TEST_MALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_009: [ gballoc_ll_malloc_2 shall call mi_malloc(nmemb * size) and returns what mi_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_SIZE_MAX_succeeds)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_malloc(SIZE_MAX));

    ///act
    void* ptr = gballoc_ll_malloc_2(3, SIZE_MAX/3);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr, TEST_MALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_009: [ gballoc_ll_malloc_2 shall call mi_malloc(nmemb * size) and returns what mi_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_SIZE_MAX_returns_NULL_when_mimalloc_returns_NULL)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_malloc(SIZE_MAX))
        .SetReturn(NULL);

    ///act
    void* ptr = gballoc_ll_malloc_2(3, SIZE_MAX / 3);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_overflow_returns_NUL)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_malloc_2(2, (SIZE_MAX - 1)/2 + 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_011: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_overflow_fails_1)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_malloc_flex(4, 2, (SIZE_MAX - 1) / 2 + 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_011: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_overflow_fails_2)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_malloc_flex(2, 2, (SIZE_MAX - 1) / 2);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_012: [ gballoc_ll_malloc_flex shall call mi_malloc(base + nmemb * size) and returns what mi_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_SIZE_MAX_succeeds)
{
    ///arrange
    
    STRICT_EXPECTED_CALL(mock_mi_malloc(SIZE_MAX));

    ///act
    void* ptr = gballoc_ll_malloc_flex(1, 2, (SIZE_MAX - 1) / 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr, TEST_MALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_012: [ gballoc_ll_malloc_flex shall call mi_malloc(base + nmemb * size) and returns what mi_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_SIZE_MAX_fails_when_mimalloc_fails)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_malloc(SIZE_MAX))
        .SetReturn(NULL);

    ///act
    void* ptr = gballoc_ll_malloc_flex(1, 2, (SIZE_MAX - 1) / 2);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_014: [ gballoc_ll_realloc_2 calls mi_realloc(ptr, nmemb * size) and returns what mi_realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_nmemb_0_succeeds)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_realloc(TEST_MALLOC_RESULT, 0));

    ///act
    void* ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 0, 5);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_013: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_2 shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_overflow_fails)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 2, (SIZE_MAX - 1) / 2 + 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}


/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_014: [ gballoc_ll_realloc_2 calls mi_realloc(ptr, nmemb * size) and returns what mi_realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_SIZE_MAX_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(mock_mi_realloc(TEST_MALLOC_RESULT, SIZE_MAX));

    ///act
    void* ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 3, SIZE_MAX/3);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_016: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_overflow_fails_1)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 1, 2, (SIZE_MAX - 1) / 2  + 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_016: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_overflow_fails_2)
{
    ///arrange

    ///act
    void* ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 4, 3, SIZE_MAX / 3 - 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_017: [ gballoc_ll_realloc_flex calls mi_realloc(ptr, base + nmemb * size) and returns what mi_realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_SIZE_MAX_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(mock_mi_realloc(TEST_MALLOC_RESULT, SIZE_MAX));

    ///act
    void* ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 3, SIZE_MAX / 3 - 1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_017: [ gballoc_ll_realloc_flex calls mi_realloc(ptr, base + nmemb * size) and returns what mi_realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_SIZE_MAX_when_realloc_fails_its_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(mock_mi_realloc(TEST_MALLOC_RESULT, SIZE_MAX))
        .SetReturn(NULL);

    ///act
    void* ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 3, SIZE_MAX / 3 - 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}


/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_004: [ gballoc_ll_free shall call mi_free(ptr). ]*/
TEST_FUNCTION(gballoc_ll_free_calls_mi_free)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_free(TEST_MALLOC_RESULT));

    ///act
    gballoc_ll_free(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_005: [ gballoc_ll_calloc shall call mi_calloc(nmemb, size) and return what mi_calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_calls_mi_calloc)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_mi_calloc(1, 2));

    ///act
    void* ptr = gballoc_ll_calloc(1, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr, TEST_CALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_006: [ gballoc_ll_realloc calls mi_realloc(ptr, size) and returns what mi_realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_calls_mi_realloc)
{
    ///arrange
    void* ptr1 = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    umock_c_reset_all_calls();


    STRICT_EXPECTED_CALL(mock_mi_realloc(TEST_MALLOC_RESULT, 2));

    ///act
    void* ptr2 = gballoc_ll_realloc(ptr1, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr2, TEST_REALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_MIMALLOC_02_007: [ gballoc_ll_size shall call mi_usable_size and return what mi_usable_size returned. ]*/
TEST_FUNCTION(gballoc_ll_size_calls_mi_usable_size)
{
    ///arrange
    size_t size;

    STRICT_EXPECTED_CALL(mock_mi_usable_size(TEST_MALLOC_RESULT))
        .SetReturn(32);

    ///act
    size = gballoc_ll_size(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(size_t, 32, size);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

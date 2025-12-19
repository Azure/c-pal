// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gballoc_ll_passthrough_ut_pch.h"

static void* TEST_MALLOC_RESULT = (void*)0x1;
static void* TEST_CALLOC_RESULT = (void*)0x2;
static void* TEST_REALLOC_RESULT = (void*)0x3;

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
    MOCKABLE_FUNCTION(, void*, mock_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void*, mock_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, mock_realloc, void*, ptr, size_t, size);
    MOCKABLE_FUNCTION(, void, mock_free, void*, ptr);

    MOCKABLE_FUNCTION(, size_t, mock_malloc_usable_size, void*, ptr);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    umock_c_init(on_umock_c_error);

    REGISTER_GLOBAL_MOCK_RETURN(mock_malloc, TEST_MALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_realloc, TEST_REALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_calloc, TEST_CALLOC_RESULT);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_001: [ gballoc_ll_init shall return 0. ]*/
TEST_FUNCTION(gballoc_ll_init_returns_0)
{
    int result;
    
    ///act
    result = gballoc_ll_init(NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_deinit();
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_001: [ gballoc_ll_init shall return 0. ]*/
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

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_002: [ gballoc_ll_deinit shall return. ]*/
TEST_FUNCTION(gballoc_ll_deinit_returns)
{
    ///arrange

    ///act
    gballoc_ll_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_003: [ gballoc_ll_malloc shall call malloc(size) and return what malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_returns_what_malloc_returns_1)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(1));

    ///act
    ptr = gballoc_ll_malloc(1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_003: [ gballoc_ll_malloc shall call malloc(size) and return what malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_returns_what_malloc_returns_2)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(1))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_malloc(1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_overflow_fails)
{
    ///arrange
    void* ptr;

    ///act
    ptr = gballoc_ll_malloc_2(2, (SIZE_MAX - 1) / 2 + 1); /*a clear overflow. Test cannot write (SIZE_MAX+1)/2 because SIZE_MAX + 1 that's already 2^64 and that cannot be represented*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_009: [ gballoc_ll_malloc_2 shall call malloc(nmemb*size) and returns what malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_SIZE_MAX_calls_malloc_and_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(SIZE_MAX));

    ///act
    ptr = gballoc_ll_malloc_2(3, SIZE_MAX / 3); /*SIZE_MAX is divisible by 3 when size_t is represented on 16 bits, 32 bits or 64 bits.*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_009: [ gballoc_ll_malloc_2 shall call malloc(nmemb*size) and returns what malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_nmemb_0_calls_malloc_and_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(0));

    ///act
    ptr = gballoc_ll_malloc_2(0, SIZE_MAX / 3); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_009: [ gballoc_ll_malloc_2 shall call malloc(nmemb*size) and returns what malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_SIZE_MAX_calls_malloc_and_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_malloc_2(3, SIZE_MAX / 3); /*SIZE_MAX is divisible by 3 when size_t is represented on 16 bits, 32 bits or 64 bits.*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_011: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_overflow_fail_1)
{
    ///arrange
    void* ptr;

    ///act
    ptr = gballoc_ll_malloc_flex(4, 2, (SIZE_MAX - 1) / 2 + 1); /*a clear overflow. Test cannot write (SIZE_MAX+1)/2 because SIZE_MAX + 1 that's already 2^64 and that cannot be represented*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_011: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_overflow_fail_2)
{
    ///arrange
    void* ptr;

    ///act
    ptr = gballoc_ll_malloc_flex(2, 2, (SIZE_MAX - 3) / 2 + 1); /*a overflow when adding base */

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_012: [ gballoc_ll_malloc_flex shall return what malloc(base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_SIZE_MAX_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(SIZE_MAX));

    ///act
    ptr = gballoc_ll_malloc_flex(1, 2, (SIZE_MAX - 3) / 2 + 1); /*no longer overflow, just SIZE_MAX*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_012: [ gballoc_ll_malloc_flex shall return what malloc(base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_nmemb_0_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(1));

    ///act
    ptr = gballoc_ll_malloc_flex(1, 0, (SIZE_MAX - 3) / 2 + 1); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_012: [ gballoc_ll_malloc_flex shall return what malloc(base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_SIZE_MAX_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_malloc(SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_malloc_flex(1, 2, (SIZE_MAX - 3) / 2 + 1); /*no longer overflow, just SIZE_MAX*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_004: [ gballoc_ll_free shall call free(ptr). ]*/
TEST_FUNCTION(gballoc_ll_free_calls_free)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_free((void*)0x22));

    ///act
    gballoc_ll_free((void*)0x22);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_005: [ gballoc_ll_calloc shall call calloc(nmemb, size) and return what calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_calls_calloc)
{
    ///arrange
    void* ptr;
    STRICT_EXPECTED_CALL(mock_calloc(1, 2));

    ///act
    ptr = gballoc_ll_calloc(1, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_CALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_005: [ gballoc_ll_calloc shall call calloc(nmemb, size) and return what calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_calls_calloc_2)
{
    ///arrange
    void* ptr;
    STRICT_EXPECTED_CALL(mock_calloc(1, 2))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_calloc(1, 2);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_006: [ gballoc_ll_realloc shall call realloc(ptr, size) and return what realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_calls_realloc)
{
    ///arrange
    void* ptr;
    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, 2));

    ///act
    ptr = gballoc_ll_realloc(TEST_MALLOC_RESULT, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_006: [ gballoc_ll_realloc shall call realloc(ptr, size) and return what realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_calls_realloc)
{
    ///arrange
    void* ptr;
    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, 2))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_realloc(TEST_MALLOC_RESULT, 2);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_013: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_2 shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_overflow_returns_NULL)
{
    ///arrange
    void* ptr;

    ///act
    ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 2, (SIZE_MAX - 1) / 2 + 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_014: [ gballoc_ll_realloc_2 shall return what realloc(ptr, nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_SIZE_MAX_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, SIZE_MAX));

    ///act
    ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 3, SIZE_MAX / 3);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_014: [ gballoc_ll_realloc_2 shall return what realloc(ptr, nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_nmemb_0_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, 0));

    ///act
    ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 0, SIZE_MAX / 3); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_014: [ gballoc_ll_realloc_2 shall return what realloc(ptr, nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_SIZE_MAX_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 3, SIZE_MAX / 3);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_016: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_overflow_fails_1)
{
    ///arrange
    void* ptr;

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 1, 2, (SIZE_MAX - 1) / 2 + 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_016: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_overflow_fails_2)
{
    ///arrange
    void* ptr;

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 4, 3, SIZE_MAX / 3 - 1);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_017: [ gballoc_ll_realloc_flex shall return what realloc(ptr, base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_SIZE_MAX_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, SIZE_MAX));

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 3, SIZE_MAX / 3 - 1); /*same as above, just 1 byte less*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_017: [ gballoc_ll_realloc_flex shall return what realloc(ptr, base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_nmemb_0_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, 3));

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 0, SIZE_MAX / 3 - 1); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_017: [ gballoc_ll_realloc_flex shall return what realloc(ptr, base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_SIZE_MAX_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_realloc(TEST_MALLOC_RESULT, SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 3, SIZE_MAX / 3 - 1); /*same as above, just 1 byte less*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_PASSTHROUGH_02_007: [ gballoc_ll_size shall return what _msize returns. ]*/
TEST_FUNCTION(gballoc_ll_size_returns)
{
    ///arrange
    size_t size;

    STRICT_EXPECTED_CALL(mock_malloc_usable_size(TEST_MALLOC_RESULT))
        .SetReturn(32);

    ///act
    size = gballoc_ll_size(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(size_t, 32, size);
}

/* Tests_SRS_GBALLOC_LL_PASSTHROUGH_28_001: [ gballoc_ll_set_option shall do nothing and return 0. ]*/
TEST_FUNCTION(gballoc_ll_set_option_returns_zero)
{
    ///arrange
    void* value = (void*)0x42;

    ///act
    int result = gballoc_ll_set_option("dirty_decay", value);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

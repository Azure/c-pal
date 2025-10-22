// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gballoc_ll_jemalloc_ut_pch.h"
#undef ENABLE_MOCKS_DECL

typedef void (*JEMALLOC_WRITE_CB)(void*, const char*);

typedef struct PRINT_FUNCTION_CB_DATA_TAG
{
    const char* text_to_print;
} PRINT_FUNCTION_CB_DATA;

#define MAX_PRINT_FUNCTION_CB 10

static size_t g_call_print_cb_count = 0;
static PRINT_FUNCTION_CB_DATA g_call_print_cb[MAX_PRINT_FUNCTION_CB];

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

MOCKABLE_FUNCTION(, void*, mock_je_malloc, size_t, size);
MOCKABLE_FUNCTION(, void*, mock_je_calloc, size_t, nmemb, size_t, size);
MOCKABLE_FUNCTION(, void*, mock_je_realloc, void*, ptr, size_t, size);
MOCKABLE_FUNCTION(, void, mock_je_free, void*, ptr);

MOCKABLE_FUNCTION(, size_t, mock_je_malloc_usable_size, void*, ptr);
MOCKABLE_FUNCTION_WITH_CODE(, void, mock_je_malloc_stats_print, JEMALLOC_WRITE_CB, write_cb, void*, cbopaque, const char*, opts)
for (size_t i = 0; i < g_call_print_cb_count; i++)
{
    write_cb(cbopaque, g_call_print_cb[i].text_to_print);
}
MOCKABLE_FUNCTION_END()
MOCKABLE_FUNCTION(, int, mock_je_mallctl, const char*, name, void*, oldp, size_t*, oldlenp, void*, newp, size_t, newlen);

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

static void* TEST_MALLOC_RESULT = (void*)0x1;
static void* TEST_CALLOC_RESULT = (void*)0x2;
static void* TEST_REALLOC_RESULT = (void*)0x3;

#define NUM_ARENAS 2

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    umock_c_init(on_umock_c_error);

    REGISTER_GLOBAL_MOCK_RETURN(mock_je_malloc, TEST_MALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_je_calloc, TEST_CALLOC_RESULT);
    REGISTER_GLOBAL_MOCK_RETURN(mock_je_realloc, TEST_REALLOC_RESULT);

    REGISTER_UMOCK_ALIAS_TYPE(JEMALLOC_WRITE_CB, void*)
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
}

/* gballoc_ll_init */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_001: [ gballoc_ll_init shall return 0. ]*/
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

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_001: [ gballoc_ll_init shall return 0. ]*/
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

/* gballoc_ll_deinit */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_002: [ gballoc_ll_deinit shall return. ]*/
TEST_FUNCTION(gballoc_ll_deinit_returns)
{
    ///arrange

    ///act
    gballoc_ll_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* gballoc_ll_malloc */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_003: [ gballoc_ll_malloc shall call je_malloc and returns what je_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_calls_jemalloc)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_je_malloc(1));

    ///act
    void* ptr = gballoc_ll_malloc(1);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr, TEST_MALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/* gballoc_ll_malloc_2 */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_001: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
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

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_002: [ gballoc_ll_malloc_2 shall call je_malloc(nmemb*size) and returns what je_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_SIZE_MAX_calls_je_malloc_and_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_malloc(SIZE_MAX));

    ///act
    ptr = gballoc_ll_malloc_2(3, SIZE_MAX / 3); /*SIZE_MAX is divisible by 3 when size_t is represented on 16 bits, 32 bits or 64 bits.*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_002: [ gballoc_ll_malloc_2 shall call je_malloc(nmemb*size) and returns what je_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_nmemb_0_calls_je_malloc_and_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_malloc(0));

    ///act
    ptr = gballoc_ll_malloc_2(0, SIZE_MAX / 3); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_002: [ gballoc_ll_malloc_2 shall call je_malloc(nmemb*size) and returns what je_malloc returned. ]*/
TEST_FUNCTION(gballoc_ll_malloc_2_with_SIZE_MAX_calls_je_malloc_and_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_malloc(SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_malloc_2(3, SIZE_MAX / 3); /*SIZE_MAX is divisible by 3 when size_t is represented on 16 bits, 32 bits or 64 bits.*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* gballoc_ll_malloc_flex */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_004: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
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

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_004: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
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

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_005: [ gballoc_ll_malloc_flex shall return what je_malloc(base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_SIZE_MAX_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_malloc(SIZE_MAX));

    ///act
    ptr = gballoc_ll_malloc_flex(1, 2, (SIZE_MAX - 3) / 2 + 1); /*no longer overflow, just SIZE_MAX*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_005: [ gballoc_ll_malloc_flex shall return what je_malloc(base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_nmemb_0_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_malloc(1));

    ///act
    ptr = gballoc_ll_malloc_flex(1, 0, (SIZE_MAX - 3) / 2 + 1); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_MALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_005: [ gballoc_ll_malloc_flex shall return what je_malloc(base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_malloc_flex_with_SIZE_MAX_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_malloc(SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_malloc_flex(1, 2, (SIZE_MAX - 3) / 2 + 1); /*no longer overflow, just SIZE_MAX*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* gballoc_ll_free */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_004: [ gballoc_ll_free shall call je_free(ptr). ]*/
TEST_FUNCTION(gballoc_ll_free_calls_je_free)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_je_free(TEST_MALLOC_RESULT));

    ///act
    gballoc_ll_free(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* gballoc_ll_calloc */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_005: [ gballoc_ll_calloc shall call je_calloc(nmemb, size) and return what je_calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_calls_je_calloc)
{
    ///arrange

    STRICT_EXPECTED_CALL(mock_je_calloc(1, 2));

    ///act
    void* ptr = gballoc_ll_calloc(1, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr, TEST_CALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr);
}

/* gballoc_ll_realloc */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_006: [ gballoc_ll_realloc calls je_realloc(ptr, size) and returns what je_realloc returned. ]*/
TEST_FUNCTION(gballoc_ll_realloc_calls_je_realloc)
{
    ///arrange
    void* ptr1 = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    umock_c_reset_all_calls();


    STRICT_EXPECTED_CALL(mock_je_realloc(TEST_MALLOC_RESULT, 2));

    ///act
    void* ptr2 = gballoc_ll_realloc(ptr1, 2);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, ptr2, TEST_REALLOC_RESULT);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_ll_free(ptr2);
}

/* gballoc_ll_realloc_2 */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_006: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_2 shall fail and return NULL. ]*/
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

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_007: [ gballoc_ll_realloc_2 shall return what je_realloc(ptr, nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_SIZE_MAX_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_realloc(TEST_MALLOC_RESULT, SIZE_MAX));

    ///act
    ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 3, SIZE_MAX / 3);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_007: [ gballoc_ll_realloc_2 shall return what je_realloc(ptr, nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_nmemb_0_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_realloc(TEST_MALLOC_RESULT, 0));

    ///act
    ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 0, SIZE_MAX / 3); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_007: [ gballoc_ll_realloc_2 shall return what je_realloc(ptr, nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_2_with_SIZE_MAX_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_realloc(TEST_MALLOC_RESULT, SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_realloc_2(TEST_MALLOC_RESULT, 3, SIZE_MAX / 3);

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_ll_realloc_flex */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
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

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_009: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
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

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_010: [ gballoc_ll_realloc_flex shall return what je_realloc(ptr, base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_SIZE_MAX_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_realloc(TEST_MALLOC_RESULT, SIZE_MAX));

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 3, SIZE_MAX / 3 - 1); /*same as above, just 1 byte less*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_010: [ gballoc_ll_realloc_flex shall return what je_realloc(ptr, base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_nmemb_0_succeeds)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_realloc(TEST_MALLOC_RESULT, 3));

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 0, SIZE_MAX / 3 - 1); /*no longer overflow, just a test for a division by 0*/

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, TEST_REALLOC_RESULT, ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_02_010: [ gballoc_ll_realloc_flex shall return what je_realloc(ptr, base + nmemb * size) returns. ]*/
TEST_FUNCTION(gballoc_ll_realloc_flex_with_SIZE_MAX_fails)
{
    ///arrange
    void* ptr;

    STRICT_EXPECTED_CALL(mock_je_realloc(TEST_MALLOC_RESULT, SIZE_MAX))
        .SetReturn(NULL);

    ///act
    ptr = gballoc_ll_realloc_flex(TEST_MALLOC_RESULT, 3, 3, SIZE_MAX / 3 - 1); /*same as above, just 1 byte less*/

    ///assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_ll_size */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_01_007: [ gballoc_ll_size shall call je_malloc_usable_size and return what je_malloc_usable_size returned. ]*/
TEST_FUNCTION(gballoc_ll_size_calls_je_malloc_usable_size)
{
    ///arrange
    size_t size;

    STRICT_EXPECTED_CALL(mock_je_malloc_usable_size(TEST_MALLOC_RESULT))
        .SetReturn(32);

    ///act
    size = gballoc_ll_size(TEST_MALLOC_RESULT);

    ///assert
    ASSERT_ARE_EQUAL(size_t, 32, size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_ll_size_print_stats */

/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_008: [ gballoc_ll_print_stats shall call je_malloc_stats_print and pass to it jemalloc_print_stats_callback as print callback. ]*/
TEST_FUNCTION(gballoc_ll_print_stats_does_not_call_the_print_function)
{
    ///arrange
    g_call_print_cb_count = 0;
    STRICT_EXPECTED_CALL(mock_je_malloc_stats_print(IGNORED_ARG, NULL, NULL));

    ///act
    gballoc_ll_print_stats();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_008: [ gballoc_ll_print_stats shall call je_malloc_stats_print and pass to it jemalloc_print_stats_callback as print callback. ]*/
/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_010: [ Otherwise, jemalloc_print_stats_callback shall print (log) text, breaking it does in chunks of LOG_MAX_MESSAGE_LENGTH / 2. ]*/
TEST_FUNCTION(gballoc_ll_print_stats_calls_the_print_function_and_prints_1_small_text_line)
{
    ///arrange
    g_call_print_cb_count = 1;
    g_call_print_cb[0].text_to_print = "gogu";
    STRICT_EXPECTED_CALL(mock_je_malloc_stats_print(IGNORED_ARG, NULL, NULL));

    ///act
    gballoc_ll_print_stats();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_008: [ gballoc_ll_print_stats shall call je_malloc_stats_print and pass to it jemalloc_print_stats_callback as print callback. ]*/
/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_010: [ Otherwise, jemalloc_print_stats_callback shall print (log) text, breaking it does in chunks of LOG_MAX_MESSAGE_LENGTH / 2. ]*/
TEST_FUNCTION(gballoc_ll_print_stats_calls_the_print_function_and_prints_multiple_small_text_lines)
{
    ///arrange
    g_call_print_cb_count = 3;
    g_call_print_cb[0].text_to_print = "Don't";
    g_call_print_cb[1].text_to_print = "Panic";
    g_call_print_cb[2].text_to_print = "!!!";
    STRICT_EXPECTED_CALL(mock_je_malloc_stats_print(IGNORED_ARG, NULL, NULL));

    ///act
    gballoc_ll_print_stats();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_008: [ gballoc_ll_print_stats shall call je_malloc_stats_print and pass to it jemalloc_print_stats_callback as print callback. ]*/
/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_010: [ Otherwise, jemalloc_print_stats_callback shall print (log) text, breaking it does in chunks of LOG_MAX_MESSAGE_LENGTH / 2. ]*/
TEST_FUNCTION(gballoc_ll_print_stats_calls_the_print_function_and_prints_one_huge_line)
{
    ///arrange
    size_t huge_line_length = 1 * 1024 * 1024;
    char* huge_line = malloc(huge_line_length + 1);
    ASSERT_IS_NOT_NULL(huge_line);

    (void)memset(huge_line, 'x', huge_line_length);
    huge_line[huge_line_length] = '\0';

    g_call_print_cb_count = 1;
    g_call_print_cb[0].text_to_print = huge_line;
    STRICT_EXPECTED_CALL(mock_je_malloc_stats_print(IGNORED_ARG, NULL, NULL));

    ///act
    gballoc_ll_print_stats();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    free(huge_line);
}

/* Tests_SRS_GBALLOC_LL_JEMALLOC_01_009: [ If text is NULL, jemalloc_print_stats_callback shall return. ]*/
TEST_FUNCTION(gballoc_ll_print_stats_calls_the_print_function_with_NULL_does_not_crash)
{
    ///arrange
    g_call_print_cb_count = 1;
    g_call_print_cb[0].text_to_print = NULL;
    STRICT_EXPECTED_CALL(mock_je_malloc_stats_print(IGNORED_ARG, NULL, NULL));

    ///act
    gballoc_ll_print_stats();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_ll_set_option */

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_001: [ If option_name is NULL, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_with_NULL_option_name_fails)
{
    ///arrange
    void* option_value = (void*)0x42;

    ///act
    int result = gballoc_ll_set_option(NULL, option_value);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_002: [ If option_value is NULL, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_with_NULL_option_value_fails)
{
    ///arrange

    ///act
    int result = gballoc_ll_set_option("dirty_decay", NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_017: [ Otherwise gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_with_unknown_option_name_fails)
{
    ///arrange
    void* option_value = (void*)0x42;

    ///act
    int result = gballoc_ll_set_option("unknown_option", option_value);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_019: [ If decay_milliseconds is less than -1, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_with_dirty_decay_and_negative_decay_milliseconds_fails)
{
    ///arrange
    int64_t decay_milliseconds = -2;

    ///act
    int result = gballoc_ll_set_option("dirty_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_019: [ If decay_milliseconds is less than -1, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_with_muzzy_decay_and_negative_decay_milliseconds_fails)
{
    ///arrange
    int64_t decay_milliseconds = -2;

    ///act
    int result = gballoc_ll_set_option("muzzy_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    ///clean
}

static void setup_option_success_expectations(char** first_command, char** second_command, char** third_command, char** fourth_command, uint32_t* num_arenas, int64_t* decay_milliseconds)
{
    int64_t old_decay_milliseconds = 24;
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_name(first_command)
        .CopyOutArgumentBuffer_oldp(&old_decay_milliseconds, sizeof(old_decay_milliseconds))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .SetReturn(0)
        .SetFailReturn(1);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, NULL, 0))
        .CaptureArgumentValue_name(second_command)
        .CopyOutArgumentBuffer_oldp(num_arenas, sizeof(*num_arenas))
        .SetReturn(0)
        .SetFailReturn(1);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .ValidateArgumentBuffer(1, third_command, sizeof(*third_command))
        .SetReturn(0)
        .SetFailReturn(1);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .ValidateArgumentBuffer(1, fourth_command, sizeof(*fourth_command))
        .SetReturn(0)
        .SetFailReturn(1);
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_003: [ If option_name has value as dirty_decay or muzzy_decay: ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_004: [ gballoc_ll_set_option shall fetch the decay_milliseconds value by casting option_value to int64_t. ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_005: [ gballoc_ll_set_option shall retrieve the old decay value and set the new decay value to decay_milliseconds for new arenas by calling je_mallctl with arenas.dirty_decay_ms if option_name is dirty_decay or arenas.muzzy_decay_ms if option_name is muzzy_decay as the command. ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_007: [ gballoc_ll_set_option shall fetch the number of existing jemalloc arenas by calling je_mallctl with opt.narenas as the command. ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_008: [ For each existing arena except last (since it is reserved for huge arena) ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_009: [ gballoc_ll_set_option shall set the decay time for the arena to decay_milliseconds milliseconds by calling je_mallctl with arena.<i>.dirty_decay_ms if option_name is dirty_decay or arena.<i>.muzzy_decay_ms if option_name is muzzy_decay as the command. ]*/
TEST_FUNCTION(gballoc_ll_set_option_with_dirty_decay_succeeds)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.dirty_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.dirty_decay_ms");

    setup_option_success_expectations(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &num_arenas, &decay_milliseconds);

    ///act
    int result = gballoc_ll_set_option("dirty_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.dirty_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_003: [ If option_name has value as dirty_decay or muzzy_decay: ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_004: [ gballoc_ll_set_option shall fetch the decay_milliseconds value by casting option_value to int64_t. ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_005: [ gballoc_ll_set_option shall retrieve the old decay value and set the new decay value to decay_milliseconds for new arenas by calling je_mallctl with arenas.dirty_decay_ms if option_name is dirty_decay or arenas.muzzy_decay_ms if option_name is muzzy_decay as the command. ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_007: [ gballoc_ll_set_option shall fetch the number of existing jemalloc arenas by calling je_mallctl with opt.narenas as the command. ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_008: [ For each existing arena except last (since it is reserved for huge arena) ]*/
/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_009: [ gballoc_ll_set_option shall set the decay time for the arena to decay_milliseconds milliseconds by calling je_mallctl with arena.<i>.dirty_decay_ms if option_name is dirty_decay or arena.<i>.muzzy_decay_ms if option_name is muzzy_decay as the command. ]*/
TEST_FUNCTION(gballoc_ll_set_option_with_muzzy_decay_succeeds)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.muzzy_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.muzzy_decay_ms");

    setup_option_success_expectations(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &num_arenas, &decay_milliseconds);

    ///act
    int result = gballoc_ll_set_option("muzzy_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.muzzy_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_fails_when_underlying_calls_fail)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.dirty_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.dirty_decay_ms");

    setup_option_success_expectations(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &num_arenas, &decay_milliseconds);

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            int result = gballoc_ll_set_option("dirty_decay", &decay_milliseconds);

            ///assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_fails_when_underlying_calls_fail_2)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.muzzy_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.muzzy_decay_ms");

    setup_option_success_expectations(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &num_arenas, &decay_milliseconds);

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            int result = gballoc_ll_set_option("muzzy_decay", &decay_milliseconds);

            ///assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }
}

static void setup_failure_expectations_when_number_of_arenas_read_fails(char** first_command, char** second_command, char** third_command, int64_t* decay_milliseconds)
{
    int64_t old_decay_milliseconds = 24;
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_name(first_command)
        .CopyOutArgumentBuffer_oldp(&old_decay_milliseconds, sizeof(old_decay_milliseconds))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .SetReturn(0);
    // Reading the number of arenas fails
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, NULL, 0))
        .CaptureArgumentValue_name(second_command)
        .SetReturn(1);
    // Set the decay back to original value
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_name(third_command)
        .ValidateArgumentBuffer(4, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_fails_for_dirty_decay_if_number_of_arenas_read_fails)
{
    ///arrange
    int64_t decay_milliseconds = 42;

    char* first_command;
    char* second_command;
    char* third_command;

    setup_failure_expectations_when_number_of_arenas_read_fails(&first_command, &second_command, &third_command, &decay_milliseconds);

    ///act
    int result = gballoc_ll_set_option("dirty_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "arenas.dirty_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.dirty_decay_ms", third_command);

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_fails_for_muzzy_decay_if_number_of_arenas_read_fails)
{
    ///arrange
    int64_t decay_milliseconds = 42;

    char* first_command;
    char* second_command;
    char* third_command;

    setup_failure_expectations_when_number_of_arenas_read_fails(&first_command, &second_command, &third_command, &decay_milliseconds);

    ///act
    int result = gballoc_ll_set_option("muzzy_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "arenas.muzzy_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.muzzy_decay_ms", third_command);

    ///clean
}

static void setup_failure_expectations_when_setting_decay_for_second_arenas_fails(char** first_command, char** second_command, char** third_command, char** fourth_command, char** fifth_command, int64_t* decay_milliseconds, uint32_t* num_arenas)
{
    int64_t old_decay_milliseconds = 24;
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_name(first_command)
        .CopyOutArgumentBuffer_oldp(&old_decay_milliseconds, sizeof(old_decay_milliseconds))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, NULL, 0))
        .CaptureArgumentValue_name(second_command)
        .CopyOutArgumentBuffer_oldp(num_arenas, sizeof(*num_arenas))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .ValidateArgumentBuffer(1, third_command, sizeof(*third_command))
        .SetReturn(0);
    // Setting decay for the second arena fails
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .ValidateArgumentBuffer(1, fourth_command, sizeof(*fourth_command))
        .SetReturn(1);
    // Undo the decay for first arena
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentBuffer(1, third_command, sizeof(*third_command));
    // // Set the decay back to original value
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_name(fifth_command)
        .ValidateArgumentBuffer(4, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_fails_for_dirty_decay_if_setting_dirty_decay_for_second_arena_fails)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.dirty_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.dirty_decay_ms");
    char* fifth_command;

    setup_failure_expectations_when_setting_decay_for_second_arenas_fails(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &fifth_command, &decay_milliseconds, &num_arenas);    

    ///act
    int result = gballoc_ll_set_option("dirty_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "arenas.dirty_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.dirty_decay_ms", fifth_command);

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_ll_set_option_fails_for_muzzy_decay_if_setting_muzzy_decay_for_second_arena_fails)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.muzzy_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.muzzy_decay_ms");
    char* fifth_command;

    setup_failure_expectations_when_setting_decay_for_second_arenas_fails(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &fifth_command, &decay_milliseconds, &num_arenas);    

    ///act
    int result = gballoc_ll_set_option("muzzy_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "arenas.muzzy_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.muzzy_decay_ms", fifth_command);

    ///clean
}

static void setup_expectations_for_mallctl_returning_EFAULT(char** first_command, char** second_command, char** third_command, char** fourth_command, uint32_t* num_arenas, int64_t* decay_milliseconds)
{
    int64_t old_decay_milliseconds = 24;
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_name(first_command)
        .CopyOutArgumentBuffer_oldp(&old_decay_milliseconds, sizeof(old_decay_milliseconds))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, NULL, 0))
        .CaptureArgumentValue_name(second_command)
        .CopyOutArgumentBuffer_oldp(num_arenas, sizeof(*num_arenas))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .ValidateArgumentBuffer(1, third_command, sizeof(*third_command))
        .SetReturn(EFAULT);
    STRICT_EXPECTED_CALL(mock_je_mallctl(IGNORED_ARG, NULL, NULL, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentBuffer(4, decay_milliseconds, sizeof(*decay_milliseconds))
        .ValidateArgumentBuffer(1, fourth_command, sizeof(*fourth_command))
        .SetReturn(EFAULT);
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_020: [ If je_mallctl returns EFAULT, gballoc_ll_set_option shall continue without failing as this error is expected when the arena doesn't exist. ]*/
TEST_FUNCTION(gballoc_ll_set_option_for_dirty_decay_succeeds_when_je_mallctl_returns_EFAULT)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.dirty_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.dirty_decay_ms");

    setup_expectations_for_mallctl_returning_EFAULT(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &num_arenas, &decay_milliseconds);

    ///act
    int result = gballoc_ll_set_option("dirty_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.dirty_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_LL_JEMALLOC_28_020: [ If je_mallctl returns EFAULT, gballoc_ll_set_option shall continue without failing as this error is expected when the arena doesn't exist. ]*/
TEST_FUNCTION(gballoc_ll_set_option_for_muzzy_decay_succeeds_when_je_mallctl_returns_EFAULT)
{
    ///arrange
    int64_t decay_milliseconds = 42;
    uint32_t num_arenas = NUM_ARENAS;

    char* first_command;
    char* second_command;
    char third_command[32];
    (void)sprintf(third_command, "arena.0.muzzy_decay_ms");
    char fourth_command[32];
    (void)sprintf(fourth_command, "arena.1.muzzy_decay_ms");

    setup_expectations_for_mallctl_returning_EFAULT(&first_command, &second_command, (char**)&third_command, (char**)&fourth_command, &num_arenas, &decay_milliseconds);

    ///act
    int result = gballoc_ll_set_option("muzzy_decay", &decay_milliseconds);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, "arenas.muzzy_decay_ms", first_command);
    ASSERT_ARE_EQUAL(char_ptr, "opt.narenas", second_command);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
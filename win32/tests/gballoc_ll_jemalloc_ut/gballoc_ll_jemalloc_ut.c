// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

static void* TEST_MALLOC_RESULT = (void*)0x1;
static void* TEST_CALLOC_RESULT = (void*)0x2;
static void* TEST_REALLOC_RESULT = (void*)0x3;

#include "umock_c/umock_c.h"

typedef void (*JEMALLOC_WRITE_CB)(void*, const char*);

typedef struct PRINT_FUNCTION_CB_DATA_TAG
{
    const char* text_to_print;
} PRINT_FUNCTION_CB_DATA;

#define MAX_PRINT_FUNCTION_CB 10

static size_t g_call_print_cb_count = 0;
static PRINT_FUNCTION_CB_DATA g_call_print_cb[MAX_PRINT_FUNCTION_CB];


#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"

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
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
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

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "gballoc_hl_passthrough_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void TEST_gballoc_hl_init(void)
{
    STRICT_EXPECTED_CALL(gballoc_ll_init(IGNORED_ARG));
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static void TEST_gballoc_hl_deinit(void)
{
    STRICT_EXPECTED_CALL(gballoc_ll_deinit());
    gballoc_hl_deinit();
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(init_suite)
{
    umock_c_init(on_umock_c_error);

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());

    REGISTER_GBALLOC_LL_GLOBAL_MOCK_HOOK();
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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_42_001: [ gballoc_hl_init shall call gballoc_ll_init as function to execute and gballoc_ll_init_params as parameter and return the result. ]*/
TEST_FUNCTION(gballoc_hl_init_happy_path)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(gballoc_ll_init((void*)0x33))
        .SetReturn(0);

    ///act
    result = gballoc_hl_init(NULL, (void*)0x33);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_42_001: [ gballoc_hl_init shall call gballoc_ll_init as function to execute and gballoc_ll_init_params as parameter and return the result. ]*/
TEST_FUNCTION(gballoc_hl_init_unhappy_path_1)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(gballoc_ll_init((void*)0x33))
        .SetReturn(1);

    ///act
    result = gballoc_hl_init(NULL, (void*)0x33);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_004: [ gballoc_hl_deinit shall call gballoc_ll_deinit. ]*/
TEST_FUNCTION(gballoc_hl_deinit_calls_ll_deinit)
{
    ///arrange
    int result;
    result = gballoc_hl_init(NULL, (void*)0x33);
    ASSERT_ARE_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_ll_deinit());

    ///act
    gballoc_hl_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_005: [ gballoc_hl_malloc shall call gballoc_ll_malloc(size) and return what gballoc_ll_malloc returned. ]*/
TEST_FUNCTION(gballoc_hl_malloc_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_malloc(3));

    ///act
    result = gballoc_hl_malloc(3);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_005: [ gballoc_hl_malloc shall call gballoc_ll_malloc(size) and return what gballoc_ll_malloc returned. ]*/
TEST_FUNCTION(gballoc_hl_malloc_unhappy_path_1)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_malloc(3))
        .SetReturn(NULL);

    ///act
    result = gballoc_hl_malloc(3);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_030: [ gballoc_hl_malloc_2 shall call gballoc_ll_malloc_2(size) and return what gballoc_ll_malloc_2 returned. ]*/
TEST_FUNCTION(gballoc_hl_malloc_2_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_malloc_2(3, 4));

    ///act
    result = gballoc_hl_malloc_2(3, 4);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_030: [ gballoc_hl_malloc_2 shall call gballoc_ll_malloc_2(size) and return what gballoc_ll_malloc_2 returned. ]*/
TEST_FUNCTION(gballoc_hl_malloc_2_unhappy_path_1)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_malloc_2(3, 4))
        .SetReturn(NULL);

    ///act
    result = gballoc_hl_malloc_2(3, 4);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_033: [ gballoc_hl_malloc_flex shall call gballoc_ll_malloc_flex(size) and return what gballoc_hl_malloc_flex returned. ]*/
TEST_FUNCTION(gballoc_hl_malloc_flex_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_malloc_flex(2, 3, 4));

    ///act
    result = gballoc_hl_malloc_flex(2, 3, 4);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_033: [ gballoc_hl_malloc_flex shall call gballoc_ll_malloc_flex(size) and return what gballoc_hl_malloc_flex returned. ]*/
TEST_FUNCTION(gballoc_hl_malloc_flex_unhappy_path_1)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_malloc_flex(2, 3, 4))
        .SetReturn(NULL);

    ///act
    result = gballoc_hl_malloc_flex(2, 3, 4);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_006: [ gballoc_hl_free shall call gballoc_ll_free(ptr). ]*/
TEST_FUNCTION(gballoc_hl_free_with_NULL_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_ll_free(NULL));

    ///act
    gballoc_hl_free(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_006: [ gballoc_hl_free shall call gballoc_ll_free(ptr). ]*/
TEST_FUNCTION(gballoc_hl_free_with_non_NULL_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_ll_free(ptr));

    ///act
    gballoc_hl_free(ptr);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_007: [ gballoc_hl_calloc shall call gballoc_ll_calloc(nmemb, size) and return what gballoc_ll_calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_calloc(1, 2));

    ///act
    result = gballoc_hl_calloc(1, 2);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_007: [ gballoc_hl_calloc shall call gballoc_ll_calloc(nmemb, size) and return what gballoc_ll_calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_when_ll_fails_it_fails)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_calloc(1, 2))
        .SetReturn(NULL);

    ///act
    result = gballoc_hl_calloc(1, 2);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_008: [ gballoc_hl_realloc shall call gballoc_ll_realloc(ptr, size) and return what gballoc_ll_realloc returned. ]*/
TEST_FUNCTION(gballoc_hl_realloc_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_realloc(ptr, 10));

    ///act
    result = gballoc_hl_realloc(ptr, 10);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_008: [ gballoc_hl_realloc shall call gballoc_ll_realloc(ptr, size) and return what gballoc_ll_realloc returned. ]*/
TEST_FUNCTION(gballoc_hl_realloc_when_ll_fails)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_realloc(ptr, 10))
        .SetReturn(NULL);

    ///act
    result = gballoc_hl_realloc(ptr, 10);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(ptr);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_036: [ gballoc_hl_realloc_2 shall call gballoc_ll_realloc_2(ptr, nmemb, size) and return what gballoc_ll_realloc_2 returned. ]*/
TEST_FUNCTION(gballoc_hl_realloc_2_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_realloc_2(ptr, 10, 10));

    ///act
    result = gballoc_hl_realloc_2(ptr, 10, 10);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_036: [ gballoc_hl_realloc_2 shall call gballoc_ll_realloc_2(ptr, nmemb, size) and return what gballoc_ll_realloc_2 returned. ]*/
TEST_FUNCTION(gballoc_hl_realloc_2_unhappy_path_1)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_realloc_2(ptr, 10, 10))
        .SetReturn(NULL);

    ///act
    result = gballoc_hl_realloc_2(ptr, 10, 10);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(ptr);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_039: [ gballoc_hl_realloc_flex shall call gballoc_ll_realloc_flex(ptr, base, nmemb, size) and return what gballoc_ll_realloc_flex returned. ]*/
TEST_FUNCTION(gballoc_hl_realloc_flex_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_realloc_flex(ptr, 3, 10, 10));

    ///act
    result = gballoc_hl_realloc_flex(ptr, 3, 10, 10);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_039: [ gballoc_hl_realloc_flex shall call gballoc_ll_realloc_flex(ptr, base, nmemb, size) and return what gballoc_ll_realloc_flex returned. ]*/
TEST_FUNCTION(gballoc_hl_realloc_flex_unhappy_path_1)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(gballoc_ll_realloc_flex(ptr, 3, 10, 10))
        .SetReturn(NULL);

    ///act
    result = gballoc_hl_realloc_flex(ptr, 3, 10, 10);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(ptr);
    TEST_gballoc_hl_deinit();
}

/* Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_014: [ gballoc_hl_get_latency_bucket_metadata shall return an array of size LATENCY_BUCKET_COUNT that contains the metadata for each latency bucket. ]*/
/* Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_015: [ The first latency bucket shall be [0-511]. ]*/
/* Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_016: [ Each consecutive bucket shall be [1 << n, (1 << (n + 1)) - 1], where n starts at 9. ]*/
TEST_FUNCTION(gballoc_hl_get_latency_bucket_metadata_returns_the_array_with_the_latency_buckets_metadata)
{
    // arrange

    // act
    const GBALLOC_LATENCY_BUCKET_METADATA* latency_buckets_metadata = gballoc_hl_get_latency_bucket_metadata();

    // assert
    for (size_t i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        ASSERT_IS_NOT_NULL(latency_buckets_metadata[i].bucket_name);
        if (i == 0)
        {
            ASSERT_ARE_EQUAL(uint32_t, 0, latency_buckets_metadata[i].size_range_low);
            ASSERT_ARE_EQUAL(uint32_t, 511, latency_buckets_metadata[i].size_range_high);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, (uint32_t)1 << (8 + i), latency_buckets_metadata[i].size_range_low);
            ASSERT_ARE_EQUAL(uint32_t, ((uint64_t)1 << (9 + i)) - 1, latency_buckets_metadata[i].size_range_high);
        }
    }

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_009: [ gballoc_hl_reset_counters shall return. ]*/
TEST_FUNCTION(gballoc_hl_reset_counters_returns)
{
    ///arrange
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();

    ///act
    gballoc_hl_reset_counters();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_010: [ gballoc_hl_get_malloc_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
TEST_FUNCTION(gballoc_hl_get_malloc_latency_buckets_zeroes)
{
    ///arrange
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();
    int result;
    GBALLOC_LATENCY_BUCKETS actual;
    (void)memset(&actual, '3', sizeof(actual)); /*set all bytes to '3'*/

    GBALLOC_LATENCY_BUCKETS expected;
    (void)memset(&expected, 0, sizeof(expected));

    ///act
    result = gballoc_hl_get_malloc_latency_buckets(&actual);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, memcmp(&actual, &expected, sizeof(actual)));

    ///clean
    gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_011: [ gballoc_hl_get_realloc_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
TEST_FUNCTION(gballoc_hl_get_realloc_latency_buckets_zeroes)
{
    ///arrange
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();
    int result;
    GBALLOC_LATENCY_BUCKETS actual;
    (void)memset(&actual, '3', sizeof(actual)); /*set all bytes to '3'*/

    GBALLOC_LATENCY_BUCKETS expected;
    (void)memset(&expected, 0, sizeof(expected));

    ///act
    result = gballoc_hl_get_realloc_latency_buckets(&actual);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, memcmp(&actual, &expected, sizeof(actual)));

    ///clean
    gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_012: [ gballoc_hl_get_calloc_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
TEST_FUNCTION(gballoc_hl_get_calloc_latency_buckets_zeroes)
{
    ///arrange
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();
    int result;
    GBALLOC_LATENCY_BUCKETS actual;
    (void)memset(&actual, '3', sizeof(actual)); /*set all bytes to '3'*/

    GBALLOC_LATENCY_BUCKETS expected;
    (void)memset(&expected, 0, sizeof(expected));

    ///act
    result = gballoc_hl_get_calloc_latency_buckets(&actual);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, memcmp(&actual, &expected, sizeof(actual)));

    ///clean
    gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_013: [ gballoc_hl_get_free_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
TEST_FUNCTION(gballoc_hl_get_free_latency_buckets_zeroes)
{
    ///arrange
    (void)gballoc_hl_init(NULL, NULL);
    umock_c_reset_all_calls();
    int result;
    GBALLOC_LATENCY_BUCKETS actual;
    (void)memset(&actual, '3', sizeof(actual)); /*set all bytes to '3'*/

    GBALLOC_LATENCY_BUCKETS expected;
    (void)memset(&expected, 0, sizeof(expected));

    ///act
    result = gballoc_hl_get_free_latency_buckets(&actual);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, memcmp(&actual, &expected, sizeof(actual)));

    ///clean
    gballoc_hl_deinit();
}

/* gballoc_hl_print_stats */

/* Tests_SRS_GBALLOC_HL_PASSTHROUGH_01_001: [ gballoc_hl_print_stats shall call into gballoc_ll_print_stats to print the memory allocator statistics. ]*/
TEST_FUNCTION(gballoc_hl_print_stats_calls_gballoc_ll_print_stats)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_ll_print_stats());

    ///act
    gballoc_hl_print_stats();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_set_option */

/* Tests_SRS_GBALLOC_HL_PASSTHROUGH_28_001: [ gballoc_hl_set_option shall call gballoc_ll_set_option with option_name and option_value as arguments. ]*/
TEST_FUNCTION(gballoc_hl_set_option_calls_gballoc_ll_set_option_and_returns_0)
{
    ///arrange
    void* value = (void*)0x42;
    char* option_name;

    STRICT_EXPECTED_CALL(gballoc_ll_set_option(IGNORED_ARG, value))
        .CaptureArgumentValue_option_name(&option_name)
        .SetReturn(0);

    ///act
    int result = gballoc_hl_set_option("dirty_decay", value);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, option_name, "dirty_decay");
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_GBALLOC_HL_PASSTHROUGH_28_001: [ gballoc_hl_set_option shall call gballoc_ll_set_option with option_name and option_value as arguments. ]*/
TEST_FUNCTION(gballoc_hl_set_option_calls_gballoc_ll_set_option_and_returns_non_zero)
{
    ///arrange
    void* value = (void*)0x42;
    char* option_name;

    STRICT_EXPECTED_CALL(gballoc_ll_set_option(IGNORED_ARG, value))
        .CaptureArgumentValue_option_name(&option_name)
        .SetReturn(1);

    ///act
    int result = gballoc_hl_set_option("dirty_decay", value);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, option_name, "dirty_decay");
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* gballoc_hl_size */

/* Tests_SRS_GBALLOC_HL_PASSTHROUGH_01_003: [ Otherwise, gballoc_hl_size shall call gballoc_ll_size with ptr as argument and return the result of gballoc_ll_size. ]*/
TEST_FUNCTION(gballoc_hl_size_returns_the_result_of_the_underlying_ll_size)
{
    ///arrange
    size_t returned_size;
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_ll_size(IGNORED_ARG))
        .CaptureReturn(&returned_size);

    ///act
    size_t size = gballoc_hl_size(ptr);

    ///assert
    ASSERT_ARE_EQUAL(size_t, returned_size, size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(ptr);
    TEST_gballoc_hl_deinit();
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_ll.h"
#include "c_pal/interlocked.h"
#include "c_pal/lazy_init.h"
#undef ENABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

#include "real_gballoc_ll.h"
#include "real_interlocked.h"
#include "real_lazy_init.h"

#include "c_pal/gballoc_hl.h"

MU_DEFINE_ENUM_STRINGS(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);

static void TEST_gballoc_hl_init(void)
{
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(gballoc_ll_init(IGNORED_ARG));
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static void TEST_gballoc_hl_deinit(void)
{
    STRICT_EXPECTED_CALL(gballoc_ll_deinit());
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, LAZY_INIT_NOT_DONE));
    gballoc_hl_deinit();
}

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(init_suite)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());

    REGISTER_UMOCK_ALIAS_TYPE(LAZY_INIT_FUNCTION, void*);

    REGISTER_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT);

    REGISTER_GBALLOC_LL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_LAZY_INIT_GLOBAL_MOCK_HOOK();
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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_017: [ gballoc_hl_init shall call lazy_init with do_init as function to execute and gballoc_ll_init_params as parameter. ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_018: [ do_init shall call gballoc_ll_init(params). ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_019: [ do_init shall return 0. ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_002: [ gballoc_hl_init shall succeed and return 0. ]*/
TEST_FUNCTION(gballoc_hl_init_happy_path)
{
    ///arrange
    int result;
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, (void*)0x33));

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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_003: [ If there are any failures then gballoc_hl_init shall fail and return a non-zero value. ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_020: [ If there are any failures then do_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_hl_init_unhappy_path_1)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, (void*)0x33))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    result = gballoc_hl_init(NULL, (void*)0x33);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_003: [ If there are any failures then gballoc_hl_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(gballoc_hl_init_unhappy_path_2)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, (void*)0x33));

    STRICT_EXPECTED_CALL(gballoc_ll_init((void*)0x33))
        .SetReturn(1);

    ///act
    result = gballoc_hl_init(NULL, (void*)0x33);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_004: [ gballoc_hl_deinit shall call gballoc_ll_deinit. ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_021: [ gballoc_hl_deinit shall switch module's state to LAZY_INIT_NOT_DONE ]*/
TEST_FUNCTION(gballoc_hl_deinit_does_nothing_when_not_init)
{
    ///arrange
    int result;
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, (void*)0x33))
        .SetReturn(LAZY_INIT_ERROR);
    result = gballoc_hl_init(NULL, (void*)0x33);
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_ll_deinit());
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, LAZY_INIT_NOT_DONE));

    ///act
    gballoc_hl_deinit();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_022: [ gballoc_hl_malloc shall call lazy_init passing as execution function do_init and NULL for argument. ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_005: [ gballoc_hl_malloc shall call gballoc_ll_malloc(size) and return what gballoc_ll_malloc returned. ]*/
TEST_FUNCTION(gballoc_hl_malloc_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL));

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

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL));

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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_023: [ If lazy_init fail then gballoc_hl_malloc shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_hl_malloc_unhappy_path_2)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    result = gballoc_hl_malloc(3);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TEST_gballoc_hl_deinit();
}

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_006: [ gballoc_hl_free shall call gballoc_hl_free(ptr). ]*/
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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_006: [ gballoc_hl_free shall call gballoc_hl_free(ptr). ]*/
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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_024: [ gballoc_hl_calloc shall call lazy_init passing as execution function do_init and NULL for argument. ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_007: [ gballoc_hl_calloc shall call gballoc_ll_calloc(nmemb, size) and return what gballoc_ll_calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL));

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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_025: [ If lazy_init fail then gballoc_hl_calloc shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_ll_calloc_fails_when_lazy_init_fails)
{
    ///arrange
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    result = gballoc_hl_calloc(1, 2);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(result);
}
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_007: [ gballoc_hl_calloc shall call gballoc_ll_calloc(nmemb, size) and return what gballoc_ll_calloc returned. ]*/
TEST_FUNCTION(gballoc_ll_calloc_when_ll_fails_it_fails)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL));

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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_026: [ gballoc_hl_realloc shall call lazy_init passing as execution function do_init and NULL for argument. ]*/
/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_008: [ gballoc_hl_realloc shall call gballoc_ll_realloc(ptr, size) and return what gballoc_ll_realloc returned. ]*/
TEST_FUNCTION(gballoc_hl_realloc_succeeds)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL));

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

/*Tests_SRS_GBALLOC_HL_PASSTHROUGH_02_027: [ If lazy_init fail then gballoc_hl_realloc shall fail and return NULL. ]*/
TEST_FUNCTION(gballoc_hl_realloc_fails_when_lazy_init_fails)
{
    ///arrange
    TEST_gballoc_hl_init();
    void* ptr = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    void* result;

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetReturn(LAZY_INIT_ERROR);

    ///act
    result = gballoc_hl_realloc(ptr, 10);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    gballoc_hl_free(ptr);
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

    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, NULL));

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

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

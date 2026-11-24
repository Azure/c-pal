// Copyright (C) Microsoft Corporation. All rights reserved.

#include "thread_count_policy_linux_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static THREAD_COUNT_POLICY_HANDLE test_create_policy(uint32_t min_thread_count, uint32_t max_thread_count)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THREAD_COUNT_POLICY_HANDLE result = thread_count_policy_create(min_thread_count, max_thread_count);
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    umock_c_reset_all_calls();
}

/* thread_count_policy_create */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_001: [ If max_thread_count is not 0 and is less than min_thread_count, thread_count_policy_create shall fail and return NULL. ]*/
TEST_FUNCTION(thread_count_policy_create_with_max_thread_count_less_than_min_thread_count_fails)
{
    ///arrange

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(5, 4);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_001: [ If max_thread_count is not 0 and is less than min_thread_count, thread_count_policy_create shall fail and return NULL. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_005: [ If max_thread_count is not 0, thread_count_policy_create shall use max_thread_count as the maximum thread count. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_008: [ thread_count_policy_create shall succeed and return the policy. ]*/
TEST_FUNCTION(thread_count_policy_create_with_max_thread_count_equal_to_min_thread_count_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(4, 4);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_IS_TRUE(thread_count_policy_can_add_thread(thread_count_policy, 3));
    ASSERT_IS_FALSE(thread_count_policy_can_add_thread(thread_count_policy, 4));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_002: [ thread_count_policy_create shall allocate a new policy. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_005: [ If max_thread_count is not 0, thread_count_policy_create shall use max_thread_count as the maximum thread count. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_008: [ thread_count_policy_create shall succeed and return the policy. ]*/
TEST_FUNCTION(thread_count_policy_create_with_max_thread_count_uses_max_thread_count_as_maximum)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(2, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_IS_TRUE(thread_count_policy_can_add_thread(thread_count_policy, 9));
    ASSERT_IS_FALSE(thread_count_policy_can_add_thread(thread_count_policy, 10));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_002: [ thread_count_policy_create shall allocate a new policy. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_004: [ If max_thread_count is 0, thread_count_policy_create shall use 500 as the maximum thread count, or min_thread_count if it is larger. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_008: [ thread_count_policy_create shall succeed and return the policy. ]*/
TEST_FUNCTION(thread_count_policy_create_with_max_thread_count_0_uses_500_as_maximum)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(0, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_IS_TRUE(thread_count_policy_can_add_thread(thread_count_policy, 499));
    ASSERT_IS_FALSE(thread_count_policy_can_add_thread(thread_count_policy, 500));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_004: [ If max_thread_count is 0, thread_count_policy_create shall use 500 as the maximum thread count, or min_thread_count if it is larger. ]*/
TEST_FUNCTION(thread_count_policy_create_with_max_thread_count_0_and_min_thread_count_below_500_uses_500_as_maximum)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(4, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_IS_TRUE(thread_count_policy_can_add_thread(thread_count_policy, 499));
    ASSERT_IS_FALSE(thread_count_policy_can_add_thread(thread_count_policy, 500));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_004: [ If max_thread_count is 0, thread_count_policy_create shall use 500 as the maximum thread count, or min_thread_count if it is larger. ]*/
TEST_FUNCTION(thread_count_policy_create_with_max_thread_count_0_and_min_thread_count_above_500_uses_min_thread_count_as_maximum)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(600, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_IS_TRUE(thread_count_policy_can_add_thread(thread_count_policy, 500));
    ASSERT_IS_TRUE(thread_count_policy_can_add_thread(thread_count_policy, 599));
    ASSERT_IS_FALSE(thread_count_policy_can_add_thread(thread_count_policy, 600));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_003: [ thread_count_policy_create shall store min_thread_count as the minimum thread count. ]*/
TEST_FUNCTION(thread_count_policy_create_stores_min_thread_count_as_minimum)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(4, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, thread_count_policy_get_retire_thread_time(thread_count_policy, 4, 1, 1000, 2000));
    ASSERT_ARE_EQUAL(uint64_t, 61000, thread_count_policy_get_retire_thread_time(thread_count_policy, 5, 1, 1000, 2000));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_006: [ thread_count_policy_create shall set the last progress time to 0. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_007: [ thread_count_policy_create shall set the earliest time to add a thread to 0. ]*/
TEST_FUNCTION(thread_count_policy_create_sets_last_progress_time_and_earliest_add_thread_time_to_0)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(0, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_ARE_EQUAL(uint64_t, 50, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_009: [ If any error occurs, thread_count_policy_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_malloc_fails_thread_count_policy_create_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(0, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(thread_count_policy);
}

/* thread_count_policy_destroy */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_010: [ If thread_count_policy is NULL, thread_count_policy_destroy shall return. ]*/
TEST_FUNCTION(thread_count_policy_destroy_with_NULL_thread_count_policy_returns)
{
    ///arrange

    ///act
    thread_count_policy_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_011: [ thread_count_policy_destroy shall free thread_count_policy. ]*/
TEST_FUNCTION(thread_count_policy_destroy_frees_the_policy)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    STRICT_EXPECTED_CALL(free(thread_count_policy));

    ///act
    thread_count_policy_destroy(thread_count_policy);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* thread_count_policy_can_add_thread */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_012: [ If thread_count_policy is NULL, thread_count_policy_can_add_thread shall return false. ]*/
TEST_FUNCTION(thread_count_policy_can_add_thread_with_NULL_thread_count_policy_returns_false)
{
    ///arrange

    ///act
    bool result = thread_count_policy_can_add_thread(NULL, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(result);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_013: [ thread_count_policy_can_add_thread shall return whether thread_count is less than the maximum thread count. ]*/
TEST_FUNCTION(thread_count_policy_can_add_thread_with_no_threads_returns_true)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    bool result = thread_count_policy_can_add_thread(thread_count_policy, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_013: [ thread_count_policy_can_add_thread shall return whether thread_count is less than the maximum thread count. ]*/
TEST_FUNCTION(thread_count_policy_can_add_thread_with_thread_count_one_below_maximum_returns_true)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    bool result = thread_count_policy_can_add_thread(thread_count_policy, 9);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_013: [ thread_count_policy_can_add_thread shall return whether thread_count is less than the maximum thread count. ]*/
TEST_FUNCTION(thread_count_policy_can_add_thread_with_thread_count_at_maximum_returns_false)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    bool result = thread_count_policy_can_add_thread(thread_count_policy, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_013: [ thread_count_policy_can_add_thread shall return whether thread_count is less than the maximum thread count. ]*/
TEST_FUNCTION(thread_count_policy_can_add_thread_with_thread_count_above_maximum_returns_false)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    bool result_above = thread_count_policy_can_add_thread(thread_count_policy, 11);
    bool result_uint32_max = thread_count_policy_can_add_thread(thread_count_policy, UINT32_MAX);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(result_above);
    ASSERT_IS_FALSE(result_uint32_max);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/* thread_count_policy_on_progress */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_014: [ If thread_count_policy is NULL, thread_count_policy_on_progress shall return. ]*/
TEST_FUNCTION(thread_count_policy_on_progress_with_NULL_thread_count_policy_returns)
{
    ///arrange

    ///act
    thread_count_policy_on_progress(NULL, 1000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_015: [ thread_count_policy_on_progress shall set the last progress time to now_ms. ]*/
TEST_FUNCTION(thread_count_policy_on_progress_sets_the_last_progress_time)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    ASSERT_ARE_EQUAL(uint64_t, 50, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

    ///act
    thread_count_policy_on_progress(thread_count_policy, 1000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1050, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_015: [ thread_count_policy_on_progress shall set the last progress time to now_ms. ]*/
TEST_FUNCTION(thread_count_policy_on_progress_twice_uses_the_latest_call)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 1000);

    ///act
    thread_count_policy_on_progress(thread_count_policy, 2000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 2050, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_015: [ thread_count_policy_on_progress shall set the last progress time to now_ms. ]*/
TEST_FUNCTION(thread_count_policy_on_progress_with_earlier_time_sets_the_last_progress_time_back)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 2000);

    ///act
    thread_count_policy_on_progress(thread_count_policy, 1500);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1550, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/* thread_count_policy_on_add_thread_failed */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_016: [ If thread_count_policy is NULL, thread_count_policy_on_add_thread_failed shall return. ]*/
TEST_FUNCTION(thread_count_policy_on_add_thread_failed_with_NULL_thread_count_policy_returns)
{
    ///arrange

    ///act
    thread_count_policy_on_add_thread_failed(NULL, 1000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_017: [ thread_count_policy_on_add_thread_failed shall set the earliest time to add a thread to now_ms plus 100. ]*/
TEST_FUNCTION(thread_count_policy_on_add_thread_failed_sets_the_earliest_add_thread_time_to_now_plus_100)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 1000);

    ///act
    thread_count_policy_on_add_thread_failed(thread_count_policy, 1060);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1160, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_017: [ thread_count_policy_on_add_thread_failed shall set the earliest time to add a thread to now_ms plus 100. ]*/
TEST_FUNCTION(thread_count_policy_on_add_thread_failed_with_earlier_time_sets_the_earliest_add_thread_time_back)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_add_thread_failed(thread_count_policy, 2000);

    ///act
    thread_count_policy_on_add_thread_failed(thread_count_policy, 1000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1100, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/* thread_count_policy_get_add_thread_time */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_018: [ If thread_count_policy is NULL, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_NULL_thread_count_policy_returns_NEVER)
{
    ///arrange

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(NULL, 1, 0, 0, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_019: [ If queued_work_count is not greater than starting_thread_count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_no_queued_work_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 0, 0, 0, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_019: [ If queued_work_count is not greater than starting_thread_count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_queued_work_count_equal_to_starting_thread_count_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 3, 3, 0, 3);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_019: [ If queued_work_count is not greater than starting_thread_count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_queued_work_count_less_than_starting_thread_count_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 2, 3, 0, 3);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_020: [ If idle_thread_count is not 0, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_an_idle_thread_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 5, 0, 1, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_021: [ If thread_count is not less than the maximum thread count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_thread_count_at_maximum_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 5, 0, 0, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_021: [ If thread_count is not less than the maximum thread count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_thread_count_above_maximum_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 5, 0, 0, 11);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_004: [ If max_thread_count is 0, thread_count_policy_create shall use 500 as the maximum thread count, or min_thread_count if it is larger. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_021: [ If thread_count is not less than the maximum thread count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_thread_count_at_default_maximum_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 0);

    ///act
    uint64_t result_below_maximum = thread_count_policy_get_add_thread_time(thread_count_policy, 5, 0, 0, 499);
    uint64_t result_at_maximum = thread_count_policy_get_add_thread_time(thread_count_policy, 5, 0, 0, 500);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 50, result_below_maximum);
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result_at_maximum);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_022: [ Otherwise, thread_count_policy_get_add_thread_time shall return the later of the last progress time plus 50 and the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_right_after_create_returns_50)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 50, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_022: [ Otherwise, thread_count_policy_get_add_thread_time shall return the later of the last progress time plus 50 and the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_queued_work_count_one_above_starting_thread_count_returns_add_thread_time)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 1000);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 3, 2, 0, 9);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1050, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_022: [ Otherwise, thread_count_policy_get_add_thread_time shall return the later of the last progress time plus 50 and the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_returns_last_progress_time_plus_50_when_it_is_later_than_earliest_add_thread_time)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_add_thread_failed(thread_count_policy, 1000);
    thread_count_policy_on_progress(thread_count_policy, 1200);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1250, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_022: [ Otherwise, thread_count_policy_get_add_thread_time shall return the later of the last progress time plus 50 and the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_returns_earliest_add_thread_time_when_it_is_later_than_last_progress_time_plus_50)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 1000);
    thread_count_policy_on_add_thread_failed(thread_count_policy, 1040);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1140, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_022: [ Otherwise, thread_count_policy_get_add_thread_time shall return the later of the last progress time plus 50 and the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_returns_the_common_time_when_last_progress_time_plus_50_equals_earliest_add_thread_time)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 1050);
    thread_count_policy_on_add_thread_failed(thread_count_policy, 1000);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1100, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/* thread_count_policy_get_retire_thread_time */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_023: [ If thread_count_policy is NULL, thread_count_policy_get_retire_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_retire_thread_time_with_NULL_thread_count_policy_returns_NEVER)
{
    ///arrange

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(NULL, 5, 1, 1000, 2000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_024: [ If thread_count is not greater than the minimum thread count, thread_count_policy_get_retire_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_retire_thread_time_with_thread_count_below_minimum_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(4, 10);

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(thread_count_policy, 3, 1, 1000, 100000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_024: [ If thread_count is not greater than the minimum thread count, thread_count_policy_get_retire_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_retire_thread_time_with_thread_count_equal_to_minimum_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(4, 10);

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(thread_count_policy, 4, 1, 1000, 100000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_024: [ If thread_count is not greater than the minimum thread count, thread_count_policy_get_retire_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_retire_thread_time_with_no_threads_and_minimum_0_returns_NEVER)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(thread_count_policy, 0, 0, 0, 100000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_025: [ If idle_thread_count is 0, thread_count_policy_get_retire_thread_time shall return now_ms plus 60000. ]*/
TEST_FUNCTION(thread_count_policy_get_retire_thread_time_with_no_idle_threads_returns_now_plus_60000)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(4, 10);

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(thread_count_policy, 5, 0, 12345, 2000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 62000, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_026: [ Otherwise, thread_count_policy_get_retire_thread_time shall return oldest_idle_since_ms plus 60000. ]*/
TEST_FUNCTION(thread_count_policy_get_retire_thread_time_with_idle_threads_returns_oldest_idle_since_plus_60000)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(4, 10);

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(thread_count_policy, 5, 2, 1000, 50000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 61000, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_026: [ Otherwise, thread_count_policy_get_retire_thread_time shall return oldest_idle_since_ms plus 60000. ]*/
TEST_FUNCTION(thread_count_policy_get_retire_thread_time_with_thread_idle_longer_than_60000_returns_time_in_the_past)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(thread_count_policy, 1, 1, 1000, 100000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 61000, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

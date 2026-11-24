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
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
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

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_002: [ thread_count_policy_create shall allocate a new policy. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_004: [ If max_thread_count is 0, thread_count_policy_create shall use 500 as the maximum thread count, or min_thread_count if it is larger. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_005: [ If max_thread_count is not 0, thread_count_policy_create shall use max_thread_count as the maximum thread count. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_008: [ thread_count_policy_create shall succeed and return the policy. ]*/
PARAMETERIZED_TEST_FUNCTION(thread_count_policy_create_succeeds_and_sets_the_maximum_thread_count,
    ARGS(uint32_t, min_thread_count, uint32_t, max_thread_count, uint32_t, expected_max_thread_count),
    CASE((2, 10, 10), min_2_max_10_uses_10),
    CASE((4, 4, 4), min_4_max_4_uses_4),
    CASE((0, 1, 1), min_0_max_1_uses_1),
    CASE((0, 0, 500), min_0_max_0_uses_500),
    CASE((4, 0, 500), min_4_max_0_uses_500),
    CASE((500, 0, 500), min_500_max_0_uses_500),
    CASE((600, 0, 600), min_600_max_0_uses_600))
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(min_thread_count, max_thread_count);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_IS_TRUE(thread_count_policy_can_add_thread(thread_count_policy, expected_max_thread_count - 1));
    ASSERT_IS_FALSE(thread_count_policy_can_add_thread(thread_count_policy, expected_max_thread_count));

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
TEST_FUNCTION(thread_count_policy_create_sets_the_last_progress_time_to_0)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(0, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_ARE_EQUAL(uint64_t, 50, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_007: [ thread_count_policy_create shall set the earliest time to add a thread to 0. ]*/
TEST_FUNCTION(thread_count_policy_create_sets_the_earliest_add_thread_time_to_0)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = thread_count_policy_create(0, 10);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(thread_count_policy);
    ASSERT_ARE_EQUAL(uint64_t, 0, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0));

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
PARAMETERIZED_TEST_FUNCTION(thread_count_policy_can_add_thread_returns_whether_thread_count_is_below_the_maximum,
    ARGS(uint32_t, thread_count, bool, expected_result),
    CASE((0, true), with_no_threads_returns_true),
    CASE((9, true), with_thread_count_one_below_maximum_returns_true),
    CASE((10, false), with_thread_count_at_maximum_returns_false),
    CASE((11, false), with_thread_count_above_maximum_returns_false),
    CASE((UINT32_MAX, false), with_thread_count_UINT32_MAX_returns_false))
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    bool result = thread_count_policy_can_add_thread(thread_count_policy, thread_count);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(bool, expected_result, result);

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

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_015: [ If now_ms is later than the last progress time, thread_count_policy_on_progress shall set the last progress time to now_ms. ]*/
TEST_FUNCTION(thread_count_policy_on_progress_sets_the_last_progress_time)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    ASSERT_ARE_EQUAL(uint64_t, 50, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1));

    ///act
    thread_count_policy_on_progress(thread_count_policy, 1000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1050, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_015: [ If now_ms is later than the last progress time, thread_count_policy_on_progress shall set the last progress time to now_ms. ]*/
TEST_FUNCTION(thread_count_policy_on_progress_with_later_time_moves_the_last_progress_time_forward)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 1000);

    ///act
    thread_count_policy_on_progress(thread_count_policy, 2000);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 2050, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_015: [ If now_ms is later than the last progress time, thread_count_policy_on_progress shall set the last progress time to now_ms. ]*/
TEST_FUNCTION(thread_count_policy_on_progress_with_earlier_time_does_not_move_the_last_progress_time_back)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 2000);

    ///act
    thread_count_policy_on_progress(thread_count_policy, 1500);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 2050, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1));

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
    ASSERT_ARE_EQUAL(uint64_t, 1160, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1));

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
    ASSERT_ARE_EQUAL(uint64_t, 1100, thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1));

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/* thread_count_policy_get_add_thread_time */

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_018: [ If thread_count_policy is NULL, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_NULL_thread_count_policy_returns_NEVER)
{
    ///arrange

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(NULL, 1, 0, 0, 1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_019: [ If queued_work_count is not greater than starting_thread_count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_020: [ If idle_thread_count is not 0, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_021: [ If thread_count is not less than the maximum thread count, thread_count_policy_get_add_thread_time shall return THREAD_COUNT_POLICY_NEVER. ]*/
PARAMETERIZED_TEST_FUNCTION(thread_count_policy_get_add_thread_time_returns_NEVER,
    ARGS(uint32_t, max_thread_count, uint32_t, queued_work_count, uint32_t, starting_thread_count, uint32_t, idle_thread_count, uint32_t, thread_count),
    CASE((10, 0, 0, 0, 1), with_no_queued_work),
    CASE((10, 0, 0, 0, 0), with_no_queued_work_and_no_threads),
    CASE((10, 3, 3, 0, 3), with_queued_work_count_equal_to_starting_thread_count),
    CASE((10, 2, 3, 0, 3), with_queued_work_count_less_than_starting_thread_count),
    CASE((10, 5, 0, 1, 1), with_an_idle_thread),
    CASE((10, 5, 0, 0, 10), with_thread_count_at_maximum),
    CASE((10, 5, 0, 0, 11), with_thread_count_above_maximum),
    CASE((0, 5, 0, 0, 500), with_thread_count_at_default_maximum))
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, max_thread_count);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, queued_work_count, starting_thread_count, idle_thread_count, thread_count);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, THREAD_COUNT_POLICY_NEVER, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_027: [ If thread_count is 0, thread_count_policy_get_add_thread_time shall return the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_no_threads_right_after_create_returns_0)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 0, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_027: [ If thread_count is 0, thread_count_policy_get_add_thread_time shall return the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_no_threads_does_not_wait_50_after_the_last_progress_time)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_progress(thread_count_policy, 1000);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 0, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_027: [ If thread_count is 0, thread_count_policy_get_add_thread_time shall return the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_no_threads_after_add_thread_failed_returns_the_retry_time)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);
    thread_count_policy_on_add_thread_failed(thread_count_policy, 1000);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1100, result);

    ///cleanup
    thread_count_policy_destroy(thread_count_policy);
}

/*Tests_SRS_THREAD_COUNT_POLICY_LINUX_42_022: [ Otherwise, thread_count_policy_get_add_thread_time shall return the later of the last progress time plus 50 and the earliest time to add a thread. ]*/
TEST_FUNCTION(thread_count_policy_get_add_thread_time_with_threads_right_after_create_returns_50)
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(0, 10);

    ///act
    uint64_t result = thread_count_policy_get_add_thread_time(thread_count_policy, 1, 0, 0, 1);

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
PARAMETERIZED_TEST_FUNCTION(thread_count_policy_get_retire_thread_time_returns_NEVER,
    ARGS(uint32_t, min_thread_count, uint32_t, thread_count, uint32_t, idle_thread_count),
    CASE((4, 3, 1), with_thread_count_below_minimum),
    CASE((4, 4, 1), with_thread_count_equal_to_minimum),
    CASE((4, 4, 0), with_thread_count_equal_to_minimum_and_no_idle_threads),
    CASE((0, 0, 0), with_no_threads_and_minimum_0))
{
    ///arrange
    THREAD_COUNT_POLICY_HANDLE thread_count_policy = test_create_policy(min_thread_count, 10);

    ///act
    uint64_t result = thread_count_policy_get_retire_thread_time(thread_count_policy, thread_count, idle_thread_count, 1000, 100000);

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

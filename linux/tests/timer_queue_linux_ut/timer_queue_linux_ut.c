// Copyright (C) Microsoft Corporation. All rights reserved.

#include "timer_queue_linux_ut_pch.h"

#define TEST_MIN_HEAP ((MIN_HEAP_HANDLE)0x4242)

// due time and period of an entry that was armed before the test arms it again
#define TEST_PREVIOUS_DUE_TIME_MS 5000
#define TEST_PREVIOUS_PERIOD_MS 7

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

// number of blocks allocated with malloc and not freed yet
static int32_t test_live_allocation_count;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void* hook_malloc(size_t size)
{
    void* result = real_gballoc_hl_malloc(size);
    if (result == NULL)
    {
        /* do nothing */
    }
    else
    {
        test_live_allocation_count++;
    }
    return result;
}

static void hook_free(void* ptr)
{
    if (ptr == NULL)
    {
        /* do nothing */
    }
    else
    {
        test_live_allocation_count--;
    }
    real_gballoc_hl_free(ptr);
}

static TIMER_QUEUE_HANDLE test_create_timer_queue(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(min_heap_create());

    TIMER_QUEUE_HANDLE result = timer_queue_create();

    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return result;
}

// fills entry as min_heap_insert does when it inserts entry with due_time_ms as key, and sets its period
static void test_set_armed_entry(TIMER_QUEUE_ENTRY* entry, uint64_t due_time_ms, uint32_t period_ms)
{
    entry->heap_entry.key = due_time_ms;
    entry->heap_entry.index = 0;
    entry->period_ms = period_ms;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_HOOK(malloc, hook_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, hook_free);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(MIN_HEAP_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_RETURNS(min_heap_create, TEST_MIN_HEAP, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(min_heap_reserve, 0, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_RETURN(min_heap_peek, NULL);

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init");
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_negative_tests_deinit();
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    umock_c_reset_all_calls();

    test_live_allocation_count = 0;
}

TEST_FUNCTION_CLEANUP(function_cleanup)
{
}

/* timer_queue_create */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_001: [ timer_queue_create shall allocate a new timer queue by calling malloc. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_002: [ timer_queue_create shall create the heap of armed timers by calling min_heap_create. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_003: [ timer_queue_create shall succeed and return the timer queue. ]*/
TEST_FUNCTION(timer_queue_create_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(min_heap_create());

    // act
    TIMER_QUEUE_HANDLE timer_queue = timer_queue_create();

    // assert
    ASSERT_IS_NOT_NULL(timer_queue);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, 1, test_live_allocation_count);

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_004: [ If any error occurs, timer_queue_create shall free the resources it allocated, fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_timer_queue_create_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(min_heap_create());

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        umock_c_negative_tests_reset();
        umock_c_negative_tests_fail_call(i);

        // act
        TIMER_QUEUE_HANDLE timer_queue = timer_queue_create();

        // assert
        ASSERT_IS_NULL(timer_queue, "test failed for call %zu", i);
        // the timer queue allocated before min_heap_create failed is freed
        ASSERT_ARE_EQUAL(int32_t, 0, test_live_allocation_count, "test failed for call %zu", i);
    }
}

/* timer_queue_destroy */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_005: [ If timer_queue is NULL, timer_queue_destroy shall return. ]*/
TEST_FUNCTION(timer_queue_destroy_with_NULL_timer_queue_returns)
{
    // arrange

    // act
    timer_queue_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_006: [ timer_queue_destroy shall destroy the heap of armed timers by calling min_heap_destroy. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_007: [ timer_queue_destroy shall free timer_queue. ]*/
TEST_FUNCTION(timer_queue_destroy_destroys_the_heap_and_frees_the_timer_queue)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    STRICT_EXPECTED_CALL(min_heap_destroy(TEST_MIN_HEAP));
    STRICT_EXPECTED_CALL(free(timer_queue));

    // act
    timer_queue_destroy(timer_queue);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, 0, test_live_allocation_count);
}

/* timer_queue_reserve */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_008: [ If timer_queue is NULL, timer_queue_reserve shall fail and return a non-zero value. ]*/
TEST_FUNCTION(timer_queue_reserve_with_NULL_timer_queue_fails)
{
    // arrange

    // act
    int result = timer_queue_reserve(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_009: [ timer_queue_reserve shall reserve room in the heap of armed timers by calling min_heap_reserve. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_011: [ timer_queue_reserve shall succeed and return 0. ]*/
TEST_FUNCTION(timer_queue_reserve_succeeds)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    STRICT_EXPECTED_CALL(min_heap_reserve(TEST_MIN_HEAP));

    // act
    int result = timer_queue_reserve(timer_queue);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_009: [ timer_queue_reserve shall reserve room in the heap of armed timers by calling min_heap_reserve. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_010: [ If min_heap_reserve fails, timer_queue_reserve shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_min_heap_reserve_fails_timer_queue_reserve_fails)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    STRICT_EXPECTED_CALL(min_heap_reserve(TEST_MIN_HEAP))
        .SetReturn(MU_FAILURE);

    // act
    int result = timer_queue_reserve(timer_queue);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/* timer_queue_unreserve */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_012: [ If timer_queue is NULL, timer_queue_unreserve shall return. ]*/
TEST_FUNCTION(timer_queue_unreserve_with_NULL_timer_queue_returns)
{
    // arrange

    // act
    timer_queue_unreserve(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_013: [ timer_queue_unreserve shall give back the room by calling min_heap_unreserve. ]*/
TEST_FUNCTION(timer_queue_unreserve_calls_min_heap_unreserve)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    STRICT_EXPECTED_CALL(min_heap_unreserve(TEST_MIN_HEAP));

    // act
    timer_queue_unreserve(timer_queue);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/* timer_queue_entry_init */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_014: [ If entry is NULL, timer_queue_entry_init shall return. ]*/
TEST_FUNCTION(timer_queue_entry_init_with_NULL_entry_returns)
{
    // arrange

    // act
    timer_queue_entry_init(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_015: [ timer_queue_entry_init shall initialize the heap entry of entry by calling min_heap_entry_init. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_016: [ timer_queue_entry_init shall set the period of entry to 0. ]*/
TEST_FUNCTION(timer_queue_entry_init_initializes_the_heap_entry_and_sets_the_period_to_0)
{
    // arrange
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, TEST_PREVIOUS_DUE_TIME_MS, TEST_PREVIOUS_PERIOD_MS);

    STRICT_EXPECTED_CALL(min_heap_entry_init(&entry.heap_entry));

    // act
    timer_queue_entry_init(&entry);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.period_ms);
}

/* timer_queue_arm */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_017: [ If timer_queue is NULL, timer_queue_arm shall return false. ]*/
TEST_FUNCTION(timer_queue_arm_with_NULL_timer_queue_returns_false)
{
    // arrange
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, TEST_PREVIOUS_DUE_TIME_MS, TEST_PREVIOUS_PERIOD_MS);

    // act
    bool result = timer_queue_arm(NULL, &entry, 1000, 100);

    // assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, TEST_PREVIOUS_DUE_TIME_MS, entry.heap_entry.key);
    ASSERT_ARE_EQUAL(uint32_t, TEST_PREVIOUS_PERIOD_MS, entry.period_ms);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_018: [ If entry is NULL, timer_queue_arm shall return false. ]*/
TEST_FUNCTION(timer_queue_arm_with_NULL_entry_returns_false)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    // act
    bool result = timer_queue_arm(timer_queue, NULL, 1000, 100);

    // assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_019: [ timer_queue_arm shall remove entry from the heap of armed timers by calling min_heap_remove. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_020: [ timer_queue_arm shall set the period of entry to period_ms. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_021: [ timer_queue_arm shall insert entry in the heap of armed timers with due_time_ms as key by calling min_heap_insert. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_022: [ timer_queue_arm shall return whether entry is the first timer of the heap of armed timers obtained by calling min_heap_peek. ]*/
PARAMETERIZED_TEST_FUNCTION(timer_queue_arm_arms_the_entry_again_and_returns_true_when_it_is_the_first_timer,
    ARGS(uint64_t, due_time_ms, uint32_t, period_ms),
    CASE((1000, 0), with_period_0),
    CASE((1000, 100), with_period_100),
    CASE((0, 1), with_due_time_0_and_period_1),
    CASE((UINT64_MAX, UINT32_MAX), with_largest_due_time_and_period))
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, TEST_PREVIOUS_DUE_TIME_MS, TEST_PREVIOUS_PERIOD_MS);

    STRICT_EXPECTED_CALL(min_heap_remove(TEST_MIN_HEAP, &entry.heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(TEST_MIN_HEAP, &entry.heap_entry, due_time_ms));
    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(&entry.heap_entry);

    // act
    bool result = timer_queue_arm(timer_queue, &entry, due_time_ms, period_ms);

    // assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, period_ms, entry.period_ms);

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_019: [ timer_queue_arm shall remove entry from the heap of armed timers by calling min_heap_remove. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_020: [ timer_queue_arm shall set the period of entry to period_ms. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_021: [ timer_queue_arm shall insert entry in the heap of armed timers with due_time_ms as key by calling min_heap_insert. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_022: [ timer_queue_arm shall return whether entry is the first timer of the heap of armed timers obtained by calling min_heap_peek. ]*/
TEST_FUNCTION(timer_queue_arm_when_another_timer_is_first_returns_false)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY first_entry;
    test_set_armed_entry(&first_entry, 500, 0);
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, TEST_PREVIOUS_DUE_TIME_MS, TEST_PREVIOUS_PERIOD_MS);

    STRICT_EXPECTED_CALL(min_heap_remove(TEST_MIN_HEAP, &entry.heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(TEST_MIN_HEAP, &entry.heap_entry, 1000));
    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(&first_entry.heap_entry);

    // act
    bool result = timer_queue_arm(timer_queue, &entry, 1000, 100);

    // assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 100, entry.period_ms);

    // cleanup
    timer_queue_destroy(timer_queue);
}

/* timer_queue_disarm */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_023: [ If timer_queue is NULL, timer_queue_disarm shall return. ]*/
TEST_FUNCTION(timer_queue_disarm_with_NULL_timer_queue_returns)
{
    // arrange
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, TEST_PREVIOUS_DUE_TIME_MS, TEST_PREVIOUS_PERIOD_MS);

    // act
    timer_queue_disarm(NULL, &entry);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, TEST_PREVIOUS_DUE_TIME_MS, entry.heap_entry.key);
    ASSERT_ARE_EQUAL(uint32_t, TEST_PREVIOUS_PERIOD_MS, entry.period_ms);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_024: [ If entry is NULL, timer_queue_disarm shall return. ]*/
TEST_FUNCTION(timer_queue_disarm_with_NULL_entry_returns)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    // act
    timer_queue_disarm(timer_queue, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_025: [ timer_queue_disarm shall remove entry from the heap of armed timers by calling min_heap_remove. ]*/
TEST_FUNCTION(timer_queue_disarm_removes_the_entry_from_the_heap)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, TEST_PREVIOUS_DUE_TIME_MS, TEST_PREVIOUS_PERIOD_MS);

    STRICT_EXPECTED_CALL(min_heap_remove(TEST_MIN_HEAP, &entry.heap_entry));

    // act
    timer_queue_disarm(timer_queue, &entry);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/* timer_queue_get_next_due_time */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_026: [ If timer_queue is NULL, timer_queue_get_next_due_time shall return TIMER_QUEUE_NO_DUE_TIME. ]*/
TEST_FUNCTION(timer_queue_get_next_due_time_with_NULL_timer_queue_returns_TIMER_QUEUE_NO_DUE_TIME)
{
    // arrange

    // act
    uint64_t result = timer_queue_get_next_due_time(NULL);

    // assert
    ASSERT_ARE_EQUAL(uint64_t, TIMER_QUEUE_NO_DUE_TIME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_027: [ timer_queue_get_next_due_time shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_028: [ If no timer is armed, timer_queue_get_next_due_time shall return TIMER_QUEUE_NO_DUE_TIME. ]*/
TEST_FUNCTION(timer_queue_get_next_due_time_with_no_armed_timer_returns_TIMER_QUEUE_NO_DUE_TIME)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(NULL);

    // act
    uint64_t result = timer_queue_get_next_due_time(timer_queue);

    // assert
    ASSERT_ARE_EQUAL(uint64_t, TIMER_QUEUE_NO_DUE_TIME, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_027: [ timer_queue_get_next_due_time shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_029: [ timer_queue_get_next_due_time shall return the due time of the first timer. ]*/
PARAMETERIZED_TEST_FUNCTION(timer_queue_get_next_due_time_returns_the_due_time_of_the_first_timer,
    ARGS(uint64_t, due_time_ms),
    CASE((0), with_due_time_0),
    CASE((1000), with_due_time_1000),
    CASE((UINT64_MAX - 1), with_largest_due_time))
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, due_time_ms, 100);

    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(&entry.heap_entry);

    // act
    uint64_t result = timer_queue_get_next_due_time(timer_queue);

    // assert
    ASSERT_ARE_EQUAL(uint64_t, due_time_ms, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/* timer_queue_pop_due */

/*Tests_SRS_TIMER_QUEUE_LINUX_42_030: [ If timer_queue is NULL, timer_queue_pop_due shall return NULL. ]*/
TEST_FUNCTION(timer_queue_pop_due_with_NULL_timer_queue_returns_NULL)
{
    // arrange

    // act
    TIMER_QUEUE_ENTRY* result = timer_queue_pop_due(NULL, 1000);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_031: [ timer_queue_pop_due shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_032: [ If no timer is armed, timer_queue_pop_due shall return NULL. ]*/
TEST_FUNCTION(timer_queue_pop_due_with_no_armed_timer_returns_NULL)
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();

    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(NULL);

    // act
    TIMER_QUEUE_ENTRY* result = timer_queue_pop_due(timer_queue, 1000);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_031: [ timer_queue_pop_due shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_033: [ If the due time of the first timer is later than now_ms, timer_queue_pop_due shall return NULL. ]*/
PARAMETERIZED_TEST_FUNCTION(timer_queue_pop_due_when_the_first_timer_is_not_due_returns_NULL,
    ARGS(uint64_t, due_time_ms, uint32_t, period_ms, uint64_t, now_ms),
    CASE((1001, 0, 1000), with_period_0_and_due_time_1_ms_later_than_now),
    CASE((1001, 100, 1000), with_period_100_and_due_time_1_ms_later_than_now),
    CASE((5000, 100, 1000), with_due_time_much_later_than_now),
    CASE((1, 100, 0), with_now_0),
    CASE((UINT64_MAX, 100, UINT64_MAX - 1), with_largest_due_time))
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, due_time_ms, period_ms);

    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(&entry.heap_entry);

    // act
    TIMER_QUEUE_ENTRY* result = timer_queue_pop_due(timer_queue, now_ms);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, due_time_ms, entry.heap_entry.key);
    ASSERT_ARE_EQUAL(uint32_t, period_ms, entry.period_ms);

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_031: [ timer_queue_pop_due shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_034: [ timer_queue_pop_due shall remove the first timer from the heap of armed timers by calling min_heap_remove. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_035: [ If the period of the timer is not 0: ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_039: [ timer_queue_pop_due shall return the timer. ]*/
PARAMETERIZED_TEST_FUNCTION(timer_queue_pop_due_with_period_0_removes_and_returns_the_timer_without_arming_it_again,
    ARGS(uint64_t, due_time_ms, uint64_t, now_ms),
    CASE((1000, 1000), with_due_time_equal_to_now),
    CASE((1000, 1001), with_due_time_1_ms_earlier_than_now),
    CASE((1000, 5000), with_due_time_much_earlier_than_now),
    CASE((0, 0), with_due_time_0_and_now_0),
    CASE((0, UINT64_MAX), with_due_time_0_and_largest_now))
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, due_time_ms, 0);

    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(&entry.heap_entry);
    STRICT_EXPECTED_CALL(min_heap_remove(TEST_MIN_HEAP, &entry.heap_entry));

    // act
    TIMER_QUEUE_ENTRY* result = timer_queue_pop_due(timer_queue, now_ms);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, &entry, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, entry.period_ms);

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_031: [ timer_queue_pop_due shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_034: [ timer_queue_pop_due shall remove the first timer from the heap of armed timers by calling min_heap_remove. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_035: [ If the period of the timer is not 0: ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_036: [ timer_queue_pop_due shall compute the next due time as the due time plus the period. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_038: [ timer_queue_pop_due shall insert the timer in the heap of armed timers with the next due time as key by calling min_heap_insert. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_039: [ timer_queue_pop_due shall return the timer. ]*/
PARAMETERIZED_TEST_FUNCTION(timer_queue_pop_due_with_period_arms_the_timer_again_at_due_time_plus_period,
    ARGS(uint64_t, due_time_ms, uint32_t, period_ms, uint64_t, now_ms, uint64_t, expected_next_due_time_ms),
    CASE((1000, 100, 1000, 1100), with_due_time_equal_to_now),
    CASE((1000, 100, 1099, 1100), with_next_due_time_1_ms_later_than_now),
    CASE((1000, 1, 1000, 1001), with_period_1),
    CASE((0, UINT32_MAX, 0, UINT32_MAX), with_largest_period))
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, due_time_ms, period_ms);

    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(&entry.heap_entry);
    STRICT_EXPECTED_CALL(min_heap_remove(TEST_MIN_HEAP, &entry.heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(TEST_MIN_HEAP, &entry.heap_entry, expected_next_due_time_ms));

    // act
    TIMER_QUEUE_ENTRY* result = timer_queue_pop_due(timer_queue, now_ms);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, &entry, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, period_ms, entry.period_ms);

    // cleanup
    timer_queue_destroy(timer_queue);
}

/*Tests_SRS_TIMER_QUEUE_LINUX_42_031: [ timer_queue_pop_due shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_034: [ timer_queue_pop_due shall remove the first timer from the heap of armed timers by calling min_heap_remove. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_035: [ If the period of the timer is not 0: ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_036: [ timer_queue_pop_due shall compute the next due time as the due time plus the period. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_037: [ If the next due time is not later than now_ms, timer_queue_pop_due shall use now_ms plus the period as the next due time. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_038: [ timer_queue_pop_due shall insert the timer in the heap of armed timers with the next due time as key by calling min_heap_insert. ]*/
/*Tests_SRS_TIMER_QUEUE_LINUX_42_039: [ timer_queue_pop_due shall return the timer. ]*/
PARAMETERIZED_TEST_FUNCTION(timer_queue_pop_due_with_period_when_the_timer_fell_behind_arms_it_again_at_now_plus_period,
    ARGS(uint64_t, due_time_ms, uint32_t, period_ms, uint64_t, now_ms, uint64_t, expected_next_due_time_ms),
    CASE((1000, 100, 1100, 1200), with_due_time_plus_period_equal_to_now),
    CASE((1000, 100, 1101, 1201), with_due_time_plus_period_1_ms_earlier_than_now),
    CASE((1000, 100, 10050, 10150), with_many_missed_periods),
    CASE((1000, 1, 1001, 1002), with_period_1))
{
    // arrange
    TIMER_QUEUE_HANDLE timer_queue = test_create_timer_queue();
    TIMER_QUEUE_ENTRY entry;
    test_set_armed_entry(&entry, due_time_ms, period_ms);

    STRICT_EXPECTED_CALL(min_heap_peek(TEST_MIN_HEAP))
        .SetReturn(&entry.heap_entry);
    STRICT_EXPECTED_CALL(min_heap_remove(TEST_MIN_HEAP, &entry.heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(TEST_MIN_HEAP, &entry.heap_entry, expected_next_due_time_ms));

    // act
    TIMER_QUEUE_ENTRY* result = timer_queue_pop_due(timer_queue, now_ms);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, &entry, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, period_ms, entry.period_ms);

    // cleanup
    timer_queue_destroy(timer_queue);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

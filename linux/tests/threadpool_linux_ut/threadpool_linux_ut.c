// Copyright (c) Microsoft. All rights reserved.

#include "threadpool_linux_ut_pch.h"

#define TEST_START_DELAY_MS 42
#define TEST_TIMER_PERIOD_MS 2000

static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4243;
static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_work = (EXECUTION_ENGINE_LINUX_WORK_HANDLE)0x4244;
static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_timer = (EXECUTION_ENGINE_LINUX_WORK_HANDLE)0x4245;
static void* test_work_function_context = (void*)0x4246;
static void* test_work_function_context_2 = (void*)0x4247;

static EXECUTION_ENGINE_LINUX_WORK_FUNCTION captured_work_function;
static void* captured_work_function_context;
static EXECUTION_ENGINE_LINUX_WORK_FUNCTION captured_timer_function;
static void* captured_timer_function_context;
static volatile_atomic int32_t* captured_pending_work_count;
static SRW_LOCK_LL* captured_timer_lock;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, void, test_work_function, void*, context)
MOCK_FUNCTION_END()

static EXECUTION_ENGINE_LINUX_WORK_HANDLE hook_execution_engine_linux_create_work(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context)
{
    (void)execution_engine;
    captured_work_function = work_function;
    captured_work_function_context = work_function_context;
    return test_work;
}

static EXECUTION_ENGINE_LINUX_WORK_HANDLE hook_execution_engine_linux_create_timer(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context)
{
    (void)execution_engine;
    captured_timer_function = work_function;
    captured_timer_function_context = work_function_context;
    return test_timer;
}

static int32_t get_pending_work_count(void)
{
    return real_interlocked_add(captured_pending_work_count, 0);
}

static void setup_threadpool_create_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CaptureArgumentValue_target(&captured_pending_work_count)
        .CallCannotFail();
}

static void setup_threadpool_create_work_item_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(execution_engine_linux_create_work(test_execution_engine, IGNORED_ARG, IGNORED_ARG));
}

static void setup_threadpool_timer_start_expectations(uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .CaptureArgumentValue_srw_lock_ll(&captured_timer_lock);
    STRICT_EXPECTED_CALL(execution_engine_linux_create_timer(test_execution_engine, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_set_timer(test_timer, start_delay_ms, timer_period_ms));
}

static THANDLE(THREADPOOL) test_create_threadpool(void)
{
    setup_threadpool_create_expectations();
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return threadpool;
}

static THANDLE(THREADPOOL_WORK_ITEM) test_create_work_item(THANDLE(THREADPOOL) threadpool, void* work_function_context)
{
    setup_threadpool_create_work_item_expectations();
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, test_work_function, work_function_context);
    ASSERT_IS_NOT_NULL(work_item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return work_item;
}

static void* test_schedule_work(THANDLE(THREADPOOL) threadpool, void* work_function_context)
{
    void* scheduled_work_context;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&scheduled_work_context);
    STRICT_EXPECTED_CALL(execution_engine_linux_create_work(test_execution_engine, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(captured_pending_work_count));
    STRICT_EXPECTED_CALL(execution_engine_linux_submit_work(test_work));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, test_work_function, work_function_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return scheduled_work_context;
}

static THANDLE(THREADPOOL_TIMER) test_start_timer(THANDLE(THREADPOOL) threadpool, void* work_function_context)
{
    setup_threadpool_timer_start_expectations(TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS);
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, test_work_function, work_function_context);
    ASSERT_IS_NOT_NULL(timer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return timer;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_LL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(execution_engine_linux_create_work, hook_execution_engine_linux_create_work);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(execution_engine_linux_create_work, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(execution_engine_linux_create_timer, hook_execution_engine_linux_create_timer);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(execution_engine_linux_create_timer, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_LINUX_WORK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_LINUX_WORK_FUNCTION, void*);

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");

    captured_work_function = NULL;
    captured_work_function_context = NULL;
    captured_timer_function = NULL;
    captured_timer_function_context = NULL;
    captured_pending_work_count = NULL;
    captured_timer_lock = NULL;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* threadpool_create */

/* Tests_SRS_THREADPOOL_LINUX_42_001: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_create_with_NULL_execution_engine_fails)
{
    // arrange

    // act
    THANDLE(THREADPOOL) threadpool = threadpool_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_42_002: [ threadpool_create shall allocate a new threadpool object with threadpool_dispose as dispose function and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_003: [ threadpool_create shall increment the reference count on execution_engine and store it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_004: [ threadpool_create shall set the count of pending scheduled work to 0. ]*/
TEST_FUNCTION(threadpool_create_succeeds)
{
    // arrange
    setup_threadpool_create_expectations();

    // act
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int32_t, 0, get_pending_work_count());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_005: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_fails)
{
    // arrange
    setup_threadpool_create_expectations();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);

            // assert
            ASSERT_IS_NULL(threadpool, "On failed call %zu", i);
        }
    }
}

/* threadpool_dispose */

/* Tests_SRS_THREADPOOL_LINUX_42_002: [ threadpool_create shall allocate a new threadpool object with threadpool_dispose as dispose function and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_006: [ threadpool_dispose shall wait for the count of pending scheduled work to reach 0 by calling InterlockedHL_WaitForValue. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_007: [ threadpool_dispose shall decrement the reference count on the execution engine. ]*/
TEST_FUNCTION(threadpool_dispose_waits_for_the_pending_scheduled_work_and_decrements_the_execution_engine_reference_count)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(captured_pending_work_count, 0, UINT32_MAX));
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_42_006: [ threadpool_dispose shall wait for the count of pending scheduled work to reach 0 by calling InterlockedHL_WaitForValue. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_007: [ threadpool_dispose shall decrement the reference count on the execution engine. ]*/
TEST_FUNCTION(when_InterlockedHL_WaitForValue_fails_threadpool_dispose_still_decrements_the_execution_engine_reference_count)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(captured_pending_work_count, 0, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_ERROR);
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_42_006: [ threadpool_dispose shall wait for the count of pending scheduled work to reach 0 by calling InterlockedHL_WaitForValue. ]*/
TEST_FUNCTION(threadpool_dispose_waits_on_the_count_maintained_by_threadpool_schedule_work)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    (void)test_schedule_work(threadpool, test_work_function_context);
    ASSERT_ARE_EQUAL(int32_t, 1, get_pending_work_count());
    captured_work_function(captured_work_function_context);
    ASSERT_ARE_EQUAL(int32_t, 0, get_pending_work_count());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(captured_pending_work_count, 0, UINT32_MAX));
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* threadpool_create_work_item */

/* Tests_SRS_THREADPOOL_LINUX_42_008: [ If threadpool is NULL, threadpool_create_work_item shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_create_work_item_with_NULL_threadpool_fails)
{
    // arrange

    // act
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(NULL, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(work_item);
}

/* Tests_SRS_THREADPOOL_LINUX_42_009: [ If work_function is NULL, threadpool_create_work_item shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_create_work_item_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    // act
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, NULL, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(work_item);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_010: [ threadpool_create_work_item shall allocate a THANDLE(THREADPOOL_WORK_ITEM) with threadpool_dispose_work_item as dispose function and store work_function and work_function_context in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_011: [ threadpool_create_work_item shall create a work object by calling execution_engine_linux_create_work with on_work_item_callback and the work item as context. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_012: [ threadpool_create_work_item shall succeed and return the work item. ]*/
TEST_FUNCTION(threadpool_create_work_item_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    setup_threadpool_create_work_item_expectations();

    // act
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(work_item);
    ASSERT_IS_NOT_NULL(captured_work_function);
    ASSERT_ARE_EQUAL(void_ptr, (void*)work_item, captured_work_function_context);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_013: [ If any error occurs, threadpool_create_work_item shall fail, free the work item and return NULL. ]*/
TEST_FUNCTION(when_execution_engine_linux_create_work_fails_threadpool_create_work_item_frees_the_work_item)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* work_item_memory;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&work_item_memory);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(execution_engine_linux_create_work(test_execution_engine, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&work_item_memory);

    // act
    THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(work_item);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_013: [ If any error occurs, threadpool_create_work_item shall fail, free the work item and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_work_item_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    setup_threadpool_create_work_item_expectations();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(THREADPOOL_WORK_ITEM) work_item = threadpool_create_work_item(threadpool, test_work_function, test_work_function_context);

            // assert
            ASSERT_IS_NULL(work_item, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* on_work_item_callback */

/* Tests_SRS_THREADPOOL_LINUX_42_014: [ on_work_item_callback shall call the work_function of the work item with its work_function_context. ]*/
TEST_FUNCTION(on_work_item_callback_calls_the_work_function)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_WORK_ITEM) work_item = test_create_work_item(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(test_work_function(test_work_function_context));

    // act
    captured_work_function(captured_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_014: [ on_work_item_callback shall call the work_function of the work item with its work_function_context. ]*/
TEST_FUNCTION(on_work_item_callback_with_NULL_work_function_context_calls_the_work_function)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_WORK_ITEM) work_item = test_create_work_item(threadpool, NULL);

    STRICT_EXPECTED_CALL(test_work_function(NULL));

    // act
    captured_work_function(captured_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_dispose_work_item */

/* Tests_SRS_THREADPOOL_LINUX_42_010: [ threadpool_create_work_item shall allocate a THANDLE(THREADPOOL_WORK_ITEM) with threadpool_dispose_work_item as dispose function and store work_function and work_function_context in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_015: [ threadpool_dispose_work_item shall wait for the pending and running callbacks of the work item by calling execution_engine_linux_wait_for_work with cancel_pending set to false. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_016: [ threadpool_dispose_work_item shall close the work object by calling execution_engine_linux_close_work. ]*/
TEST_FUNCTION(threadpool_dispose_work_item_waits_for_the_callbacks_and_closes_the_work)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_WORK_ITEM) work_item = test_create_work_item(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_wait_for_work(test_work, false));
    STRICT_EXPECTED_CALL(execution_engine_linux_close_work(test_work));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(work_item);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_schedule_work_item */

/* Tests_SRS_THREADPOOL_LINUX_42_017: [ If threadpool is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_item_with_NULL_threadpool_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_WORK_ITEM) work_item = test_create_work_item(threadpool, test_work_function_context);

    // act
    int result = threadpool_schedule_work_item(NULL, work_item);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_018: [ If threadpool_work_item is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_item_with_NULL_work_item_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    // act
    int result = threadpool_schedule_work_item(threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_019: [ threadpool_schedule_work_item shall submit the work object of threadpool_work_item by calling execution_engine_linux_submit_work. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_020: [ threadpool_schedule_work_item shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_schedule_work_item_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_WORK_ITEM) work_item = test_create_work_item(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(execution_engine_linux_submit_work(test_work));

    // act
    int result = threadpool_schedule_work_item(threadpool, work_item);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_019: [ threadpool_schedule_work_item shall submit the work object of threadpool_work_item by calling execution_engine_linux_submit_work. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_020: [ threadpool_schedule_work_item shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_schedule_work_item_twice_submits_the_work_twice)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_WORK_ITEM) work_item = test_create_work_item(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(execution_engine_linux_submit_work(test_work));
    STRICT_EXPECTED_CALL(execution_engine_linux_submit_work(test_work));

    // act
    int result_1 = threadpool_schedule_work_item(threadpool, work_item);
    int result_2 = threadpool_schedule_work_item(threadpool, work_item);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result_1);
    ASSERT_ARE_EQUAL(int, 0, result_2);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_schedule_work */

/* Tests_SRS_THREADPOOL_LINUX_42_021: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_with_NULL_threadpool_fails)
{
    // arrange

    // act
    int result = threadpool_schedule_work(NULL, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_42_022: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    // act
    int result = threadpool_schedule_work(threadpool, NULL, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int32_t, 0, get_pending_work_count());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_024: [ threadpool_schedule_work shall allocate a context that stores the threadpool, work_function and work_function_context. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_025: [ threadpool_schedule_work shall create a work object by calling execution_engine_linux_create_work with on_scheduled_work_callback and the context. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_026: [ threadpool_schedule_work shall increment the count of pending scheduled work by calling interlocked_increment. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_027: [ threadpool_schedule_work shall submit the work object by calling execution_engine_linux_submit_work and return 0. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* scheduled_work_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&scheduled_work_context);
    STRICT_EXPECTED_CALL(execution_engine_linux_create_work(test_execution_engine, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_work_function_context(&scheduled_work_context);
    STRICT_EXPECTED_CALL(interlocked_increment(captured_pending_work_count));
    STRICT_EXPECTED_CALL(execution_engine_linux_submit_work(test_work));

    // act
    int result = threadpool_schedule_work(threadpool, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(captured_work_function);
    ASSERT_ARE_EQUAL(int32_t, 1, get_pending_work_count());

    // cleanup
    captured_work_function(captured_work_function_context);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_023: [ work_function_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(threadpool_schedule_work_with_NULL_work_function_context_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_create_work(test_execution_engine, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(captured_pending_work_count));
    STRICT_EXPECTED_CALL(execution_engine_linux_submit_work(test_work));

    // act
    int result = threadpool_schedule_work(threadpool, test_work_function, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int32_t, 1, get_pending_work_count());

    // cleanup
    captured_work_function(captured_work_function_context);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_028: [ If any error occurs, threadpool_schedule_work shall free the context, fail and return a non-zero value. ]*/
TEST_FUNCTION(when_execution_engine_linux_create_work_fails_threadpool_schedule_work_frees_the_context)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* scheduled_work_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&scheduled_work_context);
    STRICT_EXPECTED_CALL(execution_engine_linux_create_work(test_execution_engine, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&scheduled_work_context);

    // act
    int result = threadpool_schedule_work(threadpool, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int32_t, 0, get_pending_work_count());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_028: [ If any error occurs, threadpool_schedule_work shall free the context, fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_schedule_work_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_create_work(test_execution_engine, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(captured_pending_work_count))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(execution_engine_linux_submit_work(test_work));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = threadpool_schedule_work(threadpool, test_work_function, test_work_function_context);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
            ASSERT_ARE_EQUAL(int32_t, 0, get_pending_work_count(), "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* on_scheduled_work_callback */

/* Tests_SRS_THREADPOOL_LINUX_42_029: [ on_scheduled_work_callback shall call work_function with work_function_context. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_030: [ on_scheduled_work_callback shall close the work object by calling execution_engine_linux_close_work. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_031: [ on_scheduled_work_callback shall free the context. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_032: [ on_scheduled_work_callback shall decrement the count of pending scheduled work of the threadpool by calling interlocked_decrement and, if it reached 0, call wake_by_address_all. ]*/
TEST_FUNCTION(on_scheduled_work_callback_calls_the_work_function_closes_the_work_frees_the_context_and_wakes_the_waiters)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* scheduled_work_context = test_schedule_work(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(test_work_function(test_work_function_context));
    STRICT_EXPECTED_CALL(execution_engine_linux_close_work(test_work));
    STRICT_EXPECTED_CALL(free(scheduled_work_context));
    STRICT_EXPECTED_CALL(interlocked_decrement(captured_pending_work_count));
    STRICT_EXPECTED_CALL(wake_by_address_all(captured_pending_work_count));

    // act
    captured_work_function(captured_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, 0, get_pending_work_count());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_029: [ on_scheduled_work_callback shall call work_function with work_function_context. ]*/
TEST_FUNCTION(on_scheduled_work_callback_with_NULL_work_function_context_calls_the_work_function)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* scheduled_work_context = test_schedule_work(threadpool, NULL);

    STRICT_EXPECTED_CALL(test_work_function(NULL));
    STRICT_EXPECTED_CALL(execution_engine_linux_close_work(test_work));
    STRICT_EXPECTED_CALL(free(scheduled_work_context));
    STRICT_EXPECTED_CALL(interlocked_decrement(captured_pending_work_count));
    STRICT_EXPECTED_CALL(wake_by_address_all(captured_pending_work_count));

    // act
    captured_work_function(captured_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_032: [ on_scheduled_work_callback shall decrement the count of pending scheduled work of the threadpool by calling interlocked_decrement and, if it reached 0, call wake_by_address_all. ]*/
TEST_FUNCTION(on_scheduled_work_callback_does_not_wake_when_other_scheduled_work_is_pending)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* scheduled_work_context_1 = test_schedule_work(threadpool, test_work_function_context);
    EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function_1 = captured_work_function;
    void* work_function_context_1 = captured_work_function_context;
    (void)test_schedule_work(threadpool, test_work_function_context_2);
    EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function_2 = captured_work_function;
    void* work_function_context_2 = captured_work_function_context;
    ASSERT_ARE_EQUAL(int32_t, 2, get_pending_work_count());

    STRICT_EXPECTED_CALL(test_work_function(test_work_function_context));
    STRICT_EXPECTED_CALL(execution_engine_linux_close_work(test_work));
    STRICT_EXPECTED_CALL(free(scheduled_work_context_1));
    STRICT_EXPECTED_CALL(interlocked_decrement(captured_pending_work_count));

    // act
    work_function_1(work_function_context_1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, 1, get_pending_work_count());

    // cleanup
    work_function_2(work_function_context_2);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_start */

/* Tests_SRS_THREADPOOL_LINUX_42_033: [ If threadpool is NULL, threadpool_timer_start shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_threadpool_fails)
{
    // arrange

    // act
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(NULL, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer);
}

/* Tests_SRS_THREADPOOL_LINUX_42_034: [ If work_function is NULL, threadpool_timer_start shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    // act
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, NULL, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_036: [ threadpool_timer_start shall allocate a THANDLE(THREADPOOL_TIMER) with threadpool_timer_dispose as dispose function and store work_function and work_function_context in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_037: [ threadpool_timer_start shall initialize the lock guarding the timer state by calling srw_lock_ll_init. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_038: [ threadpool_timer_start shall create the timer by calling execution_engine_linux_create_timer with on_timer_callback and the timer as context. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_039: [ threadpool_timer_start shall arm the timer by calling execution_engine_linux_set_timer with start_delay_ms and timer_period_ms. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_040: [ threadpool_timer_start shall succeed and return the timer. ]*/
TEST_FUNCTION(threadpool_timer_start_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    setup_threadpool_timer_start_expectations(TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS);

    // act
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(timer);
    ASSERT_IS_NOT_NULL(captured_timer_lock);
    ASSERT_IS_NOT_NULL(captured_timer_function);
    ASSERT_ARE_EQUAL(void_ptr, (void*)timer, captured_timer_function_context);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_039: [ threadpool_timer_start shall arm the timer by calling execution_engine_linux_set_timer with start_delay_ms and timer_period_ms. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_040: [ threadpool_timer_start shall succeed and return the timer. ]*/
TEST_FUNCTION(threadpool_timer_start_with_0_timer_period_ms_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    setup_threadpool_timer_start_expectations(TEST_START_DELAY_MS, 0);

    // act
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, 0, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(timer);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_035: [ work_function_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_context_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    setup_threadpool_timer_start_expectations(TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS);

    // act
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, test_work_function, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(timer);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_041: [ If any error occurs, threadpool_timer_start shall free the resources it allocated, fail and return NULL. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_threadpool_timer_start_frees_the_timer)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* timer_memory;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&timer_memory);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&timer_memory);

    // act
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_041: [ If any error occurs, threadpool_timer_start shall free the resources it allocated, fail and return NULL. ]*/
TEST_FUNCTION(when_execution_engine_linux_create_timer_fails_threadpool_timer_start_deinitializes_the_lock_and_frees_the_timer)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    void* timer_memory;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&timer_memory);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .CaptureArgumentValue_srw_lock_ll(&captured_timer_lock);
    STRICT_EXPECTED_CALL(execution_engine_linux_create_timer(test_execution_engine, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(IGNORED_ARG))
        .ValidateArgumentValue_srw_lock_ll(&captured_timer_lock);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&timer_memory);

    // act
    THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, test_work_function, test_work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_041: [ If any error occurs, threadpool_timer_start shall free the resources it allocated, fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_timer_start_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    setup_threadpool_timer_start_expectations(TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(THREADPOOL_TIMER) timer = threadpool_timer_start(threadpool, TEST_START_DELAY_MS, TEST_TIMER_PERIOD_MS, test_work_function, test_work_function_context);

            // assert
            ASSERT_IS_NULL(timer, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* on_timer_callback */

/* Tests_SRS_THREADPOOL_LINUX_42_042: [ on_timer_callback shall call the work_function of the timer with its work_function_context. ]*/
TEST_FUNCTION(on_timer_callback_calls_the_work_function)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer = test_start_timer(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(test_work_function(test_work_function_context));

    // act
    captured_timer_function(captured_timer_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_042: [ on_timer_callback shall call the work_function of the timer with its work_function_context. ]*/
TEST_FUNCTION(on_timer_callback_with_NULL_work_function_context_calls_the_work_function)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer = test_start_timer(threadpool, NULL);

    STRICT_EXPECTED_CALL(test_work_function(NULL));

    // act
    captured_timer_function(captured_timer_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_restart */

/* Tests_SRS_THREADPOOL_LINUX_42_043: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_restart_with_NULL_timer_fails)
{
    // arrange

    // act
    int result = threadpool_timer_restart(NULL, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_42_044: [ threadpool_timer_restart shall acquire the timer lock exclusively by calling srw_lock_ll_acquire_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_045: [ threadpool_timer_restart shall re-arm the timer by calling execution_engine_linux_set_timer with start_delay_ms and timer_period_ms. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_046: [ threadpool_timer_restart shall release the timer lock by calling srw_lock_ll_release_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_047: [ threadpool_timer_restart shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_restart_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer = test_start_timer(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(captured_timer_lock));
    STRICT_EXPECTED_CALL(execution_engine_linux_set_timer(test_timer, 43, 1000));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(captured_timer_lock));

    // act
    int result = threadpool_timer_restart(timer, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_044: [ threadpool_timer_restart shall acquire the timer lock exclusively by calling srw_lock_ll_acquire_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_045: [ threadpool_timer_restart shall re-arm the timer by calling execution_engine_linux_set_timer with start_delay_ms and timer_period_ms. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_046: [ threadpool_timer_restart shall release the timer lock by calling srw_lock_ll_release_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_047: [ threadpool_timer_restart shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_restart_after_threadpool_timer_cancel_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer = test_start_timer(threadpool, test_work_function_context);
    threadpool_timer_cancel(timer);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(captured_timer_lock));
    STRICT_EXPECTED_CALL(execution_engine_linux_set_timer(test_timer, 0, 0));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(captured_timer_lock));

    // act
    int result = threadpool_timer_restart(timer, 0, 0);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_cancel */

/* Tests_SRS_THREADPOOL_LINUX_42_048: [ If timer is NULL, threadpool_timer_cancel shall return. ]*/
TEST_FUNCTION(threadpool_timer_cancel_with_NULL_timer_returns)
{
    // arrange

    // act
    threadpool_timer_cancel(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_42_049: [ threadpool_timer_cancel shall acquire the timer lock exclusively by calling srw_lock_ll_acquire_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_050: [ threadpool_timer_cancel shall disarm the timer by calling execution_engine_linux_cancel_timer. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_051: [ threadpool_timer_cancel shall drop the callbacks that have not started and wait for the running callback by calling execution_engine_linux_wait_for_work with cancel_pending set to true. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_052: [ threadpool_timer_cancel shall release the timer lock by calling srw_lock_ll_release_exclusive. ]*/
TEST_FUNCTION(threadpool_timer_cancel_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer = test_start_timer(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(captured_timer_lock));
    STRICT_EXPECTED_CALL(execution_engine_linux_cancel_timer(test_timer));
    STRICT_EXPECTED_CALL(execution_engine_linux_wait_for_work(test_timer, true));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(captured_timer_lock));

    // act
    threadpool_timer_cancel(timer);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_dispose */

/* Tests_SRS_THREADPOOL_LINUX_42_036: [ threadpool_timer_start shall allocate a THANDLE(THREADPOOL_TIMER) with threadpool_timer_dispose as dispose function and store work_function and work_function_context in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_053: [ threadpool_timer_dispose shall disarm the timer by calling execution_engine_linux_cancel_timer. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_054: [ threadpool_timer_dispose shall drop the callbacks that have not started and wait for the running callback by calling execution_engine_linux_wait_for_work with cancel_pending set to true. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_055: [ threadpool_timer_dispose shall close the timer by calling execution_engine_linux_close_work. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_056: [ threadpool_timer_dispose shall de-initialize the timer lock by calling srw_lock_ll_deinit. ]*/
TEST_FUNCTION(threadpool_timer_dispose_cancels_the_timer_waits_closes_it_and_deinitializes_the_lock)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer = test_start_timer(threadpool, test_work_function_context);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_cancel_timer(test_timer));
    STRICT_EXPECTED_CALL(execution_engine_linux_wait_for_work(test_timer, true));
    STRICT_EXPECTED_CALL(execution_engine_linux_close_work(test_timer));
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(captured_timer_lock));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_42_053: [ threadpool_timer_dispose shall disarm the timer by calling execution_engine_linux_cancel_timer. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_054: [ threadpool_timer_dispose shall drop the callbacks that have not started and wait for the running callback by calling execution_engine_linux_wait_for_work with cancel_pending set to true. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_055: [ threadpool_timer_dispose shall close the timer by calling execution_engine_linux_close_work. ]*/
/* Tests_SRS_THREADPOOL_LINUX_42_056: [ threadpool_timer_dispose shall de-initialize the timer lock by calling srw_lock_ll_deinit. ]*/
TEST_FUNCTION(threadpool_timer_dispose_after_threadpool_timer_cancel_cancels_waits_closes_and_deinitializes_the_lock)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer = test_start_timer(threadpool, test_work_function_context);
    threadpool_timer_cancel(timer);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_cancel_timer(test_timer));
    STRICT_EXPECTED_CALL(execution_engine_linux_wait_for_work(test_timer, true));
    STRICT_EXPECTED_CALL(execution_engine_linux_close_work(test_timer));
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(captured_timer_lock));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright (c) Microsoft. All rights reserved.

#include "execution_engine_linux_ut_pch.h"

#define TEST_START_TIME_MS 1000000
#define TEST_MAX_THREADS 600
#define TEST_MAX_ACTIONS 4
#define TEST_MAX_DEADLINES 4

typedef void (*TEST_ACTION)(void);

typedef struct TEST_THREAD_TAG
{
    THREAD_START_FUNC thread_func;
    void* arg;
} TEST_THREAD;

// threads "created" by ThreadAPI_Create are not started, the tests run their thread functions on the test thread
static TEST_THREAD test_threads[TEST_MAX_THREADS];
static uint32_t test_thread_count;

static uint64_t test_now_ns;
// added to the clock after each clock_gettime call, to simulate time passing between two reads of the clock
static uint64_t test_clock_increment_ns;

// actions executed when the code under test waits on a condition variable (which would otherwise never return)
static TEST_ACTION test_wait_actions[TEST_MAX_ACTIONS];
static uint32_t test_wait_action_count;
static uint32_t test_wait_action_index;

// actions executed when test_work_function is called
static TEST_ACTION test_work_actions[TEST_MAX_ACTIONS];
static uint32_t test_work_action_count;
static uint32_t test_work_action_index;

// deadlines passed to pthread_cond_timedwait, in ms
static uint64_t test_wait_deadlines_ms[TEST_MAX_DEADLINES];
static uint32_t test_wait_deadline_count;

// condition variable attributes seen by the mocks
static const pthread_condattr_t* test_condattr_init_attr;
static const pthread_condattr_t* test_condattr_setclock_attr;
static const pthread_condattr_t* test_condattr_destroy_attr;
static const pthread_condattr_t* test_cond_init_attr;

static EXECUTION_ENGINE_HANDLE test_engine;
static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_work;
static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_other_work;
static int test_work_context;
static int test_other_work_context;

// state observed by the actions
static EXECUTION_ENGINE_HANDLE observed_current_execution_engine;
static void* observed_idle_head;
static uint32_t observed_pending_count;
static uint32_t observed_running_count;
static uint32_t observed_queue_count;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void set_now_ms(uint64_t now_ms)
{
    test_now_ns = now_ms * 1000000;
}

static void add_wait_action(TEST_ACTION action)
{
    ASSERT_IS_TRUE(test_wait_action_count < TEST_MAX_ACTIONS);
    test_wait_actions[test_wait_action_count++] = action;
}

static void add_work_action(TEST_ACTION action)
{
    ASSERT_IS_TRUE(test_work_action_count < TEST_MAX_ACTIONS);
    test_work_actions[test_work_action_count++] = action;
}

static void reset_actions(void)
{
    test_wait_action_count = 0;
    test_wait_action_index = 0;
    test_work_action_count = 0;
    test_work_action_index = 0;
    test_wait_deadline_count = 0;
}

static void run_next_wait_action(void)
{
    ASSERT_IS_TRUE(test_wait_action_index < test_wait_action_count, "unexpected wait on a condition variable");
    test_wait_actions[test_wait_action_index++]();
}

static void run_next_work_action(void)
{
    if (test_work_action_index < test_work_action_count)
    {
        test_work_actions[test_work_action_index++]();
    }
}

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_mutex_init, pthread_mutex_t*, mutex, const pthread_mutexattr_t*, attr)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_mutex_lock, pthread_mutex_t*, mutex)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_mutex_unlock, pthread_mutex_t*, mutex)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_mutex_destroy, pthread_mutex_t*, mutex)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_condattr_init, pthread_condattr_t*, attr)
    test_condattr_init_attr = attr;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_condattr_setclock, pthread_condattr_t*, attr, clockid_t, clock_id)
    test_condattr_setclock_attr = attr;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_condattr_destroy, pthread_condattr_t*, attr)
    test_condattr_destroy_attr = attr;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_init, pthread_cond_t*, cond, const pthread_condattr_t*, attr)
    if (attr != NULL)
    {
        test_cond_init_attr = attr;
    }
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_signal, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_broadcast, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_wait, pthread_cond_t*, cond, pthread_mutex_t*, mutex)
    run_next_wait_action();
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_timedwait, pthread_cond_t*, cond, pthread_mutex_t*, mutex, const struct timespec*, abstime)
    ASSERT_IS_TRUE(test_wait_deadline_count < TEST_MAX_DEADLINES);
    test_wait_deadlines_ms[test_wait_deadline_count++] = ((uint64_t)abstime->tv_sec * 1000) + ((uint64_t)abstime->tv_nsec / 1000000);
    run_next_wait_action();
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_destroy, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_clock_gettime, clockid_t, clock_id, struct timespec*, tp)
    tp->tv_sec = (time_t)(test_now_ns / 1000000000);
    tp->tv_nsec = (long)(test_now_ns % 1000000000);
    test_now_ns += test_clock_increment_ns;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, void, test_work_function, void*, context)
    run_next_work_action();
MOCK_FUNCTION_END()

static THREADAPI_RESULT hook_ThreadAPI_Create(THREAD_HANDLE* thread_handle, THREAD_START_FUNC func, void* arg)
{
    ASSERT_IS_TRUE(test_thread_count < TEST_MAX_THREADS);
    test_threads[test_thread_count].thread_func = func;
    test_threads[test_thread_count].arg = arg;
    *thread_handle = &test_threads[test_thread_count];
    test_thread_count++;
    return THREADAPI_OK;
}

static int run_test_thread(uint32_t index)
{
    return test_threads[index].thread_func(test_threads[index].arg);
}

static void action_stop_engine(void)
{
    test_engine_set_stop(test_engine, true);
}

static void action_observe_idle_head_and_stop_engine(void)
{
    observed_idle_head = test_engine_get_idle_head(test_engine);
    test_engine_set_stop(test_engine, true);
}

static void action_time_out_wait(void)
{
    // the clock reaches the deadline of the last pthread_cond_timedwait
    set_now_ms(test_wait_deadlines_ms[test_wait_deadline_count - 1]);
}

static void action_submit_test_work(void)
{
    execution_engine_linux_submit_work(test_work);
}

static void action_submit_test_work_and_other_work(void)
{
    execution_engine_linux_submit_work(test_work);
    execution_engine_linux_submit_work(test_other_work);
}

static void action_retire_worker_and_submit_test_work(void)
{
    // the manager retires the waiting worker and work is submitted before the worker wakes up
    test_worker_retire(test_threads[0].arg);
    execution_engine_linux_submit_work(test_work);
}

static void action_stop_engine_and_run_worker(void)
{
    test_engine_set_stop(test_engine, true);
    ASSERT_ARE_EQUAL(int, 0, run_test_thread(0));
}

static void action_observe_work(void)
{
    observed_current_execution_engine = test_get_current_execution_engine();
    observed_pending_count = test_work_get_pending_count(test_work);
    observed_running_count = test_work_get_running_count(test_work);
    observed_queue_count = test_engine_get_queue_count(test_engine);
}

static void action_close_test_work(void)
{
    execution_engine_linux_close_work(test_work);
}

static void action_dec_ref_test_engine(void)
{
    execution_engine_dec_ref(test_engine);
}

static EXECUTION_ENGINE_HANDLE test_create_execution_engine(uint32_t min_thread_count, uint32_t max_thread_count)
{
    EXECUTION_ENGINE_PARAMETERS parameters = { min_thread_count, max_thread_count };
    EXECUTION_ENGINE_HANDLE result = execution_engine_create(&parameters);
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_create_work_with_context(EXECUTION_ENGINE_HANDLE execution_engine, void* context)
{
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_work(execution_engine, test_work_function, context);
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_create_work(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return test_create_work_with_context(execution_engine, &test_work_context);
}

static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_create_timer(EXECUTION_ENGINE_HANDLE execution_engine)
{
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(execution_engine, test_work_function, &test_work_context);
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

static void test_submit_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    execution_engine_linux_submit_work(work);
    umock_c_reset_all_calls();
}

static void test_set_timer(EXECUTION_ENGINE_LINUX_WORK_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    execution_engine_linux_set_timer(timer, start_delay_ms, timer_period_ms);
    umock_c_reset_all_calls();
}

static void test_cancel_and_close_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
}

// creates an engine without minimum threads whose manager created one worker (test_threads[1]) that is idle since idle_since_ms
static void test_create_execution_engine_with_idle_manager_created_worker(uint64_t idle_since_ms)
{
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    execution_engine_linux_submit_work(test_work);
    // the manager creates the worker once the ready queue made no progress for 50 ms
    add_wait_action(action_time_out_wait);
    add_wait_action(action_stop_engine);
    ASSERT_ARE_EQUAL(int, 0, run_test_thread(0));
    ASSERT_ARE_EQUAL(uint32_t, 2, test_thread_count);
    execution_engine_linux_wait_for_work(test_work, true);
    test_worker_park(test_threads[1].arg, idle_since_ms);
    test_engine_set_stop(test_engine, false);
    reset_actions();
    umock_c_reset_all_calls();
}

static void setup_create_worker_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, test_get_worker_thread_func(), IGNORED_ARG));
}

static void setup_init_lock_and_conditions_expectations(void)
{
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_setclock(IGNORED_ARG, CLOCK_MONOTONIC));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_destroy(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL));
}

static void setup_execution_engine_create_expectations(uint32_t min_thread_count)
{
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1))
        .CallCannotFail();
    setup_init_lock_and_conditions_expectations();
    for (uint32_t i = 0; i < min_thread_count; i++)
    {
        STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG))
            .CallCannotFail();
        setup_create_worker_expectations();
    }
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, test_get_manager_thread_func(), IGNORED_ARG));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(clockid_t, int);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc_2, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, hook_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pthread_mutex_init, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pthread_condattr_init, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pthread_condattr_setclock, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pthread_cond_init, 1);
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

    test_thread_count = 0;
    set_now_ms(TEST_START_TIME_MS);
    test_clock_increment_ns = 0;
    test_condattr_init_attr = NULL;
    test_condattr_setclock_attr = NULL;
    test_condattr_destroy_attr = NULL;
    test_cond_init_attr = NULL;
    reset_actions();
    test_engine = NULL;
    test_work = NULL;
    test_other_work = NULL;
    observed_current_execution_engine = NULL;
    observed_idle_head = NULL;
    observed_pending_count = 0;
    observed_running_count = 0;
    observed_queue_count = 0;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* execution_engine_create */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_002: [ If execution_engine_parameters is NULL, execution_engine_create shall use the default DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_001: [ If max_thread_count is 0, execution_engine_create shall use 500 as the maximum number of worker threads, or min_thread_count if it is larger. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_003: [ execution_engine_create shall initialize the mutex guarding the execution engine state by calling pthread_mutex_init. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the condition variable used to wake the manager thread with an attribute that uses CLOCK_MONOTONIC by calling pthread_condattr_init, pthread_condattr_setclock, pthread_cond_init and pthread_condattr_destroy, and the condition variable used to signal completed work by calling pthread_cond_init. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_005: [ execution_engine_create shall create min_thread_count worker threads. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_006: [ execution_engine_create shall create the manager thread by calling ThreadAPI_Create with manager_thread_func. ]*/
TEST_FUNCTION(execution_engine_create_with_NULL_parameters_uses_the_defaults)
{
    // arrange
    setup_execution_engine_create_expectations(DEFAULT_MIN_THREAD_COUNT);

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);

    // assert
    ASSERT_IS_NOT_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    const EXECUTION_ENGINE_PARAMETERS* parameters = execution_engine_linux_get_parameters(execution_engine);
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MIN_THREAD_COUNT, parameters->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MAX_THREAD_COUNT, parameters->max_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 500, test_engine_get_max_worker_count(execution_engine));
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MIN_THREAD_COUNT, test_engine_get_worker_count(execution_engine));
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MIN_THREAD_COUNT + 1, test_thread_count);
    ASSERT_ARE_EQUAL(void_ptr, execution_engine, test_threads[DEFAULT_MIN_THREAD_COUNT].arg);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_003: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_004: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_003: [ execution_engine_create shall initialize the mutex guarding the execution engine state by calling pthread_mutex_init. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the condition variable used to wake the manager thread with an attribute that uses CLOCK_MONOTONIC by calling pthread_condattr_init, pthread_condattr_setclock, pthread_cond_init and pthread_condattr_destroy, and the condition variable used to signal completed work by calling pthread_cond_init. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_005: [ execution_engine_create shall create min_thread_count worker threads. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_006: [ execution_engine_create shall create the manager thread by calling ThreadAPI_Create with manager_thread_func. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_008: [ create_worker shall allocate a worker. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_009: [ create_worker shall initialize the condition variable used to wake the worker by calling pthread_cond_init. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_010: [ create_worker shall start the worker thread by calling ThreadAPI_Create with worker_thread_func. ]*/
TEST_FUNCTION(execution_engine_create_succeeds)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 2, 10 };
    setup_execution_engine_create_expectations(2);

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NOT_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    const EXECUTION_ENGINE_PARAMETERS* result_parameters = execution_engine_linux_get_parameters(execution_engine);
    ASSERT_ARE_EQUAL(uint32_t, 2, result_parameters->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 10, result_parameters->max_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 10, test_engine_get_max_worker_count(execution_engine));
    ASSERT_ARE_EQUAL(uint32_t, 2, test_engine_get_worker_count(execution_engine));
    // the manager condition variable is initialized with the attribute set to CLOCK_MONOTONIC
    ASSERT_IS_NOT_NULL(test_condattr_init_attr);
    ASSERT_ARE_EQUAL(void_ptr, test_condattr_init_attr, test_condattr_setclock_attr);
    ASSERT_ARE_EQUAL(void_ptr, test_condattr_init_attr, test_cond_init_attr);
    ASSERT_ARE_EQUAL(void_ptr, test_condattr_init_attr, test_condattr_destroy_attr);
    ASSERT_ARE_EQUAL(uint32_t, 3, test_thread_count);
    ASSERT_IS_NOT_NULL(test_threads[0].arg);
    ASSERT_IS_NOT_NULL(test_threads[1].arg);
    ASSERT_ARE_NOT_EQUAL(void_ptr, test_threads[0].arg, test_threads[1].arg);
    ASSERT_ARE_EQUAL(void_ptr, execution_engine, test_threads[2].arg);

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_001: [ If max_thread_count is 0, execution_engine_create shall use 500 as the maximum number of worker threads, or min_thread_count if it is larger. ]*/
TEST_FUNCTION(execution_engine_create_with_max_thread_count_0_and_min_thread_count_above_500_uses_min_thread_count_as_maximum)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 501, 0 };
    setup_execution_engine_create_expectations(501);

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NOT_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 501, test_engine_get_max_worker_count(execution_engine));
    ASSERT_ARE_EQUAL(uint32_t, 501, test_engine_get_worker_count(execution_engine));

    // cleanup
    execution_engine_dec_ref(execution_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_005: [ If max_thread_count is non-zero and less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_max_thread_count_is_non_zero_and_less_than_min_thread_count)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 5, 3 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_thread_count);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_007: [ If any error occurs after worker threads were created, execution_engine_create shall signal them to stop and join them. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_011: [ If any error occurs, create_worker shall free the resources it allocated and fail. ]*/
TEST_FUNCTION(when_underlying_calls_fail_execution_engine_create_fails)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 2, 4 };
    setup_execution_engine_create_expectations(2);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);
            test_thread_count = 0;

            // act
            EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

            // assert
            ASSERT_IS_NULL(execution_engine, "On failed call %zu", i);
        }
    }
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the condition variable used to wake the manager thread with an attribute that uses CLOCK_MONOTONIC by calling pthread_condattr_init, pthread_condattr_setclock, pthread_cond_init and pthread_condattr_destroy, and the condition variable used to signal completed work by calling pthread_cond_init. ]*/
TEST_FUNCTION(execution_engine_create_when_pthread_condattr_init_fails_fails)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 1, 4 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_thread_count);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the condition variable used to wake the manager thread with an attribute that uses CLOCK_MONOTONIC by calling pthread_condattr_init, pthread_condattr_setclock, pthread_cond_init and pthread_condattr_destroy, and the condition variable used to signal completed work by calling pthread_cond_init. ]*/
TEST_FUNCTION(execution_engine_create_when_pthread_condattr_setclock_fails_destroys_the_attribute_and_fails)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 1, 4 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_setclock(IGNORED_ARG, CLOCK_MONOTONIC))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_thread_count);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the condition variable used to wake the manager thread with an attribute that uses CLOCK_MONOTONIC by calling pthread_condattr_init, pthread_condattr_setclock, pthread_cond_init and pthread_condattr_destroy, and the condition variable used to signal completed work by calling pthread_cond_init. ]*/
TEST_FUNCTION(execution_engine_create_when_initializing_the_manager_condition_variable_fails_destroys_the_attribute_and_fails)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 1, 4 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_setclock(IGNORED_ARG, CLOCK_MONOTONIC));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_thread_count);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_007: [ If any error occurs after worker threads were created, execution_engine_create shall signal them to stop and join them. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_011: [ If any error occurs, create_worker shall free the resources it allocated and fail. ]*/
TEST_FUNCTION(execution_engine_create_when_starting_the_second_worker_fails_stops_and_joins_the_first_worker)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 2, 4 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    setup_init_lock_and_conditions_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    setup_create_worker_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, test_get_worker_thread_func(), IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    // stop and join the first worker
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(&test_threads[0], IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_thread_count);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_009: [ create_worker shall initialize the condition variable used to wake the worker by calling pthread_cond_init. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_011: [ If any error occurs, create_worker shall free the resources it allocated and fail. ]*/
TEST_FUNCTION(execution_engine_create_when_initializing_the_worker_condition_variable_fails_frees_the_worker_and_fails)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 1, 4 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    setup_init_lock_and_conditions_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_thread_count);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_006: [ execution_engine_create shall create the manager thread by calling ThreadAPI_Create with manager_thread_func. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_007: [ If any error occurs after worker threads were created, execution_engine_create shall signal them to stop and join them. ]*/
TEST_FUNCTION(execution_engine_create_when_creating_the_manager_thread_fails_stops_and_joins_the_workers)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { 1, 4 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    setup_init_lock_and_conditions_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    setup_create_worker_expectations();
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, test_get_manager_thread_func(), IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(&test_threads[0], IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    // assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_thread_count);
}

/* execution_engine_dec_ref */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_007: [ If execution_engine is NULL, execution_engine_dec_ref shall return. ]*/
TEST_FUNCTION(execution_engine_dec_ref_with_NULL_execution_engine_returns)
{
    // arrange

    // act
    execution_engine_dec_ref(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_008: [ Otherwise execution_engine_dec_ref shall decrement the refcount. ]*/
TEST_FUNCTION(execution_engine_dec_ref_when_not_the_last_reference_only_decrements_the_refcount)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    execution_engine_inc_ref(test_engine);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    // act
    execution_engine_dec_ref(test_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_008: [ Otherwise execution_engine_dec_ref shall decrement the refcount. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_009: [ If the refcount is zero execution_engine_dec_ref shall free the memory for EXECUTION_ENGINE. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_013: [ Otherwise, if the refcount reaches 0, execution_engine_dec_ref shall set the stop flag and signal the manager thread and every idle worker. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_014: [ execution_engine_dec_ref shall join the manager thread and all the worker threads. ]*/
TEST_FUNCTION(execution_engine_dec_ref_when_the_last_reference_stops_and_joins_the_threads_and_frees_the_execution_engine)
{
    // arrange
    test_engine = test_create_execution_engine(2, 4);
    void* worker_0 = test_threads[0].arg;
    void* worker_1 = test_threads[1].arg;
    // only worker 0 is idle
    test_worker_park(worker_0, TEST_START_TIME_MS);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);
    pthread_cond_t* manager_cond = test_engine_get_manager_cond(test_engine);
    pthread_cond_t* work_done_cond = test_engine_get_work_done_cond(test_engine);
    pthread_cond_t* worker_0_wake_cond = test_worker_get_wake_cond(worker_0);
    pthread_cond_t* worker_1_wake_cond = test_worker_get_wake_cond(worker_1);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_0_wake_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(&test_threads[2], IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(&test_threads[1], IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_1_wake_cond));
    STRICT_EXPECTED_CALL(free(worker_1));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(&test_threads[0], IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_0_wake_cond));
    STRICT_EXPECTED_CALL(free(worker_0));
    STRICT_EXPECTED_CALL(free(NULL));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(work_done_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(lock));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    execution_engine_dec_ref(test_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If the refcount reaches 0 on a worker thread of execution_engine, execution_engine_dec_ref shall terminate the process by calling LogCriticalAndTerminate. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_058: [ worker_thread_func shall record the execution engine as the current execution engine of the thread until it returns. ]*/
TEST_FUNCTION(execution_engine_dec_ref_releasing_the_last_reference_on_a_worker_thread_terminates_the_process)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    test_engine_set_stop(test_engine, true);
    add_work_action(action_dec_ref_test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ps_util_terminate_process());
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_inc_ref(test_engine);
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_inc_ref */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_010: [ If execution_engine is NULL then execution_engine_inc_ref shall return. ]*/
TEST_FUNCTION(execution_engine_inc_ref_with_NULL_execution_engine_returns)
{
    // arrange

    // act
    execution_engine_inc_ref(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_011: [ Otherwise execution_engine_inc_ref shall increment the reference count for execution_engine. ]*/
TEST_FUNCTION(execution_engine_inc_ref_increments_the_refcount)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    // act
    execution_engine_inc_ref(test_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(test_engine);
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_get_parameters */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_012: [ If execution_engine is NULL, execution_engine_linux_get_parameters shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_get_parameters_with_NULL_execution_engine_fails)
{
    // arrange

    // act
    const EXECUTION_ENGINE_PARAMETERS* result = execution_engine_linux_get_parameters(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_07_013: [ Otherwise, execution_engine_linux_get_parameters shall return the parameters in EXECUTION_ENGINE. ]*/
TEST_FUNCTION(execution_engine_linux_get_parameters_returns_the_parameters)
{
    // arrange
    test_engine = test_create_execution_engine(3, 7);

    // act
    const EXECUTION_ENGINE_PARAMETERS* result = execution_engine_linux_get_parameters(test_engine);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 3, result->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 7, result->max_thread_count);

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_create_work */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_015: [ If execution_engine is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_work_with_NULL_execution_engine_fails)
{
    // arrange

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_work(NULL, test_work_function, &test_work_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_016: [ If work_function is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_work_with_NULL_work_function_fails)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_work(test_engine, NULL, &test_work_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_017: [ execution_engine_linux_create_work shall allocate a work object, store work_function and work_function_context in it, and return it. ]*/
TEST_FUNCTION(execution_engine_linux_create_work_succeeds)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_work(test_engine, test_work_function, &test_work_context);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_work_get_work_function(result) == test_work_function);
    ASSERT_ARE_EQUAL(void_ptr, &test_work_context, test_work_get_work_function_context(result));
    ASSERT_IS_FALSE(test_work_is_timer(result));
    ASSERT_IS_FALSE(test_work_is_closed(result));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_pending_count(result));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_running_count(result));

    // cleanup
    execution_engine_linux_close_work(result);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_018: [ If any error occurs, execution_engine_linux_create_work shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_execution_engine_linux_create_work_fails)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_work(test_engine, test_work_function, &test_work_context);

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_create_timer */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If execution_engine is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_with_NULL_execution_engine_fails)
{
    // arrange

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(NULL, test_work_function, &test_work_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_020: [ If work_function is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_with_NULL_work_function_fails)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(test_engine, NULL, &test_work_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_021: [ execution_engine_linux_create_timer shall allocate a work object marked as a timer and store work_function and work_function_context in it. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_022: [ execution_engine_linux_create_timer shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_023: [ If the timer heap is full, execution_engine_linux_create_timer shall double its capacity (16 if it is empty) by calling realloc_2. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_024: [ execution_engine_linux_create_timer shall increment the timer count, release the mutex and return the timer. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_for_the_first_timer_allocates_a_timer_heap_of_16)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(EXECUTION_ENGINE_LINUX_WORK_HANDLE)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(test_engine, test_work_function, &test_work_context);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_work_is_timer(result));
    ASSERT_IS_FALSE(test_work_is_armed(result));
    ASSERT_IS_TRUE(test_work_get_work_function(result) == test_work_function);
    ASSERT_ARE_EQUAL(void_ptr, &test_work_context, test_work_get_work_function_context(result));
    ASSERT_ARE_EQUAL(uint32_t, 16, test_engine_get_timer_heap_capacity(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_heap_count(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_timer_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(result);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_024: [ execution_engine_linux_create_timer shall increment the timer count, release the mutex and return the timer. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_when_the_timer_heap_has_room_does_not_grow_it)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE first_timer = test_create_timer(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(test_engine, test_work_function, &test_work_context);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 16, test_engine_get_timer_heap_capacity(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 2, test_engine_get_timer_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(result);
    execution_engine_linux_close_work(first_timer);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_023: [ If the timer heap is full, execution_engine_linux_create_timer shall double its capacity (16 if it is empty) by calling realloc_2. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_024: [ execution_engine_linux_create_timer shall increment the timer count, release the mutex and return the timer. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_when_the_timer_heap_is_full_doubles_its_capacity)
{
    // arrange
    EXECUTION_ENGINE_LINUX_WORK_HANDLE timers[16];
    test_engine = test_create_execution_engine(0, 4);
    for (uint32_t i = 0; i < 16; i++)
    {
        timers[i] = test_create_timer(test_engine);
    }
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, 32, sizeof(EXECUTION_ENGINE_LINUX_WORK_HANDLE)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(test_engine, test_work_function, &test_work_context);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 32, test_engine_get_timer_heap_capacity(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 17, test_engine_get_timer_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(result);
    for (uint32_t i = 0; i < 16; i++)
    {
        execution_engine_linux_close_work(timers[i]);
    }
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_025: [ If any error occurs, execution_engine_linux_create_timer shall release the mutex if acquired, free the work object, fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_when_growing_the_timer_heap_fails_releases_the_mutex_and_frees_the_timer)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(EXECUTION_ENGINE_LINUX_WORK_HANDLE)))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(test_engine, test_work_function, &test_work_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_heap_capacity(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_count(test_engine));

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_025: [ If any error occurs, execution_engine_linux_create_timer shall release the mutex if acquired, free the work object, fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_execution_engine_linux_create_timer_fails)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(realloc_2(NULL, 16, sizeof(EXECUTION_ENGINE_LINUX_WORK_HANDLE)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG))
        .CallCannotFail();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            EXECUTION_ENGINE_LINUX_WORK_HANDLE result = execution_engine_linux_create_timer(test_engine, test_work_function, &test_work_context);

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
            ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_count(test_engine));
        }
    }

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_submit_work */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_026: [ If work is NULL, execution_engine_linux_submit_work shall return. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_with_NULL_work_returns)
{
    // arrange

    // act
    execution_engine_linux_submit_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_027: [ execution_engine_linux_submit_work shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_029: [ Otherwise execution_engine_linux_submit_work shall increment the pending count of work. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_030: [ If the pending count became 1, execution_engine_linux_submit_work shall link work at the tail of the ready queue and dispatch it. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_031: [ execution_engine_linux_submit_work shall release the mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_032: [ If the ready queue was empty, enqueue_and_dispatch shall record the current time as the last progress time. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_033: [ enqueue_and_dispatch shall link work at the tail of the ready queue. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_035: [ Otherwise, if the thread count is below the maximum and the manager thread is not already waiting to check the ready queue for progress, enqueue_and_dispatch shall wake the manager thread by calling pthread_cond_signal. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_queues_the_work_and_wakes_the_manager_when_no_worker_is_idle)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_queue_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, test_work, test_engine_get_queue_head(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, test_work, test_engine_get_queue_tail(test_engine));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS, test_engine_get_last_progress_ms(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_029: [ Otherwise execution_engine_linux_submit_work shall increment the pending count of work. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_030: [ If the pending count became 1, execution_engine_linux_submit_work shall link work at the tail of the ready queue and dispatch it. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_when_the_work_is_already_queued_only_increments_the_pending_count)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 2, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_queue_count(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_032: [ If the ready queue was empty, enqueue_and_dispatch shall record the current time as the last progress time. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_033: [ enqueue_and_dispatch shall link work at the tail of the ready queue. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_when_the_ready_queue_is_not_empty_links_the_work_at_the_tail_and_keeps_the_last_progress_time)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    test_other_work = test_create_work_with_context(test_engine, &test_other_work_context);
    test_submit_work(test_work);
    set_now_ms(TEST_START_TIME_MS + 10);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_other_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 2, test_engine_get_queue_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, test_work, test_engine_get_queue_head(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, test_other_work, test_engine_get_queue_tail(test_engine));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS, test_engine_get_last_progress_ms(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    test_cancel_and_close_work(test_other_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_034: [ If a worker is idle and no other worker was woken and has not started running yet, enqueue_and_dispatch shall remove the most recently idle worker from the idle list, mark it signaled and wake it by calling pthread_cond_signal. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_wakes_the_most_recently_idle_worker)
{
    // arrange
    test_engine = test_create_execution_engine(2, 4);
    void* worker_0 = test_threads[0].arg;
    void* worker_1 = test_threads[1].arg;
    test_worker_park(worker_0, TEST_START_TIME_MS);
    test_worker_park(worker_1, TEST_START_TIME_MS);
    test_work = test_create_work(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_worker_get_wake_cond(worker_1)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_worker_is_signaled(worker_1));
    ASSERT_IS_FALSE(test_worker_is_idle(worker_1));
    ASSERT_IS_FALSE(test_worker_is_signaled(worker_0));
    ASSERT_IS_TRUE(test_worker_is_idle(worker_0));
    ASSERT_ARE_EQUAL(void_ptr, worker_0, test_engine_get_idle_head(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_signaled_worker_count(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_082: [ If a worker is idle and another worker was woken and has not started running yet, enqueue_and_dispatch shall not wake another worker. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_when_a_woken_worker_did_not_start_running_does_not_wake_another_idle_worker)
{
    // arrange
    test_engine = test_create_execution_engine(2, 4);
    void* worker_0 = test_threads[0].arg;
    test_worker_park(worker_0, TEST_START_TIME_MS);
    test_worker_park(test_threads[1].arg, TEST_START_TIME_MS);
    test_work = test_create_work(test_engine);
    test_other_work = test_create_work_with_context(test_engine, &test_other_work_context);
    test_submit_work(test_work);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_other_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_worker_is_signaled(worker_0));
    ASSERT_IS_TRUE(test_worker_is_idle(worker_0));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_signaled_worker_count(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 2, test_engine_get_queue_count(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    test_cancel_and_close_work(test_other_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_035: [ Otherwise, if the thread count is below the maximum and the manager thread is not already waiting to check the ready queue for progress, enqueue_and_dispatch shall wake the manager thread by calling pthread_cond_signal. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_when_the_thread_count_is_at_the_maximum_does_not_wake_the_manager)
{
    // arrange
    test_engine = test_create_execution_engine(1, 1);
    test_work = test_create_work(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_queue_count(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_035: [ Otherwise, if the thread count is below the maximum and the manager thread is not already waiting to check the ready queue for progress, enqueue_and_dispatch shall wake the manager thread by calling pthread_cond_signal. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_when_the_manager_is_waiting_to_check_the_ready_queue_for_progress_does_not_wake_the_manager)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    test_other_work = test_create_work_with_context(test_engine, &test_other_work_context);
    execution_engine_linux_submit_work(test_work);
    // the manager waits for the ready queue to make progress for 50 ms
    add_wait_action(action_stop_engine);
    ASSERT_ARE_EQUAL(int, 0, run_test_thread(0));
    ASSERT_IS_TRUE(test_engine_is_manager_watching_queue(test_engine));
    umock_c_reset_all_calls();
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_other_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 2, test_engine_get_queue_count(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    test_cancel_and_close_work(test_other_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_028: [ If the pending count of work is UINT32_MAX, execution_engine_linux_submit_work shall log an error and not queue another invocation. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_when_the_pending_count_is_UINT32_MAX_does_not_queue_another_invocation)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    test_work_set_pending_count(test_work, UINT32_MAX);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_submit_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, UINT32_MAX, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_queue_count(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_set_timer */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_036: [ If timer is NULL, execution_engine_linux_set_timer shall return. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_with_NULL_timer_returns)
{
    // arrange

    // act
    execution_engine_linux_set_timer(NULL, 100, 50);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_037: [ If timer is not a timer, execution_engine_linux_set_timer shall log an error and return. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_with_a_work_object_that_is_not_a_timer_returns)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);

    // act
    execution_engine_linux_set_timer(test_work, 100, 50);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_work_is_armed(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_heap_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_038: [ execution_engine_linux_set_timer shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_040: [ execution_engine_linux_set_timer shall set the due time of timer to the current CLOCK_MONOTONIC time obtained by calling clock_gettime plus start_delay_ms, store timer_period_ms and insert timer in the timer heap. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_041: [ execution_engine_linux_set_timer shall wake the manager thread by calling pthread_cond_signal and release the mutex. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_arms_the_timer_and_wakes_the_manager)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_timer(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_set_timer(test_work, 100, 50);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_work_is_armed(test_work));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 100, test_work_get_due_time_ms(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 50, test_work_get_period_ms(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_timer_heap_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, test_work, test_engine_get_earliest_timer(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_pending_count(test_work));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_040: [ execution_engine_linux_set_timer shall set the due time of timer to the current CLOCK_MONOTONIC time obtained by calling clock_gettime plus start_delay_ms, store timer_period_ms and insert timer in the timer heap. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_rounds_the_current_time_up_to_the_next_millisecond)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_timer(test_engine);
    test_now_ns = ((uint64_t)TEST_START_TIME_MS * 1000000) + 1;
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_set_timer(test_work, 100, 0);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 101, test_work_get_due_time_ms(test_work));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_039: [ If timer is armed, execution_engine_linux_set_timer shall remove it from the timer heap. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_040: [ execution_engine_linux_set_timer shall set the due time of timer to the current CLOCK_MONOTONIC time obtained by calling clock_gettime plus start_delay_ms, store timer_period_ms and insert timer in the timer heap. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_re_arms_an_armed_timer)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_timer(test_engine);
    test_set_timer(test_work, 100, 0);
    set_now_ms(TEST_START_TIME_MS + 10);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_set_timer(test_work, 300, 20);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_timer_heap_count(test_engine));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 310, test_work_get_due_time_ms(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 20, test_work_get_period_ms(test_work));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_040: [ execution_engine_linux_set_timer shall set the due time of timer to the current CLOCK_MONOTONIC time obtained by calling clock_gettime plus start_delay_ms, store timer_period_ms and insert timer in the timer heap. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_keeps_the_earliest_timer_first_in_the_timer_heap)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE timer_300 = test_create_timer(test_engine);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE timer_200 = test_create_timer(test_engine);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE timer_100 = test_create_timer(test_engine);
    test_set_timer(timer_300, 300, 0);
    test_set_timer(timer_200, 200, 0);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_set_timer(timer_100, 100, 0);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 3, test_engine_get_timer_heap_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, timer_100, test_engine_get_earliest_timer(test_engine));

    // cleanup
    execution_engine_linux_close_work(timer_300);
    execution_engine_linux_close_work(timer_200);
    execution_engine_linux_close_work(timer_100);
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_cancel_timer */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_042: [ If timer is NULL, execution_engine_linux_cancel_timer shall return. ]*/
TEST_FUNCTION(execution_engine_linux_cancel_timer_with_NULL_timer_returns)
{
    // arrange

    // act
    execution_engine_linux_cancel_timer(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_043: [ If timer is not a timer, execution_engine_linux_cancel_timer shall log an error and return. ]*/
TEST_FUNCTION(execution_engine_linux_cancel_timer_with_a_work_object_that_is_not_a_timer_returns)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);

    // act
    execution_engine_linux_cancel_timer(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_044: [ execution_engine_linux_cancel_timer shall acquire the execution engine mutex, remove timer from the timer heap if it is armed and release the mutex. ]*/
TEST_FUNCTION(execution_engine_linux_cancel_timer_disarms_an_armed_timer)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_timer(test_engine);
    test_set_timer(test_work, 100, 50);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_cancel_timer(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_work_is_armed(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_heap_count(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_timer_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_044: [ execution_engine_linux_cancel_timer shall acquire the execution engine mutex, remove timer from the timer heap if it is armed and release the mutex. ]*/
TEST_FUNCTION(execution_engine_linux_cancel_timer_with_a_timer_that_is_not_armed_does_nothing)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_timer(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_cancel_timer(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_work_is_armed(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_heap_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_044: [ execution_engine_linux_cancel_timer shall acquire the execution engine mutex, remove timer from the timer heap if it is armed and release the mutex. ]*/
TEST_FUNCTION(execution_engine_linux_cancel_timer_of_the_earliest_timer_makes_the_next_timer_the_earliest)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE timer_300 = test_create_timer(test_engine);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE timer_100 = test_create_timer(test_engine);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE timer_200 = test_create_timer(test_engine);
    test_set_timer(timer_300, 300, 0);
    test_set_timer(timer_100, 100, 0);
    test_set_timer(timer_200, 200, 0);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_cancel_timer(timer_100);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_work_is_armed(timer_100));
    ASSERT_ARE_EQUAL(uint32_t, 2, test_engine_get_timer_heap_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, timer_200, test_engine_get_earliest_timer(test_engine));

    // cleanup
    execution_engine_linux_close_work(timer_300);
    execution_engine_linux_close_work(timer_100);
    execution_engine_linux_close_work(timer_200);
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_wait_for_work */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_045: [ If work is NULL, execution_engine_linux_wait_for_work shall return. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_with_NULL_work_returns)
{
    // arrange

    // act
    execution_engine_linux_wait_for_work(NULL, true);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_046: [ execution_engine_linux_wait_for_work shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_048: [ execution_engine_linux_wait_for_work shall wait by calling pthread_cond_wait until work has no pending and no running invocations. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_049: [ execution_engine_linux_wait_for_work shall release the mutex. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_without_invocations_returns_without_waiting)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_wait_for_work(test_work, false);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_047: [ If cancel_pending is true and work has pending invocations, execution_engine_linux_wait_for_work shall unlink work from the ready queue and set its pending count to 0. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_with_cancel_pending_unlinks_the_work_and_drops_its_pending_invocations)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    test_other_work = test_create_work_with_context(test_engine, &test_other_work_context);
    test_submit_work(test_work);
    test_submit_work(test_work);
    test_submit_work(test_other_work);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_wait_for_work(test_work, true);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_queue_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, test_other_work, test_engine_get_queue_head(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_other_work));

    // cleanup
    execution_engine_linux_close_work(test_work);
    test_cancel_and_close_work(test_other_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_046: [ execution_engine_linux_wait_for_work shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_048: [ execution_engine_linux_wait_for_work shall wait by calling pthread_cond_wait until work has no pending and no running invocations. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_049: [ execution_engine_linux_wait_for_work shall release the mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_067: [ If the work object has no pending and no running invocations, it is not closed and threads are waiting for it, wake them by calling pthread_cond_broadcast. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_waits_until_a_worker_runs_the_pending_invocation)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    // while the test waits a worker runs the work
    add_wait_action(action_stop_engine_and_run_worker);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);
    pthread_cond_t* work_done_cond = test_engine_get_work_done_cond(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(work_done_cond, lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_broadcast(work_done_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_wait_for_work(test_work, false);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_running_count(test_work));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* execution_engine_linux_close_work */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_050: [ If work is NULL, execution_engine_linux_close_work shall return. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_with_NULL_work_returns)
{
    // arrange

    // act
    execution_engine_linux_close_work(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_051: [ execution_engine_linux_close_work shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_053: [ If work has no pending and no running invocations, execution_engine_linux_close_work shall free it. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_055: [ execution_engine_linux_close_work shall release the mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_057: [ free_work shall free work. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_without_invocations_frees_the_work)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(free(test_work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_close_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_052: [ If work is an armed timer, execution_engine_linux_close_work shall remove it from the timer heap. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_053: [ If work has no pending and no running invocations, execution_engine_linux_close_work shall free it. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_056: [ If work is a timer, free_work shall decrement the timer count. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_057: [ free_work shall free work. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_with_an_armed_timer_disarms_and_frees_it)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_timer(test_engine);
    test_set_timer(test_work, 100, 50);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(free(test_work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_close_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_heap_count(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_count(test_engine));

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_054: [ Otherwise execution_engine_linux_close_work shall mark work as closed so that the worker running its last invocation frees it. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_055: [ execution_engine_linux_close_work shall release the mutex. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_with_pending_invocations_marks_the_work_closed)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    execution_engine_linux_close_work(test_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_work_is_closed(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_work));

    // cleanup
    // the worker runs the last invocation and frees the work
    test_engine_set_stop(test_engine, true);
    ASSERT_ARE_EQUAL(int, 0, run_test_thread(0));
    execution_engine_dec_ref(test_engine);
}

/* worker_thread_func */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_058: [ worker_thread_func shall record the execution engine as the current execution engine of the thread until it returns. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_059: [ worker_thread_func shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_060: [ Otherwise, if the ready queue is not empty, worker_thread_func shall: ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_061: [ unlink the work object at the head of the ready queue and decrement its pending count. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_063: [ Increment the running count of the work object and record the current time as the last progress time. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_064: [ Release the mutex, call the work function with the work function context and acquire the mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_065: [ Decrement the running count of the work object. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_068: [ Otherwise, if the stop flag is set, worker_thread_func shall release the mutex and return. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_069: [ Otherwise worker_thread_func shall add the worker to the front of the idle list and wait by calling pthread_cond_wait until it is signaled, retired or the stop flag is set. ]*/
TEST_FUNCTION(worker_thread_func_runs_the_queued_work_then_waits_idle_and_returns_when_stopped)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    void* worker = test_threads[0].arg;
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    set_now_ms(TEST_START_TIME_MS + 5);
    add_work_action(action_observe_work);
    add_wait_action(action_observe_idle_head_and_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_worker_get_wake_cond(worker), lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, test_engine, observed_current_execution_engine);
    ASSERT_IS_NULL(test_get_current_execution_engine());
    ASSERT_ARE_EQUAL(uint32_t, 0, observed_pending_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, observed_running_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, observed_queue_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_running_count(test_work));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 5, test_engine_get_last_progress_ms(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, worker, observed_idle_head);
    ASSERT_IS_FALSE(test_worker_is_idle(worker));
    ASSERT_IS_NULL(test_engine_get_idle_head(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_060: [ Otherwise, if the ready queue is not empty, worker_thread_func shall: ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_062: [ If the work object still has pending invocations, link it back at the tail of the ready queue and dispatch it. ]*/
TEST_FUNCTION(worker_thread_func_links_back_and_dispatches_a_work_object_that_still_has_pending_invocations)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    test_submit_work(test_work);
    test_engine_set_stop(test_engine, true);
    add_work_action(action_observe_work);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, observed_pending_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, observed_queue_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_queue_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_083: [ If the ready queue is still not empty, a worker is idle and no other worker was woken and has not started running yet, wake the most recently idle worker. ]*/
TEST_FUNCTION(worker_thread_func_when_work_is_still_queued_after_taking_a_work_object_wakes_the_most_recently_idle_worker)
{
    // arrange
    test_engine = test_create_execution_engine(2, 4);
    void* worker_0 = test_threads[0].arg;
    void* worker_1 = test_threads[1].arg;
    test_worker_park(worker_0, TEST_START_TIME_MS);
    test_work = test_create_work(test_engine);
    test_other_work = test_create_work_with_context(test_engine, &test_other_work_context);
    // while worker 1 waits two work objects are submitted, the first one wakes worker 1
    add_wait_action(action_submit_test_work_and_other_work);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);
    pthread_cond_t* worker_1_wake_cond = test_worker_get_wake_cond(worker_1);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(worker_1_wake_cond, lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_1_wake_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_worker_get_wake_cond(worker_0)));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_other_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(worker_1_wake_cond, lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(1);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_worker_is_signaled(worker_0));
    ASSERT_IS_FALSE(test_worker_is_idle(worker_0));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_signaled_worker_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_linux_close_work(test_other_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_066: [ If the work object has no pending and no running invocations and it is closed, free it. ]*/
TEST_FUNCTION(worker_thread_func_frees_a_work_object_closed_by_its_own_callback_after_the_callback_returns)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    test_engine_set_stop(test_engine, true);
    add_work_action(action_close_test_work);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(free(test_work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_069: [ Otherwise worker_thread_func shall add the worker to the front of the idle list and wait by calling pthread_cond_wait until it is signaled, retired or the stop flag is set. ]*/
TEST_FUNCTION(worker_thread_func_woken_by_submitted_work_runs_it)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    void* worker = test_threads[0].arg;
    test_work = test_create_work(test_engine);
    add_wait_action(action_submit_test_work);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);
    pthread_cond_t* wake_cond = test_worker_get_wake_cond(worker);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(wake_cond, lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(wake_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_work_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(wake_cond, lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_signaled_worker_count(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_081: [ If the worker was retired, worker_thread_func shall release the mutex and return. ]*/
TEST_FUNCTION(worker_thread_func_when_the_worker_was_retired_returns_even_if_the_ready_queue_is_not_empty)
{
    // arrange
    test_engine = test_create_execution_engine(1, 2);
    void* worker = test_threads[0].arg;
    test_work = test_create_work(test_engine);
    add_wait_action(action_retire_worker_and_submit_test_work);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_worker_get_wake_cond(worker), lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(test_engine_get_manager_cond(test_engine)));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_queue_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, test_work, test_engine_get_queue_head(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* manager_thread_func */

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_070: [ manager_thread_func shall acquire the execution engine mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_071: [ Until the stop flag is set, manager_thread_func shall: ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_072: [ Get the current CLOCK_MONOTONIC time by calling clock_gettime. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_079: [ Otherwise wait for the earliest of the next timer due time, the next time a worker may need to be created and, while the thread count is above min_thread_count, the next time an idle worker may need to be retired (at most 60000 ms) by calling pthread_cond_timedwait, or by calling pthread_cond_wait if there is no such time. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_080: [ manager_thread_func shall release the mutex and return. ]*/
TEST_FUNCTION(manager_thread_func_with_nothing_to_do_waits_without_a_deadline_and_returns_when_stopped)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_engine_get_manager_cond(test_engine), lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_073: [ For each timer in the timer heap whose due time has passed, remove it from the heap, submit it and, if its period is not 0, insert it back with its due time advanced by the period (or the current time plus the period if that is not later than the current time). ]*/
TEST_FUNCTION(manager_thread_func_submits_a_due_one_shot_timer_and_does_not_re_arm_it)
{
    // arrange
    test_engine = test_create_execution_engine(1, 1);
    test_work = test_create_timer(test_engine);
    test_set_timer(test_work, 0, 0);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_engine_get_manager_cond(test_engine), lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(1);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(void_ptr, test_work, test_engine_get_queue_head(test_engine));
    ASSERT_IS_FALSE(test_work_is_armed(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_timer_heap_count(test_engine));

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_073: [ For each timer in the timer heap whose due time has passed, remove it from the heap, submit it and, if its period is not 0, insert it back with its due time advanced by the period (or the current time plus the period if that is not later than the current time). ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_079: [ Otherwise wait for the earliest of the next timer due time, the next time a worker may need to be created and, while the thread count is above min_thread_count, the next time an idle worker may need to be retired (at most 60000 ms) by calling pthread_cond_timedwait, or by calling pthread_cond_wait if there is no such time. ]*/
TEST_FUNCTION(manager_thread_func_submits_a_due_periodic_timer_re_arms_it_by_its_period_and_waits_for_the_next_due_time)
{
    // arrange
    test_engine = test_create_execution_engine(1, 1);
    test_work = test_create_timer(test_engine);
    test_other_work = test_create_timer(test_engine);
    test_set_timer(test_work, 100, 50);
    test_set_timer(test_other_work, 1000, 0);
    set_now_ms(TEST_START_TIME_MS + 120);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(test_engine_get_manager_cond(test_engine), lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(1);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_work));
    ASSERT_IS_TRUE(test_work_is_armed(test_work));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 150, test_work_get_due_time_ms(test_work));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_work_get_pending_count(test_other_work));
    ASSERT_IS_TRUE(test_work_is_armed(test_other_work));
    ASSERT_ARE_EQUAL(void_ptr, test_work, test_engine_get_earliest_timer(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_wait_deadline_count);
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 150, test_wait_deadlines_ms[0]);

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_linux_close_work(test_other_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_073: [ For each timer in the timer heap whose due time has passed, remove it from the heap, submit it and, if its period is not 0, insert it back with its due time advanced by the period (or the current time plus the period if that is not later than the current time). ]*/
TEST_FUNCTION(manager_thread_func_re_arms_a_late_periodic_timer_relative_to_the_current_time)
{
    // arrange
    test_engine = test_create_execution_engine(1, 1);
    test_work = test_create_timer(test_engine);
    test_set_timer(test_work, 100, 50);
    set_now_ms(TEST_START_TIME_MS + 300);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(test_engine_get_manager_cond(test_engine), lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(1);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_work_get_pending_count(test_work));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 350, test_work_get_due_time_ms(test_work));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 350, test_wait_deadlines_ms[0]);

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_008: [ create_worker shall allocate a worker. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_009: [ create_worker shall initialize the condition variable used to wake the worker by calling pthread_cond_init. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_010: [ create_worker shall start the worker thread by calling ThreadAPI_Create with worker_thread_func. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_076: [ If the ready queue has more work objects than workers being started, no worker is idle, the thread count is below the maximum, the thread creation retry time has passed and no work object was taken from the ready queue for 50 ms, increment the thread count, release the mutex, create a worker and acquire the mutex. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_078: [ If creating the worker succeeds, record the current time as the last progress time. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_079: [ Otherwise wait for the earliest of the next timer due time, the next time a worker may need to be created and, while the thread count is above min_thread_count, the next time an idle worker may need to be retired (at most 60000 ms) by calling pthread_cond_timedwait, or by calling pthread_cond_wait if there is no such time. ]*/
TEST_FUNCTION(manager_thread_func_creates_a_worker_when_the_ready_queue_made_no_progress_for_50_ms)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    set_now_ms(TEST_START_TIME_MS + 10);
    add_wait_action(action_time_out_wait);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);
    pthread_cond_t* manager_cond = test_engine_get_manager_cond(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(manager_cond, lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    setup_create_worker_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(manager_cond, lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // first it waits until 50 ms passed since the work was queued
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 50, test_wait_deadlines_ms[0]);
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_worker_count(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 2, test_thread_count);
    ASSERT_IS_TRUE(test_threads[1].thread_func == test_get_worker_thread_func());
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 50, test_engine_get_last_progress_ms(test_engine));
    // above min_thread_count it checks for idle workers to retire at least every 60000 ms
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 50 + 60000, test_wait_deadlines_ms[1]);

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_011: [ If any error occurs, create_worker shall free the resources it allocated and fail. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_077: [ If creating the worker fails, decrement the thread count and set the thread creation retry time to 100 ms from now. ]*/
TEST_FUNCTION(manager_thread_func_when_creating_a_worker_fails_retries_100_ms_later)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_work(test_engine);
    test_submit_work(test_work);
    set_now_ms(TEST_START_TIME_MS + 50);
    add_wait_action(action_time_out_wait);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);
    pthread_cond_t* manager_cond = test_engine_get_manager_cond(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(manager_cond, lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    setup_create_worker_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(manager_cond, lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 150, test_engine_get_next_create_attempt_ms(test_engine));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 150, test_wait_deadlines_ms[0]);
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_worker_count(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 2, test_thread_count);

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_076: [ If the ready queue has more work objects than workers being started, no worker is idle, the thread count is below the maximum, the thread creation retry time has passed and no work object was taken from the ready queue for 50 ms, increment the thread count, release the mutex, create a worker and acquire the mutex. ]*/
TEST_FUNCTION(manager_thread_func_does_not_create_a_worker_when_the_last_progress_time_is_later_than_the_current_time)
{
    // arrange
    test_engine = test_create_execution_engine(0, 4);
    test_work = test_create_timer(test_engine);
    test_set_timer(test_work, 0, 0);
    // the clock moves 1 ms between the manager reading the time and the expired timer being queued
    test_clock_increment_ns = 1000000;
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);
    pthread_cond_t* manager_cond = test_engine_get_manager_cond(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(manager_cond, lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 1, test_engine_get_last_progress_ms(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_worker_count(test_engine));
    ASSERT_ARE_EQUAL(uint32_t, 1, test_thread_count);
    // it waits until the ready queue made no progress for 50 ms
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 1 + 50, test_wait_deadlines_ms[0]);

    // cleanup
    test_cancel_and_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_074: [ While the thread count is above min_thread_count and the least recently idle worker has been idle for at least 60000 ms, remove it from the idle list, mark it retired, wake it by calling pthread_cond_signal and decrement the thread count. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_075: [ For each retired worker, release the mutex, join it by calling ThreadAPI_Join, free it and acquire the mutex. ]*/
TEST_FUNCTION(manager_thread_func_retires_and_joins_a_worker_idle_for_60000_ms_above_min_thread_count)
{
    // arrange
    test_create_execution_engine_with_idle_manager_created_worker(TEST_START_TIME_MS);
    void* worker = test_threads[1].arg;
    pthread_cond_t* wake_cond = test_worker_get_wake_cond(worker);
    set_now_ms(TEST_START_TIME_MS + 60000);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(wake_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(&test_threads[1], IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(wake_cond));
    STRICT_EXPECTED_CALL(free(worker));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_engine_get_manager_cond(test_engine), lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 0, test_engine_get_worker_count(test_engine));
    ASSERT_IS_NULL(test_engine_get_idle_head(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_074: [ While the thread count is above min_thread_count and the least recently idle worker has been idle for at least 60000 ms, remove it from the idle list, mark it retired, wake it by calling pthread_cond_signal and decrement the thread count. ]*/
/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_079: [ Otherwise wait for the earliest of the next timer due time, the next time a worker may need to be created and, while the thread count is above min_thread_count, the next time an idle worker may need to be retired (at most 60000 ms) by calling pthread_cond_timedwait, or by calling pthread_cond_wait if there is no such time. ]*/
TEST_FUNCTION(manager_thread_func_does_not_retire_a_worker_idle_for_less_than_60000_ms_and_waits_until_it_may_need_to_be_retired)
{
    // arrange
    test_create_execution_engine_with_idle_manager_created_worker(TEST_START_TIME_MS);
    void* worker = test_threads[1].arg;
    set_now_ms(TEST_START_TIME_MS + 59999);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(test_engine_get_manager_cond(test_engine), lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 60000, test_wait_deadlines_ms[0]);
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_worker_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, worker, test_engine_get_idle_head(test_engine));

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_074: [ While the thread count is above min_thread_count and the least recently idle worker has been idle for at least 60000 ms, remove it from the idle list, mark it retired, wake it by calling pthread_cond_signal and decrement the thread count. ]*/
TEST_FUNCTION(manager_thread_func_does_not_retire_a_worker_whose_idle_time_is_later_than_the_current_time)
{
    // arrange
    test_create_execution_engine_with_idle_manager_created_worker(TEST_START_TIME_MS + 100);
    void* worker = test_threads[1].arg;
    set_now_ms(TEST_START_TIME_MS + 50);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(test_engine_get_manager_cond(test_engine), lock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(0);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_worker_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, worker, test_engine_get_idle_head(test_engine));
    ASSERT_ARE_EQUAL(uint64_t, TEST_START_TIME_MS + 100 + 60000, test_wait_deadlines_ms[0]);

    // cleanup
    execution_engine_linux_close_work(test_work);
    execution_engine_dec_ref(test_engine);
}

/* Tests_SRS_EXECUTION_ENGINE_LINUX_42_074: [ While the thread count is above min_thread_count and the least recently idle worker has been idle for at least 60000 ms, remove it from the idle list, mark it retired, wake it by calling pthread_cond_signal and decrement the thread count. ]*/
TEST_FUNCTION(manager_thread_func_does_not_retire_idle_workers_at_min_thread_count)
{
    // arrange
    test_engine = test_create_execution_engine(1, 4);
    void* worker = test_threads[0].arg;
    test_worker_park(worker, TEST_START_TIME_MS);
    set_now_ms(TEST_START_TIME_MS + 120000);
    add_wait_action(action_stop_engine);
    pthread_mutex_t* lock = test_engine_get_lock(test_engine);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(lock));
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_MONOTONIC, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_engine_get_manager_cond(test_engine), lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(lock));

    // act
    int result = run_test_thread(1);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, test_engine_get_worker_count(test_engine));
    ASSERT_ARE_EQUAL(void_ptr, worker, test_engine_get_idle_head(test_engine));

    // cleanup
    execution_engine_dec_ref(test_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

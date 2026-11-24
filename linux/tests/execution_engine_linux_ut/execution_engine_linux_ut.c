// Copyright (C) Microsoft Corporation. All rights reserved.

#include "execution_engine_linux_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);

#define TEST_THREAD_COUNT_POLICY ((THREAD_COUNT_POLICY_HANDLE)0x4201)
#define TEST_WORKER_POOL ((WORKER_POOL_HANDLE)0x4202)
#define TEST_MANAGER_THREAD ((pthread_t)0x4203)
#define TEST_MANAGER_THREAD_STACK_SIZE (1024 * 1024)

typedef void* (*PTHREAD_START_ROUTINE)(void*);

typedef struct TEST_WORK_CONTEXT_TAG TEST_WORK_CONTEXT;

typedef void (*TEST_ACTION)(TEST_WORK_CONTEXT* test_context);

// the context of the work functions, a work function runs the action of its context
struct TEST_WORK_CONTEXT_TAG
{
    TEST_ACTION action;
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE other_work;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer;
};

// captured when the execution engine is created
static void* g_execution_engine_memory;
static pthread_mutex_t* g_lock;
static pthread_cond_t* g_manager_cond;
static pthread_cond_t* g_work_done_cond;
static MIN_HEAP_HANDLE g_timer_heap;
static PDLIST_ENTRY g_ready_queue;
static WORKER_POOL_RUN_FUNCTION g_run_next_work;
static void* g_run_next_work_context;

// the condition variable attributes seen by the mocks
static void* g_condattr_init_attr;
static void* g_condattr_setclock_attr;
static void* g_manager_cond_init_attr;
static void* g_condattr_destroy_attr;

// returned by the hooks of the dependencies, unless an expected call sets another return value
static double g_now_ms;
static WORKER_POOL_COUNTS g_worker_pool_counts;
static uint64_t g_add_thread_time;
static uint64_t g_retire_thread_time;

// the deadline passed to the last pthread_cond_timedwait
static struct timespec g_timedwait_deadline;

// what another thread does while the test thread waits on a condition variable, which would never return otherwise
static TEST_ACTION g_cond_wait_action;
static TEST_WORK_CONTEXT* g_cond_wait_action_context;

// The manager thread does not run on a real thread. It runs on its own stack (ucontext) on the test thread:
// waiting on a condition variable switches back to the test, as if the manager thread blocked, and the test
// switches to the manager thread when it should run, as if it was woken and scheduled. Joining the manager
// thread runs it until it exits. This makes the interleaving of the manager thread and the test deterministic.
static PTHREAD_START_ROUTINE g_manager_thread_func;
static void* g_manager_thread_arg;
static ucontext_t g_manager_thread_context;
static ucontext_t g_test_context;
static void* g_manager_thread_stack;
static bool g_is_manager_thread_started;
static bool g_is_manager_thread_running;
static bool g_has_manager_thread_exited;
static void* g_manager_thread_result;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void manager_thread_entry(void)
{
    g_manager_thread_result = g_manager_thread_func(g_manager_thread_arg);
    g_has_manager_thread_exited = true;
}

// runs the manager thread until it waits on a condition variable or exits
static void run_manager_thread(void)
{
    ASSERT_IS_TRUE(g_manager_thread_func != NULL, "the manager thread was not created");
    ASSERT_IS_FALSE(g_is_manager_thread_running, "the manager thread cannot run itself");
    ASSERT_IS_FALSE(g_has_manager_thread_exited, "the manager thread already exited");

    if (g_is_manager_thread_started)
    {
        // the manager thread continues where it waits
    }
    else
    {
        g_manager_thread_stack = real_gballoc_hl_malloc(TEST_MANAGER_THREAD_STACK_SIZE);
        ASSERT_IS_NOT_NULL(g_manager_thread_stack);
        ASSERT_ARE_EQUAL(int, 0, getcontext(&g_manager_thread_context));
        g_manager_thread_context.uc_stack.ss_sp = g_manager_thread_stack;
        g_manager_thread_context.uc_stack.ss_size = TEST_MANAGER_THREAD_STACK_SIZE;
        g_manager_thread_context.uc_link = &g_test_context;
        makecontext(&g_manager_thread_context, manager_thread_entry, 0);
        g_is_manager_thread_started = true;
    }

    g_is_manager_thread_running = true;
    ASSERT_ARE_EQUAL(int, 0, swapcontext(&g_test_context, &g_manager_thread_context));
    g_is_manager_thread_running = false;
}

static void wait_on_condition_variable(void)
{
    if (g_is_manager_thread_running)
    {
        // the manager thread blocks: switch back to the test until the test runs the manager thread again
        ASSERT_ARE_EQUAL(int, 0, swapcontext(&g_manager_thread_context, &g_test_context));
    }
    else
    {
        // the test thread blocks: the action does what another thread does meanwhile
        TEST_ACTION action = g_cond_wait_action;
        ASSERT_IS_TRUE(action != NULL, "unexpected wait on a condition variable on the test thread");
        g_cond_wait_action = NULL;
        action(g_cond_wait_action_context);
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
    g_condattr_init_attr = attr;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_condattr_setclock, pthread_condattr_t*, attr, clockid_t, clock_id)
    g_condattr_setclock_attr = attr;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_condattr_destroy, pthread_condattr_t*, attr)
    g_condattr_destroy_attr = attr;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_init, pthread_cond_t*, cond, const pthread_condattr_t*, attr)
    if (attr == NULL)
    {
        // the condition variable signaled when work objects have no more pending and running invocations
    }
    else
    {
        g_manager_cond_init_attr = (void*)attr;
    }
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_signal, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_broadcast, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_wait, pthread_cond_t*, cond, pthread_mutex_t*, mutex)
    wait_on_condition_variable();
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_timedwait, pthread_cond_t*, cond, pthread_mutex_t*, mutex, const struct timespec*, abstime)
    g_timedwait_deadline = *abstime;
    wait_on_condition_variable();
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_destroy, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_create, pthread_t*, thread, const pthread_attr_t*, attr, PTHREAD_START_ROUTINE, start_routine, void*, arg)
    // the manager thread does not run until the test runs it
    g_manager_thread_func = start_routine;
    g_manager_thread_arg = arg;
    *thread = TEST_MANAGER_THREAD;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_join, pthread_t, thread, void**, retval)
    // the joined manager thread runs until it exits
    run_manager_thread();
    ASSERT_IS_TRUE(g_has_manager_thread_exited, "the joined manager thread did not exit");
    if (retval == NULL)
    {
        // the result of the manager thread is not needed
    }
    else
    {
        *retval = g_manager_thread_result;
    }
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_detach, pthread_t, thread)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, void, test_work_function, void*, context)
    TEST_WORK_CONTEXT* test_context = context;
    if (test_context->action == NULL)
    {
        // the work function does nothing else
    }
    else
    {
        test_context->action(test_context);
    }
MOCK_FUNCTION_END()

static double hook_timer_global_get_elapsed_ms(void)
{
    return g_now_ms;
}

static void hook_worker_pool_get_counts(WORKER_POOL_HANDLE worker_pool, WORKER_POOL_COUNTS* counts)
{
    (void)worker_pool;
    *counts = g_worker_pool_counts;
}

static uint64_t hook_thread_count_policy_get_add_thread_time(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t queued_work_count, uint32_t starting_thread_count, uint32_t idle_thread_count, uint32_t thread_count)
{
    (void)thread_count_policy;
    (void)queued_work_count;
    (void)starting_thread_count;
    (void)idle_thread_count;
    (void)thread_count;
    return g_add_thread_time;
}

static uint64_t hook_thread_count_policy_get_retire_thread_time(THREAD_COUNT_POLICY_HANDLE thread_count_policy, uint32_t thread_count, uint32_t idle_thread_count, uint64_t oldest_idle_since_ms, uint64_t now_ms)
{
    (void)thread_count_policy;
    (void)thread_count;
    (void)idle_thread_count;
    (void)oldest_idle_since_ms;
    (void)now_ms;
    return g_retire_thread_time;
}

static void action_run_next_work(TEST_WORK_CONTEXT* test_context)
{
    (void)test_context;
    ASSERT_IS_TRUE(g_run_next_work(g_run_next_work_context));
}

static void action_cancel_pending_invocations(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_wait_for_work(test_context->work, true);
}

static void action_close_work(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_close_work(test_context->work);
}

static void action_close_work_and_release_execution_engine(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_close_work(test_context->work);
    execution_engine_dec_ref(test_context->execution_engine);
}

static void action_disassociate_twice_and_wait(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_disassociate_current_thread_from_work(test_context->work);
    execution_engine_linux_disassociate_current_thread_from_work(test_context->work);
    execution_engine_linux_wait_for_work(test_context->work, false);
}

static void action_disassociate_and_close(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_disassociate_current_thread_from_work(test_context->work);
    execution_engine_linux_close_work(test_context->work);
}

static void action_close_and_disassociate(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_close_work(test_context->work);
    execution_engine_linux_disassociate_current_thread_from_work(test_context->work);
}

static void action_disassociate_from_other_work(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_disassociate_current_thread_from_work(test_context->other_work);
}

static void action_disassociate_from_timer(TEST_WORK_CONTEXT* test_context)
{
    execution_engine_linux_disassociate_current_thread_from_timer(test_context->timer);
}

static void setup_create_allocation_expectations(uint32_t min_thread_count, uint32_t max_thread_count)
{
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0))
        .CaptureReturn(&g_execution_engine_memory);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(thread_count_policy_create(min_thread_count, max_thread_count));
    STRICT_EXPECTED_CALL(min_heap_create())
        .CaptureReturn(&g_timer_heap);
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG))
        .CaptureArgumentValue_listHead(&g_ready_queue);
}

static void setup_init_synchronization_expectations(void)
{
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL))
        .CaptureArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_setclock(IGNORED_ARG, CLOCK_MONOTONIC));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_cond(&g_manager_cond);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL))
        .CaptureArgumentValue_cond(&g_work_done_cond);
}

static void setup_start_worker_threads_expectations(uint32_t min_thread_count)
{
    STRICT_EXPECTED_CALL(worker_pool_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_lock(&g_lock)
        .CaptureArgumentValue_run_function(&g_run_next_work)
        .CaptureArgumentValue_run_function_context(&g_run_next_work_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    for (uint32_t i = 0; i < min_thread_count; i++)
    {
        STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL));
    }
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
}

static void setup_start_threads_expectations(uint32_t min_thread_count)
{
    setup_start_worker_threads_expectations(min_thread_count);
    STRICT_EXPECTED_CALL(mocked_pthread_create(IGNORED_ARG, NULL, IGNORED_ARG, IGNORED_ARG));
}

static void setup_destroy_synchronization_expectations(void)
{
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG))
        .ValidateArgumentValue_cond(&g_work_done_cond);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG))
        .ValidateArgumentValue_cond(&g_manager_cond);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
}

static void setup_destroy_allocation_expectations(void)
{
    STRICT_EXPECTED_CALL(min_heap_destroy(IGNORED_ARG))
        .ValidateArgumentValue_min_heap(&g_timer_heap);
    STRICT_EXPECTED_CALL(thread_count_policy_destroy(TEST_THREAD_COUNT_POLICY));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&g_execution_engine_memory);
}

static void setup_teardown_expectations(void)
{
    STRICT_EXPECTED_CALL(worker_pool_destroy(TEST_WORKER_POOL));
    setup_destroy_synchronization_expectations();
    setup_destroy_allocation_expectations();
}

static void setup_release_last_reference_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(g_manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
}

static EXECUTION_ENGINE_HANDLE test_create_execution_engine(uint32_t min_thread_count, uint32_t max_thread_count)
{
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = min_thread_count, .max_thread_count = max_thread_count };
    setup_create_allocation_expectations(min_thread_count, max_thread_count);
    setup_init_synchronization_expectations();
    setup_start_threads_expectations(min_thread_count);
    EXECUTION_ENGINE_HANDLE result = execution_engine_create(&parameters);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return result;
}

static void test_init_work_context(TEST_WORK_CONTEXT* test_context, EXECUTION_ENGINE_HANDLE execution_engine, TEST_ACTION action)
{
    test_context->action = action;
    test_context->execution_engine = execution_engine;
    test_context->work = NULL;
    test_context->other_work = NULL;
    test_context->timer = NULL;
}

static EXECUTION_ENGINE_LINUX_WORK_HANDLE test_create_work(EXECUTION_ENGINE_HANDLE execution_engine, TEST_WORK_CONTEXT* test_context, TEST_ACTION action)
{
    test_init_work_context(test_context, execution_engine, action);
    test_context->work = execution_engine_linux_create_work(execution_engine, test_work_function, test_context);
    ASSERT_IS_NOT_NULL(test_context->work);
    umock_c_reset_all_calls();
    return test_context->work;
}

static EXECUTION_ENGINE_LINUX_TIMER_HANDLE test_create_timer(EXECUTION_ENGINE_HANDLE execution_engine, TEST_WORK_CONTEXT* test_context, TEST_ACTION action, MIN_HEAP_ENTRY** timer_heap_entry)
{
    test_init_work_context(test_context, execution_engine, action);
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(min_heap_entry_init(IGNORED_ARG))
        .CaptureArgumentValue_entry(timer_heap_entry);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_reserve(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    test_context->timer = execution_engine_linux_create_timer(execution_engine, test_work_function, test_context);
    ASSERT_IS_NOT_NULL(test_context->timer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return test_context->timer;
}

static void test_submit_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    execution_engine_linux_submit_work(work);
    umock_c_reset_all_calls();
}

static void test_set_timer(EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer, double now_ms, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    g_now_ms = now_ms;
    execution_engine_linux_set_timer(timer, start_delay_ms, timer_period_ms);
    umock_c_reset_all_calls();
}

static void test_run_next_work(void)
{
    ASSERT_IS_TRUE(g_run_next_work(g_run_next_work_context));
    umock_c_reset_all_calls();
}

static void test_run_manager_thread_until_it_waits(void)
{
    run_manager_thread();
    ASSERT_IS_FALSE(g_has_manager_thread_exited);
    umock_c_reset_all_calls();
}

// arms the timer to be due now and runs the manager thread, which submits the work object of the timer, until it waits
static void test_submit_timer(EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer)
{
    execution_engine_linux_set_timer(timer, 0, 0);
    test_run_manager_thread_until_it_waits();
    ASSERT_ARE_EQUAL(int, 0, real_DList_IsListEmpty(g_ready_queue));
}

// expectations for run_next_work taking the only work object of the ready queue, until it calls the work function
static void setup_run_next_work_start_expectations(TEST_WORK_CONTEXT* test_context)
{
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, (uint64_t)g_now_ms));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(test_work_function(test_context));
}

// expectations for enqueue inserting a work object in the empty ready queue
static void setup_enqueue_in_empty_ready_queue_expectations(void)
{
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, (uint64_t)g_now_ms));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_ready_queue, IGNORED_ARG));
}

// expectations for the manager loop retiring no worker thread, joining the retired worker threads and getting the time to add a worker thread
static void setup_manager_retire_join_and_get_add_thread_time_expectations(uint32_t queued_work_count, uint64_t add_thread_time)
{
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_get_retire_thread_time(TEST_THREAD_COUNT_POLICY, g_worker_pool_counts.thread_count, g_worker_pool_counts.idle_thread_count, g_worker_pool_counts.oldest_idle_since_ms, (uint64_t)g_now_ms))
        .SetReturn(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(worker_pool_join_retired_workers(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_get_add_thread_time(TEST_THREAD_COUNT_POLICY, queued_work_count, g_worker_pool_counts.starting_thread_count, g_worker_pool_counts.idle_thread_count, g_worker_pool_counts.thread_count))
        .SetReturn(add_thread_time);
}

// expectations for wait_for_next_event until it waits
static void setup_wait_for_next_event_expectations(uint64_t retire_thread_time)
{
    STRICT_EXPECTED_CALL(thread_count_policy_get_retire_thread_time(TEST_THREAD_COUNT_POLICY, g_worker_pool_counts.thread_count, g_worker_pool_counts.idle_thread_count, g_worker_pool_counts.oldest_idle_since_ms, (uint64_t)g_now_ms))
        .SetReturn(retire_thread_time);
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
}

// expectations for an iteration of the manager loop with nothing to do, which waits without a deadline
static void setup_manager_idle_iteration_expectations(uint32_t queued_work_count)
{
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(queued_work_count, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(g_manager_cond, g_lock));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_MIN_HEAP_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_RETURN(thread_count_policy_create, TEST_THREAD_COUNT_POLICY);
    REGISTER_GLOBAL_MOCK_RETURN(thread_count_policy_can_add_thread, false);
    REGISTER_GLOBAL_MOCK_HOOK(thread_count_policy_get_add_thread_time, hook_thread_count_policy_get_add_thread_time);
    REGISTER_GLOBAL_MOCK_HOOK(thread_count_policy_get_retire_thread_time, hook_thread_count_policy_get_retire_thread_time);
    REGISTER_GLOBAL_MOCK_RETURN(worker_pool_create, TEST_WORKER_POOL);
    REGISTER_GLOBAL_MOCK_RETURN(worker_pool_add_worker, 0);
    REGISTER_GLOBAL_MOCK_RETURN(worker_pool_wake_idle_worker, WORKER_POOL_WAKE_RESULT_WOKEN);
    REGISTER_GLOBAL_MOCK_HOOK(worker_pool_get_counts, hook_worker_pool_get_counts);
    REGISTER_GLOBAL_MOCK_HOOK(timer_global_get_elapsed_ms, hook_timer_global_get_elapsed_ms);

    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MIN_HEAP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MIN_HEAP_ENTRY*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_COUNT_POLICY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(WORKER_POOL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(WORKER_POOL_RUN_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(WORKER_POOL_COUNTS*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_mutex_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const pthread_mutexattr_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_cond_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_condattr_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const pthread_condattr_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const struct timespec*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(clockid_t, int);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_t, unsigned long);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const pthread_attr_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTHREAD_START_ROUTINE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(void**, void*);

    REGISTER_TYPE(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();

    g_execution_engine_memory = NULL;
    g_lock = NULL;
    g_manager_cond = NULL;
    g_work_done_cond = NULL;
    g_timer_heap = NULL;
    g_ready_queue = NULL;
    g_run_next_work = NULL;
    g_run_next_work_context = NULL;

    g_condattr_init_attr = NULL;
    g_condattr_setclock_attr = NULL;
    g_manager_cond_init_attr = NULL;
    g_condattr_destroy_attr = NULL;

    g_now_ms = 1000.0;
    g_worker_pool_counts = (WORKER_POOL_COUNTS){ 0 };
    g_add_thread_time = THREAD_COUNT_POLICY_NEVER;
    g_retire_thread_time = THREAD_COUNT_POLICY_NEVER;

    g_timedwait_deadline = (struct timespec){ 0 };

    g_cond_wait_action = NULL;
    g_cond_wait_action_context = NULL;

    g_manager_thread_func = NULL;
    g_manager_thread_arg = NULL;
    g_manager_thread_stack = NULL;
    g_is_manager_thread_started = false;
    g_is_manager_thread_running = false;
    g_has_manager_thread_exited = false;
    g_manager_thread_result = NULL;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    if (g_manager_thread_stack == NULL)
    {
        // the manager thread did not run
    }
    else
    {
        real_gballoc_hl_free(g_manager_thread_stack);
        g_manager_thread_stack = NULL;
    }
}

/* execution_engine_create */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_002: [ If execution_engine_parameters is NULL, execution_engine_create shall use the default DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
TEST_FUNCTION(execution_engine_create_with_NULL_parameters_uses_the_default_thread_counts)
{
    ///arrange
    setup_create_allocation_expectations(DEFAULT_MIN_THREAD_COUNT, DEFAULT_MAX_THREAD_COUNT);
    setup_init_synchronization_expectations();
    setup_start_threads_expectations(DEFAULT_MIN_THREAD_COUNT);

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);

    ///assert
    ASSERT_IS_NOT_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    const EXECUTION_ENGINE_PARAMETERS* parameters = execution_engine_linux_get_parameters(execution_engine);
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MIN_THREAD_COUNT, parameters->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, DEFAULT_MAX_THREAD_COUNT, parameters->max_thread_count);

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_003: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_004: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_001: [ execution_engine_create shall create the thread count policy by calling thread_count_policy_create with the minimum and the maximum number of threads. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_002: [ execution_engine_create shall create the timer heap by calling min_heap_create. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_003: [ execution_engine_create shall initialize the ready queue by calling DList_InitializeListHead. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the mutex and the condition variables. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_005: [ execution_engine_create shall start the threads. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_006: [ init_synchronization shall initialize the mutex by calling pthread_mutex_init. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_007: [ init_synchronization shall initialize the condition variable that wakes the manager thread. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_008: [ init_manager_cond shall initialize a condition variable attribute by calling pthread_condattr_init. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_009: [ init_manager_cond shall set the clock of the attribute to CLOCK_MONOTONIC by calling pthread_condattr_setclock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_010: [ init_manager_cond shall initialize the condition variable that wakes the manager thread with the attribute by calling pthread_cond_init. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_175: [ init_manager_cond shall destroy the attribute by calling pthread_condattr_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_011: [ init_synchronization shall initialize the condition variable signaled when work objects have no more pending and running invocations by calling pthread_cond_init. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_013: [ start_threads shall create the worker pool by calling worker_pool_create with the mutex, run_next_work and the execution engine. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_014: [ start_threads shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_015: [ start_threads shall add min_thread_count worker threads by calling worker_pool_add_worker. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_016: [ start_threads shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_017: [ start_threads shall start the manager thread by calling pthread_create with manager_thread_func and the execution engine. ]*/
TEST_FUNCTION(execution_engine_create_succeeds)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    setup_init_synchronization_expectations();
    setup_start_threads_expectations(2);

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NOT_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    const EXECUTION_ENGINE_PARAMETERS* result_parameters = execution_engine_linux_get_parameters(execution_engine);
    ASSERT_ARE_EQUAL(uint32_t, 2, result_parameters->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 10, result_parameters->max_thread_count);
    // the condition variable that wakes the manager thread is initialized with the attribute that uses CLOCK_MONOTONIC
    ASSERT_IS_NOT_NULL(g_condattr_init_attr);
    ASSERT_ARE_EQUAL(void_ptr, g_condattr_init_attr, g_condattr_setclock_attr);
    ASSERT_ARE_EQUAL(void_ptr, g_condattr_init_attr, g_manager_cond_init_attr);
    ASSERT_ARE_EQUAL(void_ptr, g_condattr_init_attr, g_condattr_destroy_attr);
    ASSERT_ARE_NOT_EQUAL(void_ptr, g_manager_cond, g_work_done_cond);
    // the worker pool and the manager thread get the execution engine
    ASSERT_ARE_EQUAL(void_ptr, execution_engine, g_run_next_work_context);
    ASSERT_ARE_EQUAL(void_ptr, execution_engine, g_manager_thread_arg);
    ASSERT_IS_FALSE(g_is_manager_thread_started);

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_004: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_005: [ If max_thread_count is non-zero and less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_with_max_thread_count_0_succeeds)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 3, .max_thread_count = 0 };
    setup_create_allocation_expectations(3, 0);
    setup_init_synchronization_expectations();
    setup_start_threads_expectations(3);

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NOT_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_015: [ start_threads shall add min_thread_count worker threads by calling worker_pool_add_worker. ]*/
TEST_FUNCTION(execution_engine_create_with_min_thread_count_0_adds_no_worker_thread)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 0, .max_thread_count = 4 };
    setup_create_allocation_expectations(0, 4);
    setup_init_synchronization_expectations();
    setup_start_threads_expectations(0);

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NOT_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_005: [ If max_thread_count is non-zero and less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_with_max_thread_count_less_than_min_thread_count_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 5, .max_thread_count = 4 };

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_allocating_the_execution_engine_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0))
        .SetReturn(NULL);

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_thread_count_policy_create_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0))
        .CaptureReturn(&g_execution_engine_memory);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(thread_count_policy_create(2, 10))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&g_execution_engine_memory);

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_min_heap_create_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 0))
        .CaptureReturn(&g_execution_engine_memory);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(thread_count_policy_create(2, 10));
    STRICT_EXPECTED_CALL(min_heap_create())
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(thread_count_policy_destroy(TEST_THREAD_COUNT_POLICY));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&g_execution_engine_memory);

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, init_synchronization shall destroy what it initialized and fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_pthread_mutex_init_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL))
        .SetReturn(1);
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, init_synchronization shall destroy what it initialized and fail. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_176: [ If any error occurs, init_manager_cond shall fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_pthread_condattr_init_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL))
        .CaptureArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, init_synchronization shall destroy what it initialized and fail. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_175: [ init_manager_cond shall destroy the attribute by calling pthread_condattr_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_176: [ If any error occurs, init_manager_cond shall fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_pthread_condattr_setclock_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL))
        .CaptureArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_setclock(IGNORED_ARG, CLOCK_MONOTONIC))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, g_condattr_init_attr, g_condattr_destroy_attr);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, init_synchronization shall destroy what it initialized and fail. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_175: [ init_manager_cond shall destroy the attribute by calling pthread_condattr_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_176: [ If any error occurs, init_manager_cond shall fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_initializing_the_condition_variable_that_wakes_the_manager_thread_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL))
        .CaptureArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_setclock(IGNORED_ARG, CLOCK_MONOTONIC));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, init_synchronization shall destroy what it initialized and fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_initializing_the_condition_variable_signaled_when_work_objects_complete_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_init(IGNORED_ARG, NULL))
        .CaptureArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_setclock(IGNORED_ARG, CLOCK_MONOTONIC));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_cond(&g_manager_cond);
    STRICT_EXPECTED_CALL(mocked_pthread_condattr_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG))
        .ValidateArgumentValue_cond(&g_manager_cond);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_destroy(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If any error occurs, start_threads shall fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_worker_pool_create_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    setup_init_synchronization_expectations();
    STRICT_EXPECTED_CALL(worker_pool_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL);
    setup_destroy_synchronization_expectations();
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_018: [ If any error occurs after the worker pool was created, start_threads shall stop the worker threads by calling worker_pool_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If any error occurs, start_threads shall fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_adding_the_first_worker_thread_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    setup_init_synchronization_expectations();
    STRICT_EXPECTED_CALL(worker_pool_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_lock(&g_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(worker_pool_destroy(TEST_WORKER_POOL));
    setup_destroy_synchronization_expectations();
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_018: [ If any error occurs after the worker pool was created, start_threads shall stop the worker threads by calling worker_pool_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If any error occurs, start_threads shall fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_adding_the_last_worker_thread_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 3, .max_thread_count = 10 };
    setup_create_allocation_expectations(3, 10);
    setup_init_synchronization_expectations();
    STRICT_EXPECTED_CALL(worker_pool_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_lock(&g_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(IGNORED_ARG))
        .ValidateArgumentValue_mutex(&g_lock);
    STRICT_EXPECTED_CALL(worker_pool_destroy(TEST_WORKER_POOL));
    setup_destroy_synchronization_expectations();
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_018: [ If any error occurs after the worker pool was created, start_threads shall stop the worker threads by calling worker_pool_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If any error occurs, start_threads shall fail. ]*/
TEST_FUNCTION(execution_engine_create_fails_when_starting_the_manager_thread_fails)
{
    ///arrange
    EXECUTION_ENGINE_PARAMETERS parameters = { .min_thread_count = 2, .max_thread_count = 10 };
    setup_create_allocation_expectations(2, 10);
    setup_init_synchronization_expectations();
    setup_start_worker_threads_expectations(2);
    STRICT_EXPECTED_CALL(mocked_pthread_create(IGNORED_ARG, NULL, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(worker_pool_destroy(TEST_WORKER_POOL));
    setup_destroy_synchronization_expectations();
    setup_destroy_allocation_expectations();

    ///act
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&parameters);

    ///assert
    ASSERT_IS_NULL(execution_engine);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* execution_engine_dec_ref */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_007: [ If execution_engine is NULL, execution_engine_dec_ref shall return. ]*/
TEST_FUNCTION(execution_engine_dec_ref_with_NULL_execution_engine_returns)
{
    ///arrange

    ///act
    execution_engine_dec_ref(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_008: [ Otherwise execution_engine_dec_ref shall decrement the refcount. ]*/
TEST_FUNCTION(execution_engine_dec_ref_does_not_stop_the_execution_engine_while_references_remain)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    execution_engine_inc_ref(execution_engine);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    ///act
    execution_engine_dec_ref(execution_engine);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_008: [ Otherwise execution_engine_dec_ref shall decrement the refcount. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_020: [ If the refcount reaches 0: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_021: [ execution_engine_dec_ref shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_022: [ execution_engine_dec_ref shall mark the execution engine as stopping. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_024: [ execution_engine_dec_ref shall wake the manager thread by calling pthread_cond_signal. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_025: [ execution_engine_dec_ref shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_026: [ If the current thread is not running a work function of execution_engine: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_027: [ execution_engine_dec_ref shall wait for the manager thread to exit by calling pthread_join. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_028: [ execution_engine_dec_ref shall tear down the execution engine. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_029: [ teardown shall stop the worker threads and wait for them to exit by calling worker_pool_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_030: [ teardown shall destroy the condition variable signaled when work objects have no more pending and running invocations by calling pthread_cond_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_031: [ teardown shall destroy the condition variable that wakes the manager thread by calling pthread_cond_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_032: [ teardown shall destroy the mutex by calling pthread_mutex_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_033: [ teardown shall destroy the timer heap by calling min_heap_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_034: [ teardown shall destroy the thread count policy by calling thread_count_policy_destroy. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_035: [ teardown shall free the execution engine. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_141: [ Until the execution engine is stopping: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_150: [ manager_thread_func shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_154: [ manager_thread_func shall return NULL. ]*/
TEST_FUNCTION(execution_engine_dec_ref_with_the_last_reference_stops_the_manager_thread_and_tears_down_the_execution_engine)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    test_run_manager_thread_until_it_waits();

    setup_release_last_reference_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_join(TEST_MANAGER_THREAD, NULL));
    // the manager thread wakes up, sees the execution engine stopping and exits
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    setup_teardown_expectations();

    ///act
    execution_engine_dec_ref(execution_engine);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(g_has_manager_thread_exited);
    ASSERT_IS_NULL(g_manager_thread_result);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_020: [ If the refcount reaches 0: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_027: [ execution_engine_dec_ref shall wait for the manager thread to exit by calling pthread_join. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_028: [ execution_engine_dec_ref shall tear down the execution engine. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_140: [ manager_thread_func shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_141: [ Until the execution engine is stopping: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_150: [ manager_thread_func shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_154: [ manager_thread_func shall return NULL. ]*/
TEST_FUNCTION(execution_engine_dec_ref_with_the_last_reference_before_the_manager_thread_runs_stops_the_manager_thread)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);

    setup_release_last_reference_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_join(TEST_MANAGER_THREAD, NULL));
    // the manager thread starts, sees the execution engine stopping and exits
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    setup_teardown_expectations();

    ///act
    execution_engine_dec_ref(execution_engine);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(g_has_manager_thread_exited);
    ASSERT_IS_NULL(g_manager_thread_result);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_020: [ If the refcount reaches 0: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_022: [ execution_engine_dec_ref shall mark the execution engine as stopping. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_023: [ If the current thread is running a work function of execution_engine, execution_engine_dec_ref shall mark the teardown as deferred to the manager thread. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_024: [ execution_engine_dec_ref shall wake the manager thread by calling pthread_cond_signal. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_025: [ execution_engine_dec_ref shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_026: [ If the current thread is not running a work function of execution_engine: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_069: [ run_next_work shall record the execution engine as the execution engine running a work function on the current thread. ]*/
TEST_FUNCTION(execution_engine_dec_ref_with_the_last_reference_from_a_work_function_defers_the_teardown_to_the_manager_thread)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, action_close_work_and_release_execution_engine);
    test_submit_work(work);

    setup_run_next_work_start_expectations(&test_context);
    // the work function closes its work object
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    // the work function releases the last reference: no pthread_join and no teardown
    setup_release_last_reference_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(g_is_manager_thread_started);

    ///cleanup
    // the manager thread tears down the execution engine
    run_manager_thread();
    ASSERT_IS_TRUE(g_has_manager_thread_exited);
}

/* execution_engine_inc_ref */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_010: [ If execution_engine is NULL then execution_engine_inc_ref shall return. ]*/
TEST_FUNCTION(execution_engine_inc_ref_with_NULL_execution_engine_returns)
{
    ///arrange

    ///act
    execution_engine_inc_ref(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_011: [ Otherwise execution_engine_inc_ref shall increment the reference count for execution_engine. ]*/
TEST_FUNCTION(execution_engine_inc_ref_increments_the_reference_count)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    ///act
    execution_engine_inc_ref(execution_engine);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_get_parameters */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_012: [ If execution_engine is NULL, execution_engine_linux_get_parameters shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_get_parameters_with_NULL_execution_engine_fails)
{
    ///arrange

    ///act
    const EXECUTION_ENGINE_PARAMETERS* parameters = execution_engine_linux_get_parameters(NULL);

    ///assert
    ASSERT_IS_NULL(parameters);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_07_013: [ Otherwise, execution_engine_linux_get_parameters shall return the parameters in EXECUTION_ENGINE. ]*/
TEST_FUNCTION(execution_engine_linux_get_parameters_returns_the_parameters)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(3, 7);

    ///act
    const EXECUTION_ENGINE_PARAMETERS* parameters = execution_engine_linux_get_parameters(execution_engine);

    ///assert
    ASSERT_IS_NOT_NULL(parameters);
    ASSERT_ARE_EQUAL(uint32_t, 3, parameters->min_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 7, parameters->max_thread_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_create_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_036: [ If execution_engine is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_work_with_NULL_execution_engine_fails)
{
    ///arrange
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, NULL, NULL);

    ///act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = execution_engine_linux_create_work(NULL, test_work_function, &test_context);

    ///assert
    ASSERT_IS_NULL(work);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_037: [ If work_function is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_work_with_NULL_work_function_fails)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, execution_engine, NULL);

    ///act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = execution_engine_linux_create_work(execution_engine, NULL, &test_context);

    ///assert
    ASSERT_IS_NULL(work);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_038: [ execution_engine_linux_create_work shall allocate a work object. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_041: [ If any error occurs, execution_engine_linux_create_work shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_work_fails_when_allocating_the_work_object_fails)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, execution_engine, NULL);
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = execution_engine_linux_create_work(execution_engine, test_work_function, &test_context);

    ///assert
    ASSERT_IS_NULL(work);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_038: [ execution_engine_linux_create_work shall allocate a work object. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_040: [ execution_engine_linux_create_work shall succeed and return the work object. ]*/
TEST_FUNCTION(execution_engine_linux_create_work_succeeds)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, execution_engine, NULL);
    void* allocated_work;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&allocated_work);

    ///act
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = execution_engine_linux_create_work(execution_engine, test_work_function, &test_context);

    ///assert
    ASSERT_IS_NOT_NULL(work);
    ASSERT_ARE_EQUAL(void_ptr, allocated_work, work);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/* init_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_042: [ init_work shall store execution_engine, work_function, work_function_context and free_function in work. ]*/
TEST_FUNCTION(init_work_stores_the_work_function_and_its_context)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);

    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_043: [ init_work shall set the pending count, the running count and the waiter count of work to 0. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_039: [ execution_engine_linux_create_work shall initialize the work object with free_work as its free function. ]*/
TEST_FUNCTION(init_work_sets_no_pending_and_no_running_invocations)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);

    // closing frees the work object at once because it has no invocations
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_close_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_043: [ init_work shall set the pending count, the running count and the waiter count of work to 0. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_044: [ init_work shall mark work as not closed. ]*/
TEST_FUNCTION(init_work_marks_the_work_object_as_not_closed_with_no_waiter)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);

    // the end of the last invocation neither frees the work object nor wakes waiting threads
    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/* free_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_045: [ free_work shall free work. ]*/
TEST_FUNCTION(free_work_frees_the_work_object)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    test_run_next_work();

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_close_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_submit_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_046: [ If work is NULL, execution_engine_linux_submit_work shall return. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_with_NULL_work_returns)
{
    ///arrange

    ///act
    execution_engine_linux_submit_work(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_047: [ execution_engine_linux_submit_work shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_048: [ execution_engine_linux_submit_work shall submit work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_049: [ execution_engine_linux_submit_work shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_051: [ submit_work_locked shall increment the pending count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_052: [ If the pending count of work became 1: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_053: [ submit_work_locked shall insert work in the ready queue. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_054: [ submit_work_locked shall dispatch the ready queue. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_055: [ If the ready queue is empty, enqueue shall record progress by calling thread_count_policy_on_progress with the current time obtained by calling timer_global_get_elapsed_ms. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_056: [ enqueue shall insert work at the tail of the ready queue by calling DList_InsertTailList. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_057: [ dispatch shall wake an idle worker thread by calling worker_pool_wake_idle_worker. ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_inserts_the_work_object_in_the_ready_queue_and_wakes_an_idle_worker_thread)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    g_now_ms = 1234.5;
    PDLIST_ENTRY work_entry;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, 1234));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_ready_queue, IGNORED_ARG))
        .CaptureArgumentValue_listEntry(&work_entry);
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL))
        .SetReturn(WORKER_POOL_WAKE_RESULT_WOKEN);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, work_entry, g_ready_queue->Flink);
    ASSERT_ARE_EQUAL(void_ptr, work_entry, g_ready_queue->Blink);

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_051: [ submit_work_locked shall increment the pending count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_052: [ If the pending count of work became 1: ]*/
TEST_FUNCTION(execution_engine_linux_submit_work_of_a_pending_work_object_only_increments_its_pending_count)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/* enqueue */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_055: [ If the ready queue is empty, enqueue shall record progress by calling thread_count_policy_on_progress with the current time obtained by calling timer_global_get_elapsed_ms. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_056: [ enqueue shall insert work at the tail of the ready queue by calling DList_InsertTailList. ]*/
TEST_FUNCTION(enqueue_inserts_at_the_tail_of_a_non_empty_ready_queue_without_recording_progress)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context_1;
    TEST_WORK_CONTEXT test_context_2;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_1 = test_create_work(execution_engine, &test_context_1, NULL);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_2 = test_create_work(execution_engine, &test_context_2, NULL);
    test_submit_work(work_1);
    PDLIST_ENTRY work_1_entry = g_ready_queue->Flink;
    PDLIST_ENTRY work_2_entry;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_ready_queue, IGNORED_ARG))
        .CaptureArgumentValue_listEntry(&work_2_entry);
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work_2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(void_ptr, work_1_entry, work_2_entry);
    ASSERT_ARE_EQUAL(void_ptr, work_1_entry, g_ready_queue->Flink);
    ASSERT_ARE_EQUAL(void_ptr, work_2_entry, g_ready_queue->Blink);

    ///cleanup
    execution_engine_linux_wait_for_work(work_1, true);
    execution_engine_linux_wait_for_work(work_2, true);
    execution_engine_linux_close_work(work_1);
    execution_engine_linux_close_work(work_2);
    execution_engine_dec_ref(execution_engine);
}

/* dispatch */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_057: [ dispatch shall wake an idle worker thread by calling worker_pool_wake_idle_worker. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_058: [ If worker_pool_wake_idle_worker returns WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER and the manager thread is not watching the ready queue: ]*/
TEST_FUNCTION(dispatch_does_not_wake_the_manager_thread_when_an_idle_worker_thread_is_already_waking)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL))
        .SetReturn(WORKER_POOL_WAKE_RESULT_ALREADY_WAKING);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_058: [ If worker_pool_wake_idle_worker returns WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER and the manager thread is not watching the ready queue: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_059: [ dispatch shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_060: [ If thread_count_policy_can_add_thread returns true for the thread count, dispatch shall wake the manager thread by calling pthread_cond_signal. ]*/
TEST_FUNCTION(dispatch_wakes_the_manager_thread_when_no_worker_thread_is_idle_and_one_can_be_added)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    g_worker_pool_counts = (WORKER_POOL_COUNTS){ .thread_count = 3, .starting_thread_count = 0, .idle_thread_count = 0, .oldest_idle_since_ms = 0 };

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL))
        .SetReturn(WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER);
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_can_add_thread(TEST_THREAD_COUNT_POLICY, 3))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(g_manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_059: [ dispatch shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_060: [ If thread_count_policy_can_add_thread returns true for the thread count, dispatch shall wake the manager thread by calling pthread_cond_signal. ]*/
TEST_FUNCTION(dispatch_does_not_wake_the_manager_thread_when_no_worker_thread_can_be_added)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    g_worker_pool_counts = (WORKER_POOL_COUNTS){ .thread_count = 4, .starting_thread_count = 0, .idle_thread_count = 0, .oldest_idle_since_ms = 0 };

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL))
        .SetReturn(WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER);
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_can_add_thread(TEST_THREAD_COUNT_POLICY, 4))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_058: [ If worker_pool_wake_idle_worker returns WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER and the manager thread is not watching the ready queue: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_169: [ wait_for_next_event shall record that the manager thread is watching the ready queue if add_thread_time is not THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(dispatch_does_not_wake_the_manager_thread_while_it_watches_the_ready_queue)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    g_now_ms = 5000.0;
    // the manager thread waits until the time to add a worker thread, watching the ready queue
    g_add_thread_time = 5050;
    test_run_manager_thread_until_it_waits();

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL))
        .SetReturn(WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_058: [ If worker_pool_wake_idle_worker returns WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER and the manager thread is not watching the ready queue: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_169: [ wait_for_next_event shall record that the manager thread is watching the ready queue if add_thread_time is not THREAD_COUNT_POLICY_NEVER. ]*/
TEST_FUNCTION(dispatch_wakes_the_manager_thread_once_it_stopped_watching_the_ready_queue)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    g_now_ms = 5000.0;
    // the manager thread first watches the ready queue, then waits without watching it
    g_add_thread_time = 5050;
    test_run_manager_thread_until_it_waits();
    g_add_thread_time = THREAD_COUNT_POLICY_NEVER;
    test_run_manager_thread_until_it_waits();

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL))
        .SetReturn(WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER);
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_can_add_thread(TEST_THREAD_COUNT_POLICY, 0))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(g_manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_submit_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/* run_next_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_061: [ If the ready queue is empty, run_next_work shall return false. ]*/
TEST_FUNCTION(run_next_work_returns_false_when_the_ready_queue_is_empty)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_062: [ If the ready queue is not empty: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_063: [ run_next_work shall remove the work object at the head of the ready queue by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_064: [ run_next_work shall decrement the pending count of the work object. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_066: [ If the ready queue is not empty, run_next_work shall dispatch the ready queue. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_067: [ run_next_work shall increment the running count of the work object. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_068: [ run_next_work shall record progress by calling thread_count_policy_on_progress with the current time obtained by calling timer_global_get_elapsed_ms. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_071: [ run_next_work shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_072: [ run_next_work shall call the work function of the work object with its work function context. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_073: [ run_next_work shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_075: [ If the work object is still the work object running on the current thread: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_077: [ run_next_work shall end the invocation of the work object. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_079: [ run_next_work shall return true. ]*/
TEST_FUNCTION(run_next_work_runs_the_work_object_at_the_head_of_the_ready_queue)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    g_now_ms = 2000.7;

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, 2000));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, real_DList_IsListEmpty(g_ready_queue));

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_064: [ run_next_work shall decrement the pending count of the work object. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_065: [ If the pending count of the work object is not 0, run_next_work shall insert the work object at the tail of the ready queue by calling DList_InsertTailList. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_066: [ If the ready queue is not empty, run_next_work shall dispatch the ready queue. ]*/
TEST_FUNCTION(run_next_work_inserts_the_work_object_at_the_tail_of_the_ready_queue_when_invocations_are_still_pending)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context_1;
    TEST_WORK_CONTEXT test_context_2;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_1 = test_create_work(execution_engine, &test_context_1, NULL);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_2 = test_create_work(execution_engine, &test_context_2, NULL);
    test_submit_work(work_1);
    PDLIST_ENTRY work_1_entry = g_ready_queue->Flink;
    test_submit_work(work_2);
    PDLIST_ENTRY work_2_entry = g_ready_queue->Blink;
    test_submit_work(work_1);
    g_now_ms = 2000.0;

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_ready_queue, work_1_entry));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, 2000));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_context_1));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, work_2_entry, g_ready_queue->Flink);
    ASSERT_ARE_EQUAL(void_ptr, work_1_entry, g_ready_queue->Blink);

    ///cleanup
    execution_engine_linux_wait_for_work(work_1, true);
    execution_engine_linux_wait_for_work(work_2, true);
    execution_engine_linux_close_work(work_1);
    execution_engine_linux_close_work(work_2);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_066: [ If the ready queue is not empty, run_next_work shall dispatch the ready queue. ]*/
TEST_FUNCTION(run_next_work_dispatches_the_ready_queue_when_other_work_objects_wait)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context_1;
    TEST_WORK_CONTEXT test_context_2;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_1 = test_create_work(execution_engine, &test_context_1, NULL);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_2 = test_create_work(execution_engine, &test_context_2, NULL);
    test_submit_work(work_1);
    test_submit_work(work_2);
    PDLIST_ENTRY work_2_entry = g_ready_queue->Blink;
    g_now_ms = 2000.0;

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, 2000));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_context_1));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, work_2_entry, g_ready_queue->Flink);
    ASSERT_ARE_EQUAL(void_ptr, work_2_entry, g_ready_queue->Blink);

    ///cleanup
    execution_engine_linux_wait_for_work(work_2, true);
    execution_engine_linux_close_work(work_1);
    execution_engine_linux_close_work(work_2);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_074: [ run_next_work shall clear the execution engine running a work function on the current thread. ]*/
TEST_FUNCTION(run_next_work_clears_the_execution_engine_running_a_work_function_on_the_current_thread)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    test_run_next_work();
    execution_engine_linux_close_work(work);
    umock_c_reset_all_calls();

    // releasing the last reference after the work function returned joins the manager thread and tears down the execution engine
    setup_release_last_reference_expectations();
    STRICT_EXPECTED_CALL(mocked_pthread_join(TEST_MANAGER_THREAD, NULL));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    setup_teardown_expectations();

    ///act
    execution_engine_dec_ref(execution_engine);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(g_has_manager_thread_exited);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_076: [ run_next_work shall clear the work object running on the current thread. ]*/
TEST_FUNCTION(run_next_work_clears_the_work_object_running_on_the_current_thread)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    test_run_next_work();

    ///act
    execution_engine_linux_disassociate_current_thread_from_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_078: [ If the current thread was disassociated from the work object, run_next_work shall not access the work object, which can be freed by then. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_079: [ run_next_work shall return true. ]*/
TEST_FUNCTION(run_next_work_does_not_access_the_work_object_after_the_current_thread_was_disassociated_from_it)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, action_disassociate_and_close);
    test_submit_work(work);

    setup_run_next_work_start_expectations(&test_context);
    // the work function disassociates the current thread from its work object
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    // the work function closes its work object, which has no more invocations and is freed
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* end_invocation */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_080: [ end_invocation shall decrement the running count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_081: [ If work has no pending and no running invocations: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_082: [ If work is closed, end_invocation shall call the free function of work. ]*/
TEST_FUNCTION(end_invocation_frees_a_closed_work_object_after_its_last_invocation)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    execution_engine_linux_close_work(work);
    umock_c_reset_all_calls();

    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_080: [ end_invocation shall decrement the running count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_081: [ If work has no pending and no running invocations: ]*/
TEST_FUNCTION(end_invocation_does_not_free_a_closed_work_object_with_pending_invocations)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    test_submit_work(work);
    execution_engine_linux_close_work(work);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_ready_queue, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, (uint64_t)g_now_ms));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(test_work_function(&test_context));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    // the last invocation frees the work object
    test_run_next_work();
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_wait_for_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_084: [ If work is NULL, execution_engine_linux_wait_for_work shall return. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_with_NULL_work_returns)
{
    ///arrange

    ///act
    execution_engine_linux_wait_for_work(NULL, false);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_085: [ execution_engine_linux_wait_for_work shall wait for the invocations of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_086: [ wait_for_invocations shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_090: [ wait_for_invocations shall increment the waiter count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_091: [ wait_for_invocations shall wait by calling pthread_cond_wait until work has no pending and no running invocations. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_092: [ wait_for_invocations shall decrement the waiter count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_093: [ wait_for_invocations shall release the mutex by calling pthread_mutex_unlock. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_returns_at_once_when_the_work_object_has_no_invocations)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_work(work, false);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_085: [ execution_engine_linux_wait_for_work shall wait for the invocations of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_086: [ wait_for_invocations shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_087: [ If cancel_pending is true and the pending count of work is not 0: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_090: [ wait_for_invocations shall increment the waiter count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_091: [ wait_for_invocations shall wait by calling pthread_cond_wait until work has no pending and no running invocations. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_092: [ wait_for_invocations shall decrement the waiter count of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_093: [ wait_for_invocations shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_083: [ If work is not closed and the waiter count of work is not 0, end_invocation shall wake the waiting threads by calling pthread_cond_broadcast. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_waits_until_the_invocations_of_the_work_object_complete)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    // while the test thread waits, a worker thread runs the work object
    g_cond_wait_action = action_run_next_work;
    g_cond_wait_action_context = &test_context;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(g_work_done_cond, g_lock));
    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_broadcast(g_work_done_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_work(work, false);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_087: [ If cancel_pending is true and the pending count of work is not 0: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_088: [ wait_for_invocations shall remove work from the ready queue by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_089: [ wait_for_invocations shall set the pending count of work to 0. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_177: [ If work has no running invocations and the waiter count of work is not 0, wait_for_invocations shall wake the threads waiting for work by calling pthread_cond_broadcast. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_with_cancel_pending_removes_the_work_object_from_the_ready_queue)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    PDLIST_ENTRY work_entry = g_ready_queue->Flink;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(work_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_work(work, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, real_DList_IsListEmpty(g_ready_queue));

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_089: [ wait_for_invocations shall set the pending count of work to 0. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_with_cancel_pending_drops_all_the_pending_invocations)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    test_submit_work(work);
    test_submit_work(work);
    PDLIST_ENTRY work_entry = g_ready_queue->Flink;

    // no pending invocation is left to wait for
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(work_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_work(work, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_087: [ If cancel_pending is true and the pending count of work is not 0: ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_with_cancel_pending_and_no_pending_invocation_does_not_touch_the_ready_queue)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_work(work, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_177: [ If work has no running invocations and the waiter count of work is not 0, wait_for_invocations shall wake the threads waiting for work by calling pthread_cond_broadcast. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_work_with_cancel_pending_wakes_the_threads_waiting_for_the_work_object)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);
    PDLIST_ENTRY work_entry = g_ready_queue->Flink;
    // while the test thread waits for the work object, another thread cancels its pending invocations
    g_cond_wait_action = action_cancel_pending_invocations;
    g_cond_wait_action_context = &test_context;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(g_work_done_cond, g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(work_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_broadcast(g_work_done_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_work(work, false);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_close_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_094: [ If work is NULL, execution_engine_linux_close_work shall return. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_with_NULL_work_returns)
{
    ///arrange

    ///act
    execution_engine_linux_close_work(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_095: [ execution_engine_linux_close_work shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_096: [ execution_engine_linux_close_work shall close work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_097: [ execution_engine_linux_close_work shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_098: [ If work has no pending and no running invocations, close_work_locked shall call the free function of work. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_frees_a_work_object_without_invocations)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_close_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_096: [ execution_engine_linux_close_work shall close work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_099: [ If work has pending or running invocations, close_work_locked shall mark work as closed. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_with_pending_invocations_does_not_free_the_work_object)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_close_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    // the last invocation frees the work object
    test_run_next_work();
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_099: [ If work has pending or running invocations, close_work_locked shall mark work as closed. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_082: [ If work is closed, end_invocation shall call the free function of work. ]*/
TEST_FUNCTION(execution_engine_linux_close_work_from_its_work_function_frees_the_work_object_when_the_work_function_returns)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, action_close_work);
    test_submit_work(work);

    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_disassociate_current_thread_from_work */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_100: [ If work is NULL, execution_engine_linux_disassociate_current_thread_from_work shall return. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_work_with_NULL_work_returns)
{
    ///arrange

    ///act
    execution_engine_linux_disassociate_current_thread_from_work(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_101: [ execution_engine_linux_disassociate_current_thread_from_work shall disassociate the current thread from work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_102: [ If work is not the work object running on the current thread, disassociate_current_thread shall return. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_work_outside_of_a_work_function_returns)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, NULL);
    test_submit_work(work);

    ///act
    execution_engine_linux_disassociate_current_thread_from_work(work);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_wait_for_work(work, true);
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_102: [ If work is not the work object running on the current thread, disassociate_current_thread shall return. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_work_from_the_work_function_of_another_work_object_returns)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    TEST_WORK_CONTEXT other_test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, action_disassociate_from_other_work);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE other_work = test_create_work(execution_engine, &other_test_context, NULL);
    test_context.other_work = other_work;
    test_submit_work(work);

    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_work(work);
    execution_engine_linux_close_work(other_work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_070: [ run_next_work shall record the work object as the work object running on the current thread. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_101: [ execution_engine_linux_disassociate_current_thread_from_work shall disassociate the current thread from work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_103: [ disassociate_current_thread shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_104: [ disassociate_current_thread shall clear the work object running on the current thread. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_105: [ disassociate_current_thread shall end the invocation of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_106: [ disassociate_current_thread shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_078: [ If the current thread was disassociated from the work object, run_next_work shall not access the work object, which can be freed by then. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_work_from_its_work_function_ends_the_invocation)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, action_disassociate_twice_and_wait);
    test_submit_work(work);

    setup_run_next_work_start_expectations(&test_context);
    // the first disassociation ends the invocation, the second one does nothing
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    // waiting for the work object does not wait for the current thread
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    // the work object has no invocations, so closing it frees it
    execution_engine_linux_close_work(work);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_105: [ disassociate_current_thread shall end the invocation of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_082: [ If work is closed, end_invocation shall call the free function of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_078: [ If the current thread was disassociated from the work object, run_next_work shall not access the work object, which can be freed by then. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_work_frees_its_closed_work_object)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, action_close_and_disassociate);
    test_submit_work(work);

    setup_run_next_work_start_expectations(&test_context);
    // the work function closes its work object, which still runs
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    // the disassociation ends the last invocation of the closed work object, which frees it
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(free(work));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_create_timer */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_107: [ If execution_engine is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_with_NULL_execution_engine_fails)
{
    ///arrange
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, NULL, NULL);

    ///act
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = execution_engine_linux_create_timer(NULL, test_work_function, &test_context);

    ///assert
    ASSERT_IS_NULL(timer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_108: [ If work_function is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_with_NULL_work_function_fails)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, execution_engine, NULL);

    ///act
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = execution_engine_linux_create_timer(execution_engine, NULL, &test_context);

    ///assert
    ASSERT_IS_NULL(timer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_109: [ execution_engine_linux_create_timer shall allocate a timer. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_116: [ If any error occurs, execution_engine_linux_create_timer shall free the timer, fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_fails_when_allocating_the_timer_fails)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, execution_engine, NULL);
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = execution_engine_linux_create_timer(execution_engine, test_work_function, &test_context);

    ///assert
    ASSERT_IS_NULL(timer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_113: [ execution_engine_linux_create_timer shall reserve room for the timer in the timer heap by calling min_heap_reserve. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_116: [ If any error occurs, execution_engine_linux_create_timer shall free the timer, fail and return NULL. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_fails_when_min_heap_reserve_fails)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, execution_engine, NULL);
    void* allocated_timer;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&allocated_timer);
    STRICT_EXPECTED_CALL(min_heap_entry_init(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_reserve(g_timer_heap))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&allocated_timer);

    ///act
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = execution_engine_linux_create_timer(execution_engine, test_work_function, &test_context);

    ///assert
    ASSERT_IS_NULL(timer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_109: [ execution_engine_linux_create_timer shall allocate a timer. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_110: [ execution_engine_linux_create_timer shall initialize the work object of the timer with free_timer as its free function. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_111: [ execution_engine_linux_create_timer shall initialize the timer heap entry of the timer by calling min_heap_entry_init. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_112: [ execution_engine_linux_create_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_113: [ execution_engine_linux_create_timer shall reserve room for the timer in the timer heap by calling min_heap_reserve. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_114: [ execution_engine_linux_create_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_115: [ execution_engine_linux_create_timer shall succeed and return the timer. ]*/
TEST_FUNCTION(execution_engine_linux_create_timer_succeeds)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    test_init_work_context(&test_context, execution_engine, NULL);
    void* allocated_timer;
    MIN_HEAP_ENTRY* timer_heap_entry;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&allocated_timer);
    STRICT_EXPECTED_CALL(min_heap_entry_init(IGNORED_ARG))
        .CaptureArgumentValue_entry(&timer_heap_entry);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_reserve(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = execution_engine_linux_create_timer(execution_engine, test_work_function, &test_context);

    ///assert
    ASSERT_IS_NOT_NULL(timer);
    ASSERT_ARE_EQUAL(void_ptr, allocated_timer, timer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, MIN_HEAP_ENTRY_NOT_IN_HEAP, timer_heap_entry->index);

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/* free_timer */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_110: [ execution_engine_linux_create_timer shall initialize the work object of the timer with free_timer as its free function. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_082: [ If work is closed, end_invocation shall call the free function of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_117: [ free_timer shall release the room reserved for the timer in the timer heap by calling min_heap_unreserve. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_118: [ free_timer shall free the timer. ]*/
TEST_FUNCTION(free_timer_releases_the_room_reserved_in_the_timer_heap_and_frees_the_timer)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_submit_timer(timer);
    execution_engine_linux_close_timer(timer);
    umock_c_reset_all_calls();

    // the end of the last invocation of the closed timer frees the timer
    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_unreserve(g_timer_heap));
    STRICT_EXPECTED_CALL(free(timer));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_set_timer */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_119: [ If timer is NULL, execution_engine_linux_set_timer shall return. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_with_NULL_timer_returns)
{
    ///arrange

    ///act
    execution_engine_linux_set_timer(NULL, 100, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_120: [ execution_engine_linux_set_timer shall get the current time rounded up to the next millisecond by calling timer_global_get_elapsed_ms. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_121: [ execution_engine_linux_set_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_122: [ execution_engine_linux_set_timer shall remove timer from the timer heap by calling min_heap_remove. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_123: [ execution_engine_linux_set_timer shall store timer_period_ms as the period of timer. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_124: [ execution_engine_linux_set_timer shall insert timer in the timer heap with the current time plus start_delay_ms as due time by calling min_heap_insert. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_125: [ If timer is the first timer of the timer heap obtained by calling min_heap_peek, execution_engine_linux_set_timer shall wake the manager thread by calling pthread_cond_signal. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_126: [ execution_engine_linux_set_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_arms_the_timer_from_the_current_time_rounded_up_and_wakes_the_manager_thread)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    g_now_ms = 1000.2;

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(g_timer_heap, timer_heap_entry, 1101));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(g_manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_set_timer(timer, 100, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, timer_heap_entry, real_min_heap_peek(g_timer_heap));
    ASSERT_ARE_EQUAL(uint64_t, 1101, timer_heap_entry->key);

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_120: [ execution_engine_linux_set_timer shall get the current time rounded up to the next millisecond by calling timer_global_get_elapsed_ms. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_124: [ execution_engine_linux_set_timer shall insert timer in the timer heap with the current time plus start_delay_ms as due time by calling min_heap_insert. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_with_a_whole_number_of_milliseconds_does_not_round_the_current_time)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    g_now_ms = 1000.0;

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(g_timer_heap, timer_heap_entry, 1100));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(g_manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_set_timer(timer, 100, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1100, timer_heap_entry->key);

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_124: [ execution_engine_linux_set_timer shall insert timer in the timer heap with the current time plus start_delay_ms as due time by calling min_heap_insert. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_with_start_delay_0_makes_the_timer_due_now)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    g_now_ms = 3000.0;

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(g_timer_heap, timer_heap_entry, 3000));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(g_manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_set_timer(timer, 0, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_125: [ If timer is the first timer of the timer heap obtained by calling min_heap_peek, execution_engine_linux_set_timer shall wake the manager thread by calling pthread_cond_signal. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_does_not_wake_the_manager_thread_when_another_timer_is_due_first)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context_1;
    TEST_WORK_CONTEXT test_context_2;
    MIN_HEAP_ENTRY* timer_heap_entry_1;
    MIN_HEAP_ENTRY* timer_heap_entry_2;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer_1 = test_create_timer(execution_engine, &test_context_1, NULL, &timer_heap_entry_1);
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer_2 = test_create_timer(execution_engine, &test_context_2, NULL, &timer_heap_entry_2);
    test_set_timer(timer_1, 1000.0, 10, 0);

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry_2));
    STRICT_EXPECTED_CALL(min_heap_insert(g_timer_heap, timer_heap_entry_2, 1100));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_set_timer(timer_2, 100, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, timer_heap_entry_1, real_min_heap_peek(g_timer_heap));

    ///cleanup
    execution_engine_linux_close_timer(timer_1);
    execution_engine_linux_close_timer(timer_2);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_122: [ execution_engine_linux_set_timer shall remove timer from the timer heap by calling min_heap_remove. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_124: [ execution_engine_linux_set_timer shall insert timer in the timer heap with the current time plus start_delay_ms as due time by calling min_heap_insert. ]*/
TEST_FUNCTION(execution_engine_linux_set_timer_replaces_the_previous_setting_of_the_timer)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 1000.0, 5000, 0);
    g_now_ms = 2000.0;

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(g_timer_heap, timer_heap_entry, 2010));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(g_manager_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_set_timer(timer, 10, 0);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 2010, timer_heap_entry->key);
    ASSERT_ARE_EQUAL(void_ptr, timer_heap_entry, real_min_heap_peek(g_timer_heap));

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_cancel_timer */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_127: [ If timer is NULL, execution_engine_linux_cancel_timer shall return. ]*/
TEST_FUNCTION(execution_engine_linux_cancel_timer_with_NULL_timer_returns)
{
    ///arrange

    ///act
    execution_engine_linux_cancel_timer(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_128: [ execution_engine_linux_cancel_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_129: [ execution_engine_linux_cancel_timer shall remove timer from the timer heap by calling min_heap_remove. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_130: [ execution_engine_linux_cancel_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
TEST_FUNCTION(execution_engine_linux_cancel_timer_disarms_the_timer)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 1000.0, 100, 50);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_cancel_timer(timer);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(real_min_heap_peek(g_timer_heap));

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_wait_for_timer */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_131: [ If timer is NULL, execution_engine_linux_wait_for_timer shall return. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_timer_with_NULL_timer_returns)
{
    ///arrange

    ///act
    execution_engine_linux_wait_for_timer(NULL, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_132: [ execution_engine_linux_wait_for_timer shall wait for the invocations of the work object of timer. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_timer_returns_at_once_when_the_timer_has_no_invocations)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_timer(timer, false);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_132: [ execution_engine_linux_wait_for_timer shall wait for the invocations of the work object of timer. ]*/
TEST_FUNCTION(execution_engine_linux_wait_for_timer_with_cancel_pending_drops_the_pending_invocation)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_submit_timer(timer);
    PDLIST_ENTRY timer_work_entry = g_ready_queue->Flink;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(timer_work_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_wait_for_timer(timer, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, real_DList_IsListEmpty(g_ready_queue));

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_close_timer */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_133: [ If timer is NULL, execution_engine_linux_close_timer shall return. ]*/
TEST_FUNCTION(execution_engine_linux_close_timer_with_NULL_timer_returns)
{
    ///arrange

    ///act
    execution_engine_linux_close_timer(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_134: [ execution_engine_linux_close_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_135: [ execution_engine_linux_close_timer shall remove timer from the timer heap by calling min_heap_remove. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_136: [ execution_engine_linux_close_timer shall close the work object of timer. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_137: [ execution_engine_linux_close_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_098: [ If work has no pending and no running invocations, close_work_locked shall call the free function of work. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_117: [ free_timer shall release the room reserved for the timer in the timer heap by calling min_heap_unreserve. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_118: [ free_timer shall free the timer. ]*/
TEST_FUNCTION(execution_engine_linux_close_timer_disarms_and_frees_a_timer_without_invocations)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 1000.0, 100, 50);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(min_heap_unreserve(g_timer_heap));
    STRICT_EXPECTED_CALL(free(timer));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_close_timer(timer);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(real_min_heap_peek(g_timer_heap));

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_136: [ execution_engine_linux_close_timer shall close the work object of timer. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_099: [ If work has pending or running invocations, close_work_locked shall mark work as closed. ]*/
TEST_FUNCTION(execution_engine_linux_close_timer_with_a_pending_invocation_does_not_free_the_timer)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_submit_timer(timer);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));

    ///act
    execution_engine_linux_close_timer(timer);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    // the last invocation frees the timer
    test_run_next_work();
    execution_engine_dec_ref(execution_engine);
}

/* execution_engine_linux_disassociate_current_thread_from_timer */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_138: [ If timer is NULL, execution_engine_linux_disassociate_current_thread_from_timer shall return. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_timer_with_NULL_timer_returns)
{
    ///arrange

    ///act
    execution_engine_linux_disassociate_current_thread_from_timer(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_139: [ execution_engine_linux_disassociate_current_thread_from_timer shall disassociate the current thread from the work object of timer. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_timer_outside_of_its_work_function_returns)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);

    ///act
    execution_engine_linux_disassociate_current_thread_from_timer(timer);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_139: [ execution_engine_linux_disassociate_current_thread_from_timer shall disassociate the current thread from the work object of timer. ]*/
TEST_FUNCTION(execution_engine_linux_disassociate_current_thread_from_timer_from_its_work_function_ends_the_invocation)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, action_disassociate_from_timer, &timer_heap_entry);
    test_submit_timer(timer);

    setup_run_next_work_start_expectations(&test_context);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));

    ///act
    bool result = g_run_next_work(g_run_next_work_context);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    // the timer has no invocations, so closing it frees it
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/* manager_thread_func */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_140: [ manager_thread_func shall acquire the mutex by calling pthread_mutex_lock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_141: [ Until the execution engine is stopping: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_142: [ manager_thread_func shall get the current time by calling timer_global_get_elapsed_ms. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_143: [ manager_thread_func shall submit the due timers. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_144: [ manager_thread_func shall retire the worker threads idle for too long. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_145: [ manager_thread_func shall join the retired worker threads by calling worker_pool_join_retired_workers. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_146: [ manager_thread_func shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_147: [ manager_thread_func shall get the time to add a worker thread by calling thread_count_policy_get_add_thread_time with the number of work objects in the ready queue and the counts. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_149: [ If the time to add a worker thread is later than the current time, manager_thread_func shall wait for the next event. ]*/
TEST_FUNCTION(manager_thread_func_waits_for_the_next_event_when_no_worker_thread_needs_to_be_added)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context_1;
    TEST_WORK_CONTEXT test_context_2;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_1 = test_create_work(execution_engine, &test_context_1, NULL);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work_2 = test_create_work(execution_engine, &test_context_2, NULL);
    test_submit_work(work_1);
    test_submit_work(work_2);
    g_now_ms = 5000.6;
    g_worker_pool_counts = (WORKER_POOL_COUNTS){ .thread_count = 3, .starting_thread_count = 1, .idle_thread_count = 2, .oldest_idle_since_ms = 4000 };

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_get_retire_thread_time(TEST_THREAD_COUNT_POLICY, 3, 2, 4000, 5000));
    STRICT_EXPECTED_CALL(worker_pool_join_retired_workers(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_get_add_thread_time(TEST_THREAD_COUNT_POLICY, 2, 1, 2, 3));
    STRICT_EXPECTED_CALL(thread_count_policy_get_retire_thread_time(TEST_THREAD_COUNT_POLICY, 3, 2, 4000, 5000));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(g_manager_cond, g_lock));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(g_has_manager_thread_exited);

    ///cleanup
    execution_engine_linux_wait_for_work(work_1, true);
    execution_engine_linux_wait_for_work(work_2, true);
    execution_engine_linux_close_work(work_1);
    execution_engine_linux_close_work(work_2);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_148: [ If the time to add a worker thread is not later than the current time, manager_thread_func shall add a worker thread. ]*/
TEST_FUNCTION(manager_thread_func_adds_a_worker_thread_when_the_time_to_add_one_is_not_later_than_now)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    g_now_ms = 5000.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, 5000);
    STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, 5000));
    // the manager thread checks again and waits
    setup_manager_idle_iteration_expectations(0);

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(g_has_manager_thread_exited);

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_141: [ Until the execution engine is stopping: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_143: [ manager_thread_func shall submit the due timers. ]*/
TEST_FUNCTION(manager_thread_func_checks_again_after_it_is_woken)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_run_manager_thread_until_it_waits();
    // arming the timer wakes the manager thread
    test_set_timer(timer, 2000.0, 0, 0);

    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(1, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(g_manager_cond, g_lock));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(g_has_manager_thread_exited);

    ///cleanup
    execution_engine_linux_wait_for_timer(timer, true);
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_141: [ Until the execution engine is stopping: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_150: [ manager_thread_func shall release the mutex by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_151: [ If the teardown is deferred to the manager thread: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_152: [ manager_thread_func shall detach the manager thread by calling pthread_detach. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_153: [ manager_thread_func shall tear down the execution engine. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_154: [ manager_thread_func shall return NULL. ]*/
TEST_FUNCTION(manager_thread_func_tears_down_the_execution_engine_when_the_teardown_is_deferred_to_it)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = test_create_work(execution_engine, &test_context, action_close_work_and_release_execution_engine);
    test_submit_work(work);
    test_run_manager_thread_until_it_waits();
    // the work function releases the last reference to the execution engine
    test_run_next_work();

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(g_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_detach(pthread_self()));
    setup_teardown_expectations();

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(g_has_manager_thread_exited);
    ASSERT_IS_NULL(g_manager_thread_result);
}

/* submit_due_timers */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_155: [ For each timer whose due time is not later than now, in due time order, as obtained by calling min_heap_peek: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_156: [ submit_due_timers shall remove the timer from the timer heap by calling min_heap_remove. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_157: [ If the period of the timer is not 0: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_161: [ submit_due_timers shall submit the work object of the timer. ]*/
TEST_FUNCTION(submit_due_timers_submits_a_due_timer_that_fires_once)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 1000.0, 100, 0);
    g_now_ms = 1100.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(1, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(g_manager_cond, g_lock));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(real_min_heap_peek(g_timer_heap));
    ASSERT_ARE_EQUAL(int, 0, real_DList_IsListEmpty(g_ready_queue));

    ///cleanup
    execution_engine_linux_wait_for_timer(timer, true);
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_155: [ For each timer whose due time is not later than now, in due time order, as obtained by calling min_heap_peek: ]*/
TEST_FUNCTION(submit_due_timers_does_not_submit_a_timer_that_is_not_due)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 1000.0, 100, 0);
    g_now_ms = 1099.9;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, timer_heap_entry, real_min_heap_peek(g_timer_heap));
    ASSERT_ARE_NOT_EQUAL(int, 0, real_DList_IsListEmpty(g_ready_queue));

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_123: [ execution_engine_linux_set_timer shall store timer_period_ms as the period of timer. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_157: [ If the period of the timer is not 0: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_158: [ submit_due_timers shall compute the next due time as the due time plus the period. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_160: [ submit_due_timers shall insert the timer in the timer heap with the next due time by calling min_heap_insert. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_161: [ submit_due_timers shall submit the work object of the timer. ]*/
TEST_FUNCTION(submit_due_timers_rearms_a_periodic_timer_on_its_schedule)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 1000.0, 100, 50);
    g_now_ms = 1120.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(g_timer_heap, timer_heap_entry, 1150));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(1, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1150, timer_heap_entry->key);
    ASSERT_ARE_EQUAL(int64_t, 1, (int64_t)g_timedwait_deadline.tv_sec);
    ASSERT_ARE_EQUAL(int64_t, 150000000, (int64_t)g_timedwait_deadline.tv_nsec);

    ///cleanup
    execution_engine_linux_cancel_timer(timer);
    execution_engine_linux_wait_for_timer(timer, true);
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_158: [ submit_due_timers shall compute the next due time as the due time plus the period. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_159: [ If the next due time is not later than now, submit_due_timers shall use now plus the period as the next due time. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_160: [ submit_due_timers shall insert the timer in the timer heap with the next due time by calling min_heap_insert. ]*/
TEST_FUNCTION(submit_due_timers_rearms_a_periodic_timer_that_fell_behind_from_now)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 1000.0, 100, 50);
    // the next due time 1150 is not later than now
    g_now_ms = 1150.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry));
    STRICT_EXPECTED_CALL(min_heap_insert(g_timer_heap, timer_heap_entry, 1200));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(1, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint64_t, 1200, timer_heap_entry->key);

    ///cleanup
    execution_engine_linux_cancel_timer(timer);
    execution_engine_linux_wait_for_timer(timer, true);
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_155: [ For each timer whose due time is not later than now, in due time order, as obtained by calling min_heap_peek: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_156: [ submit_due_timers shall remove the timer from the timer heap by calling min_heap_remove. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_161: [ submit_due_timers shall submit the work object of the timer. ]*/
TEST_FUNCTION(submit_due_timers_submits_the_due_timers_in_due_time_order)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context_1;
    TEST_WORK_CONTEXT test_context_2;
    TEST_WORK_CONTEXT test_context_3;
    MIN_HEAP_ENTRY* timer_heap_entry_1;
    MIN_HEAP_ENTRY* timer_heap_entry_2;
    MIN_HEAP_ENTRY* timer_heap_entry_3;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer_1 = test_create_timer(execution_engine, &test_context_1, NULL, &timer_heap_entry_1);
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer_2 = test_create_timer(execution_engine, &test_context_2, NULL, &timer_heap_entry_2);
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer_3 = test_create_timer(execution_engine, &test_context_3, NULL, &timer_heap_entry_3);
    test_set_timer(timer_1, 1000.0, 100, 0);
    test_set_timer(timer_2, 1000.0, 50, 0);
    test_set_timer(timer_3, 1000.0, 2000, 0);
    g_now_ms = 1200.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry_2));
    setup_enqueue_in_empty_ready_queue_expectations();
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(min_heap_remove(g_timer_heap, timer_heap_entry_1));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_ready_queue));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_ready_queue, IGNORED_ARG));
    STRICT_EXPECTED_CALL(worker_pool_wake_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(2, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, timer_heap_entry_3, real_min_heap_peek(g_timer_heap));
    ASSERT_ARE_EQUAL(int64_t, 3, (int64_t)g_timedwait_deadline.tv_sec);
    ASSERT_ARE_EQUAL(int64_t, 0, (int64_t)g_timedwait_deadline.tv_nsec);

    ///cleanup
    execution_engine_linux_wait_for_timer(timer_1, true);
    execution_engine_linux_wait_for_timer(timer_2, true);
    execution_engine_linux_close_timer(timer_1);
    execution_engine_linux_close_timer(timer_2);
    execution_engine_linux_close_timer(timer_3);
    execution_engine_dec_ref(execution_engine);
}

/* retire_idle_workers */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_162: [ retire_idle_workers shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_163: [ While the time to retire a worker thread obtained by calling thread_count_policy_get_retire_thread_time with the counts and now is not later than now: ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_164: [ retire_idle_workers shall retire the worker thread idle the longest by calling worker_pool_retire_oldest_idle_worker. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_165: [ retire_idle_workers shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
TEST_FUNCTION(retire_idle_workers_retires_worker_threads_while_the_time_to_retire_one_is_not_later_than_now)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    g_now_ms = 5000.0;
    WORKER_POOL_COUNTS counts_1 = { .thread_count = 4, .starting_thread_count = 0, .idle_thread_count = 3, .oldest_idle_since_ms = 1000 };
    WORKER_POOL_COUNTS counts_2 = { .thread_count = 3, .starting_thread_count = 0, .idle_thread_count = 2, .oldest_idle_since_ms = 2000 };
    WORKER_POOL_COUNTS counts_3 = { .thread_count = 2, .starting_thread_count = 0, .idle_thread_count = 1, .oldest_idle_since_ms = 3000 };

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG))
        .CopyOutArgumentBuffer_counts(&counts_1, sizeof(counts_1));
    STRICT_EXPECTED_CALL(thread_count_policy_get_retire_thread_time(TEST_THREAD_COUNT_POLICY, 4, 3, 1000, 5000))
        .SetReturn(4000);
    STRICT_EXPECTED_CALL(worker_pool_retire_oldest_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG))
        .CopyOutArgumentBuffer_counts(&counts_2, sizeof(counts_2));
    STRICT_EXPECTED_CALL(thread_count_policy_get_retire_thread_time(TEST_THREAD_COUNT_POLICY, 3, 2, 2000, 5000))
        .SetReturn(5000);
    STRICT_EXPECTED_CALL(worker_pool_retire_oldest_idle_worker(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG))
        .CopyOutArgumentBuffer_counts(&counts_3, sizeof(counts_3));
    STRICT_EXPECTED_CALL(thread_count_policy_get_retire_thread_time(TEST_THREAD_COUNT_POLICY, 2, 1, 3000, 5000))
        .SetReturn(5001);
    STRICT_EXPECTED_CALL(worker_pool_join_retired_workers(TEST_WORKER_POOL));
    STRICT_EXPECTED_CALL(worker_pool_get_counts(TEST_WORKER_POOL, IGNORED_ARG));
    STRICT_EXPECTED_CALL(thread_count_policy_get_add_thread_time(TEST_THREAD_COUNT_POLICY, 0, 0, 0, 0));
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(g_manager_cond, g_lock));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* add_worker */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_166: [ add_worker shall add a worker thread by calling worker_pool_add_worker. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_168: [ If worker_pool_add_worker succeeds, add_worker shall record progress by calling thread_count_policy_on_progress with now. ]*/
TEST_FUNCTION(add_worker_records_progress_when_adding_a_worker_thread_succeeds)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    g_now_ms = 7000.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, 6950);
    STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(thread_count_policy_on_progress(TEST_THREAD_COUNT_POLICY, 7000));
    setup_manager_idle_iteration_expectations(0);

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_166: [ add_worker shall add a worker thread by calling worker_pool_add_worker. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_167: [ If worker_pool_add_worker fails, add_worker shall call thread_count_policy_on_add_thread_failed with now. ]*/
TEST_FUNCTION(add_worker_reports_the_failure_to_the_thread_count_policy_when_adding_a_worker_thread_fails)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    g_now_ms = 7000.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, 7000);
    STRICT_EXPECTED_CALL(worker_pool_add_worker(TEST_WORKER_POOL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(thread_count_policy_on_add_thread_failed(TEST_THREAD_COUNT_POLICY, 7000));
    setup_manager_idle_iteration_expectations(0);

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/* wait_for_next_event */

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_169: [ wait_for_next_event shall record that the manager thread is watching the ready queue if add_thread_time is not THREAD_COUNT_POLICY_NEVER. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_170: [ wait_for_next_event shall get the time to retire a worker thread by calling thread_count_policy_get_retire_thread_time with counts and now. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_171: [ wait_for_next_event shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_172: [ wait_for_next_event shall compute the deadline as the earliest of add_thread_time, the time to retire a worker thread and the due time of the first timer, if any. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_173: [ If the deadline is THREAD_COUNT_POLICY_NEVER, wait_for_next_event shall wait by calling pthread_cond_wait. ]*/
TEST_FUNCTION(wait_for_next_event_waits_without_a_deadline_when_nothing_is_scheduled)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    g_now_ms = 5000.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    setup_manager_idle_iteration_expectations(0);

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_169: [ wait_for_next_event shall record that the manager thread is watching the ready queue if add_thread_time is not THREAD_COUNT_POLICY_NEVER. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_172: [ wait_for_next_event shall compute the deadline as the earliest of add_thread_time, the time to retire a worker thread and the due time of the first timer, if any. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_174: [ If the deadline is not THREAD_COUNT_POLICY_NEVER, wait_for_next_event shall wait until the deadline by calling pthread_cond_timedwait. ]*/
TEST_FUNCTION(wait_for_next_event_waits_until_the_time_to_add_a_worker_thread)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    g_now_ms = 12000.0;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, 12345);
    setup_wait_for_next_event_expectations(THREAD_COUNT_POLICY_NEVER);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 12, (int64_t)g_timedwait_deadline.tv_sec);
    ASSERT_ARE_EQUAL(int64_t, 345000000, (int64_t)g_timedwait_deadline.tv_nsec);

    ///cleanup
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_170: [ wait_for_next_event shall get the time to retire a worker thread by calling thread_count_policy_get_retire_thread_time with counts and now. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_172: [ wait_for_next_event shall compute the deadline as the earliest of add_thread_time, the time to retire a worker thread and the due time of the first timer, if any. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_174: [ If the deadline is not THREAD_COUNT_POLICY_NEVER, wait_for_next_event shall wait until the deadline by calling pthread_cond_timedwait. ]*/
TEST_FUNCTION(wait_for_next_event_waits_until_the_time_to_retire_a_worker_thread_when_it_is_the_earliest)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 5000.0, 3000, 0);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, THREAD_COUNT_POLICY_NEVER);
    setup_wait_for_next_event_expectations(7000);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 7, (int64_t)g_timedwait_deadline.tv_sec);
    ASSERT_ARE_EQUAL(int64_t, 0, (int64_t)g_timedwait_deadline.tv_nsec);

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_171: [ wait_for_next_event shall get the first timer by calling min_heap_peek. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_172: [ wait_for_next_event shall compute the deadline as the earliest of add_thread_time, the time to retire a worker thread and the due time of the first timer, if any. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_174: [ If the deadline is not THREAD_COUNT_POLICY_NEVER, wait_for_next_event shall wait until the deadline by calling pthread_cond_timedwait. ]*/
TEST_FUNCTION(wait_for_next_event_waits_until_the_first_timer_is_due_when_it_is_the_earliest)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 5000.0, 3001, 0);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, 9000);
    setup_wait_for_next_event_expectations(9500);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 8, (int64_t)g_timedwait_deadline.tv_sec);
    ASSERT_ARE_EQUAL(int64_t, 1000000, (int64_t)g_timedwait_deadline.tv_nsec);

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_172: [ wait_for_next_event shall compute the deadline as the earliest of add_thread_time, the time to retire a worker thread and the due time of the first timer, if any. ]*/
/*Tests_SRS_EXECUTION_ENGINE_LINUX_42_174: [ If the deadline is not THREAD_COUNT_POLICY_NEVER, wait_for_next_event shall wait until the deadline by calling pthread_cond_timedwait. ]*/
TEST_FUNCTION(wait_for_next_event_waits_until_the_time_to_add_a_worker_thread_when_it_is_the_earliest)
{
    ///arrange
    EXECUTION_ENGINE_HANDLE execution_engine = test_create_execution_engine(1, 4);
    TEST_WORK_CONTEXT test_context;
    MIN_HEAP_ENTRY* timer_heap_entry;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer = test_create_timer(execution_engine, &test_context, NULL, &timer_heap_entry);
    test_set_timer(timer, 5000.0, 3000, 0);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(g_lock));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(min_heap_peek(g_timer_heap));
    setup_manager_retire_join_and_get_add_thread_time_expectations(0, 6020);
    setup_wait_for_next_event_expectations(7000);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_timedwait(g_manager_cond, g_lock, IGNORED_ARG));

    ///act
    run_manager_thread();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 6, (int64_t)g_timedwait_deadline.tv_sec);
    ASSERT_ARE_EQUAL(int64_t, 20000000, (int64_t)g_timedwait_deadline.tv_nsec);

    ///cleanup
    execution_engine_linux_close_timer(timer);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

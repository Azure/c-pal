// Copyright (C) Microsoft Corporation. All rights reserved.

#include "worker_pool_linux_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);

// implemented in mock_worker_pool_linux.c, which includes worker_pool_linux.c and so can read the state of a worker
const char* mock_worker_pool_linux_get_worker_state(void* worker);
WORKER_POOL_HANDLE mock_worker_pool_linux_get_worker_pool(void* worker);

#define TEST_RUN_FUNCTION_CONTEXT ((void*)0x4242)
#define MAX_TEST_WORKERS 4
#define TEST_WORKER_STACK_SIZE (512 * 1024)
#define MAX_RUN_FUNCTION_CALLS 100

// the pthread functions are mocked, so test_lock is only used as an argument
static pthread_mutex_t test_lock = PTHREAD_MUTEX_INITIALIZER;

// The worker threads do not run on real threads. Each one runs on its own stack (ucontext) on the test thread:
// pthread_cond_wait switches back to the test, as if the worker thread blocked, and the test switches to the
// worker thread when it should run, as if it was scheduled. This makes the interleaving of the worker threads
// and the calls the test makes deterministic.
typedef struct TEST_WORKER_TAG
{
    THREAD_START_FUNC thread_func;
    void* worker; // the arg passed to ThreadAPI_Create, which is the worker allocated by malloc
    pthread_cond_t* wake_cond; // the condition variable initialized for the worker
    PDLIST_ENTRY worker_list_entry; // the entry inserted in the list of workers
    PDLIST_ENTRY idle_list_entry; // the entry inserted in the list of idle workers
    const char* state_in_run_function; // the state of the worker the last time it called run_function
    const char* state_in_cond_wait; // the state of the worker the last time it called pthread_cond_wait
    WORKER_POOL_COUNTS counts_when_joined; // the counts of g_worker_pool_to_count_when_joined when the worker thread is joined
    ucontext_t context;
    void* stack;
    bool is_started;
    bool has_exited;
    int thread_result;
} TEST_WORKER;

static TEST_WORKER g_test_workers[MAX_TEST_WORKERS];
static size_t g_test_worker_count;
static TEST_WORKER* g_running_test_worker;
static ucontext_t g_test_context;
static pthread_cond_t* g_last_initialized_cond;
static size_t g_run_function_call_count;
static WORKER_POOL_HANDLE g_worker_pool_to_count_when_joined;

static PDLIST_ENTRY g_workers_list_head;
static PDLIST_ENTRY g_idle_workers_list_head;
static PDLIST_ENTRY g_retired_workers_list_head;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_mutex_lock, pthread_mutex_t*, mutex)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_mutex_unlock, pthread_mutex_t*, mutex)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_init, pthread_cond_t*, cond, const pthread_condattr_t*, attr)
    g_last_initialized_cond = cond;
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_destroy, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_wait, pthread_cond_t*, cond, pthread_mutex_t*, mutex)
    // the worker thread blocks: switch back to the test until the test runs the worker thread again
    TEST_WORKER* waiting_test_worker = g_running_test_worker;
    ASSERT_IS_NOT_NULL(waiting_test_worker, "pthread_cond_wait shall only be called on a worker thread");
    waiting_test_worker->state_in_cond_wait = mock_worker_pool_linux_get_worker_state(waiting_test_worker->worker);
    ASSERT_ARE_EQUAL(int, 0, swapcontext(&waiting_test_worker->context, &g_test_context));
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_signal, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, bool, test_run_function, void*, context)
    TEST_WORKER* running_test_worker = g_running_test_worker;
    ASSERT_IS_NOT_NULL(running_test_worker, "run_function shall only be called on a worker thread");
    running_test_worker->state_in_run_function = mock_worker_pool_linux_get_worker_state(running_test_worker->worker);
    // fail instead of hanging if a worker thread spins
    g_run_function_call_count++;
    ASSERT_IS_TRUE(g_run_function_call_count < MAX_RUN_FUNCTION_CALLS, "run_function called too many times");
MOCK_FUNCTION_END(false)

static void test_worker_thread_entry(void)
{
    TEST_WORKER* test_worker = g_running_test_worker;
    test_worker->thread_result = test_worker->thread_func(test_worker->worker);
    test_worker->has_exited = true;
}

// runs the worker thread until it waits in pthread_cond_wait or exits
static void run_worker_thread(TEST_WORKER* test_worker)
{
    ASSERT_IS_NULL(g_running_test_worker, "a worker thread cannot run another worker thread");
    ASSERT_IS_FALSE(test_worker->has_exited, "the worker thread already exited");

    if (test_worker->is_started)
    {
        // the worker thread continues where it waited
    }
    else
    {
        test_worker->stack = real_gballoc_hl_malloc(TEST_WORKER_STACK_SIZE);
        ASSERT_IS_NOT_NULL(test_worker->stack);
        ASSERT_ARE_EQUAL(int, 0, getcontext(&test_worker->context));
        test_worker->context.uc_stack.ss_sp = test_worker->stack;
        test_worker->context.uc_stack.ss_size = TEST_WORKER_STACK_SIZE;
        test_worker->context.uc_link = &g_test_context;
        makecontext(&test_worker->context, test_worker_thread_entry, 0);
        test_worker->is_started = true;
    }

    g_running_test_worker = test_worker;
    ASSERT_ARE_EQUAL(int, 0, swapcontext(&g_test_context, &test_worker->context));
    g_running_test_worker = NULL;
}

static THREADAPI_RESULT hook_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    ASSERT_IS_TRUE(g_test_worker_count < MAX_TEST_WORKERS, "too many worker threads");
    TEST_WORKER* test_worker = &g_test_workers[g_test_worker_count];
    g_test_worker_count++;

    test_worker->thread_func = func;
    test_worker->worker = arg;
    test_worker->wake_cond = g_last_initialized_cond;
    test_worker->worker_list_entry = NULL;
    test_worker->idle_list_entry = NULL;
    test_worker->state_in_run_function = NULL;
    test_worker->state_in_cond_wait = NULL;
    (void)memset(&test_worker->counts_when_joined, 0xFF, sizeof(test_worker->counts_when_joined));
    test_worker->stack = NULL;
    test_worker->is_started = false;
    test_worker->has_exited = false;
    test_worker->thread_result = -1;

    // the worker thread does not run until the test runs it
    *threadHandle = test_worker;
    return THREADAPI_OK;
}

static THREADAPI_RESULT hook_ThreadAPI_Join(THREAD_HANDLE threadHandle, int* res)
{
    TEST_WORKER* test_worker = threadHandle;

    if (test_worker->has_exited)
    {
        // the worker thread already exited
    }
    else
    {
        // the joined worker thread runs until it exits
        run_worker_thread(test_worker);
        ASSERT_IS_TRUE(test_worker->has_exited, "the joined worker thread did not exit");
    }

    if (g_worker_pool_to_count_when_joined == NULL)
    {
        // the test does not check the counts when the worker thread is joined
    }
    else
    {
        worker_pool_get_counts(g_worker_pool_to_count_when_joined, &test_worker->counts_when_joined);
    }

    *res = test_worker->thread_result;
    return THREADAPI_OK;
}

static void assert_counts_are_equal(const WORKER_POOL_COUNTS* counts, uint32_t expected_thread_count, uint32_t expected_starting_thread_count, uint32_t expected_idle_thread_count, uint64_t expected_oldest_idle_since_ms)
{
    ASSERT_ARE_EQUAL(uint32_t, expected_thread_count, counts->thread_count);
    ASSERT_ARE_EQUAL(uint32_t, expected_starting_thread_count, counts->starting_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, expected_idle_thread_count, counts->idle_thread_count);
    ASSERT_ARE_EQUAL(uint64_t, expected_oldest_idle_since_ms, counts->oldest_idle_since_ms);
}

static void assert_worker_pool_counts(WORKER_POOL_HANDLE worker_pool, uint32_t expected_thread_count, uint32_t expected_starting_thread_count, uint32_t expected_idle_thread_count, uint64_t expected_oldest_idle_since_ms)
{
    WORKER_POOL_COUNTS counts;
    (void)memset(&counts, 0xFF, sizeof(counts));
    worker_pool_get_counts(worker_pool, &counts);
    assert_counts_are_equal(&counts, expected_thread_count, expected_starting_thread_count, expected_idle_thread_count, expected_oldest_idle_since_ms);
}

static void setup_worker_pool_create_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG))
        .CaptureArgumentValue_listHead(&g_workers_list_head);
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG))
        .CaptureArgumentValue_listHead(&g_idle_workers_list_head);
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG))
        .CaptureArgumentValue_listHead(&g_retired_workers_list_head);
}

static WORKER_POOL_HANDLE test_worker_pool_create(void)
{
    setup_worker_pool_create_expectations();
    WORKER_POOL_HANDLE result = worker_pool_create(&test_lock, test_run_function, TEST_RUN_FUNCTION_CONTEXT);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return result;
}

static void setup_worker_pool_add_worker_expectations(void** allocated_worker, PDLIST_ENTRY* worker_list_entry)
{
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_arg(allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_workers_list_head, IGNORED_ARG))
        .CaptureArgumentValue_listEntry(worker_list_entry);
}

static TEST_WORKER* test_worker_pool_add_worker(WORKER_POOL_HANDLE worker_pool)
{
    void* allocated_worker;
    PDLIST_ENTRY worker_list_entry;
    setup_worker_pool_add_worker_expectations(&allocated_worker, &worker_list_entry);
    ASSERT_ARE_EQUAL(int, 0, worker_pool_add_worker(worker_pool));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    TEST_WORKER* result = &g_test_workers[g_test_worker_count - 1];
    ASSERT_ARE_EQUAL(void_ptr, allocated_worker, result->worker);
    result->worker_list_entry = worker_list_entry;
    umock_c_reset_all_calls();
    return result;
}

// runs a worker thread that did not start yet until it finds no work and waits while idle
static void test_run_worker_thread_until_idle(TEST_WORKER* test_worker, double now_ms)
{
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(now_ms);
    STRICT_EXPECTED_CALL(DList_InsertHeadList(g_idle_workers_list_head, IGNORED_ARG))
        .CaptureArgumentValue_listEntry(&test_worker->idle_list_entry);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_worker->wake_cond, &test_lock));
    run_worker_thread(test_worker);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_worker->has_exited);
    umock_c_reset_all_calls();
}

// runs a woken worker thread until it finds no work and waits while idle again
static void test_run_woken_worker_thread_until_idle(TEST_WORKER* test_worker, double now_ms)
{
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(now_ms);
    STRICT_EXPECTED_CALL(DList_InsertHeadList(g_idle_workers_list_head, test_worker->idle_list_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(test_worker->wake_cond, &test_lock));
    run_worker_thread(test_worker);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_worker->has_exited);
    umock_c_reset_all_calls();
}

// runs a retired worker thread until it exits
static void test_run_retired_worker_thread_until_exit(TEST_WORKER* test_worker)
{
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    run_worker_thread(test_worker);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_worker->has_exited);
    umock_c_reset_all_calls();
}

static void setup_worker_pool_wake_idle_worker_expectations(TEST_WORKER* woken_test_worker)
{
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(woken_test_worker->wake_cond));
}

static void test_worker_pool_wake_idle_worker(WORKER_POOL_HANDLE worker_pool, TEST_WORKER* woken_test_worker)
{
    setup_worker_pool_wake_idle_worker_expectations(woken_test_worker);
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, worker_pool_wake_idle_worker(worker_pool));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

static void setup_worker_pool_retire_oldest_idle_worker_expectations(TEST_WORKER* retired_test_worker)
{
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveTailList(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(retired_test_worker->wake_cond));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(retired_test_worker->worker_list_entry));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_retired_workers_list_head, retired_test_worker->worker_list_entry));
}

static void test_worker_pool_retire_oldest_idle_worker(WORKER_POOL_HANDLE worker_pool, TEST_WORKER* retired_test_worker)
{
    setup_worker_pool_retire_oldest_idle_worker_expectations(retired_test_worker);
    worker_pool_retire_oldest_idle_worker(worker_pool);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
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
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pthread_cond_init, 1);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, hook_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Join, hook_ThreadAPI_Join);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Join, THREADAPI_ERROR);

    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_mutex_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_cond_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const pthread_condattr_t*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(WORKER_POOL_RUN_FUNCTION, void*);

    REGISTER_TYPE(THREADAPI_RESULT, THREADAPI_RESULT);
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
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");

    g_test_worker_count = 0;
    g_running_test_worker = NULL;
    g_last_initialized_cond = NULL;
    g_run_function_call_count = 0;
    g_worker_pool_to_count_when_joined = NULL;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    for (size_t i = 0; i < g_test_worker_count; i++)
    {
        if (g_test_workers[i].stack == NULL)
        {
            // the worker thread never ran
        }
        else
        {
            real_gballoc_hl_free(g_test_workers[i].stack);
            g_test_workers[i].stack = NULL;
        }
    }

    umock_c_negative_tests_deinit();
}

/* worker_pool_create */

/*Tests_SRS_WORKER_POOL_LINUX_42_001: [ If lock is NULL, worker_pool_create shall fail and return NULL. ]*/
TEST_FUNCTION(worker_pool_create_with_NULL_lock_fails)
{
    ///arrange

    ///act
    WORKER_POOL_HANDLE worker_pool = worker_pool_create(NULL, test_run_function, TEST_RUN_FUNCTION_CONTEXT);

    ///assert
    ASSERT_IS_NULL(worker_pool);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_002: [ If run_function is NULL, worker_pool_create shall fail and return NULL. ]*/
TEST_FUNCTION(worker_pool_create_with_NULL_run_function_fails)
{
    ///arrange

    ///act
    WORKER_POOL_HANDLE worker_pool = worker_pool_create(&test_lock, NULL, TEST_RUN_FUNCTION_CONTEXT);

    ///assert
    ASSERT_IS_NULL(worker_pool);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_003: [ worker_pool_create shall allocate a new pool. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_005: [ worker_pool_create shall initialize the list of workers, the list of idle workers and the list of retired workers by calling DList_InitializeListHead. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_006: [ worker_pool_create shall set the thread count, the starting thread count, the idle thread count and the signaled thread count to 0. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_008: [ worker_pool_create shall succeed and return the pool. ]*/
TEST_FUNCTION(worker_pool_create_succeeds)
{
    ///arrange
    setup_worker_pool_create_expectations();

    ///act
    WORKER_POOL_HANDLE worker_pool = worker_pool_create(&test_lock, test_run_function, TEST_RUN_FUNCTION_CONTEXT);

    ///assert
    ASSERT_IS_NOT_NULL(worker_pool);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(g_workers_list_head);
    ASSERT_IS_NOT_NULL(g_idle_workers_list_head);
    ASSERT_IS_NOT_NULL(g_retired_workers_list_head);
    ASSERT_ARE_NOT_EQUAL(void_ptr, g_workers_list_head, g_idle_workers_list_head);
    ASSERT_ARE_NOT_EQUAL(void_ptr, g_workers_list_head, g_retired_workers_list_head);
    ASSERT_ARE_NOT_EQUAL(void_ptr, g_idle_workers_list_head, g_retired_workers_list_head);
    assert_worker_pool_counts(worker_pool, 0, 0, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_009: [ If any error occurs, worker_pool_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_malloc_fails_worker_pool_create_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    WORKER_POOL_HANDLE worker_pool = worker_pool_create(&test_lock, test_run_function, TEST_RUN_FUNCTION_CONTEXT);

    ///assert
    ASSERT_IS_NULL(worker_pool);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* worker_pool_destroy */

/*Tests_SRS_WORKER_POOL_LINUX_42_010: [ If worker_pool is NULL, worker_pool_destroy shall return. ]*/
TEST_FUNCTION(worker_pool_destroy_with_NULL_worker_pool_returns)
{
    ///arrange

    ///act
    worker_pool_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_011: [ worker_pool_destroy shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_014: [ worker_pool_destroy shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_016: [ For each worker in the list of retired workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_017: [ worker_pool_destroy shall free the pool. ]*/
TEST_FUNCTION(worker_pool_destroy_with_no_workers_frees_the_pool)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(free(worker_pool));

    ///act
    worker_pool_destroy(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_011: [ worker_pool_destroy shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_012: [ worker_pool_destroy shall mark the pool as stopped. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_014: [ worker_pool_destroy shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_082: [ worker_pool_destroy shall remove the worker from the list of workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_096: [ worker_pool_destroy shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_097: [ If ThreadAPI_Join succeeds: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_098: [ worker_pool_destroy shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_099: [ worker_pool_destroy shall free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_017: [ worker_pool_destroy shall free the pool. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_040: [ internal_worker_pool_worker_thread_func shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_041: [ internal_worker_pool_worker_thread_func shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_044: [ If run_function returns false: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_045: [ If the pool is stopped, internal_worker_pool_worker_thread_func shall stop. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_047: [ internal_worker_pool_worker_thread_func shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_048: [ internal_worker_pool_worker_thread_func shall return 0. ]*/
TEST_FUNCTION(worker_pool_destroy_destroys_a_worker_thread_that_did_not_start_yet)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG));
    // the worker thread starts after the pool is stopped
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    // back in worker_pool_destroy
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_a->worker));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(free(worker_pool));

    ///act
    worker_pool_destroy(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(worker_a->has_exited);
    ASSERT_ARE_EQUAL(int, 0, worker_a->thread_result);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_013: [ For each idle worker, worker_pool_destroy shall wake it by calling pthread_cond_signal on its condition variable. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_082: [ worker_pool_destroy shall remove the worker from the list of workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_096: [ worker_pool_destroy shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_097: [ If ThreadAPI_Join succeeds: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_098: [ worker_pool_destroy shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_099: [ worker_pool_destroy shall free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_045: [ If the pool is stopped, internal_worker_pool_worker_thread_func shall stop. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ internal_worker_pool_worker_thread_func shall wait by calling pthread_cond_wait on the condition variable of the worker while the worker is idle and the pool is not stopped. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_054: [ If the worker is still idle: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_091: [ internal_worker_pool_worker_thread_func shall remove the worker from the list of idle workers by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_092: [ internal_worker_pool_worker_thread_func shall decrement the idle thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_093: [ internal_worker_pool_worker_thread_func shall set the state of the worker to running. ]*/
TEST_FUNCTION(worker_pool_destroy_wakes_the_idle_worker_threads_and_destroys_them)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);
    g_worker_pool_to_count_when_joined = worker_pool;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_b->wake_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG));
    // worker A wakes up because the pool is stopped
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(worker_a->idle_list_entry));
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    // the test gets the counts when worker A is joined
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));
    // back in worker_pool_destroy
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_a->worker));

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_b, IGNORED_ARG));
    // worker B wakes up because the pool is stopped
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(worker_b->idle_list_entry));
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    // the test gets the counts when worker B is joined
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));
    // back in worker_pool_destroy
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_b->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_b->worker));

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(free(worker_pool));

    ///act
    worker_pool_destroy(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(worker_a->has_exited);
    ASSERT_IS_TRUE(worker_b->has_exited);
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_RUNNING", worker_a->state_in_run_function);
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_RUNNING", worker_b->state_in_run_function);
    // worker A left the list of idle workers, worker B is still idle
    assert_counts_are_equal(&worker_a->counts_when_joined, 2, 0, 1, 2000);
    assert_counts_are_equal(&worker_b->counts_when_joined, 2, 0, 0, 0);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_043: [ internal_worker_pool_worker_thread_func shall call run_function with run_function_context. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_045: [ If the pool is stopped, internal_worker_pool_worker_thread_func shall stop. ]*/
TEST_FUNCTION(worker_pool_destroy_lets_the_worker_threads_run_the_remaining_work)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG));
    // worker A runs the work queued before it stops
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(worker_a->idle_list_entry));
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    // back in worker_pool_destroy
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_a->worker));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(free(worker_pool));

    ///act
    worker_pool_destroy(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(worker_a->has_exited);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_016: [ For each worker in the list of retired workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_084: [ worker_pool_destroy shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_100: [ worker_pool_destroy shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_101: [ If ThreadAPI_Join succeeds: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_102: [ worker_pool_destroy shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_103: [ worker_pool_destroy shall free the worker. ]*/
TEST_FUNCTION(worker_pool_destroy_destroys_the_retired_workers)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_a);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG));
    // worker A wakes up because it is retired
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    // back in worker_pool_destroy
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_a->worker));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(free(worker_pool));

    ///act
    worker_pool_destroy(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(worker_a->has_exited);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_082: [ worker_pool_destroy shall remove the worker from the list of workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_096: [ worker_pool_destroy shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_097: [ If ThreadAPI_Join succeeds: ]*/
TEST_FUNCTION(when_ThreadAPI_Join_fails_worker_pool_destroy_does_not_free_the_worker_from_the_list_of_workers)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    // worker A is not freed, worker B is still destroyed
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_b, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_b->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_b->worker));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(free(worker_pool));

    ///act
    worker_pool_destroy(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    ASSERT_IS_TRUE(worker_b->has_exited);

    ///cleanup
    // the worker that could not be joined is leaked by the pool
    real_gballoc_hl_free(worker_a->worker);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_016: [ For each worker in the list of retired workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_084: [ worker_pool_destroy shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_100: [ worker_pool_destroy shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_101: [ If ThreadAPI_Join succeeds: ]*/
TEST_FUNCTION(when_ThreadAPI_Join_fails_worker_pool_destroy_does_not_free_the_worker_from_the_list_of_retired_workers)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_a);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_b);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_workers_list_head));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    // worker A is not freed, worker B is still destroyed
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_b, IGNORED_ARG));
    // worker B wakes up because it is retired
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    // back in worker_pool_destroy
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_b->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_b->worker));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(free(worker_pool));

    ///act
    worker_pool_destroy(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    ASSERT_IS_TRUE(worker_b->has_exited);

    ///cleanup
    // the worker that could not be joined is leaked by the pool
    real_gballoc_hl_free(worker_a->worker);
}

/* worker_pool_add_worker */

/*Tests_SRS_WORKER_POOL_LINUX_42_018: [ If worker_pool is NULL, worker_pool_add_worker shall fail and return a non-zero value. ]*/
TEST_FUNCTION(worker_pool_add_worker_with_NULL_worker_pool_fails)
{
    ///arrange

    ///act
    int result = worker_pool_add_worker(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_019: [ worker_pool_add_worker shall increment the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_020: [ worker_pool_add_worker shall increment the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_021: [ worker_pool_add_worker shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_031: [ worker_pool_add_worker shall allocate a worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_086: [ worker_pool_add_worker shall store worker_pool in the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_032: [ worker_pool_add_worker shall set the state of the worker to running. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_033: [ worker_pool_add_worker shall initialize the condition variable of the worker by calling pthread_cond_init. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_034: [ worker_pool_add_worker shall start the worker thread by calling ThreadAPI_Create with internal_worker_pool_worker_thread_func and the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_023: [ worker_pool_add_worker shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_028: [ If no error occurs: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_029: [ worker_pool_add_worker shall insert the worker in the list of workers by calling DList_InsertTailList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_030: [ worker_pool_add_worker shall succeed and return 0. ]*/
TEST_FUNCTION(worker_pool_add_worker_succeeds)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    void* allocated_worker;
    PDLIST_ENTRY worker_list_entry;
    setup_worker_pool_add_worker_expectations(&allocated_worker, &worker_list_entry);

    ///act
    int result = worker_pool_add_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, 1, g_test_worker_count);
    ASSERT_ARE_EQUAL(void_ptr, allocated_worker, g_test_workers[0].worker);
    ASSERT_IS_NOT_NULL(g_test_workers[0].thread_func);
    ASSERT_IS_FALSE(g_test_workers[0].is_started);
    ASSERT_ARE_EQUAL(void_ptr, worker_pool, mock_worker_pool_linux_get_worker_pool(allocated_worker));
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_RUNNING", mock_worker_pool_linux_get_worker_state(allocated_worker));
    assert_worker_pool_counts(worker_pool, 1, 1, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_019: [ worker_pool_add_worker shall increment the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_020: [ worker_pool_add_worker shall increment the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_029: [ worker_pool_add_worker shall insert the worker in the list of workers by calling DList_InsertTailList. ]*/
TEST_FUNCTION(worker_pool_add_worker_adds_one_more_worker_thread_each_time)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    void* allocated_worker;
    PDLIST_ENTRY worker_list_entry;
    setup_worker_pool_add_worker_expectations(&allocated_worker, &worker_list_entry);

    ///act
    int result = worker_pool_add_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, 2, g_test_worker_count);
    ASSERT_ARE_NOT_EQUAL(void_ptr, worker_a->worker, allocated_worker);
    ASSERT_ARE_NOT_EQUAL(void_ptr, worker_a->worker_list_entry, worker_list_entry);
    assert_worker_pool_counts(worker_pool, 2, 2, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_024: [ If any error occurs: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_025: [ worker_pool_add_worker shall decrement the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_026: [ worker_pool_add_worker shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_027: [ worker_pool_add_worker shall fail and return a non-zero value. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, worker_pool_add_worker shall free the resources it allocated for the worker. ]*/
TEST_FUNCTION(when_underlying_calls_fail_worker_pool_add_worker_fails)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    (void)test_worker_pool_add_worker(worker_pool);
    void* allocated_worker;
    PDLIST_ENTRY worker_list_entry;
    setup_worker_pool_add_worker_expectations(&allocated_worker, &worker_list_entry);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            int result = worker_pool_add_worker(worker_pool);

            ///assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
            // only the worker added before is counted
            assert_worker_pool_counts(worker_pool, 1, 1, 0, 0);
        }
    }

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_024: [ If any error occurs: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_025: [ worker_pool_add_worker shall decrement the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_026: [ worker_pool_add_worker shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_027: [ worker_pool_add_worker shall fail and return a non-zero value. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, worker_pool_add_worker shall free the resources it allocated for the worker. ]*/
TEST_FUNCTION(when_ThreadAPI_Create_fails_worker_pool_add_worker_destroys_the_condition_variable_and_frees_the_worker)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    (void)test_worker_pool_add_worker(worker_pool);
    void* allocated_worker;
    pthread_cond_t* initialized_cond;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL))
        .CaptureArgumentValue_cond(&initialized_cond);
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_arg(&allocated_worker)
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(IGNORED_ARG))
        .ValidateArgumentValue_cond(&initialized_cond);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));

    ///act
    int result = worker_pool_add_worker(worker_pool);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(size_t, 1, g_test_worker_count);
    assert_worker_pool_counts(worker_pool, 1, 1, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/* internal_worker_pool_worker_thread_func */

/*Tests_SRS_WORKER_POOL_LINUX_42_004: [ worker_pool_create shall store lock, run_function and run_function_context in the pool. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_007: [ worker_pool_create shall mark the pool as not stopped. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_040: [ internal_worker_pool_worker_thread_func shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_041: [ internal_worker_pool_worker_thread_func shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_042: [ Until the worker is retired or the worker stops: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_043: [ internal_worker_pool_worker_thread_func shall call run_function with run_function_context. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_044: [ If run_function returns false: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_046: [ If the pool is not stopped: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_049: [ internal_worker_pool_worker_thread_func shall set the state of the worker to idle. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_050: [ internal_worker_pool_worker_thread_func shall record the current time obtained by calling timer_global_get_elapsed_ms as the time the worker became idle. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_051: [ internal_worker_pool_worker_thread_func shall insert the worker at the head of the list of idle workers by calling DList_InsertHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_088: [ internal_worker_pool_worker_thread_func shall increment the idle thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ internal_worker_pool_worker_thread_func shall wait by calling pthread_cond_wait on the condition variable of the worker while the worker is idle and the pool is not stopped. ]*/
TEST_FUNCTION(internal_worker_pool_worker_thread_func_runs_work_until_there_is_none_and_waits_while_idle)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000);
    STRICT_EXPECTED_CALL(DList_InsertHeadList(g_idle_workers_list_head, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(worker_a->wake_cond, &test_lock));

    ///act
    run_worker_thread(worker_a);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_RUNNING", worker_a->state_in_run_function);
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_IDLE", worker_a->state_in_cond_wait);
    assert_worker_pool_counts(worker_pool, 1, 0, 1, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_043: [ internal_worker_pool_worker_thread_func shall call run_function with run_function_context. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_053: [ If the worker was woken: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_090: [ internal_worker_pool_worker_thread_func shall set the state of the worker to running. ]*/
TEST_FUNCTION(internal_worker_pool_worker_thread_func_runs_work_again_when_woken)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_worker_pool_wake_idle_worker(worker_pool, worker_a);

    // worker_pool_wake_idle_worker removed worker A from the list of idle workers, so worker A does not remove itself
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(2000);
    STRICT_EXPECTED_CALL(DList_InsertHeadList(g_idle_workers_list_head, worker_a->idle_list_entry));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(worker_a->wake_cond, &test_lock));

    ///act
    run_worker_thread(worker_a);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_RUNNING", worker_a->state_in_run_function);
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_IDLE", worker_a->state_in_cond_wait);
    assert_worker_pool_counts(worker_pool, 1, 0, 1, 2000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ internal_worker_pool_worker_thread_func shall wait by calling pthread_cond_wait on the condition variable of the worker while the worker is idle and the pool is not stopped. ]*/
TEST_FUNCTION(internal_worker_pool_worker_thread_func_keeps_waiting_when_woken_while_idle)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);

    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(worker_a->wake_cond, &test_lock));

    ///act
    // spurious wakeup: the worker is still idle and the pool is not stopped
    run_worker_thread(worker_a);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_IDLE", mock_worker_pool_linux_get_worker_state(worker_a->worker));
    assert_worker_pool_counts(worker_pool, 1, 0, 1, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_042: [ Until the worker is retired or the worker stops: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_047: [ internal_worker_pool_worker_thread_func shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_048: [ internal_worker_pool_worker_thread_func shall return 0. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ internal_worker_pool_worker_thread_func shall wait by calling pthread_cond_wait on the condition variable of the worker while the worker is idle and the pool is not stopped. ]*/
TEST_FUNCTION(internal_worker_pool_worker_thread_func_exits_when_the_worker_is_retired)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_a);

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));

    ///act
    run_worker_thread(worker_a);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(worker_a->has_exited);
    ASSERT_ARE_EQUAL(int, 0, worker_a->thread_result);
    assert_worker_pool_counts(worker_pool, 0, 0, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/* worker_pool_wake_idle_worker */

/*Tests_SRS_WORKER_POOL_LINUX_42_055: [ If worker_pool is NULL, worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_INVALID_ARGS. ]*/
TEST_FUNCTION(worker_pool_wake_idle_worker_with_NULL_worker_pool_returns_INVALID_ARGS)
{
    ///arrange

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(NULL);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_056: [ If no worker is idle, worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER. ]*/
TEST_FUNCTION(worker_pool_wake_idle_worker_with_no_idle_worker_returns_NO_IDLE_WORKER)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    (void)test_worker_pool_add_worker(worker_pool);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_006: [ worker_pool_create shall set the thread count, the starting thread count, the idle thread count and the signaled thread count to 0. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_058: [ worker_pool_wake_idle_worker shall remove the worker at the head of the list of idle workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_094: [ worker_pool_wake_idle_worker shall decrement the idle thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_059: [ worker_pool_wake_idle_worker shall set the state of the worker to woken. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_061: [ worker_pool_wake_idle_worker shall wake the worker by calling pthread_cond_signal on its condition variable. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_062: [ worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_WOKEN. ]*/
TEST_FUNCTION(worker_pool_wake_idle_worker_wakes_the_most_recently_idle_worker)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);

    setup_worker_pool_wake_idle_worker_expectations(worker_b);

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_WOKEN", mock_worker_pool_linux_get_worker_state(worker_b->worker));
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_IDLE", mock_worker_pool_linux_get_worker_state(worker_a->worker));
    assert_worker_pool_counts(worker_pool, 2, 0, 1, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_057: [ If the signaled thread count is not 0, worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_ALREADY_WAKING. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_060: [ worker_pool_wake_idle_worker shall increment the signaled thread count. ]*/
TEST_FUNCTION(worker_pool_wake_idle_worker_while_the_woken_worker_did_not_run_yet_returns_ALREADY_WAKING)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);
    test_worker_pool_wake_idle_worker(worker_pool, worker_b);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_ALREADY_WAKING, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_IDLE", mock_worker_pool_linux_get_worker_state(worker_a->worker));
    assert_worker_pool_counts(worker_pool, 2, 0, 1, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_053: [ If the worker was woken: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_089: [ internal_worker_pool_worker_thread_func shall decrement the signaled thread count. ]*/
TEST_FUNCTION(worker_pool_wake_idle_worker_wakes_a_worker_again_after_the_woken_worker_ran)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_worker_pool_wake_idle_worker(worker_pool, worker_a);
    test_run_woken_worker_thread_until_idle(worker_a, 2000);

    setup_worker_pool_wake_idle_worker_expectations(worker_a);

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_WOKEN", mock_worker_pool_linux_get_worker_state(worker_a->worker));

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/* worker_pool_get_counts */

/*Tests_SRS_WORKER_POOL_LINUX_42_063: [ If worker_pool is NULL, worker_pool_get_counts shall return. ]*/
TEST_FUNCTION(worker_pool_get_counts_with_NULL_worker_pool_returns)
{
    ///arrange
    WORKER_POOL_COUNTS counts = { 1, 2, 3, 4 };

    ///act
    worker_pool_get_counts(NULL, &counts);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_counts_are_equal(&counts, 1, 2, 3, 4);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_064: [ If counts is NULL, worker_pool_get_counts shall return. ]*/
TEST_FUNCTION(worker_pool_get_counts_with_NULL_counts_returns)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();

    ///act
    worker_pool_get_counts(worker_pool, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_065: [ worker_pool_get_counts shall store the thread count, the starting thread count and the idle thread count in counts. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_066: [ If no worker is idle, worker_pool_get_counts shall set oldest_idle_since_ms to 0. ]*/
TEST_FUNCTION(worker_pool_get_counts_with_no_idle_worker_returns_0_as_oldest_idle_since_ms)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    (void)test_worker_pool_add_worker(worker_pool);
    WORKER_POOL_COUNTS counts;
    (void)memset(&counts, 0xFF, sizeof(counts));

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));

    ///act
    worker_pool_get_counts(worker_pool, &counts);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_counts_are_equal(&counts, 1, 1, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_065: [ worker_pool_get_counts shall store the thread count, the starting thread count and the idle thread count in counts. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_067: [ If a worker is idle, worker_pool_get_counts shall set oldest_idle_since_ms to the time the worker at the tail of the list of idle workers became idle. ]*/
TEST_FUNCTION(worker_pool_get_counts_returns_the_time_the_worker_idle_the_longest_became_idle)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    (void)test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);
    WORKER_POOL_COUNTS counts;
    (void)memset(&counts, 0xFF, sizeof(counts));

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));

    ///act
    worker_pool_get_counts(worker_pool, &counts);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_counts_are_equal(&counts, 3, 1, 2, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/* worker_pool_retire_oldest_idle_worker */

/*Tests_SRS_WORKER_POOL_LINUX_42_068: [ If worker_pool is NULL, worker_pool_retire_oldest_idle_worker shall return. ]*/
TEST_FUNCTION(worker_pool_retire_oldest_idle_worker_with_NULL_worker_pool_returns)
{
    ///arrange

    ///act
    worker_pool_retire_oldest_idle_worker(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_069: [ If no worker is idle, worker_pool_retire_oldest_idle_worker shall return. ]*/
TEST_FUNCTION(worker_pool_retire_oldest_idle_worker_with_no_idle_worker_returns)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));

    ///act
    worker_pool_retire_oldest_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_RUNNING", mock_worker_pool_linux_get_worker_state(worker_a->worker));
    assert_worker_pool_counts(worker_pool, 1, 1, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_070: [ worker_pool_retire_oldest_idle_worker shall remove the worker at the tail of the list of idle workers by calling DList_RemoveTailList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_095: [ worker_pool_retire_oldest_idle_worker shall decrement the idle thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_071: [ worker_pool_retire_oldest_idle_worker shall set the state of the worker to retired. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_072: [ worker_pool_retire_oldest_idle_worker shall wake the worker by calling pthread_cond_signal on its condition variable. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_073: [ worker_pool_retire_oldest_idle_worker shall decrement the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_074: [ worker_pool_retire_oldest_idle_worker shall remove the worker from the list of workers by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_075: [ worker_pool_retire_oldest_idle_worker shall insert the worker in the list of retired workers by calling DList_InsertTailList. ]*/
TEST_FUNCTION(worker_pool_retire_oldest_idle_worker_retires_the_worker_idle_the_longest)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);

    setup_worker_pool_retire_oldest_idle_worker_expectations(worker_a);

    ///act
    worker_pool_retire_oldest_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_RETIRED", mock_worker_pool_linux_get_worker_state(worker_a->worker));
    ASSERT_ARE_EQUAL(char_ptr, "WORKER_STATE_IDLE", mock_worker_pool_linux_get_worker_state(worker_b->worker));
    assert_worker_pool_counts(worker_pool, 1, 0, 1, 2000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/* worker_pool_join_retired_workers */

/*Tests_SRS_WORKER_POOL_LINUX_42_076: [ If worker_pool is NULL, worker_pool_join_retired_workers shall return. ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_with_NULL_worker_pool_returns)
{
    ///arrange

    ///act
    worker_pool_join_retired_workers(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WORKER_POOL_LINUX_42_077: [ For each worker in the list of retired workers: ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_with_no_retired_worker_returns)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    (void)test_worker_pool_add_worker(worker_pool);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));

    ///act
    worker_pool_join_retired_workers(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_worker_pool_counts(worker_pool, 1, 1, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_077: [ For each worker in the list of retired workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_078: [ worker_pool_join_retired_workers shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_079: [ worker_pool_join_retired_workers shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_104: [ worker_pool_join_retired_workers shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_105: [ If ThreadAPI_Join succeeds: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_106: [ worker_pool_join_retired_workers shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_107: [ worker_pool_join_retired_workers shall free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_081: [ worker_pool_join_retired_workers shall acquire lock by calling pthread_mutex_lock. ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_destroys_the_retired_worker)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_a);
    test_run_retired_worker_thread_until_exit(worker_a);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_a->worker));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));

    ///act
    worker_pool_join_retired_workers(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_worker_pool_counts(worker_pool, 0, 0, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_077: [ For each worker in the list of retired workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_078: [ worker_pool_join_retired_workers shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_079: [ worker_pool_join_retired_workers shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_104: [ worker_pool_join_retired_workers shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_105: [ If ThreadAPI_Join succeeds: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_106: [ worker_pool_join_retired_workers shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_107: [ worker_pool_join_retired_workers shall free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_081: [ worker_pool_join_retired_workers shall acquire lock by calling pthread_mutex_lock. ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_destroys_all_retired_workers)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_a);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_b);
    test_run_retired_worker_thread_until_exit(worker_a);
    test_run_retired_worker_thread_until_exit(worker_b);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_a->worker));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_b, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_b->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_b->worker));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));

    ///act
    worker_pool_join_retired_workers(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_worker_pool_counts(worker_pool, 0, 0, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_104: [ worker_pool_join_retired_workers shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_waits_for_the_retired_worker_thread_to_exit)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_a);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG));
    // worker A wakes up because it is retired and exits
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    // back in worker_pool_join_retired_workers
    STRICT_EXPECTED_CALL(mocked_pthread_cond_destroy(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(free(worker_a->worker));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));

    ///act
    worker_pool_join_retired_workers(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(worker_a->has_exited);
    ASSERT_ARE_EQUAL(int, 0, worker_a->thread_result);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_077: [ For each worker in the list of retired workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_078: [ worker_pool_join_retired_workers shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_079: [ worker_pool_join_retired_workers shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_104: [ worker_pool_join_retired_workers shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_105: [ If ThreadAPI_Join succeeds: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_081: [ worker_pool_join_retired_workers shall acquire lock by calling pthread_mutex_lock. ]*/
TEST_FUNCTION(when_ThreadAPI_Join_fails_worker_pool_join_retired_workers_does_not_free_the_worker)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_worker_pool_retire_oldest_idle_worker(worker_pool, worker_a);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_retired_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(worker_a, IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    // worker A is not freed
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_retired_workers_list_head));

    ///act
    worker_pool_join_retired_workers(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    assert_worker_pool_counts(worker_pool, 0, 0, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
    // the worker that could not be joined is leaked by the pool
    real_gballoc_hl_free(worker_a->worker);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

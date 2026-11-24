// Copyright (C) Microsoft Corporation. All rights reserved.

#include "worker_pool_linux_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);

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
    void* worker; // the arg passed to ThreadAPI_Create
    pthread_cond_t* wake_cond; // the condition variable initialized for the worker
    PDLIST_ENTRY worker_list_entry; // the entry inserted in the list of workers
    PDLIST_ENTRY idle_list_entry; // the entry inserted in the list of idle workers
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
    ASSERT_ARE_EQUAL(int, 0, swapcontext(&waiting_test_worker->context, &g_test_context));
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_cond_signal, pthread_cond_t*, cond)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, bool, test_run_function, void*, context)
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

    if (!test_worker->is_started)
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
    else
    {
        // the worker thread continues where it waited
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

    if (!test_worker->has_exited)
    {
        // the joined worker thread runs until it exits
        run_worker_thread(test_worker);
        ASSERT_IS_TRUE(test_worker->has_exited, "the joined worker thread did not exit");
    }
    else
    {
        // the worker thread already exited
    }

    *res = test_worker->thread_result;
    return THREADAPI_OK;
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
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_arg(allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));
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

static void assert_worker_pool_counts(WORKER_POOL_HANDLE worker_pool, uint32_t expected_thread_count, uint32_t expected_starting_thread_count, uint32_t expected_idle_thread_count, uint64_t expected_oldest_idle_since_ms)
{
    WORKER_POOL_COUNTS counts;
    (void)memset(&counts, 0xFF, sizeof(counts));
    worker_pool_get_counts(worker_pool, &counts);
    ASSERT_ARE_EQUAL(uint32_t, expected_thread_count, counts.thread_count);
    ASSERT_ARE_EQUAL(uint32_t, expected_starting_thread_count, counts.starting_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, expected_idle_thread_count, counts.idle_thread_count);
    ASSERT_ARE_EQUAL(uint64_t, expected_oldest_idle_since_ms, counts.oldest_idle_since_ms);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, hook_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Join, hook_ThreadAPI_Join);

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
    g_test_worker_count = 0;
    g_running_test_worker = NULL;
    g_last_initialized_cond = NULL;
    g_run_function_call_count = 0;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    for (size_t i = 0; i < g_test_worker_count; i++)
    {
        if (g_test_workers[i].stack != NULL)
        {
            real_gballoc_hl_free(g_test_workers[i].stack);
            g_test_workers[i].stack = NULL;
        }
        else
        {
            // the worker thread never ran
        }
    }
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
/*Tests_SRS_WORKER_POOL_LINUX_42_004: [ worker_pool_create shall store lock, run_function and run_function_context in the pool. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_005: [ worker_pool_create shall initialize the list of workers, the list of idle workers and the list of retired workers by calling DList_InitializeListHead. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_006: [ worker_pool_create shall set the thread count, the starting thread count, the idle thread count and the signaled thread count to 0. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_007: [ worker_pool_create shall mark the pool as not stopped. ]*/
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
/*Tests_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers, worker_pool_destroy shall remove it from the list and join and free it. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_016: [ For each worker in the list of retired workers, worker_pool_destroy shall remove it from the list and join and free it. ]*/
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
/*Tests_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers, worker_pool_destroy shall remove it from the list and join and free it. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_017: [ worker_pool_destroy shall free the pool. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_037: [ join_and_free_worker shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_038: [ join_and_free_worker shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_039: [ join_and_free_worker shall free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_040: [ worker_thread_func shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_041: [ worker_thread_func shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_044: [ If run_function returns false: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_045: [ If the pool is stopped, worker_thread_func shall stop. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_047: [ worker_thread_func shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_048: [ worker_thread_func shall return 0. ]*/
TEST_FUNCTION(worker_pool_destroy_joins_a_worker_thread_that_did_not_start_yet)
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
/*Tests_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers, worker_pool_destroy shall remove it from the list and join and free it. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_045: [ If the pool is stopped, worker_thread_func shall stop. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ wait_while_idle shall wait by calling pthread_cond_wait on the condition variable of the worker until the worker is signaled, the worker is retired or the pool is stopped. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_054: [ If the worker is still in the list of idle workers, wait_while_idle shall remove it by calling DList_RemoveEntryList. ]*/
TEST_FUNCTION(worker_pool_destroy_wakes_the_idle_worker_threads_and_joins_them)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);

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
}

/*Tests_SRS_WORKER_POOL_LINUX_42_043: [ worker_thread_func shall call run_function with run_function_context. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_045: [ If the pool is stopped, worker_thread_func shall stop. ]*/
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

/*Tests_SRS_WORKER_POOL_LINUX_42_016: [ For each worker in the list of retired workers, worker_pool_destroy shall remove it from the list and join and free it. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_037: [ join_and_free_worker shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_038: [ join_and_free_worker shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_039: [ join_and_free_worker shall free the worker. ]*/
TEST_FUNCTION(worker_pool_destroy_joins_the_retired_worker_threads)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    worker_pool_retire_oldest_idle_worker(worker_pool);
    umock_c_reset_all_calls();

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
/*Tests_SRS_WORKER_POOL_LINUX_42_022: [ worker_pool_add_worker shall create a worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_023: [ worker_pool_add_worker shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_028: [ If creating the worker succeeds: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_029: [ worker_pool_add_worker shall insert the worker in the list of workers by calling DList_InsertTailList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_030: [ worker_pool_add_worker shall succeed and return 0. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_031: [ create_worker shall allocate a worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_033: [ create_worker shall initialize the condition variable of the worker by calling pthread_cond_init. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_034: [ create_worker shall start the worker thread by calling ThreadAPI_Create with worker_thread_func and the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_035: [ create_worker shall return the worker. ]*/
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

/*Tests_SRS_WORKER_POOL_LINUX_42_024: [ If creating the worker fails: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_025: [ worker_pool_add_worker shall decrement the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_026: [ worker_pool_add_worker shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_027: [ worker_pool_add_worker shall fail and return a non-zero value. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, create_worker shall free the resources it allocated and return NULL. ]*/
TEST_FUNCTION(when_malloc_fails_worker_pool_add_worker_fails)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));

    ///act
    int result = worker_pool_add_worker(worker_pool);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_worker_pool_counts(worker_pool, 0, 0, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_024: [ If creating the worker fails: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_025: [ worker_pool_add_worker shall decrement the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_026: [ worker_pool_add_worker shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_027: [ worker_pool_add_worker shall fail and return a non-zero value. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, create_worker shall free the resources it allocated and return NULL. ]*/
TEST_FUNCTION(when_pthread_cond_init_fails_worker_pool_add_worker_fails)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    void* allocated_worker;

    STRICT_EXPECTED_CALL(mocked_pthread_mutex_unlock(&test_lock));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_init(IGNORED_ARG, NULL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&allocated_worker);
    STRICT_EXPECTED_CALL(mocked_pthread_mutex_lock(&test_lock));

    ///act
    int result = worker_pool_add_worker(worker_pool);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_worker_pool_counts(worker_pool, 0, 0, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_024: [ If creating the worker fails: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_025: [ worker_pool_add_worker shall decrement the thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_026: [ worker_pool_add_worker shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_027: [ worker_pool_add_worker shall fail and return a non-zero value. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, create_worker shall free the resources it allocated and return NULL. ]*/
TEST_FUNCTION(when_ThreadAPI_Create_fails_worker_pool_add_worker_fails)
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

/* worker_thread_func */

/*Tests_SRS_WORKER_POOL_LINUX_42_004: [ worker_pool_create shall store lock, run_function and run_function_context in the pool. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_007: [ worker_pool_create shall mark the pool as not stopped. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_032: [ create_worker shall mark the worker as not signaled, not retired and not idle. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_040: [ worker_thread_func shall acquire lock by calling pthread_mutex_lock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_041: [ worker_thread_func shall decrement the starting thread count. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_042: [ Until the worker is retired or it stops, worker_thread_func shall: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_043: [ worker_thread_func shall call run_function with run_function_context. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_044: [ If run_function returns false: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_046: [ If the pool is not stopped, worker_thread_func shall wait while the worker is idle. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_050: [ wait_while_idle shall record the current time obtained by calling timer_global_get_elapsed_ms as the time the worker became idle. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_051: [ wait_while_idle shall insert the worker at the head of the list of idle workers by calling DList_InsertHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ wait_while_idle shall wait by calling pthread_cond_wait on the condition variable of the worker until the worker is signaled, the worker is retired or the pool is stopped. ]*/
TEST_FUNCTION(worker_thread_func_runs_work_until_there_is_none_and_waits_while_idle)
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
    assert_worker_pool_counts(worker_pool, 1, 0, 1, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_043: [ worker_thread_func shall call run_function with run_function_context. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_049: [ wait_while_idle shall mark the worker as not signaled. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_054: [ If the worker is still in the list of idle workers, wait_while_idle shall remove it by calling DList_RemoveEntryList. ]*/
TEST_FUNCTION(worker_thread_func_runs_work_again_when_woken)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, worker_pool_wake_idle_worker(worker_pool));
    umock_c_reset_all_calls();

    // the worker was removed from the list of idle workers by worker_pool_wake_idle_worker, so it does not remove itself
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(true);
    STRICT_EXPECTED_CALL(test_run_function(TEST_RUN_FUNCTION_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(2000);
    STRICT_EXPECTED_CALL(DList_InsertHeadList(g_idle_workers_list_head, IGNORED_ARG))
        .ValidateArgumentValue_listEntry(&worker_a->idle_list_entry);
    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(worker_a->wake_cond, &test_lock));

    ///act
    run_worker_thread(worker_a);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    assert_worker_pool_counts(worker_pool, 1, 0, 1, 2000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ wait_while_idle shall wait by calling pthread_cond_wait on the condition variable of the worker until the worker is signaled, the worker is retired or the pool is stopped. ]*/
TEST_FUNCTION(worker_thread_func_keeps_waiting_when_woken_without_being_signaled)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);

    STRICT_EXPECTED_CALL(mocked_pthread_cond_wait(worker_a->wake_cond, &test_lock));

    ///act
    run_worker_thread(worker_a);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(worker_a->has_exited);
    assert_worker_pool_counts(worker_pool, 1, 0, 1, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_042: [ Until the worker is retired or it stops, worker_thread_func shall: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_047: [ worker_thread_func shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_048: [ worker_thread_func shall return 0. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_052: [ wait_while_idle shall wait by calling pthread_cond_wait on the condition variable of the worker until the worker is signaled, the worker is retired or the pool is stopped. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_071: [ worker_pool_retire_oldest_idle_worker shall mark the worker as retired. ]*/
TEST_FUNCTION(worker_thread_func_exits_when_the_worker_is_retired)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    worker_pool_retire_oldest_idle_worker(worker_pool);
    umock_c_reset_all_calls();

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

/*Tests_SRS_WORKER_POOL_LINUX_42_058: [ worker_pool_wake_idle_worker shall remove the worker at the head of the list of idle workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_059: [ worker_pool_wake_idle_worker shall mark the worker as signaled. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_060: [ worker_pool_wake_idle_worker shall increment the signaled thread count. ]*/
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

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_b->wake_cond));

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
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
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, worker_pool_wake_idle_worker(worker_pool));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_ALREADY_WAKING, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_worker_pool_counts(worker_pool, 2, 0, 1, 1000);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_053: [ If the worker is signaled, wait_while_idle shall decrement the signaled thread count. ]*/
TEST_FUNCTION(worker_pool_wake_idle_worker_wakes_a_worker_again_after_the_woken_worker_ran)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, worker_pool_wake_idle_worker(worker_pool));
    // worker A runs, finds no work and waits while idle again
    run_worker_thread(worker_a);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_a->wake_cond));

    ///act
    WORKER_POOL_WAKE_RESULT result = worker_pool_wake_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_WOKEN, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

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
    ASSERT_ARE_EQUAL(uint32_t, 1, counts.thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 2, counts.starting_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 3, counts.idle_thread_count);
    ASSERT_ARE_EQUAL(uint64_t, 4, counts.oldest_idle_since_ms);
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
    ASSERT_ARE_EQUAL(uint32_t, 1, counts.thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, counts.starting_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, counts.idle_thread_count);
    ASSERT_ARE_EQUAL(uint64_t, 0, counts.oldest_idle_since_ms);

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
    ASSERT_ARE_EQUAL(uint32_t, 3, counts.thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, counts.starting_thread_count);
    ASSERT_ARE_EQUAL(uint32_t, 2, counts.idle_thread_count);
    ASSERT_ARE_EQUAL(uint64_t, 1000, counts.oldest_idle_since_ms);

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

/*Tests_SRS_WORKER_POOL_LINUX_42_069: [ If no worker is idle, worker_pool_retire_oldest_idle_worker shall log an error and return. ]*/
TEST_FUNCTION(worker_pool_retire_oldest_idle_worker_with_no_idle_worker_returns)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    (void)test_worker_pool_add_worker(worker_pool);

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));

    ///act
    worker_pool_retire_oldest_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    assert_worker_pool_counts(worker_pool, 1, 1, 0, 0);

    ///cleanup
    worker_pool_destroy(worker_pool);
}

/*Tests_SRS_WORKER_POOL_LINUX_42_070: [ worker_pool_retire_oldest_idle_worker shall remove the worker at the tail of the list of idle workers by calling DList_RemoveTailList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_071: [ worker_pool_retire_oldest_idle_worker shall mark the worker as retired. ]*/
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

    STRICT_EXPECTED_CALL(DList_IsListEmpty(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(DList_RemoveTailList(g_idle_workers_list_head));
    STRICT_EXPECTED_CALL(mocked_pthread_cond_signal(worker_a->wake_cond));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(worker_a->worker_list_entry));
    STRICT_EXPECTED_CALL(DList_InsertTailList(g_retired_workers_list_head, worker_a->worker_list_entry));

    ///act
    worker_pool_retire_oldest_idle_worker(worker_pool);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
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

/*Tests_SRS_WORKER_POOL_LINUX_42_037: [ join_and_free_worker shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_038: [ join_and_free_worker shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_039: [ join_and_free_worker shall free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_077: [ For each worker in the list of retired workers: ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_078: [ worker_pool_join_retired_workers shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_079: [ worker_pool_join_retired_workers shall release lock by calling pthread_mutex_unlock. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_080: [ worker_pool_join_retired_workers shall join and free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_081: [ worker_pool_join_retired_workers shall acquire lock by calling pthread_mutex_lock. ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_joins_and_frees_the_retired_worker)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    worker_pool_retire_oldest_idle_worker(worker_pool);
    // worker A exits
    run_worker_thread(worker_a);
    umock_c_reset_all_calls();

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
/*Tests_SRS_WORKER_POOL_LINUX_42_080: [ worker_pool_join_retired_workers shall join and free the worker. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_081: [ worker_pool_join_retired_workers shall acquire lock by calling pthread_mutex_lock. ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_joins_and_frees_all_retired_workers)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    TEST_WORKER* worker_b = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    test_run_worker_thread_until_idle(worker_b, 2000);
    worker_pool_retire_oldest_idle_worker(worker_pool);
    worker_pool_retire_oldest_idle_worker(worker_pool);
    // worker A and worker B exit
    run_worker_thread(worker_a);
    run_worker_thread(worker_b);
    umock_c_reset_all_calls();

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

/*Tests_SRS_WORKER_POOL_LINUX_42_037: [ join_and_free_worker shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
/*Tests_SRS_WORKER_POOL_LINUX_42_080: [ worker_pool_join_retired_workers shall join and free the worker. ]*/
TEST_FUNCTION(worker_pool_join_retired_workers_waits_for_the_retired_worker_thread_to_exit)
{
    ///arrange
    WORKER_POOL_HANDLE worker_pool = test_worker_pool_create();
    TEST_WORKER* worker_a = test_worker_pool_add_worker(worker_pool);
    test_run_worker_thread_until_idle(worker_a, 1000);
    worker_pool_retire_oldest_idle_worker(worker_pool);
    umock_c_reset_all_calls();

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

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

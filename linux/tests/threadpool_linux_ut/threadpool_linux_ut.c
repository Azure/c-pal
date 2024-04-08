// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>
#include <bits/types/__sigval_t.h>         // for __sigval_t
#include <bits/types/sigevent_t.h>         // for sigevent, sigev_notify_fun...
#include <bits/types/sigval_t.h>           // for sigval_t
#include <bits/types/timer_t.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/srw_lock.h"
#include "c_pal/sm.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#undef ENABLE_MOCKS

#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"

#include "real_interlocked.h"
#include "real_gballoc_hl.h"
#include "real_sm.h"
#include "../reals/real_interlocked_hl.h"

#include "c_pal/threadpool.h"

#define DEFAULT_TASK_ARRAY_SIZE 2048
#define MIN_THREAD_COUNT 5
#define MAX_THREAD_COUNT 10
#define MAX_TIMER_INSTANCE_COUNT 64

struct itimerspec;
struct sigevent;
struct timespec;

static EXECUTION_ENGINE_PARAMETERS execution_engine = {MIN_THREAD_COUNT, MAX_THREAD_COUNT};
static SRW_LOCK_HANDLE test_srw_lock = (SRW_LOCK_HANDLE)0x4242;
static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4243;
static THREAD_HANDLE test_thread_handle = (THREAD_HANDLE)0x4200;
static THREAD_START_FUNC g_saved_worker_thread_func;
static void* g_saved_worker_thread_func_context;

THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    g_saved_worker_thread_func = func;
    g_saved_worker_thread_func_context = arg;
    *threadHandle = test_thread_handle;
    return THREADAPI_OK;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, void, mocked_internal_close, THREADPOOL*, threadpool)
    real_gballoc_hl_free(threadpool);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_work_function, void*, parameter)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_get_next_timer_instance, THREADPOOL*, threadpool)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, int, mocked_sem_init, sem_t*, sem, int, pshared, unsigned int, value)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_sem_post, sem_t*, sem)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_sem_timedwait, sem_t*, sem, const struct timespec*, abs_timeout)
MOCK_FUNCTION_END(0)

struct sigevent g_sigevent;

MOCK_FUNCTION_WITH_CODE(, int, mocked_timer_create, clockid_t,clockid, struct sigevent*, sevp, timer_t *, timerid)
    g_sigevent = *sevp;
   timer_create(clockid, sevp, timerid);
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_timer_settime, timer_t, timerid, int, flags, const struct itimerspec* , new_value, struct itimerspec* , old_value)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_timer_delete, timer_t, timerid)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_clock_gettime, clockid_t, clockid, struct timespec*, tp)
MOCK_FUNCTION_END(0)

static void threadpool_create_succeed_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_get_parameters(test_execution_engine));
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_2(DEFAULT_TASK_ARRAY_SIZE, IGNORED_ARG));
    for (int32_t i = 0; i < DEFAULT_TASK_ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(srw_lock_create(false, IGNORED_ARG)).SetReturn(test_srw_lock);
    STRICT_EXPECTED_CALL(mocked_sem_init(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0));
}

static void threadpool_schedule_work_succeed_expectations(void)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
}

static void threadpool_task(void* parameter)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)parameter;

    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static THANDLE(THREADPOOL) test_create_and_open_threadpool()
{
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    umock_c_reset_all_calls();

    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    umock_c_reset_all_calls();

    return threadpool;
}

static void test_create_threadpool_and_start_timer(uint32_t start_delay_ms, uint32_t timer_period_ms, void* work_function_context, THANDLE(THREADPOOL)* threadpool, TIMER_INSTANCE_HANDLE* timer_instance)
{
    THANDLE(THREADPOOL) test_result = test_create_and_open_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(test_result, start_delay_ms, timer_period_ms, test_work_function, work_function_context, timer_instance));
    THANDLE_MOVE(THREADPOOL)(threadpool, &test_result);
    umock_c_reset_all_calls();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(clockid_t, int);
    REGISTER_UMOCK_ALIAS_TYPE(timer_t, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_RETURN(execution_engine_linux_get_parameters, &execution_engine);
    REGISTER_GLOBAL_MOCK_RETURNS(srw_lock_create, test_srw_lock, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_sem_init, 0, -1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_clock_gettime, 0, -1);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_RETURNS(ThreadAPI_Create, THREADAPI_OK, THREADAPI_ERROR);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SM_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_timer_create, 0, 1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_timer_settime, 0, -1);

    REGISTER_TYPE(SM_RESULT, SM_RESULT);

    REGISTER_UMOCKC_PAIRED_CREATE_DESTROY_CALLS(sm_create, sm_destroy);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
    real_gballoc_ll_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");
    g_saved_worker_thread_func = NULL;
    g_saved_worker_thread_func_context = NULL;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* threadpool_create */

/* Tests_SRS_THREADPOOL_LINUX_07_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_create_with_NULL_execution_engine_fails)
{
    // arrange

    // act
    THANDLE(THREADPOOL) threadpool = threadpool_create(NULL);

    // assert
    ASSERT_IS_NULL(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_07_001: [ threadpool_create shall allocate memory for a threadpool object and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_004: [ threadpool_create shall get the min_thread_count and max_thread_count thread parameters from the execution_engine. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_005: [ threadpool_create shall allocate memory for an array of thread handles of size min_thread_count and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_003: [ threadpool_create shall create a SM_HANDLE by calling sm_create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_006: [ threadpool_create shall allocate memory with default task array size 2048 for an array of tasks and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_007: [ threadpool_create shall initialize every task item in the tasks array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_009: [ threadpool_create shall create a shared semaphore with initialized value zero. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_008: [ threadpool_create shall create a SRW lock by calling srw_lock_create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_010: [ insert_idx and consume_idx for the task array shall be initialized to 0. ]*/
TEST_FUNCTION(threadpool_create_succeeds)
{
    //arrange
    threadpool_create_succeed_expectations();

    //act
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

}

/* Tests_SRS_THREADPOOL_LINUX_07_011: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_also_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_get_parameters(test_execution_engine))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));
    for (int32_t i = 0; i < DEFAULT_TASK_ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
            .CallCannotFail();
    }
    STRICT_EXPECTED_CALL(srw_lock_create(false, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_init(IGNORED_ARG, 0, 0));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if(umock_c_negative_tests_can_call_fail(i))
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

/* Tests_SRS_THREADPOOL_LINUX_07_001: [ threadpool_create shall allocate memory for a threadpool object and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_004: [ threadpool_create shall get the min_thread_count and max_thread_count thread parameters from the execution_engine. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_005: [ threadpool_create shall allocate memory for an array of thread handles of size min_thread_count and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_003: [ threadpool_create shall create a SM_HANDLE by calling sm_create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_006: [ threadpool_create shall allocate memory with default task array size 2048 for an array of tasks and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_007: [ threadpool_create shall initialize every task item in the tasks array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_009: [ threadpool_create shall create a shared semaphore with initialized value zero. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_008: [ threadpool_create shall create a SRW lock by calling srw_lock_create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_010: [ insert_idx and consume_idx for the task array shall be initialized to 0. ]*/
TEST_FUNCTION(creating_2_threadpool_succeeds)
{
    // arrange
    threadpool_create_succeed_expectations();
    threadpool_create_succeed_expectations();

    // act
    THANDLE(THREADPOOL) threadpool_1 = threadpool_create(test_execution_engine);
    THANDLE(THREADPOOL) threadpool_2 = threadpool_create(test_execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool_1);
    ASSERT_IS_NOT_NULL(threadpool_2);
    ASSERT_ARE_NOT_EQUAL(void_ptr, threadpool_1, threadpool_2);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool_1, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool_2, NULL);
}

/* threadpool_destroy */


/* Tests_SRS_THREADPOOL_LINUX_07_016: [ threadpool_destroy shall free the memory allocated in threadpool_create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_014: [ threadpool_destroy shall destroy the semphore by calling sem_destroy. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_015: [ threadpool_destroy shall destroy the SRW lock by calling srw_lock_destroy. ]*/
TEST_FUNCTION(threadpool_destroy_frees_resources)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_07_013: [ threadpool_destroy shall perform an implicit close if threadpool is open. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_016: [ threadpool_destroy shall free the memory allocated in threadpool_create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_014: [ threadpool_destroy shall destroy the semphore by calling sem_destroy. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_015: [ threadpool_destroy shall destroy the SRW lock by calling srw_lock_destroy. ]*/
TEST_FUNCTION(threadpool_destroy_performs_an_implicit_close)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWakeAll(IGNORED_ARG, 1));
    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* threadpool_open */

/* Tests_SRS_THREADPOOL_LINUX_07_017: [ If threadpool is NULL, threadpool_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_with_NULL_threadpool_fails)
{
    // arrange
    int result;

    // act
    result = threadpool_open(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_07_018: [ threadpool_open shall call sm_open_begin. ]*/
// Tests_SRS_THREADPOOL_LINUX_11_001: [ threadpool_open shall initialize internal threapool data items ]
/* Tests_SRS_THREADPOOL_LINUX_07_020: [ threadpool_open shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_023: [ Otherwise, threadpool_open shall shall call sm_open_end with true for success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_024: [ threadpool_open shall succeed, indicate open success to the user by calling the on_open_complete callback with THREADPOOL_OPEN_OK and return zero. ]*/
TEST_FUNCTION(threadpool_open_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    for (int32_t i = 0; i < DEFAULT_TASK_ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0));

    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    // act
    result = threadpool_open(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_073: [ If param is NULL, threadpool_work_func shall fail and return. ]*/
TEST_FUNCTION(threadpool_work_func_parameter_NULL_succeeds)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    umock_c_reset_all_calls();

    //act
    g_saved_worker_thread_func(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_074: [ threadpool_work_func shall get the real time by calling clock_gettime to set the waiting time for semaphore. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_088: [ If clock_gettime fails, threadpool_work_func shall run the loop again. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds_when_clock_gettime_fails)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG)).SetReturn(-1);
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_087: [ If sem_timedwait fails, threadpool_work_func shall timeout and run the loop again. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds_when_sem_timedwait_fails)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_timedwait(IGNORED_ARG, IGNORED_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_074: [ threadpool_work_func shall get the real time by calling clock_gettime to set the waiting time for semaphore. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_075: [ threadpool_work_func shall wait on the semaphore with a time limit. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_076: [ threadpool_work_func shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_077: [ threadpool_work_func shall get the current task array size by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_078: [ threadpool_work_func shall increment the current consume index by calling interlocked_increment_64. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_079: [ threadpool_work_func shall get the next waiting task consume index from incremented consume index modulo current task array size. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_080: [ If consume index has task state TASK_WAITING, threadpool_work_func shall set the task state to TASK_WORKING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_081: [ threadpool_work_func shall copy the function and parameter to local variables. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_082: [ threadpool_work_func shall set the task state to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_083: [ threadpool_work_func shall release the shared SRW lock by calling srw_lock_release_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_084: [ If the work item function is not NULL, threadpool_work_func shall execute it with work_function_ctx. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_085: [ threadpool_work_func shall loop until threadpool_close or threadpool_destroy is called. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    threadpool_schedule_work(threadpool, test_work_function, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_timedwait(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_work_function(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_018: [ threadpool_open shall call sm_open_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_020: [ threadpool_open shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_022: [ If one of the thread creation fails, threadpool_open shall fail and return a non-zero value, terminate all threads already created. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_021: [ If any error occurs, threadpool_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_fails_when_threadAPI_create_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    for (int32_t i = 0; i < DEFAULT_TASK_ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0));

    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    // act
    result = threadpool_open(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_018: [ threadpool_open shall call sm_open_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_019: [ If sm_open_begin indicates the open cannot be performed, threadpool_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_after_open_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    int result;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));

    // act
    result = threadpool_open(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_close */

/* Tests_SRS_THREADPOOL_LINUX_07_025: [ If threadpool is NULL, threadpool_close shall fail and return. ]*/
TEST_FUNCTION(threadpool_close_with_NULL_handle_returns)
{
    // arrange

    // act
    threadpool_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_07_026: [ Otherwise, threadpool_close shall call sm_close_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_089: [ threadpool_close shall signal all threads threadpool is closing by calling InterlockedHL_SetAndWakeAll. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_027: [ threadpool_close shall join all threads in the threadpool. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_028: [ threadpool_close shall call sm_close_end. ]*/
TEST_FUNCTION(threadpool_close_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWakeAll(IGNORED_ARG, IGNORED_ARG));
    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_026: [ Otherwise, threadpool_close shall call sm_close_begin. ]*/
TEST_FUNCTION(threadpool_close_when_not_open_returns)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_026: [ Otherwise, threadpool_close shall call sm_close_begin. ]*/
TEST_FUNCTION(threadpool_close_after_close_returns)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    threadpool_close(threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_schedule_work */

/* Tests_SRS_THREADPOOL_LINUX_07_029: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_with_NULL_threadpool_fails)
{
    // arrange
    int result;

    // act
    result = threadpool_schedule_work(NULL, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_07_030: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    // act
    result = threadpool_schedule_work(threadpool, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_033: [ threadpool_schedule_work shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_034: [ threadpool_schedule_work shall increment the insert_pos. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_035: [ If task state is TASK_NOT_USED, threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_049: [ threadpool_schedule_work shall copy the work function and work function context into insert position in the task array and return zero on success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_050: [ threadpool_schedule_work shall set the task_state to TASK_WAITING and then release the shared SRW lock. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_051: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_047: [ threadpool_schedule_work shall return zero on success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_053: [ threadpool_schedule_work shall call sm_exec_end. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();
    int result;

    threadpool_schedule_work_succeed_expectations();

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_033: [ threadpool_schedule_work shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_034: [ threadpool_schedule_work shall increment the insert_pos. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_035: [ If task state is TASK_NOT_USED, threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_049: [ threadpool_schedule_work shall copy the work function and work function context into insert position in the task array and return zero on success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_050: [ threadpool_schedule_work shall set the task_state to TASK_WAITING and then release the shared SRW lock. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_051: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_047: [ threadpool_schedule_work shall return zero on success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_053: [ threadpool_schedule_work shall call sm_exec_end. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds_with_NULL_work_function_context)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();
    int result;

    threadpool_schedule_work_succeed_expectations();

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_032: [ If sm_exec_begin returns SM_EXEC_REFUSED, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_fails_when_threadpool_not_open)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_033: [ threadpool_schedule_work shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_034: [ threadpool_schedule_work shall increment the insert_pos. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_035: [ If task state is TASK_NOT_USED, threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_036: [ Otherwise, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity: ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_037: [ threadpool_schedule_work shall acquire the SRW lock in exclusive mode by calling srw_lock_acquire_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_038: [ threadpool_schedule_work shall get the current size of task array by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_040: [ Otherwise, threadpool_schedule_work shall double the current task array size. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_041: [ threadpool_schedule_work shall realloc the memory used for the array items. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_043: [ threadpool_schedule_work shall initialize every task item in the new task array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_044: [ threadpool_schedule_work shall shall memmove everything between the consume index and the size of the array before resize to the end of the new resized array. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_045: [ threadpool_schedule_work shall reset the consume_idx and insert_idx to 0 after resize the task array. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_046: [ threadpool_schedule_work shall release the SRW lock by calling srw_lock_release_exclusive. ]*/
TEST_FUNCTION(threadpool_schedule_work_realloc_array_with_no_empty_space)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();
    int result;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(0);

    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG)).IgnoreAllCalls();
    STRICT_EXPECTED_CALL(interlocked_add_64(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_add_64(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}
/* Tests_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_033: [ threadpool_schedule_work shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_034: [ threadpool_schedule_work shall increment the insert_pos. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_035: [ If task state is TASK_NOT_USED, threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_036: [ Otherwise, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity: ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_037: [ threadpool_schedule_work shall acquire the SRW lock in exclusive mode by calling srw_lock_acquire_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_038: [ threadpool_schedule_work shall get the current size of task array by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_039: [ If there is any overflow computing the new size, threadpool_schedule_work shall fail and return a non-zero value . ]*/
TEST_FUNCTION(threadpool_schedule_work_fails_when_new_array_size_overflows)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();
    int result;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(0);

    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(INT32_MAX/2+1);
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_033: [ threadpool_schedule_work shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_034: [ threadpool_schedule_work shall increment the insert_pos. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_035: [ If task state is TASK_NOT_USED, threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_036: [ Otherwise, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity: ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_048: [ If reallocating the task array fails, threadpool_schedule_work shall fail and return a non-zero value. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_042: [ If any error occurs, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_fails_when_realloc_array_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();
    int result;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(0);

    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(realloc_2(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_start */

/* Tests_SRS_THREADPOOL_LINUX_07_054: [ If threadpool is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_threadpool_fails)
{
    // arrange
    TIMER_INSTANCE_HANDLE timer_instance;

    // act
    int result = threadpool_timer_start(NULL, 42, 2000, test_work_function, (void*)0x4243, &timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_07_055: [ If work_function is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();
    TIMER_INSTANCE_HANDLE timer_instance;

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, NULL, (void*)0x4243, &timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_056: [ If timer_handle is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_timer_handle_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_011: [ threadpool_timer_start shall call interlocked_exchange to set the timer_work_guard to OK_TO_WORK. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_059: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_061: [ threadpool_timer_start shall return and allocated handle in timer_handle. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_062: [ threadpool_timer_start shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_start_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, &timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(timer_instance);

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_011: [ threadpool_timer_start shall call interlocked_exchange to set the timer_work_guard to OK_TO_WORK. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_059: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_061: [ threadpool_timer_start shall return and allocated handle in timer_handle. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_062: [ threadpool_timer_start shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_context_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, NULL, &timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(timer_instance);

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_011: [ threadpool_timer_start shall call interlocked_exchange to set the timer_work_guard to OK_TO_WORK. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_059: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_063: [ If timer_settime fails, threadpool_timer_start shall delete the timer by calling timer_delete. ]*/
TEST_FUNCTION(threadpool_timer_start_delete_timer_when_timer_set_time_functions_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL)).SetReturn(-1);
    STRICT_EXPECTED_CALL(mocked_timer_delete(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, &timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_060: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_fails_when_underlying_functions_fail)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();
    TIMER_INSTANCE_HANDLE timer_instance;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, &timer_instance);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_restart */

/* Tests_SRS_THREADPOOL_LINUX_07_064: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_restart_with_NULL_timer_fails)
{
    // arrange

    // act
    int result = threadpool_timer_restart(NULL, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_07_065: [ threadpool_timer_restart shall call timer_settime to change the delay and period. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_067: [ threadpool_timer_restart shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_restart_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &timer_instance);

    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    int result = threadpool_timer_restart(timer_instance, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_066: [ If timer_settime fails, threadpool_timer_restart shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_restart_fails_when_timer_set_time_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &timer_instance);

    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL)).SetReturn(-1);

    // act
    int result = threadpool_timer_restart(timer_instance, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_cancel */

/* Tests_SRS_THREADPOOL_LINUX_07_068: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
TEST_FUNCTION(threadpool_timer_cancel_with_NULL_timer_fails)
{
    // arrange

    // act
    threadpool_timer_cancel(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_07_069: [ threadpool_timer_cancel shall call timer_settime with 0 for flags and NULL for old_value and {0} for new_value to cancel the ongoing timers. ]*/
TEST_FUNCTION(threadpool_timer_cancel_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &timer_instance);

    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    threadpool_timer_cancel(timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_destroy */

/* Tests_SRS_THREADPOOL_LINUX_07_070: [ If timer is NULL, threadpool_timer_destroy shall fail and return. ]*/
TEST_FUNCTION(threadpool_timer_destroy_with_NULL_timer_fails)
{
    // arrange

    // act
    threadpool_timer_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_07_071: [ threadpool_timer_cancel shall call timer_delete to destroy the ongoing timers. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_072: [ threadpool_timer_destroy shall free all resources in timer. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_012: [ threadpool_timer_cancel shall call ThreadAPI_Sleep to allow timer resources to clean up. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_007: [ Until timer_work_guard can be set to TIMER_DELETING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_008: [ threadpool_timer_destroy shall call InterlockedHL_WaitForNotValue to wait until timer_work_guard is not TIMER_WORKING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_009: [ threadpool_timer_destroy shall call interlocked_add to add 0 to timer_work_guard to get current value of timer_work_guard. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_010: [ threadpool_timer_destroy shall call interlocked_compare_exchange on timer_work_guard with the current value of timer_work_guard as the comparison and TIMER_DELETING as the exchange. ]*/
TEST_FUNCTION(threadpool_timer_destroy_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &timer_instance);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(IGNORED_ARG, IGNORED_ARG, UINT32_MAX));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG,IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_delete(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    threadpool_timer_destroy(timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_45_002: [ on_timer_callback shall set the timer instance to timer_data.sival_ptr. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_001: [ If timer instance is NULL, then on_timer_callback shall return. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_003: [ on_timer_callback shall call interlocked_compare_exchange with the timer_work_guard of this timer instance with OK_TO_WORK as the comparison, and TIMER_WORKING as the exchange. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_004: [ If timer_work_guard is successfully set to TIMER_WORKING, then on_timer_callback shall call the timer's work_function with work_function_ctx. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_005: [ on_timer_callback shall call interlocked_compare_exchange with the timer_work_guard of this timer instance with TIMER_WORKING as the comparison, and OK_TO_WORK as the exchange. ]*/
TEST_FUNCTION(on_timer_callback_calls_work_function)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, &timer_instance);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(timer_instance);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    sigval_t timer_data = {0};
    timer_data.sival_ptr = timer_instance;

    // act
    g_sigevent.sigev_notify_function(timer_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_45_002: [ on_timer_callback shall set the timer instance to timer_data.sival_ptr. ]*/
/* Tests_SRS_THREADPOOL_LINUX_45_001: [ If timer instance is NULL, then on_timer_callback shall return. ]*/
TEST_FUNCTION(on_timer_callback_does_nothing_with_null_pointer)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, &timer_instance);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(timer_instance);
    umock_c_reset_all_calls();

    sigval_t timer_data = {0};
    timer_data.sival_ptr = NULL;

    // act
    g_sigevent.sigev_notify_function(timer_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <semaphore.h>
#include <time.h>
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
#include "c_pal/sync.h"
#include "c_pal/srw_lock.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#undef ENABLE_MOCKS

#include "real_interlocked.h"
#include "real_gballoc_hl.h"

#include "c_pal/threadpool.h"

#define DEFAULT_TASK_ARRAY_SIZE 2048
#define MIN_THREAD_COUNT 5
#define MAX_THREAD_COUNT 10

struct itimerspec;
struct sigevent;
struct timespec;

static TEST_MUTEX_HANDLE test_serialize_mutex;
static EXECUTION_ENGINE_PARAMETERS_LINUX execution_engine = {MIN_THREAD_COUNT, MAX_THREAD_COUNT};
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

TEST_DEFINE_ENUM_TYPE(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, void, mocked_internal_close, THREADPOOL_HANDLE, threadpool)
    real_gballoc_hl_free(threadpool);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_on_open_complete, void*, context, THREADPOOL_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_threadpool_work_func, void*, context, THREADPOOL_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_work_function, void*, parameter)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, int, mocked_sem_init, sem_t*, sem, int, pshared, unsigned int, value)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_sem_post, sem_t*, sem)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_sem_timedwait, sem_t*, sem, const struct timespec*, abs_timeout)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_timer_create, clockid_t,clockid, struct sigevent*, sevp, timer_t *, timerid)
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
    STRICT_EXPECTED_CALL(execution_engine_linux_get_parameters(test_execution_engine));
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_2(DEFAULT_TASK_ARRAY_SIZE, IGNORED_ARG));
    for (int32_t i = 0; i < DEFAULT_TASK_ARRAY_SIZE; i++)
    {
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(srw_lock_create(false, IGNORED_ARG)).SetReturn(test_srw_lock);
    STRICT_EXPECTED_CALL(mocked_sem_init(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, -1));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, -1));
}

static void threadpool_schedule_work_succeed_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
}

static void on_threadpool_open_complete(void* context, THREADPOOL_OPEN_RESULT open_result)
{
    (void)context;
    ASSERT_ARE_EQUAL(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_OK, open_result);
}

static void threadpool_task(void* parameter)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)parameter;

    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static THREADPOOL_HANDLE test_create_and_open_threadpool()
{
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    umock_c_reset_all_calls();

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242));
    umock_c_reset_all_calls();

    return threadpool;
}

static void test_create_threadpool_and_start_timer(uint32_t start_delay_ms, uint32_t timer_period_ms, void* work_function_context, THREADPOOL_HANDLE* threadpool, TIMER_INSTANCE_HANDLE* timer_instance)
{
    *threadpool = test_create_and_open_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(*threadpool, start_delay_ms, timer_period_ms, test_work_function, work_function_context, timer_instance));
    umock_c_reset_all_calls();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(clockid_t, int);
    REGISTER_UMOCK_ALIAS_TYPE(timer_t, void*);
    
    REGISTER_GLOBAL_MOCK_RETURN(execution_engine_linux_get_parameters, &execution_engine);
    REGISTER_GLOBAL_MOCK_RETURNS(srw_lock_create, test_srw_lock, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_sem_init, 0, -1);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_RETURNS(ThreadAPI_Create, THREADAPI_OK, THREADAPI_ERROR);

    REGISTER_TYPE(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_timer_create, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_timer_settime, -1);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
    real_gballoc_ll_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");
    g_saved_worker_thread_func = NULL;
    g_saved_worker_thread_func_context = NULL;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/* threadpool_create */

/* Tests_SRS_THREADPOOL_LINUX_11_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_create_with_NULL_execution_engine_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool;

    // act
    threadpool = threadpool_create(NULL);

    // assert
    ASSERT_IS_NULL(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_001: [ threadpool_create shall allocate memory for a threadpool object and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_003: [ threadpool_create shall initialize thread count parameters by calling execution_engine_linux_get_parameters with parameter execution_engine. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_004: [ threadpool_create shall allocate memory for an array of thread objects and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_005: [ threadpool_create shall allocate memory for an array of tasks and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_006: [ threadpool_create shall initialize every task item in the tasks array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_007: [ threadpool_create shall create a shared semaphore between threads with initialized value zero. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_008: [ threadpool_create shall initilize the state to THREADPOOL_STATE_NOT_OPEN. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_009: [ insert_idx and consume_idx shall be intialzied to -1 to make the first increment start at zero. ]*/
TEST_FUNCTION(threadpool_create_succeeds)
{
    //arrange
    THREADPOOL_HANDLE threadpool;
    threadpool_create_succeed_expectations();

    //act
    threadpool = threadpool_create(test_execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool);

    // cleanup
    threadpool_destroy(threadpool);
   
}

/* Tests_SRS_THREADPOOL_LINUX_11_010: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_also_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
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
            threadpool = threadpool_create(test_execution_engine);

            // assert
            ASSERT_IS_NULL(threadpool, "On failed call %zu", i);
        }
    }
}

/* Tests_SRS_THREADPOOL_LINUX_11_001: [ threadpool_create shall allocate memory for a threadpool object and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_003: [ threadpool_create shall initialize thread count parameters by calling execution_engine_linux_get_parameters with parameter execution_engine. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_004: [ threadpool_create shall allocate memory for an array of thread objects and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_005: [ threadpool_create shall allocate memory for an array of tasks and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_006: [ threadpool_create shall initialize every task item in the tasks array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_007: [ threadpool_create shall create a shared semaphore between threads with initialized value zero. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_008: [ threadpool_create shall initilize the state to THREADPOOL_STATE_NOT_OPEN. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_009: [ insert_idx and consume_idx shall be intialzied to -1 to make the first increment start at zero. ]*/
TEST_FUNCTION(creating_2_threadpool_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool_1;
    THREADPOOL_HANDLE threadpool_2;

    threadpool_create_succeed_expectations();
    threadpool_create_succeed_expectations();

    // act
    threadpool_1 = threadpool_create(test_execution_engine);
    threadpool_2 = threadpool_create(test_execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool_1);
    ASSERT_IS_NOT_NULL(threadpool_2);
    ASSERT_ARE_NOT_EQUAL(void_ptr, threadpool_1, threadpool_2);

    // cleanup
    threadpool_destroy(threadpool_1);
    threadpool_destroy(threadpool_2);
}

/* threadpool_destroy */

/* Tests_SRS_THREADPOOL_LINUX_11_011: [ If threadpool is NULL, threadpool_destroy shall return. ]*/
TEST_FUNCTION(threadpool_destroy_with_NULL_threadpool_returns)
{
    // arrange

    // act
    threadpool_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_11_013: [ threadpool_destroy shall free the resources associated with the threadpool handle. ]*/ 
/* Tests_SRS_THREADPOOL_LINUX_11_014: [ threadpool_destroy shall destroy the semphore by calling sem_destroy. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_015: [ threadpool_destroy shall destroy the SRW lock by calling srw_lock_destroy. ]*/
TEST_FUNCTION(threadpool_destroy_frees_resources)
{
    // arrange
    THREADPOOL_HANDLE threadpool;
    threadpool = threadpool_create(test_execution_engine);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(threadpool));

    // act
    threadpool_destroy(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_11_012: [ threadpool_destroy shall implicit close if threadpool state is set to THREADPOOL_STATE_OPEN. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_013: [ threadpool_destroy shall free the resources associated with the threadpool handle. ]*/ 
/* Tests_SRS_THREADPOOL_LINUX_11_014: [ threadpool_destroy shall destroy the semphore by calling sem_destroy. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_015: [ threadpool_destroy shall destroy the SRW lock by calling srw_lock_destroy. ]*/
TEST_FUNCTION(threadpool_destroy_performs_an_implicit_close)
{
    // arrange
    THREADPOOL_HANDLE threadpool;
    threadpool = threadpool_create(test_execution_engine);
    threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(threadpool));

    // act
    threadpool_destroy(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* threadpool_open_async */

/* Tests_SRS_THREADPOOL_LINUX_11_016: [ If threadpool is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_async_with_NULL_threadpool_fails)
{
    // arrange
    int result;

    // act
    result = threadpool_open_async(NULL, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_11_074: [ If on_open_complete is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_async_with_NULL_on_open_complete_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    // act
    result = threadpool_open_async(threadpool, NULL, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_017: [ threadpool_open_async shall set the state to THREADPOOL_STATE_OPENING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_019: [ threadpool_open_async shall create the threads for threadpool using ThreadAPI_Create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_021: [ Otherwise, threadpool_open_async shall set the state to THREADPOOL_STATE_OPEN, indicate open success to the user by calling the on_open_complete callback with THREADPOOL_OPEN_OK and return zero. ]*/
TEST_FUNCTION(threadpool_open_async_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, threadpool));
    }
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_open_complete((void*)0x4242, THREADPOOL_OPEN_OK));

    // act
    result = threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_017: [ threadpool_open_async shall set the state to THREADPOOL_STATE_OPENING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_019: [ threadpool_open_async shall create the threads for threadpool using ThreadAPI_Create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_021: [ Otherwise, threadpool_open_async shall set the state to THREADPOOL_STATE_OPEN, indicate open success to the user by calling the on_open_complete callback with THREADPOOL_OPEN_OK and return zero. ]*/
TEST_FUNCTION(threadpool_open_async_succeeds_with_NULL_context)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, threadpool));
    }
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_open_complete(NULL, THREADPOOL_OPEN_OK));

    // act
    result = threadpool_open_async(threadpool, test_on_open_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_055: [ If param is NULL, threadpool_work_func shall return. ]*/
TEST_FUNCTION(threadpool_work_func_parameter_NULL_succeeds)
{
    //arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242));
    umock_c_reset_all_calls();

    //act
    g_saved_worker_thread_func(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_056: [ threadpool_work_func shall get the real time by calling clock_gettime. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_080: [ If clock_gettime fails, threadpool_work_func shall return. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds_when_clock_gettime_fails)
{
    //arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242));
    umock_c_reset_all_calls();
    
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG)).SetReturn(-1);
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(4);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_081: [ If sem_timedwait fails, threadpool_work_func shall timeout and return. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds_when_sem_timedwait_fails)
{
    //arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242));
    umock_c_reset_all_calls();
    
    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_timedwait(IGNORED_ARG, IGNORED_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(4);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_056: [ threadpool_work_func shall get the real time by calling clock_gettime. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_057: [ threadpool_work_func shall decrement the threadpool semaphore with a time limit for 2 seconds. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_058: [ threadpool_work_func shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_059: [ threadpool_work_func shall get the current task array size and next waiting task consume index. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_060: [ If consume index has task state TASK_WAITING, threadpool_work_func shall set the task state to TASK_WORKING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_061: [ threadpool_work_func shall initialize task_func and task_param and then set the task state to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_062: [ threadpool_work_func shall release the shared SRW lock by calling srw_lock_release_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_063: [ If task_param is not NULL, threadpool_work_func shall execute it with parameter task_param. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds)
{
    //arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242));
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
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(4);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_017: [ threadpool_open_async shall set the state to THREADPOOL_STATE_OPENING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_019: [ threadpool_open_async shall create the threads for threadpool using ThreadAPI_Create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_020: [ If one of the thread creation fails, threadpool_open_async shall fail and return a non-zero value, terminate all threads already created, indicate an error to the user by calling the on_open_complete callback with THREADPOOL_OPEN_ERROR and set threadpool state to THREADPOOL_STATE_NOT_OPEN. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_076: [ If ThreadAPI_Create fails, threadpool_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_async_fails_when_threadAPI_create_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, threadpool));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, threadpool))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_open_complete(NULL, THREADPOOL_OPEN_ERROR));

    // act
    result = threadpool_open_async(threadpool, test_on_open_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_017: [ threadpool_open_async shall set the state to THREADPOOL_STATE_OPENING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_018: [ If threadpool has already been opened, threadpool_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_async_after_open_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    int result;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    result = threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* threadpool_close */

/* Tests_SRS_THREADPOOL_LINUX_11_022: [ If threadpool is NULL, threadpool_close shall fail and return. ]*/
TEST_FUNCTION(threadpool_close_with_NULL_handle_returns)
{
    // arrange

    // act
    threadpool_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_11_023: [ Otherwise, threadpool_close shall terminate all threads in the threadpool and set the state to THREADPOOL_STATE_NOT_OPEN. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_078: [ internal_close shall wait and close every thread in threadpool by calling ThreadAPI_Join. ]*/
TEST_FUNCTION(threadpool_close_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_077: [ If current_state is not THREADPOOL_STATE_NOT_OPEN, internal_close shall return. ]*/
TEST_FUNCTION(threadpool_close_when_not_open_returns)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_077: [ If current_state is not THREADPOOL_STATE_NOT_OPEN, internal_close shall return. ]*/
TEST_FUNCTION(threadpool_close_after_close_returns)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    threadpool_close(threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* threadpool_schedule_work */

/* Tests_SRS_THREADPOOL_LINUX_11_024: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
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

/* Tests_SRS_THREADPOOL_LINUX_11_025: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_with_NULL_work_function_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    // act
    result = threadpool_schedule_work(threadpool, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_026: [ threadpool_schedule_work shall get the threadpool state by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_028: [ threadpool_schedule_work shall increment the count of pending call that are in progress to be executed. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_029: [ threadpool_schedule_work shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_030: [ threadpool_schedule_work shall increment the insert_pos by one. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_031: [ threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_034: [ threadpool_schedule_work shall obtain task information in next available task array index and return zero on success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_035: [ threadpool_schedule_work shall set the task_state to TASK_WAITING and then release the shared SRW lock. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_036: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_037: [ threadpool_schedule_work shall decrement the count of pending call that are in progress to be executed. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();
    int result;

    threadpool_schedule_work_succeed_expectations();

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_026: [ threadpool_schedule_work shall get the threadpool state by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_028: [ threadpool_schedule_work shall increment the count of pending call that are in progress to be executed. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_029: [ threadpool_schedule_work shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_030: [ threadpool_schedule_work shall increment the insert_pos by one. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_031: [ threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_034: [ threadpool_schedule_work shall obtain task information in next available task array index and return zero on success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_035: [ threadpool_schedule_work shall set the task_state to TASK_WAITING and then release the shared SRW lock. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_036: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_037: [ threadpool_schedule_work shall decrement the count of pending call that are in progress to be executed. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds_with_NULL_work_function_context)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();
    int result;

    threadpool_schedule_work_succeed_expectations();

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_026: [ threadpool_schedule_work shall get the threadpool state by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_027: [ threadpool_schedule_work shall validate that it's state is THREADPOOL_STATE_OPEN and if not shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_fails_when_threadpool_not_open)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_026: [ threadpool_schedule_work shall get the threadpool state by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_028: [ threadpool_schedule_work shall increment the count of pending call that are in progress to be executed. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_029: [ threadpool_schedule_work shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_030: [ threadpool_schedule_work shall increment the insert_pos by one. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_031: [ threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_032: [ If task has been already initialized, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity by calling reallocate_threadpool_array. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_064: [ reallocate_threadpool_array shall acquire the SRW lock in exclusive mode by calling srw_lock_acquire_exclusive. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_065: [ reallocate_threadpool_array shall get the current size of task array. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_067: [ Otherwise, reallocate_threadpool_array shall double the current task array size and return zero in success. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_068: [ reallocate_threadpool_array shall realloc the memory used for the doubled array items and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_070: [ reallocate_threadpool_array shall initialize every task item in the new task array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_071: [ reallocate_threadpool_array shall remove any gap in the task array. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_072: [ reallocate_threadpool_array shall reset the consume_idx and insert_idx to -1 after resize the task array. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_073: [ reallocate_threadpool_array shall release the SRW lock by calling srw_lock_release_exclusive. ]*/
TEST_FUNCTION(threadpool_schedule_work_realloc_array_with_no_empty_space)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();
    int result;

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

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
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, -1));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_026: [ threadpool_schedule_work shall get the threadpool state by calling interlocked_add. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_028: [ threadpool_schedule_work shall increment the count of pending call that are in progress to be executed. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_029: [ threadpool_schedule_work shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_030: [ threadpool_schedule_work shall increment the insert_pos by one. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_031: [ threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_032: [ If task has been already initialized, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity by calling reallocate_threadpool_array. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_033: [ If reallcate task array fails, threadpool_schedule_work shall fail and return a non-zero value. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_069: [ If any error occurs, reallocate_threadpool_array shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_fails_when_realloc_array_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();
    int result;

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

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

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* threadpool_timer_start */

/* Tests_SRS_THREADPOOL_LINUX_11_038: [ If threadpool is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
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

/* Tests_SRS_THREADPOOL_LINUX_11_039: [ If work_function is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();
    TIMER_INSTANCE_HANDLE timer_instance;

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, NULL, (void*)0x4243, &timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_040: [ If timer_handle is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_timer_handle_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_042: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_043: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_045: [ threadpool_timer_start shall return and allocated handle in timer_handle. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_046: [ threadpool_timer_start shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_start_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;


    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_041: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_042: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_043: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_045: [ threadpool_timer_start shall return and allocated handle in timer_handle. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_046: [ threadpool_timer_start shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_context_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_041: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_042: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_043: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_075: [ If timer_settime fails, threadpool_timer_start shall delete the timer by calling timer_delete. ]*/
TEST_FUNCTION(threadpool_timer_start_delete_timer_when_timer_set_time_functions_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();

    TIMER_INSTANCE_HANDLE timer_instance;


    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_044: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_fails_when_underlying_functions_fail)
{
    // arrange
    THREADPOOL_HANDLE threadpool = test_create_and_open_threadpool();
    TIMER_INSTANCE_HANDLE timer_instance;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
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
    threadpool_destroy(threadpool);
}

/* threadpool_timer_restart */

/* Tests_SRS_THREADPOOL_LINUX_11_047: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_restart_with_NULL_timer_fails)
{
    // arrange

    // act
    int result = threadpool_timer_restart(NULL, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_LINUX_11_048: [ threadpool_timer_restart` shall call timer_settime to changethe delay and period. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_049: [ threadpool_timer_restart shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_restart_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool;
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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_LINUX_11_079: [ If timer_settime fails, threadpool_timer_restart shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_restart_fails_when_timer_set_time_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool;
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
    threadpool_destroy(threadpool);
}

/* threadpool_timer_cancel */

/* Tests_SRS_THREADPOOL_LINUX_11_050: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
TEST_FUNCTION(threadpool_timer_cancel_with_NULL_timer_fails)
{
    // arrange

    // act
    threadpool_timer_cancel(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_11_051: [ threadpool_timer_cancel shall call timer_settime with 0 for flags and NULL for old_value and {0} for new_value to cancel the ongoing timers. ]*/
TEST_FUNCTION(threadpool_timer_cancel_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &timer_instance);

    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    threadpool_timer_cancel(timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    threadpool_destroy(threadpool);
}

/* threadpool_timer_destroy */

/* Tests_SRS_THREADPOOL_LINUX_11_052: [ If timer is NULL, threadpool_timer_destroy shall fail and return. ]*/
TEST_FUNCTION(threadpool_timer_destroy_with_NULL_timer_fails)
{
    // arrange

    // act
    threadpool_timer_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_LINUX_11_053: [ threadpool_timer_cancel shall call timer_delete  to destroy the ongoing timers. ]*/
/* Tests_SRS_THREADPOOL_LINUX_11_054: [ threadpool_timer_destroy shall free all resources in timer. ]*/
TEST_FUNCTION(threadpool_timer_destroy_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &timer_instance);

    STRICT_EXPECTED_CALL(mocked_timer_delete(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    threadpool_timer_destroy(timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

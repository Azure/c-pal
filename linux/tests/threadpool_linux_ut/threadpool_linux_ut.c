// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <bits/types/sigevent_t.h>         // for sigevent, sigev_notify_fun...
#include <bits/types/sigval_t.h>           // for sigval_t
#include <bits/types/timer_t.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"
#include "c_pal/tqueue.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/lazy_init.h"
#include "c_pal/sync.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#include "c_pal/tqueue_threadpool_work_item.h"

#undef ENABLE_MOCKS

#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"

#include "real_interlocked.h"
#include "real_gballoc_hl.h"
#include "real_tqueue_threadpool_work_item.h"
#include "real_lazy_init.h"
#include "real_interlocked_hl.h"

#include "c_pal/threadpool.h"

#define DEFAULT_TASK_ARRAY_SIZE 2048
#define MIN_THREAD_COUNT 5
#define MAX_THREAD_COUNT 10
#define MAX_THREADPOOL_TIMER_COUNT 64
#define MAX_TIMERS 1024

struct itimerspec;
struct timespec;

static EXECUTION_ENGINE_PARAMETERS execution_engine = {MIN_THREAD_COUNT, MAX_THREAD_COUNT};
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

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);

IMPLEMENT_UMOCK_C_ENUM_TYPE(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TQUEUE_POP_RESULT, TQUEUE_POP_RESULT_VALUES);

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
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(execution_engine_linux_get_parameters(test_execution_engine))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_init(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(TQUEUE_CREATE(THANDLE(THREADPOOL_WORK_ITEM))(2048, UINT32_MAX, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TQUEUE_INITIALIZE_MOVE(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();

    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    }
}

static void threadpool_schedule_work_success_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));  // THANDLE_MALLOC
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG)).CallCannotFail(); // THANDLE_MALLOC
    STRICT_EXPECTED_CALL(TQUEUE_PUSH(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetFailReturn(TQUEUE_PUSH_ERROR);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG)).CallCannotFail(); // THANDLE_ASSIGN
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG)).CallCannotFail();
}

static void threadpool_create_work_item_success_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
}

static void threadpool_task(void* parameter)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)parameter;

    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static THANDLE(THREADPOOL) test_create_threadpool(void)
{
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    umock_c_reset_all_calls();

    return threadpool;
}

static void setup_lazy_init(void)
{
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    // create one timer 
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // state set to ARMED
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool, 42, 2000, test_work_function, NULL);
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    umock_c_reset_all_calls();
}

static void setup_epoch_increment_expectations(void)
{
    STRICT_EXPECTED_CALL(interlocked_add_64(IGNORED_ARG, 0)).CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange_64(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).CallCannotFail();
}

static THANDLE(THREADPOOL_TIMER) test_create_threadpool_and_start_timer(uint32_t start_delay_ms, uint32_t timer_period_ms, void* work_function_context, THANDLE(THREADPOOL)* threadpool)
{
    setup_lazy_init();
    THANDLE(THREADPOOL) test_result = test_create_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // state set to ARMED
    setup_epoch_increment_expectations();
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(test_result, start_delay_ms, timer_period_ms, test_work_function, work_function_context);
    ASSERT_IS_NOT_NULL(timer_instance);
    THANDLE_MOVE(THREADPOOL)(threadpool, &test_result);
    umock_c_reset_all_calls();
    return timer_instance;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADAPI_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(clockid_t, int);
    REGISTER_UMOCK_ALIAS_TYPE(timer_t, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LAZY_INIT_FUNCTION, void*);

    REGISTER_GLOBAL_MOCK_RETURN(execution_engine_linux_get_parameters, &execution_engine);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_sem_init, 0, -1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_clock_gettime, 0, -1);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);
    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_RETURNS(ThreadAPI_Create, THREADAPI_OK, THREADAPI_ERROR);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();
    REGISTER_LAZY_INIT_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_timer_create, 0, 1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_timer_settime, 0, -1);

    REGISTER_UMOCK_ALIAS_TYPE(TQUEUE_COPY_ITEM_FUNC(THANDLE(THREADPOOL_WORK_ITEM)), void*);
    REGISTER_UMOCK_ALIAS_TYPE(TQUEUE_DISPOSE_ITEM_FUNC(THANDLE(THREADPOOL_WORK_ITEM)), void*);
    REGISTER_UMOCK_ALIAS_TYPE(TQUEUE_CONDITION_FUNC(THANDLE(THREADPOOL_WORK_ITEM)), void*);
    REGISTER_UMOCK_ALIAS_TYPE(TQUEUE(THANDLE(THREADPOOL_WORK_ITEM)), void*);

    REGISTER_TQUEUE_THREADPOOL_WORK_ITEM_GLOBAL_MOCK_HOOK();

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
    REGISTER_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT);
    REGISTER_TYPE(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_RESULT);
    REGISTER_TYPE(TQUEUE_POP_RESULT, TQUEUE_POP_RESULT);
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
/* Tests_SRS_THREADPOOL_LINUX_01_013: [ threadpool_create shall create a queue of threadpool work items by calling TQUEUE_CREATE(THANDLE(THREADPOOL_WORK_ITEM)) with initial size 2048 and max size UINT32_MAX. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_009: [ threadpool_create shall create a shared semaphore with initialized value zero. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_020: [ threadpool_create shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
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
    threadpool_create_succeed_expectations();

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
/* Tests_SRS_THREADPOOL_LINUX_01_013: [ threadpool_create shall create a queue of threadpool work items by calling TQUEUE_CREATE(THANDLE(THREADPOOL_WORK_ITEM)) with initial size 2048 and max size UINT32_MAX. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_009: [ threadpool_create shall create a shared semaphore with initialized value zero. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_020: [ threadpool_create shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
TEST_FUNCTION(creating_2_threadpool_succeeds)
{
    // arrange
    threadpool_create_succeed_expectations();
    threadpool_create_succeed_expectations();

    // act
    THANDLE(THREADPOOL) threadpool_1 = test_create_threadpool();
    THANDLE(THREADPOOL) threadpool_2 = test_create_threadpool();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool_1);
    ASSERT_IS_NOT_NULL(threadpool_2);
    ASSERT_ARE_NOT_EQUAL(void_ptr, threadpool_1, threadpool_2);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool_1, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool_2, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_020: [ threadpool_create shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_022: [ If one of the thread creation fails, threadpool_create shall fail, terminate all threads already created and return NULL. ]*/
TEST_FUNCTION(threadpool_create_fails_when_ThreadAPI_Create_fails)
{
    // arrange

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_linux_get_parameters(test_execution_engine));
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_init(IGNORED_ARG, 0, 0));
    STRICT_EXPECTED_CALL(TQUEUE_CREATE(THANDLE(THREADPOOL_WORK_ITEM))(2048, UINT32_MAX, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TQUEUE_INITIALIZE_MOVE(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));

    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TQUEUE_ASSIGN(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NULL(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* threadpool_dispose */

/* Tests_SRS_THREADPOOL_LINUX_07_016: [ threadpool_dispose shall free the memory allocated in threadpool_create. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_014: [ threadpool_dispose shall destroy the semphore by calling sem_destroy. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_027: [ threadpool_dispose shall join all threads in the threadpool. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_089: [ threadpool_dispose shall signal all threads to return. ]*/
TEST_FUNCTION(threadpool_dispose_frees_resources)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWakeAll(IGNORED_ARG, IGNORED_ARG));
    for(size_t i = 0; i < MIN_THREAD_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(TQUEUE_ASSIGN(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool);
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
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    int result;

    // act
    result = threadpool_schedule_work(threadpool, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_051: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_014: [ threadpool_schedule_work shall create a new THANDLE(THREADPOOL_WORK_ITEM) and save the work_function and work_function_context in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_015: [ threadpool_schedule_work shall push the newly created THANDLE(THREADPOOL_WORK_ITEM) in the work item queue. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_047: [ threadpool_schedule_work shall return zero on success. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    int result;

    threadpool_schedule_work_success_expectations();

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_051: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_014: [ threadpool_schedule_work shall create a new THANDLE(THREADPOOL_WORK_ITEM) and save the work_function and work_function_context in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_015: [ threadpool_schedule_work shall push the newly created THANDLE(THREADPOOL_WORK_ITEM) in the work item queue. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_047: [ threadpool_schedule_work shall return zero on success. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds_with_NULL_work_function_context)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    int result;

    threadpool_schedule_work_success_expectations();

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_042: [ If any error occurs, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_schedule_work_also_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    int result;

    threadpool_schedule_work_success_expectations();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if(umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_work_func */

/* Tests_SRS_THREADPOOL_LINUX_07_073: [ If param is NULL, threadpool_work_func shall fail and return. ]*/
TEST_FUNCTION(threadpool_work_func_parameter_NULL_succeeds)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

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
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

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
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

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
/* Tests_SRS_THREADPOOL_LINUX_01_016: [ threadpool_work_func shall pop an item from the task queue by calling TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM)). ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_017: [ If the pop returns TQUEUE_POP_OK: ]*/
    /* Tests_SRS_THREADPOOL_LINUX_07_084: [ threadpool_work_func shall execute the work_function with work_function_ctx. ]*/
    /* Tests_SRS_THREADPOOL_LINUX_01_018: [ threadpool_work_func shall release the reference to the work item. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_085: [ threadpool_work_func shall loop until the flag to stop the threads is not set to 1. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds_for_threadpool_schedule_work)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    threadpool_schedule_work_success_expectations();
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, test_work_function, (void*)0x4242));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_timedwait(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL, NULL, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4242));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // 2nd item attempt
    STRICT_EXPECTED_CALL(TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL, NULL, NULL));
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
/* Tests_SRS_THREADPOOL_LINUX_01_016: [ threadpool_work_func shall pop an item from the task queue by calling TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM)). ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_017: [ If the pop returns TQUEUE_POP_OK: ]*/
    /* Tests_SRS_THREADPOOL_LINUX_07_084: [ threadpool_work_func shall execute the work_function with work_function_ctx. ]*/
    /* Tests_SRS_THREADPOOL_LINUX_01_018: [ threadpool_work_func shall release the reference to the work item. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_085: [ threadpool_work_func shall loop until the flag to stop the threads is not set to 1. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds_for_threadpool_schedule_work_2_items)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    threadpool_schedule_work_success_expectations();
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, test_work_function, (void*)0x4242));
    threadpool_schedule_work_success_expectations();
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, test_work_function, (void*)0x4242));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_timedwait(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL, NULL, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4242));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    STRICT_EXPECTED_CALL(TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL, NULL, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4242));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // 3rd item attempt
    STRICT_EXPECTED_CALL(TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL, NULL, NULL));
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
/* Tests_SRS_THREADPOOL_LINUX_01_016: [ threadpool_work_func shall pop an item from the task queue by calling TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM)). ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_017: [ If the pop returns TQUEUE_POP_OK: ]*/
    /* Tests_SRS_THREADPOOL_LINUX_07_084: [ threadpool_work_func shall execute the work_function with work_function_ctx. ]*/
    /* Tests_SRS_THREADPOOL_LINUX_01_018: [ threadpool_work_func shall release the reference to the work item. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_085: [ threadpool_work_func shall loop until the flag to stop the threads is not set to 1. ]*/
TEST_FUNCTION(threadpool_work_func_succeeds_for_threadpool_schedule_work_item)
{
    //arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4242);
    ASSERT_IS_NOT_NULL(threadpool_work_item);
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(threadpool, threadpool_work_item));
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_clock_gettime(CLOCK_REALTIME, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_timedwait(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL, NULL, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4242));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // 2nd item attempt
    STRICT_EXPECTED_CALL(TQUEUE_POP(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL, NULL, NULL));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0)).SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_start */

/* Tests_SRS_THREADPOOL_LINUX_07_054: [ If threadpool is NULL, threadpool_timer_start shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_threadpool_fails)
{
    // arrange

    // act
    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(NULL, 42, 2000, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer_instance);
}

/* Tests_SRS_THREADPOOL_LINUX_07_055: [ If work_function is NULL, threadpool_timer_start shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    // act
    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool, 42, 2000, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer_instance);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_01_002: [ threadpool_timer_start shall find an unused entry in the timers table maintained by the module. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_001: [ If all timer entries are used, threadpool_timer_start shall fail and return NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_003: [ 2048 timers shall be supported. ]*/
TEST_FUNCTION(threadpool_timer_start_fails_when_no_available_slot_in_timer_table)
{
    // arrange
    setup_lazy_init();
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    THANDLE(THREADPOOL_TIMER)* timers = malloc_2(MAX_TIMERS, sizeof(THANDLE(THREADPOOL_TIMER)));
    ASSERT_IS_NOT_NULL(timers);
    for (uint32_t i = 0; i < MAX_TIMERS; i++)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
        THANDLE(THREADPOOL_TIMER) timer_temp = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243);

        THANDLE_INITIALIZE_MOVE(THREADPOOL_TIMER)((void*)&timers[i], &timer_temp);
        ASSERT_IS_NOT_NULL(timers[i]);
        umock_c_reset_all_calls();

        LogInfo("Timer %" PRIu32 " created", i);
    }

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (uint32_t i = 0; i < MAX_TIMERS; i++)
    {
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); // attempt to set state to ARMED
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer_instance);

    // cleanup
    for (uint32_t i = 0; i < MAX_TIMERS; i++)
    {
        THANDLE_ASSIGN(THREADPOOL_TIMER)((void*)&timers[i], NULL);
    }
    free((void*)timers);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate memory for THANDLE(THREADPOOL_TIMER), passing threadpool_timer_dispose as dispose function, and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_096: [ threadpool_timer_start shall call lazy_init with do_init as initialization function.  ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_004: [ do_init shall initialize the state for each timer to NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_099: [ do_init shall succeed and return 0. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_005: [ If an unused entry is found, it's state shall be marked as ARMED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_011: [ threadpool_timer_start shall increment the timer epoch number and store it in the selected entry in the timer table. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_059: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_062: [ threadpool_timer_start shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(threadpool_timer_start_succeeds)
{
    // arrange
    setup_lazy_init();
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); // state set to ARMED
    setup_epoch_increment_expectations();
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(timer_instance);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate memory for THANDLE_MALLOC(THREADPOOL_TIMER) and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_096: [ threadpool_timer_start shall call lazy_init with do_init as initialization function.  ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_004: [ do_init shall initialize the state for each timer to NOT_USED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_099: [ do_init shall succeed and return 0. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_005: [ If an unused entry is found, it's state shall be marked as ARMED. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_011: [ threadpool_timer_start shall increment the timer epoch number and store it in the selected entry in the timer table. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_059: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_062: [ threadpool_timer_start shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_context_succeeds)
{
    // arrange
    setup_lazy_init();
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); // state set to ARMED
    setup_epoch_increment_expectations();
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool, 42, 2000, test_work_function, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(timer_instance);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate memory for THANDLE_MALLOC(THREADPOOL_TIMER) and store work_function and work_function_ctx in it. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_059: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
/* Tests_SRS_THREADPOOL_LINUX_07_063: [ If timer_settime fails, threadpool_timer_start shall delete the timer by calling timer_delete. ]*/
TEST_FUNCTION(when_timer_set_time_functions_fails_threadpool_timer_start_fails)
{
    // arrange
    setup_lazy_init();
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // state set to ARMED
    setup_epoch_increment_expectations();
    STRICT_EXPECTED_CALL(mocked_timer_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL)).SetReturn(-1);

    STRICT_EXPECTED_CALL(mocked_timer_delete(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG)); // state revert to NOT_USED
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));

    // act
    THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(timer_instance);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_060: [ If any error occurs, threadpool_timer_start shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_timer_start_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1)).CallCannotFail();
    STRICT_EXPECTED_CALL(lazy_init(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetFailReturn(LAZY_INIT_ERROR);
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).CallCannotFail();  // state set to ARMED
    setup_epoch_increment_expectations();
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
            THANDLE(THREADPOOL_TIMER) timer_instance = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243);

            // assert
            ASSERT_IS_NULL(timer_instance, "On failed call %zu", i);
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
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);

    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    int result = threadpool_timer_restart(timer_instance, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_07_066: [ If timer_settime fails, threadpool_timer_restart shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_restart_fails_when_timer_set_time_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);

    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL)).SetReturn(-1);

    // act
    int result = threadpool_timer_restart(timer_instance, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
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
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);

    STRICT_EXPECTED_CALL(mocked_timer_settime(IGNORED_ARG, 0, IGNORED_ARG, NULL));

    // act
    threadpool_timer_cancel(timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_dispose */

/* Tests_SRS_THREADPOOL_LINUX_07_071: [ threadpool_timer_dispose shall call timer_delete to destroy the ongoing timers. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_006: [ If the timer state is ARMED, threadpool_timer_dispose shall set the state of the timer to NOT_USED. ]*/
TEST_FUNCTION(threadpool_timer_dispose_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_delete(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // state set to NOT_USED
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_01_010: [ Otherwise, threadpool_timer_dispose shall block until the state is ARMED and reattempt to set the state to NOT_USED. ]*/
TEST_FUNCTION(threadpool_timer_dispose_waits_for_state_to_be_ARMED)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    int32_t armed_state_value;
    // create one timer and capture the ARMED state
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_delete(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&armed_state_value);  // state set to NOT_USED
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);

    // now do it again so we can simulate that a different value is returned from the interlocked_compare_exchange
    THANDLE(THREADPOOL_TIMER) timer_instance_2 = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_timer_delete(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(armed_state_value + 1);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(IGNORED_ARG, armed_state_value + 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_all(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance_2, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* on_timer_callback */

/* Tests_SRS_THREADPOOL_LINUX_45_002: [ on_timer_callback shall extract from the lower 10 bits of timer_data.sival_ptr the information indicating which timer table entry is being triggered. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_012: [ on_timer_callback shall use the rest of the higher bits of timer_data.sival_ptr as timer epoch. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_008: [ If the timer is in the state ARMED: ]*/
    /* Tests_SRS_THREADPOOL_LINUX_01_007: [ on_timer_callback shall transition it to CALLING_CALLBACK. ]*/
    /* Tests_SRS_THREADPOOL_LINUX_45_004: [ If the timer epoch of the timer table entry is the same like the timer epoch in timer_data.sival_ptr, on_timer_callback shall call the timer's work_function with work_function_ctx. ]*/
    /* Tests_SRS_THREADPOOL_LINUX_01_009: [ on_timer_callback shall transition it to ARMED. ]*/
TEST_FUNCTION(on_timer_callback_calls_work_function)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // state set to CALLING_CALLBACK
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, IGNORED_ARG));  // state set to ARMED

    sigval_t timer_data = {0};
    timer_data.sival_ptr = g_sigevent.sigev_value.sival_ptr;

    // act
    g_sigevent.sigev_notify_function(timer_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_45_002: [ on_timer_callback shall extract from the lower bits of timer_data.sival_ptr the information indicating which timer table entry is being triggered. ]*/
TEST_FUNCTION(on_timer_callback_does_nothing_after_dispose)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // attempt to set state to CALLING_CALLBACK

    sigval_t timer_data = {0};
    timer_data.sival_ptr = g_sigevent.sigev_value.sival_ptr;

    // act
    g_sigevent.sigev_notify_function(timer_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_45_004: [ If the timer epoch of the timer table entry is the same like the timer epoch in timer_data.sival_ptr, on_timer_callback shall call the timer's work_function with work_function_ctx. ]*/
TEST_FUNCTION(on_timer_for_an_old_timer_does_not_call_the_timer_user_callback)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);
    sigval_t timer_data_1 = {0};
    timer_data_1.sival_ptr = g_sigevent.sigev_value.sival_ptr;

    // dispose of the first timer
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);

    THANDLE(THREADPOOL_TIMER) timer_instance_2 = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4244, &threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // set state to CALLING_CALLBACK
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, IGNORED_ARG));  // state set to ARMED

    // act
    g_sigevent.sigev_notify_function(timer_data_1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance_2, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_45_004: [ If the timer epoch of the timer table entry is the same like the timer epoch in timer_data.sival_ptr, on_timer_callback shall call the timer's work_function with work_function_ctx. ]*/
TEST_FUNCTION(on_timer_for_timer_with_correct_epoch_for_2nd_timer_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    THANDLE(THREADPOOL_TIMER) timer_instance = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool);
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance, NULL);
    THANDLE(THREADPOOL_TIMER) timer_instance_2 = test_create_threadpool_and_start_timer(42, 2000, (void*)0x4244, &threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // state set to CALLING_CALLBACK
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4244));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, IGNORED_ARG));  // state set to ARMED

    sigval_t timer_data = {0};
    timer_data.sival_ptr = g_sigevent.sigev_value.sival_ptr;

    // act
    g_sigevent.sigev_notify_function(timer_data);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_TIMER)(&timer_instance_2, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_create_work_item */

/* Tests_SRS_THREADPOOL_LINUX_05_001: [ If threadpool is NULL, threadpool_create_work_item shall fail and return a NULL value. ]*/
TEST_FUNCTION(threadpool_create_work_item_with_NULL_threadpool_fails)
{
    //arrange

    // act
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(NULL, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool_work_item);
}

/* Tests_SRS_THREADPOOL_LINUX_05_002: [ If work_function is NULL, threadpool_create_work_item shall fail and return a NULL value. ] */
TEST_FUNCTION(threadpool_create_work_item_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    // act
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item = threadpool_create_work_item(threadpool, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool_work_item);

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_05_005: [ threadpool_create_work_item shall allocate memory for threadpool_work_item of type THANDLE(THREADPOOL_WORK_ITEM) . ]*/
/* Tests_SRS_THREADPOOL_LINUX_05_007: [ threadpool_create_work_item shall copy the work_function and work_function_context into the threadpool work item. ]*/
TEST_FUNCTION(threadpool_create_work_item_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    threadpool_create_work_item_success_expectations();

    // act
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // cleanup
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_05_005: [ threadpool_create_work_item shall allocate memory for threadpool_work_item of type THANDLE(THREADPOOL_WORK_ITEM) . ] */
/* Tests_SRS_THREADPOOL_LINUX_05_007: [ threadpool_create_work_item shall copy the work_function and work_function_context into the threadpool work item. ]*/
TEST_FUNCTION(threadpool_create_work_item_succeeds_with_NULL_work_function_context)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    threadpool_create_work_item_success_expectations();

    // act
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item = threadpool_create_work_item(threadpool, test_work_function, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool_work_item);

    // cleanup
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}


/* Tests_SRS_THREADPOOL_LINUX_05_006: [ If during the initialization of threadpool_work_item, malloc fails then threadpool_create_work_item shall fail and return a NULL value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_work_item_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);

            // assert
            ASSERT_IS_NULL(threadpool_work_item, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_schedule_work_item */

/* Tests_SRS_THREADPOOL_LINUX_05_010: [ If threadpool is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_item_with_NULL_threadpool_fails)
{
    // arrange
    int work_item_schedule_result;

    // act
    work_item_schedule_result = threadpool_schedule_work_item(NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, work_item_schedule_result);
}

/* Tests_SRS_THREADPOOL_LINUX_05_011: [ If threadpool_work_item is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ] */
TEST_FUNCTION(threadpool_schedule_work_item_with_NULL_work_function_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();
    int work_item_schedule_result;

    // act
    work_item_schedule_result = threadpool_schedule_work_item(threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, work_item_schedule_result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_05_025: [ threadpool_schedule_work_item shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_019: [ threadpool_schedule_work_item shall TQUEUE_PUSH the threadpool_work_item into the task queue. ]*/
/* Tests_SRS_THREADPOOL_LINUX_05_026: [ threadpool_schedule_work_item shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_schedule_work_item_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    threadpool_create_work_item_success_expectations();
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    ASSERT_IS_NOT_NULL(threadpool_work_item);
    umock_c_reset_all_calls();

    int result;

    STRICT_EXPECTED_CALL(TQUEUE_PUSH(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    // act
    result = threadpool_schedule_work_item(threadpool, threadpool_work_item);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_05_025: [ threadpool_schedule_work_item shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_019: [ threadpool_schedule_work_item shall TQUEUE_PUSH the threadpool_work_item into the task queue. ]*/
/* Tests_SRS_THREADPOOL_LINUX_05_026: [ threadpool_schedule_work_item shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_schedule_work_item_twice_same_item_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    threadpool_create_work_item_success_expectations();
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    ASSERT_IS_NOT_NULL(threadpool_work_item);
    umock_c_reset_all_calls();

    int result_1;
    int result_2;

    STRICT_EXPECTED_CALL(TQUEUE_PUSH(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    STRICT_EXPECTED_CALL(TQUEUE_PUSH(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    // act
    result_1 = threadpool_schedule_work_item(threadpool, threadpool_work_item);
    result_2 = threadpool_schedule_work_item(threadpool, threadpool_work_item);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result_1);
    ASSERT_ARE_EQUAL(int, 0, result_2);

    // cleanup
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_05_025: [ threadpool_schedule_work_item shall unblock the threadpool semaphore by calling sem_post. ]*/
/* Tests_SRS_THREADPOOL_LINUX_01_019: [ threadpool_schedule_work_item shall TQUEUE_PUSH the threadpool_work_item into the task queue. ]*/
/* Tests_SRS_THREADPOOL_LINUX_05_026: [ threadpool_schedule_work_item shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_schedule_work_2_items_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    threadpool_create_work_item_success_expectations();
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item_1 = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4242);
    ASSERT_IS_NOT_NULL(threadpool_work_item_1);

    threadpool_create_work_item_success_expectations();
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item_2 = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    ASSERT_IS_NOT_NULL(threadpool_work_item_2);
    umock_c_reset_all_calls();

    int result_1;
    int result_2;

    STRICT_EXPECTED_CALL(TQUEUE_PUSH(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    STRICT_EXPECTED_CALL(TQUEUE_PUSH(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG));

    // act
    result_1 = threadpool_schedule_work_item(threadpool, threadpool_work_item_1);
    result_2 = threadpool_schedule_work_item(threadpool, threadpool_work_item_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result_1);
    ASSERT_ARE_EQUAL(int, 0, result_2);

    // cleanup
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item_1, NULL);
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item_2, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_LINUX_01_020: [ If any error occurrs, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_schedule_work_item_fails)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = test_create_threadpool();

    threadpool_create_work_item_success_expectations();
    THANDLE(THREADPOOL_WORK_ITEM)  threadpool_work_item = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    ASSERT_IS_NOT_NULL(threadpool_work_item);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(TQUEUE_PUSH(THANDLE(THREADPOOL_WORK_ITEM))(IGNORED_ARG, IGNORED_ARG, NULL))
        .SetFailReturn(TQUEUE_PUSH_ERROR);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG)).CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_sem_post(IGNORED_ARG)).CallCannotFail();

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = threadpool_schedule_work_item(threadpool, threadpool_work_item);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM)(&threadpool_work_item, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"
#include "c_pal/interlocked.h"
#include "c_pal/ps_util.h"

#undef ENABLE_MOCKS

#include "c_pal/thandle.h"

#include "real_gballoc_hl.h"

#include "c_pal/string_utils.h"
#include "c_pal/threadpool.h"

static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4243;
static PTP_POOL test_pool = (PTP_POOL)0x4244;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static char* umocktypes_stringify_PFILETIME(PFILETIME* value)
{
    char* result;

    if (value == NULL)
    {
        LogError("umocktypes_stringify_PFILETIME: NULL value.");
        result = NULL;
    }
    else
    {
        if (*value == NULL)
        {
            result = (char*)real_gballoc_hl_malloc(sizeof("NULL"));
            if (result != NULL)
            {
                (void)memcpy(result, "NULL", sizeof("NULL"));
            }
        }
        else
        {
            result = sprintf_char("FILETIME{dwLowDateTime=%" PRIu32 ", dwHighDateTime=%" PRIu32 "}", (*value)->dwLowDateTime, (*value)->dwHighDateTime);
            if (result == NULL)
            {
                LogError("umocktypes_stringify_PFILETIME: Cannot allocate memory for result.");
            }
            else
            {
                // ok
            }
        }
    }
    return result;
}

static int umocktypes_are_equal_PFILETIME(PFILETIME* left, PFILETIME* right)
{
    int result;

    if (
        (left == NULL) ||
        (right == NULL))
    {
        LogError("umocktypes_are_equal_PFILETIME: Bad arguments:left = %p, right = %p.", left, right);
        result = -1;
    }
    else if (*left == *right)
    {
        result = 1;
    }
    else if ((*left == NULL) || (*right == NULL))
    {
        result = 0;
    }
    else
    {
        if ((*left)->dwLowDateTime == (*right)->dwLowDateTime &&
            (*left)->dwHighDateTime == (*right)->dwHighDateTime)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static int umocktypes_copy_PFILETIME(PFILETIME* destination, const PFILETIME* source)
{
    int result;

    if ((destination == NULL) || (source == NULL))
    {
        UMOCK_LOG("umocktypes_copy_PFILETIME: Bad arguments: destination = %p, source = %p.",
            destination, source);
        result = MU_FAILURE;
    }
    else
    {
        if (*source == NULL)
        {
            *destination = NULL;
            result = 0;
        }
        else
        {
            *destination = (PFILETIME)real_gballoc_hl_malloc(sizeof(FILETIME));
            if (*destination == NULL)
            {
                UMOCK_LOG("umocktypes_copy_PFILETIME: Failed allocating memory for the destination.");
                result = MU_FAILURE;
            }
            else
            {
                (*destination)->dwLowDateTime = (*source)->dwLowDateTime;
                (*destination)->dwHighDateTime = (*source)->dwHighDateTime;
                result = 0;
            }
        }
    }
    return result;
}

static void umocktypes_free_PFILETIME(PFILETIME* value)
{
    if (value != NULL)
    {
        real_gballoc_hl_free(*value);
    }
}

MOCK_FUNCTION_WITH_CODE(, void, mocked_InitializeThreadpoolEnvironment, PTP_CALLBACK_ENVIRON, pcbe)
    // We are using the Pool member to force a memory allocation to check for leaks
    pcbe->Pool = (PTP_POOL)real_gballoc_hl_malloc(1);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_SetThreadpoolCallbackPool, PTP_CALLBACK_ENVIRON, pcbe, PTP_POOL, ptpp)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, PTP_CLEANUP_GROUP, mocked_CreateThreadpoolCleanupGroup)
MOCK_FUNCTION_END((PTP_CLEANUP_GROUP)real_gballoc_hl_malloc(1))

MOCK_FUNCTION_WITH_CODE(, VOID, mocked_SetThreadpoolCallbackCleanupGroup, PTP_CALLBACK_ENVIRON, pcbe, PTP_CLEANUP_GROUP, ptpcg, PTP_CLEANUP_GROUP_CANCEL_CALLBACK, pfng)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, PTP_WORK, mocked_CreateThreadpoolWork, PTP_WORK_CALLBACK, pfnwk, PVOID, pv, PTP_CALLBACK_ENVIRON, pcbe)
MOCK_FUNCTION_END((PTP_WORK)real_gballoc_hl_malloc(1))

MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolWork, PTP_WORK, pwk)
    real_gballoc_hl_free(pwk);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, PTP_TIMER, mocked_CreateThreadpoolTimer, PTP_TIMER_CALLBACK, pfnti, PVOID, pv, PTP_CALLBACK_ENVIRON, pcbe)
MOCK_FUNCTION_END((PTP_TIMER)real_gballoc_hl_malloc(1))

MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolTimer, PTP_TIMER, pti)
    real_gballoc_hl_free(pti);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolCleanupGroup, PTP_CLEANUP_GROUP, ptpcg)
    real_gballoc_hl_free(ptpcg);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_DestroyThreadpoolEnvironment, PTP_CALLBACK_ENVIRON, pcbe)
    // We are using the Pool member to force a memory allocation to check for leaks
    real_gballoc_hl_free(pcbe->Pool);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolCleanupGroupMembers, PTP_CLEANUP_GROUP, ptpcg, BOOL, fCancelPendingCallbacks, PVOID, pvCleanupContext)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_SubmitThreadpoolWork, PTP_WORK, pwk)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_SetThreadpoolTimer, PTP_TIMER, pti, PFILETIME, pftDueTime, DWORD, msPeriod, DWORD, msWindowLength)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_WaitForThreadpoolTimerCallbacks, PTP_TIMER, pti, BOOL, fCancelPendingCallbacks)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, mocked_WaitForThreadpoolWorkCallbacks, PTP_WORK, pwk, BOOL, fCancelPendingCallbacks)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_work_function, void*, context)
MOCK_FUNCTION_END()

static THANDLE(THREADPOOL) test_create_threadpool(PTP_CALLBACK_ENVIRON* cbe)
{
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(cbe);
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    umock_c_reset_all_calls();
    return threadpool;
}

static void test_create_threadpool_and_start_timer(uint32_t start_delay_ms, uint32_t timer_period_ms, void* work_function_context, THANDLE(THREADPOOL)* threadpool, PTP_TIMER* ptp_timer, PTP_TIMER_CALLBACK* test_timer_callback, PVOID* test_timer_callback_context, TIMER_INSTANCE_HANDLE* timer_instance)
{
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) test_result = test_create_threadpool(&cbe);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnti(test_timer_callback)
        .CaptureArgumentValue_pv(test_timer_callback_context)
        .CaptureReturn(ptp_timer);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, 2000, 0));

    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(test_result, start_delay_ms, timer_period_ms, test_work_function, work_function_context, timer_instance));

    THANDLE_MOVE(THREADPOOL)(threadpool, &test_result);
    umock_c_reset_all_calls();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types(), "umocktypes_windows_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, REGISTER_TYPE(PFILETIME, PFILETIME));

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_RETURN(execution_engine_win32_get_threadpool, test_pool);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(execution_engine_win32_get_threadpool, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_CreateThreadpoolCleanupGroup, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_CreateThreadpoolWork, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_TIMER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_TIMER_CALLBACK, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* threadpool_create */

/* Tests_SRS_THREADPOOL_WIN32_01_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_create_with_NULL_execution_engine_fails)
{
    // arrange

    // act
    THANDLE(THREADPOOL) threadpool = threadpool_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_001: [ threadpool_create shall allocate a new threadpool object and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_025: [ threadpool_create shall obtain the PTP_POOL from the execution engine by calling execution_engine_win32_get_threadpool. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_026: [ threadpool_create shall initialize a thread pool environment by calling InitializeThreadpoolEnvironment. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_027: [ threadpool_create shall set the thread pool for the environment to the pool obtained from the execution engine by calling SetThreadpoolCallbackPool. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_028: [ threadpool_create shall create a threadpool cleanup group by calling CreateThreadpoolCleanupGroup. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_029: [ threadpool_create shall associate the cleanup group with the just created environment by calling SetThreadpoolCallbackCleanupGroup. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_027: [ threadpool_create shall increment the reference count on the execution_engine. ]*/
TEST_FUNCTION(threadpool_create_succeeds)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_win32_get_threadpool(test_execution_engine));
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackCleanupGroup(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_ptpcg(&test_cleanup_group);
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));

    // act
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_01_003: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_fails)
{
    // arrange
    size_t i;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(execution_engine_win32_get_threadpool(test_execution_engine));
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackCleanupGroup(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
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


/* Tests_SRS_THREADPOOL_WIN32_42_028: [ threadpool_dispose shall decrement the reference count on the execution_engine. ]*/
TEST_FUNCTION(threadpool_dispose_frees_resources)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroupMembers(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroup(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_DestroyThreadpoolEnvironment(IGNORED_ARG));

    // destroy calls
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_WIN32_01_030: [ threadpool_dispose shall wait for any executing callbacks by calling CloseThreadpoolCleanupGroupMembers, passing FALSE as fCancelPendingCallbacks. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_032: [ threadpool_dispose shall close the threadpool cleanup group by calling CloseThreadpoolCleanupGroup. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_033: [ threadpool_dispose shall destroy the thread pool environment created in threadpool_create. ]*/
TEST_FUNCTION(threadpool_dispose_performs_an_implicit_close)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_win32_get_threadpool(test_execution_engine));
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackCleanupGroup(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_ptpcg(&test_cleanup_group);
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    umock_c_reset_all_calls();

    // destroy calls
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroupMembers(test_cleanup_group, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroup(test_cleanup_group));
    STRICT_EXPECTED_CALL(mocked_DestroyThreadpoolEnvironment(cbe));
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* threadpool_open */

/* Tests_SRS_THREADPOOL_WIN32_01_008: [ If threadpool is NULL, threadpool_open shall fail and return a non-zero value. ]*/
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

/* Tests_SRS_THREADPOOL_WIN32_01_012: [ On success, threadpool_open shall return 0. ]*/
TEST_FUNCTION(threadpool_open_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = threadpool_create(test_execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    int result;
    umock_c_reset_all_calls();

    // act
    result = threadpool_open(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}


/* threadpool_close */

/* Tests_SRS_THREADPOOL_WIN32_05_019: [ If threadpool is NULL, threadpool_close shall return. ]*/
TEST_FUNCTION(threadpool_close_with_NULL_handle_returns)
{
    // arrange

    // act
    threadpool_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* threadpool_schedule_work */

/* Tests_SRS_THREADPOOL_WIN32_01_020: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
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

/* Tests_SRS_THREADPOOL_WIN32_01_021: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_schedule_work_with_NULL_work_function_fails)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);
    int result;

    // act
    result = threadpool_schedule_work(threadpool, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_01_023: [ Otherwise threadpool_schedule_work shall allocate a context where work_function and context shall be saved. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_034: [ threadpool_schedule_work shall call CreateThreadpoolWork to schedule execution the callback while passing to it the on_work_callback function and the newly created context. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_041: [ threadpool_schedule_work shall call SubmitThreadpoolWork to submit the work item for execution. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    int result;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback)
        .CaptureArgumentValue_pv(&test_work_callback_context)
        .CaptureReturn(&ptp_work);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work);

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    test_work_callback(NULL, test_work_callback_context, ptp_work);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_01_022: [ work_function_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds_with_NULL_work_function_context)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    int result;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback)
        .CaptureArgumentValue_pv(&test_work_callback_context)
        .CaptureReturn(&ptp_work);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work);

    // act
    result = threadpool_schedule_work(threadpool, test_work_function, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    test_work_callback(NULL, test_work_callback_context, ptp_work);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_01_024: [ If any error occurs, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_schedule_work_fails)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    int result;
    size_t i;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureReturn(&ptp_work);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work);

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
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

/* on_work_callback */

/* Tests_SRS_THREADPOOL_WIN32_01_035: [ If context is NULL, on_work_callback shall return. ]*/
TEST_FUNCTION(on_work_callback_with_NULL_context_returns)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback)
        .CaptureArgumentValue_pv(&test_work_callback_context)
        .CaptureReturn(&ptp_work);
    threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);
    umock_c_reset_all_calls();

    // act
    test_work_callback(NULL, NULL, ptp_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_work_callback(NULL, test_work_callback_context, ptp_work);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_01_036: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_037: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_038: [ on_work_callback shall call CloseThreadpoolWork. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_039: [ on_work_callback shall free the context allocated in threadpool_schedule_work. ]*/
TEST_FUNCTION(on_work_callback_triggers_the_user_work_function)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback)
        .CaptureArgumentValue_pv(&test_work_callback_context)
        .CaptureReturn(&ptp_work);
    threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolWork(ptp_work));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_work_callback(NULL, test_work_callback_context, ptp_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_01_036: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_037: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_038: [ on_work_callback shall call CloseThreadpoolWork. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_039: [ on_work_callback shall free the context allocated in threadpool_schedule_work. ]*/
TEST_FUNCTION(on_work_callback_triggers_2_user_work_functions)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context_1;
    PVOID test_work_callback_context_2;
    PTP_WORK ptp_work_1;
    PTP_WORK ptp_work_2;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback)
        .CaptureArgumentValue_pv(&test_work_callback_context_1)
        .CaptureReturn(&ptp_work_1);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work_1);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback)
        .CaptureArgumentValue_pv(&test_work_callback_context_2)
        .CaptureReturn(&ptp_work_2);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work_2);

    threadpool_schedule_work(threadpool, test_work_function, (void*)0x4243);
    threadpool_schedule_work(threadpool, test_work_function, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolWork(ptp_work_1));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    STRICT_EXPECTED_CALL(test_work_function((void*)0x4244));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolWork(ptp_work_2));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_work_callback(NULL, test_work_callback_context_1, ptp_work_1);
    test_work_callback(NULL, test_work_callback_context_2, ptp_work_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_01_036: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_037: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_038: [ on_work_callback shall call CloseThreadpoolWork. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_039: [ on_work_callback shall free the context allocated in threadpool_schedule_work. ]*/
TEST_FUNCTION(on_work_callback_triggers_the_user_work_function_with_NULL_work_function_context)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback)
        .CaptureArgumentValue_pv(&test_work_callback_context)
        .CaptureReturn(&ptp_work);
    threadpool_schedule_work(threadpool, test_work_function, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_work_function(NULL));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolWork(ptp_work));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_work_callback(NULL, test_work_callback_context, ptp_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_start */

/* Tests_SRS_THREADPOOL_WIN32_42_001: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
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

/* Tests_SRS_THREADPOOL_WIN32_42_002: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_fails)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    TIMER_INSTANCE_HANDLE timer_instance;

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, NULL, (void*)0x4243, &timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_42_003: [ If timer_handle is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_timer_handle_fails)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    // act
    int result = threadpool_timer_start(threadpool, 42, 2000, test_work_function, (void*)0x4243, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_42_005: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_context in it. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_006: [ threadpool_timer_start shall call CreateThreadpoolTimer to schedule execution the callback while passing to it the on_timer_callback function and the newly created context. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_007: [ threadpool_timer_start shall call SetThreadpoolTimer, passing negative start_delay_ms as pftDueTime, timer_period_ms as msPeriod, and 0 as msWindowLength. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_009: [ threadpool_timer_start shall return the allocated handle in timer_handle. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_010: [ threadpool_timer_start shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_start_succeeds)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;

    TIMER_INSTANCE_HANDLE timer_instance;

    ULARGE_INTEGER ularge_due_time;
    ularge_due_time.QuadPart = (ULONGLONG)-((int64_t)42 * 10000);
    FILETIME filetime_expected;
    filetime_expected.dwHighDateTime = ularge_due_time.HighPart;
    filetime_expected.dwLowDateTime = ularge_due_time.LowPart;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnti(&test_timer_callback)
        .CaptureArgumentValue_pv(&test_timer_callback_context)
        .CaptureReturn(&ptp_timer);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, &filetime_expected, 2000, 0))
        .ValidateArgumentValue_pti(&ptp_timer);

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

/* Tests_SRS_THREADPOOL_WIN32_42_004: [ work_function_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(threadpool_timer_start_with_NULL_work_function_context_succeeds)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;

    TIMER_INSTANCE_HANDLE timer_instance;

    ULARGE_INTEGER ularge_due_time;
    ularge_due_time.QuadPart = (ULONGLONG)-((int64_t)42 * 10000);
    FILETIME filetime_expected;
    filetime_expected.dwHighDateTime = ularge_due_time.HighPart;
    filetime_expected.dwLowDateTime = ularge_due_time.LowPart;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnti(&test_timer_callback)
        .CaptureArgumentValue_pv(&test_timer_callback_context)
        .CaptureReturn(&ptp_timer);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, &filetime_expected, 2000, 0))
        .ValidateArgumentValue_pti(&ptp_timer);

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

/* Tests_SRS_THREADPOOL_WIN32_42_008: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_start_fails_when_underlying_functions_fail)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;

    TIMER_INSTANCE_HANDLE timer_instance;

    ULARGE_INTEGER ularge_due_time;
    ularge_due_time.QuadPart = (ULONGLONG)-((int64_t)42 * 10000);
    FILETIME filetime_expected;
    filetime_expected.dwHighDateTime = ularge_due_time.HighPart;
    filetime_expected.dwLowDateTime = ularge_due_time.LowPart;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnti(&test_timer_callback)
        .CaptureArgumentValue_pv(&test_timer_callback_context)
        .CaptureReturn(&ptp_timer);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, &filetime_expected, 2000, 0))
        .ValidateArgumentValue_pti(&ptp_timer);


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

/* Tests_SRS_THREADPOOL_WIN32_42_019: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_timer_restart_with_NULL_timer_fails)
{
    // arrange

    // act
    int result = threadpool_timer_restart(NULL, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_THREADPOOL_WIN32_42_022: [ threadpool_timer_restart shall call SetThreadpoolTimer, passing negative start_delay_ms as pftDueTime, timer_period_ms as msPeriod, and 0 as msWindowLength. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_023: [ threadpool_timer_restart shall succeed and return 0. ]*/
TEST_FUNCTION(threadpool_timer_restart_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &ptp_timer, &test_timer_callback, &test_timer_callback_context, &timer_instance);

    ULARGE_INTEGER ularge_due_time;
    ularge_due_time.QuadPart = (ULONGLONG)-((int64_t)43 * 10000);
    FILETIME filetime_expected;
    filetime_expected.dwHighDateTime = ularge_due_time.HighPart;
    filetime_expected.dwLowDateTime = ularge_due_time.LowPart;

    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, &filetime_expected, 1000, 0))
        .ValidateArgumentValue_pti(&ptp_timer);

    // act
    int result = threadpool_timer_restart(timer_instance, 43, 1000);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_cancel */

/*Tests_SRS_THREADPOOL_WIN32_42_024: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
TEST_FUNCTION(threadpool_timer_cancel_with_NULL_timer_fails)
{
    // arrange

    // act
    threadpool_timer_cancel(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THREADPOOL_WIN32_42_025: [ threadpool_timer_cancel shall call SetThreadpoolTimer with NULL for pftDueTime and 0 for msPeriod and msWindowLength to cancel ongoing timers. ]*/
/*Tests_SRS_THREADPOOL_WIN32_42_026: [ threadpool_timer_cancel shall call WaitForThreadpoolTimerCallbacks. ]*/
TEST_FUNCTION(threadpool_timer_cancel_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &ptp_timer, &test_timer_callback, &test_timer_callback_context, &timer_instance);

    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(ptp_timer, NULL, 0, 0));
    STRICT_EXPECTED_CALL(mocked_WaitForThreadpoolTimerCallbacks(ptp_timer, TRUE));

    // act
    threadpool_timer_cancel(timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_timer_destroy */

/* Tests_SRS_THREADPOOL_WIN32_42_011: [ If timer is NULL, threadpool_timer_destroy shall fail and return. ]*/
TEST_FUNCTION(threadpool_timer_destroy_with_NULL_timer_fails)
{
    // arrange

    // act
    threadpool_timer_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_WIN32_42_012: [ threadpool_timer_destroy shall call SetThreadpoolTimer with NULL for pftDueTime and 0 for msPeriod and msWindowLength to cancel ongoing timers. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_013: [ threadpool_timer_destroy shall call WaitForThreadpoolTimerCallbacks. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_014: [ threadpool_timer_destroy shall call CloseThreadpoolTimer. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_015: [ threadpool_timer_destroy shall free all resources in timer. ]*/
TEST_FUNCTION(threadpool_timer_destroy_succeeds)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &ptp_timer, &test_timer_callback, &test_timer_callback_context, &timer_instance);

    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(ptp_timer, NULL, 0, 0));
    STRICT_EXPECTED_CALL(mocked_WaitForThreadpoolTimerCallbacks(ptp_timer, TRUE));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolTimer(ptp_timer));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    threadpool_timer_destroy(timer_instance);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* on_timer_callback */

/* Tests_SRS_THREADPOOL_WIN32_42_016: [ If context is NULL, on_work_callback shall return. ]*/
TEST_FUNCTION(on_timer_callback_with_NULL_context_returns)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &ptp_timer, &test_timer_callback, &test_timer_callback_context, &timer_instance);

    // act
    test_timer_callback(NULL, NULL, ptp_timer);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_42_017: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_018: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
TEST_FUNCTION(on_timer_callback_calls_user_callback)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &ptp_timer, &test_timer_callback, &test_timer_callback_context, &timer_instance);

    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));

    // act
    test_timer_callback(NULL, test_timer_callback_context, ptp_timer);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_42_017: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_42_018: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
TEST_FUNCTION(on_timer_callback_calls_user_callback_multiple_times_as_timer_fires)
{
    // arrange
    THANDLE(THREADPOOL) threadpool = NULL;
    PTP_TIMER_CALLBACK test_timer_callback;
    PVOID test_timer_callback_context;
    PTP_TIMER ptp_timer;
    TIMER_INSTANCE_HANDLE timer_instance;
    test_create_threadpool_and_start_timer(42, 2000, (void*)0x4243, &threadpool, &ptp_timer, &test_timer_callback, &test_timer_callback_context, &timer_instance);

    uint32_t timer_fire_count = 10;

    for (uint32_t i = 0; i < timer_fire_count; i++)
    {
        STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));
    }

    // act
    for (uint32_t i = 0; i < timer_fire_count; i++)
    {
        test_timer_callback(NULL, test_timer_callback_context, ptp_timer);
    }

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_timer_destroy(timer_instance);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* on_work_callback_v2 */

/* Tests_SRS_THREADPOOL_WIN32_05_001: [ If context is NULL, on_work_callback_v2 shall Log Message with severity CRITICAL and terminate. ]*/
TEST_FUNCTION(on_work_callback_v2_with_NULL_context_returns)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PVOID test_work_item_context;
    PTP_WORK_CALLBACK test_work_callback_v2;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context)
        .CaptureReturn(&ptp_work);

    // act
    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    ASSERT_IS_NOT_NULL(return_work_item_context);
    // assert
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT))test_work_item_context);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    // act
    test_work_callback_v2(NULL, NULL, ptp_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    //TODO Needs to change, the cleanup park
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_05_002: [ Otherwise context shall be used as the context created in thread_create_work_item. ] */
/* Tests_SRS_THREADPOOL_WIN32_05_003: [ The work_function callback passed to threadpool_create_work_item shall be called with the work_function_context as an argument. work_function_context was set inside the threadpool_create_work_item as an argument to CreateThreadpoolContext. ] */
TEST_FUNCTION(on_work_callback_v2_triggers_the_user_work_function)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PVOID test_work_item_context;
    PTP_WORK_CALLBACK test_work_callback_v2;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context)
        .CaptureReturn(&ptp_work);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work);

    // act
    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    threadpool_schedule_work_item(threadpool, return_work_item_context);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT))test_work_item_context);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));

    // act
    test_work_callback_v2(NULL, test_work_item_context, ptp_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_05_002: [ Otherwise context shall be used as the context created in thread_create_work_item. ] */
/* Tests_SRS_THREADPOOL_WIN32_05_003: [ The work_function callback passed to threadpool_create_work_item shall be called with the work_function_context as an argument. work_function_context was set inside the threadpool_create_work_item as an argument to CreateThreadpoolContext. ] */
TEST_FUNCTION(on_work_callback_v2_triggers_2_user_work_functions)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PVOID test_work_item_context_1;
    PTP_WORK_CALLBACK test_work_callback_v2;
    PTP_WORK ptp_work_1;
    PVOID test_work_item_context_2;
    PTP_WORK ptp_work_2;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context_1)
        .CaptureReturn(&ptp_work_1);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work_1);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context_2)
        .CaptureReturn(&ptp_work_2);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work_2);


    // act
    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context_1 = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    threadpool_schedule_work_item(threadpool, return_work_item_context_1);
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context_1, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT))test_work_item_context_1);

    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context_2 = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4244);
    threadpool_schedule_work_item(threadpool, return_work_item_context_2);
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context_2, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT))test_work_item_context_2);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4244));

    // act
    test_work_callback_v2(NULL, test_work_item_context_1, ptp_work_1);
    test_work_callback_v2(NULL, test_work_item_context_2, ptp_work_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context_1, NULL);
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context_2, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_05_002: [ Otherwise context shall be used as the context created in thread_create_work_item. ] */
/* Tests_SRS_THREADPOOL_WIN32_05_003: [ The work_function callback passed to threadpool_create_work_item shall be called with the work_function_context as an argument. work_function_context was set inside the threadpool_create_work_item as an argument to CreateThreadpoolContext. ] */
TEST_FUNCTION(on_work_callback_v2_triggers_the_user_work_function_with_NULL_work_function_context)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PVOID test_work_item_context;
    PTP_WORK_CALLBACK test_work_callback_v2;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context)
        .CaptureReturn(&ptp_work);
    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work);

    // act
    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(threadpool, test_work_function, NULL);
    threadpool_schedule_work_item(threadpool, return_work_item_context);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT)) test_work_item_context);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_work_function(NULL));

    // act
    test_work_callback_v2(NULL, test_work_item_context, ptp_work);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_create_work_item */

/* Tests_SRS_THREADPOOL_WIN32_05_004: [ If threadpool is NULL, threadpool_create_work_item shall fail and return a NULL value. ] */

TEST_FUNCTION(threadpool_create_work_item_fails_for_NULL_threadpool)
{
    // act
    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(NULL, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, NULL, return_work_item_context);
}

/* Tests_SRS_THREADPOOL_WIN32_05_005: [ If work_function is NULL, threadpool_create_work_item shall fail and return a NULL value. ] */

TEST_FUNCTION(threadpool_create_work_item_fails_for_NULL_test_work_function)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    // act
    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(threadpool, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, NULL, return_work_item_context);

    // cleanup

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_05_007: [ If any error occurs, threadpool_create_work_item shall fail and return a NULL value. ]*/
/* Tests_SRS_THREADPOOL_WIN32_05_010: [ If any error occurs, threadpool_create_work_item shall fail, free the newly created context and return a NULL value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_work_item_fails)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    size_t i;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureReturn(&ptp_work);

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) result = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);

            // assert
            ASSERT_ARE_EQUAL(void_ptr, NULL, result, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_05_006: [ Otherwise threadpool_create_work_item shall allocate a context work_item_context of type THREADPOOL_WORK_ITEM_HANDLE where work_function, work_function_context, and ptp_work shall be saved. ] */
/* Tests_SRS_THREADPOOL_WIN32_05_008: [ threadpool_create_work_item shall create work_item_context member variable ptp_work of type PTP_WORK by calling CreateThreadpoolWork to set the callback function as on_work_callback_v2. ] */
/* Tests_SRS_THREADPOOL_WIN32_05_009: [ If there are no errors then this work_item_context of type THREADPOOL_WORK_ITEM_HANDLE would be returned indicating a succcess to the caller. ] */
TEST_FUNCTION(threadpool_create_work_item_succeeds)
{
    // arrange
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PVOID test_work_item_context;
    PTP_WORK_CALLBACK test_work_callback_v2;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context)
        .CaptureReturn(&ptp_work);

    // act
    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT))test_work_item_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_schedule_work_item */

/* Tests_SRS_THREADPOOL_WIN32_05_011: [ If threadpool is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ] */
TEST_FUNCTION(threadpool_schedule_work_item_fails_for_NULL_threadpool)
{
    // arrange
    int schedule_return_status;

    // act
    schedule_return_status = threadpool_schedule_work_item(NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, schedule_return_status);
}

/* Tests_SRS_THREADPOOL_WIN32_05_012: [ If work_item_context is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ] */
TEST_FUNCTION(threadpool_schedule_work_item_fails_for_NULL_work_item_context)
{
    // arrange
    int schedule_return_status;
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    // act
    schedule_return_status = threadpool_schedule_work_item(threadpool, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, schedule_return_status);

    // cleanup

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* Tests_SRS_THREADPOOL_WIN32_05_013: [ threadpool_schedule_work_item shall call SubmitThreadpoolWork to submit the work item for execution. ] */
TEST_FUNCTION(threadpool_schedule_work_item_succeeds)
{
    // arrange
    int schedule_return_status = 0;
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PVOID test_work_item_context;
    PTP_WORK_CALLBACK test_work_callback_v2;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context)
        .CaptureReturn(&ptp_work);

    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT))test_work_item_context);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work);

    // act
    schedule_return_status = threadpool_schedule_work_item(threadpool, return_work_item_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, schedule_return_status);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

/* threadpool_destroy_work_item */

/* Tests_SRS_THREADPOOL_WIN32_05_016: [ threadpool_destroy_work_item shall call WaitForThreadpoolWorkCallbacks to wait on all outstanding tasks being scheduled on this ptp_work. ] * /
/* Tests_SRS_THREADPOOL_WIN32_05_017: [ threadpool_destroy_work_item shall call CloseThreadpoolWork to close ptp_work. ] */
TEST_FUNCTION(threadpool_destroy_work_item_succeeds)
{
    // arrange
    int schedule_return_status = 0;
    PTP_CALLBACK_ENVIRON cbe;
    THANDLE(THREADPOOL) threadpool = test_create_threadpool(&cbe);

    PVOID test_work_item_context;
    PTP_WORK_CALLBACK test_work_callback_v2;
    PTP_WORK ptp_work;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolWork(IGNORED_ARG, IGNORED_ARG, cbe))
        .CaptureArgumentValue_pfnwk(&test_work_callback_v2)
        .CaptureArgumentValue_pv(&test_work_item_context)
        .CaptureReturn(&ptp_work);

    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) return_work_item_context = threadpool_create_work_item(threadpool, test_work_function, (void*)0x4243);
    ASSERT_ARE_EQUAL(void_ptr, return_work_item_context, (THANDLE(THREADPOOL_WORK_ITEM_CONTEXT))test_work_item_context);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_SubmitThreadpoolWork(IGNORED_ARG))
        .ValidateArgumentValue_pwk(&ptp_work);
    STRICT_EXPECTED_CALL(test_work_function((void*)0x4243));

    // act
    schedule_return_status = threadpool_schedule_work_item(threadpool, return_work_item_context);
    ASSERT_ARE_EQUAL(int, 0, schedule_return_status);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_WaitForThreadpoolWorkCallbacks(ptp_work, false));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolWork(ptp_work));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT)(&return_work_item_context, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

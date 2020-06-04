// Copyright (c) Microsoft. All rights reserved.

#ifdef __cplusplus
#include <cstdlib>
#include <cinttypes>
#else
#include <stdlib.h>
#include <inttypes.h>
#endif

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "azure_macro_utils/macro_utils.h"

void* real_malloc(size_t size)
{
    return malloc(size);
}

void real_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "execution_engine.h"
#include "execution_engine_win32.h"

#include "gballoc.h"

#undef ENABLE_MOCKS

#include "threadpool.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;
static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4243;
static PTP_POOL test_pool = (PTP_POOL)0x4244;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#ifdef __cplusplus
extern "C"
{
#endif

MOCK_FUNCTION_WITH_CODE(, void, mocked_InitializeThreadpoolEnvironment, PTP_CALLBACK_ENVIRON, pcbe)
    // We are using the Pool member to force a memory allocation to check for leaks
    pcbe->Pool = (PTP_POOL)real_malloc(1);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_SetThreadpoolCallbackPool, PTP_CALLBACK_ENVIRON, pcbe, PTP_POOL, ptpp)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, PTP_CLEANUP_GROUP, mocked_CreateThreadpoolCleanupGroup)
MOCK_FUNCTION_END((PTP_CLEANUP_GROUP)real_malloc(1))
MOCK_FUNCTION_WITH_CODE(, VOID, mocked_SetThreadpoolCallbackCleanupGroup, PTP_CALLBACK_ENVIRON, pcbe, PTP_CLEANUP_GROUP, ptpcg, PTP_CLEANUP_GROUP_CANCEL_CALLBACK, pfng)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, PTP_WORK, mocked_CreateThreadpoolWork, PTP_WORK_CALLBACK, pfnwk, PVOID, pv, PTP_CALLBACK_ENVIRON, pcbe)
MOCK_FUNCTION_END((PTP_WORK)real_malloc(1))
MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolWork, PTP_WORK, pwk)
    real_free(pwk);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolCleanupGroup, PTP_CLEANUP_GROUP, ptpcg)
    real_free(ptpcg);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_DestroyThreadpoolEnvironment, PTP_CALLBACK_ENVIRON, pcbe)
    // We are using the Pool member to force a memory allocation to check for leaks
    real_free(pcbe->Pool);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolCleanupGroupMembers, PTP_CLEANUP_GROUP, ptpcg, BOOL, fCancelPendingCallbacks, PVOID, pvCleanupContext)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_SubmitThreadpoolWork, PTP_WORK, pwk)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_on_open_complete, void*, context, THREADPOOL_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, test_work_function, void*, context)
MOCK_FUNCTION_END()

#ifdef __cplusplus
}
#endif

BEGIN_TEST_SUITE(threadpool_win32_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_init failed");

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_stdint_register_types failed");

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, real_free);

    REGISTER_GLOBAL_MOCK_RETURN(execution_engine_win32_get_threadpool, test_pool);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(execution_engine_win32_get_threadpool, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_CreateThreadpoolCleanupGroup, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_CreateThreadpoolWork, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(PTP_WORK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CALLBACK_ENVIRON, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_POOL, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CLEANUP_GROUP, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CLEANUP_GROUP_CANCEL_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, int);
    REGISTER_UMOCK_ALIAS_TYPE(PVOID, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_WORK_CALLBACK, void*);
    
    REGISTER_TYPE(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/* threadpool_create */

/* Tests_SRS_THREADPOOL_WIN32_01_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(threadpool_create_with_NULL_execution_engine_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool;

    // act
    threadpool = threadpool_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_001: [ threadpool_create shall allocate a new threadpool object and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_025: [ threadpool_create shall obtain the PTP_POOL from the execution engine by calling execution_engine_win32_get_threadpool. ]*/
TEST_FUNCTION(threadpool_create_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_win32_get_threadpool(test_execution_engine));

    // act
    threadpool = threadpool_create(test_execution_engine);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(threadpool);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_003: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_create_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool;
    size_t i;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_win32_get_threadpool(test_execution_engine));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
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

/* threadpool_destroy */

/* Tests_SRS_THREADPOOL_WIN32_01_004: [ If threadpool is NULL, threadpool_destroy shall return. ]*/
TEST_FUNCTION(threadpool_destroy_with_NULL_threadpool_returns)
{
    // arrange

    // act
    threadpool_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_WIN32_01_005: [ Otherwise, threadpool_destroy shall free all resources associated with threadpool. ]*/
TEST_FUNCTION(threadpool_destroy_frees_resources)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    threadpool_destroy(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_WIN32_01_006: [ While threadpool is OPENING or CLOSING, threadpool_destroy shall wait for the open to complete either succesfully or with error. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_007: [ threadpool_destroy shall perform an implicit close if threadpool is OPEN. ]*/
TEST_FUNCTION(threadpool_destroy_performs_an_implicit_close)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    // close
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroupMembers(test_cleanup_group, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroup(test_cleanup_group));
    STRICT_EXPECTED_CALL(mocked_DestroyThreadpoolEnvironment(cbe));

    // destroy calls
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    threadpool_destroy(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* threadpool_open_async */

/* Tests_SRS_THREADPOOL_WIN32_01_008: [ If threadpool is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
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

/* Tests_SRS_THREADPOOL_WIN32_01_009: [ If on_open_complete is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
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

/* Tests_SRS_THREADPOOL_WIN32_01_011: [ Otherwise, threadpool_open_async shall switch the state to OPENING. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_026: [ threadpool_open_async shall initialize a thread pool environment by calling InitializeThreadpoolEnvironment. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_027: [ threadpool_open_async shall set the thread pool for the environment to the pool obtained from the execution engine by calling SetThreadpoolCallbackPool. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_028: [ threadpool_open_async shall create a threadpool cleanup group by calling CreateThreadpoolCleanupGroup. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_029: [ threadpool_open_async shall associate the cleanup group with the just created environment by calling SetThreadpoolCallbackCleanupGroup. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_015: [ threadpool_open_async shall set the state to OPEN. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_012: [ On success, threadpool_open_async shall return 0. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_014: [ On success, threadpool_open_async shall call on_open_complete_context shall with THREADPOOL_OPEN_OK. ]*/
TEST_FUNCTION(threadpool_open_async_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackCleanupGroup(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_ptpcg(&test_cleanup_group);
    STRICT_EXPECTED_CALL(test_on_open_complete((void*)0x4242, THREADPOOL_OPEN_OK));

    // act
    result = threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_010: [ on_open_complete_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(threadpool_open_async_succeeds_with_NULL_context)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackCleanupGroup(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_ptpcg(&test_cleanup_group);
    STRICT_EXPECTED_CALL(test_on_open_complete(NULL, THREADPOOL_OPEN_OK));

    // act
    result = threadpool_open_async(threadpool, test_on_open_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_040: [ If any error occurrs, threadpool_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_open_async_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    int result;
    size_t i;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackCleanupGroup(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_ptpcg(&test_cleanup_group);
    STRICT_EXPECTED_CALL(test_on_open_complete((void*)0x4242, THREADPOOL_OPEN_OK));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_013: [ If threadpool is already OPEN or OPENING, threadpool_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(threadpool_open_async_after_open_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    int result;
    umock_c_reset_all_calls();

    // act
    result = threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* threadpool_close */

/* Tests_SRS_THREADPOOL_WIN32_01_016: [ If threadpool is NULL, threadpool_close shall return. ]*/
TEST_FUNCTION(threadpool_close_with_NULL_handle_returns)
{
    // arrange

    // act
    threadpool_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREADPOOL_WIN32_01_017: [ Otherwise, threadpool_close shall switch the state to CLOSING. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_030: [ threadpool_close shall wait for any executing callbacks by calling CloseThreadpoolCleanupGroupMembers, passing FALSE as fCancelPendingCallbacks. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_032: [ threadpool_close shall close the threadpool cleanup group by calling CloseThreadpoolCleanupGroup. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_033: [ threadpool_close shall destroy the thread pool environment created in threadpool_open_async. ]*/
TEST_FUNCTION(threadpool_close_reverses_the_open_actions)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroupMembers(test_cleanup_group, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroup(test_cleanup_group));
    STRICT_EXPECTED_CALL(mocked_DestroyThreadpoolEnvironment(cbe));

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_019: [ If threadpool is not OPEN, threadpool_close shall return. ]*/
TEST_FUNCTION(threadpool_close_when_not_open_returns)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    umock_c_reset_all_calls();

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_019: [ If threadpool is not OPEN, threadpool_close shall return. ]*/
TEST_FUNCTION(threadpool_close_after_close_returns)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    threadpool_close(threadpool);
    umock_c_reset_all_calls();

    // act
    threadpool_close(threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    threadpool_destroy(threadpool);
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
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    threadpool_close(threadpool);
    umock_c_reset_all_calls();

    // act
    result = threadpool_schedule_work(threadpool, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_023: [ Otherwise threadpool_schedule_work shall allocate a context where work_function and context shall be saved. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_034: [ threadpool_schedule_work shall call CreateThreadpoolWork to schedule executiong the callback while apssing to it the on_work_callback function and the newly created context. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_041: [ threadpool_schedule_work shall call SubmitThreadpoolWork to submit the work item for execution. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_022: [ work_function_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(threadpool_schedule_work_succeeds_with_NULL_work_function_context)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_024: [ If any error occurs, threadpool_schedule_work shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_threadpool_schedule_work_fails)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    int result;
    PTP_CALLBACK_ENVIRON cbe;
    size_t i;
    PTP_WORK ptp_work;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

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
    threadpool_destroy(threadpool);
}

/* on_work_callback */

/* Tests_SRS_THREADPOOL_WIN32_01_035: [ If context is NULL, on_work_callback shall return. ]*/
TEST_FUNCTION(on_work_callback_with_NULL_context_returns)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_036: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_037: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_038: [ on_work_callback shall call CloseThreadpoolWork. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_039: [ on_work_callback shall free the context allocated in threadpool_schedule_work. ]*/
TEST_FUNCTION(on_work_callback_triggers_the_user_work_function)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_036: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_037: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_038: [ on_work_callback shall call CloseThreadpoolWork. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_039: [ on_work_callback shall free the context allocated in threadpool_schedule_work. ]*/
TEST_FUNCTION(on_work_callback_triggers_2_user_work_functions)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context_1;
    PVOID test_work_callback_context_2;
    PTP_WORK ptp_work_1;
    PTP_WORK ptp_work_2;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

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
    threadpool_destroy(threadpool);
}

/* Tests_SRS_THREADPOOL_WIN32_01_036: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_037: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_038: [ on_work_callback shall call CloseThreadpoolWork. ]*/
/* Tests_SRS_THREADPOOL_WIN32_01_039: [ on_work_callback shall free the context allocated in threadpool_schedule_work. ]*/
TEST_FUNCTION(on_work_callback_triggers_the_user_work_function_with_NULL_work_function_context)
{
    // arrange
    THREADPOOL_HANDLE threadpool = threadpool_create(test_execution_engine);
    PTP_CALLBACK_ENVIRON cbe;
    PTP_WORK_CALLBACK test_work_callback;
    PVOID test_work_callback_context;
    PTP_WORK ptp_work;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    (void)threadpool_open_async(threadpool, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

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
    threadpool_destroy(threadpool);
}

END_TEST_SUITE(threadpool_win32_unittests)

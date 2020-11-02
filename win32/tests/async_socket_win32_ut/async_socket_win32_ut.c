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
#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"
void* real_malloc(size_t size)
{
    return real_gballoc_ll_malloc(size);
}

void real_free(void* ptr)
{
    real_gballoc_ll_free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/async_socket.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;
static SOCKET_HANDLE test_socket = (SOCKET_HANDLE)0x4242;
static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4243;
static PTP_POOL test_pool = (PTP_POOL)0x4244;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)

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
MOCK_FUNCTION_WITH_CODE(WINAPI, PTP_IO, mocked_CreateThreadpoolIo, HANDLE, fl, PTP_WIN32_IO_CALLBACK, pfnio, PVOID, pv, PTP_CALLBACK_ENVIRON, pcbe)
MOCK_FUNCTION_END((PTP_IO)real_malloc(1))
MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolIo, PTP_IO, pio)
    real_free(pio);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_CloseThreadpoolCleanupGroup, PTP_CLEANUP_GROUP, ptpcg)
    real_free(ptpcg);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_DestroyThreadpoolEnvironment, PTP_CALLBACK_ENVIRON, pcbe)
    // We are using the Pool member to force a memory allocation to check for leaks
    real_free(pcbe->Pool);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, HANDLE, mocked_CreateEventA, LPSECURITY_ATTRIBUTES, lpEventAttributes, BOOL, bManualReset, BOOL, bInitialState, LPCSTR, lpName)
MOCK_FUNCTION_END((HANDLE)real_malloc(1))
MOCK_FUNCTION_WITH_CODE(, void, mocked_StartThreadpoolIo, PTP_IO, pio)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, int, mocked_WSASend, SOCKET, s, LPWSABUF, lpBuffers, DWORD, dwBufferCount, LPDWORD, lpNumberOfBytesSent, DWORD, dwFlags, LPWSAOVERLAPPED, lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE, lpCompletionRoutine)
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_WSAGetLastError)
MOCK_FUNCTION_END(ERROR_SUCCESS)
MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_CloseHandle, HANDLE, hObject)
    real_free(hObject);
MOCK_FUNCTION_END(TRUE)
MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_WSARecv, SOCKET, s, LPWSABUF, lpBuffers, DWORD, dwBufferCount, LPDWORD, lpNumberOfBytesRecvd, LPDWORD, lpFlags, LPWSAOVERLAPPED, lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE, lpCompletionRoutine)
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, void, mocked_WaitForThreadpoolIoCallbacks, PTP_IO, pio, BOOL, fCancelPendingCallbacks)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mocked_CancelThreadpoolIo, PTP_IO, pio)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_on_open_complete, void*, context, ASYNC_SOCKET_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, test_on_send_complete, void*, context, ASYNC_SOCKET_SEND_RESULT, send_result)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, test_on_receive_complete, void*, context, ASYNC_SOCKET_RECEIVE_RESULT, receive_result, uint32_t, bytes_received)
MOCK_FUNCTION_END()

#ifdef __cplusplus
}
#endif

BEGIN_TEST_SUITE(async_socket_win32_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_init failed");

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_stdint_register_types failed");

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_charptr_register_types failed");

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, real_free);

    REGISTER_GLOBAL_MOCK_RETURN(execution_engine_win32_get_threadpool, test_pool);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_CreateThreadpoolCleanupGroup, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_CreateThreadpoolIo, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_CreateEventA, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_WSASend, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_WSARecv, 1);

    REGISTER_UMOCK_ALIAS_TYPE(PTP_IO, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CALLBACK_ENVIRON, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_POOL, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CLEANUP_GROUP, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CLEANUP_GROUP_CANCEL_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_WIN32_IO_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PVOID, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, int);
    REGISTER_UMOCK_ALIAS_TYPE(LPSECURITY_ATTRIBUTES, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPCSTR, char*);
    REGISTER_UMOCK_ALIAS_TYPE(SOCKET, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSABUF, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, unsigned long);
    REGISTER_UMOCK_ALIAS_TYPE(LPDWORD, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED_COMPLETION_ROUTINE, void*);

    REGISTER_TYPE(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT);
    REGISTER_TYPE(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT);
    REGISTER_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(test_serialize_mutex);

    real_gballoc_hl_deinit();
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

/* async_socket_create */

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_002: [ If execution_engine is NULL, async_socket_create shall fail and return NULL. ]*/
TEST_FUNCTION(async_socket_create_with_NULL_execution_engine_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket;

    // act
    async_socket = async_socket_create(NULL, test_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_034: [ If socket_handle is INVALID_SOCKET, async_socket_create shall fail and return NULL. ]*/
TEST_FUNCTION(async_socket_create_with_INVALID_SOCKET_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket;

    // act
    async_socket = async_socket_create(test_execution_engine, (SOCKET_HANDLE)INVALID_SOCKET);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_001: [ async_socket_create shall allocate a new async socket and on success shall return a non-NULL handle. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_035: [ Otherwise, async_socket_open_async shall obtain the PTP_POOL from the execution engine passed to async_socket_create by calling execution_engine_win32_get_threadpool. ]*/
TEST_FUNCTION(async_socket_create_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_win32_get_threadpool(test_execution_engine));

    // act
    async_socket = async_socket_create(test_execution_engine, test_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(async_socket);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_003: [ If any error occurs, async_socket_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fails_async_socket_create_also_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket;
    size_t i;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            async_socket = async_socket_create(NULL, test_socket);

            // assert
            ASSERT_IS_NULL(async_socket, "On failed call %zu", i);
        }
    }
}

/* async_socket_destroy */

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_004: [ If async_socket is NULL, async_socket_destroy shall return. ]*/
TEST_FUNCTION(async_socket_destroy_with_NULL_returns)
{
    // arrange

    // act
    async_socket_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_005: [ Otherwise, async_socket_destroy shall free all resources associated with async_socket. ]*/
TEST_FUNCTION(async_socket_destroy_frees_resources)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    async_socket_destroy(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_006: [ async_socket_destroy shall perform an implicit close if async_socket is OPEN. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_093: [ While async_socket is OPENING or CLOSING, async_socket_destroy shall wait for the open to complete either successfully or with error. ]*/
TEST_FUNCTION(async_socket_destroy_closes_first_if_open)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    PTP_CLEANUP_GROUP test_cleanup_group;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_IO test_ptp_io;
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    // close first
    STRICT_EXPECTED_CALL(mocked_WaitForThreadpoolIoCallbacks(test_ptp_io, FALSE));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroup(test_cleanup_group));
    STRICT_EXPECTED_CALL(mocked_DestroyThreadpoolEnvironment(cbe));

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    async_socket_destroy(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* async_socket_open_async */

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_007: [ If async_socket is NULL, async_socket_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_open_async_with_NULL_async_socket_fails)
{
    // arrange
    int result;

    // act
    result = async_socket_open_async(NULL, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_008: [ If on_open_complete is NULL, async_socket_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_open_async_with_NULL_on_open_complete_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    umock_c_reset_all_calls();

    // act
    result = async_socket_open_async(async_socket, NULL, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_023: [ Otherwise, async_socket_open_async shall switch the state to OPENING. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_014: [ On success, async_socket_open_async shall return 0. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_016: [ Otherwise async_socket_open_async shall initialize a thread pool environment by calling InitializeThreadpoolEnvironment. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_035: [ async_socket_open_async shall obtain the PTP_POOL from the execution engine passed to async_socket_create by calling execution_engine_win32_get_threadpool. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_036: [ async_socket_open_async shall set the thread pool for the environment to the pool obtained from the execution engine by calling SetThreadpoolCallbackPool. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_037: [ async_socket_open_async shall create a threadpool cleanup group by calling CreateThreadpoolCleanupGroup. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_058: [ async_socket_open_async shall create a threadpool IO by calling CreateThreadpoolIo and passing socket_handle, the callback environment to it and on_io_complete as callback. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_017: [ On success async_socket_open_async shall call on_open_complete_context with ASYNC_SOCKET_OPEN_OK. ]*/
TEST_FUNCTION(async_socket_open_async_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(test_on_open_complete((void*)0x4242, ASYNC_SOCKET_OPEN_OK));

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_009: [ on_open_complete_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(async_socket_open_async_succeeds_with_NULL_context)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(test_on_open_complete(NULL, ASYNC_SOCKET_OPEN_OK));

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_039: [ If any error occurs, async_socket_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_async_socket_open_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    size_t i;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(test_on_open_complete((void*)0x4242, ASYNC_SOCKET_OPEN_OK));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_015: [ If async_socket is already OPEN or OPENING, async_socket_open_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_open_async_after_async_socket_open_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    umock_c_reset_all_calls();

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_017: [ On success async_socket_open_async shall call on_open_complete_context with ASYNC_SOCKET_OPEN_OK. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_094: [ async_socket_open_async shall set the state to OPEN. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_021: [ Then async_socket_close shall close the async socket, leaving it in a state where an async_socket_open_async can be performed. ]*/
TEST_FUNCTION(async_socket_open_async_after_close_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    async_socket_close(async_socket);
    int result;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_CLEANUP_GROUP test_cleanup_group;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .ValidateArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(test_on_open_complete((void*)0x4242, ASYNC_SOCKET_OPEN_OK));

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* async_socket_close */

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_018: [ If async_socket is NULL, async_socket_close shall return. ]*/
TEST_FUNCTION(async_socket_close_with_NULL_returns)
{
    // arrange

    // act
    async_socket_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_019: [ Otherwise, async_socket_close shall switch the state to CLOSING. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_040: [ async_socket_close shall wait for any executing callbacks by calling WaitForThreadpoolIoCallbacks, passing FALSE as fCancelPendingCallbacks. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_059: [ async_socket_close shall close the threadpool IO created in async_socket_open_async by calling CloseThreadpoolIo. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_041: [ async_socket_close shall close the threadpool cleanup group by calling CloseThreadpoolCleanupGroup. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_042: [ async_socket_close shall destroy the thread pool environment created in async_socket_open_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_020: [ async_socket_close shall wait for all executing async_socket_send_async and async_socket_receive_async APIs. ]*/
TEST_FUNCTION(async_socket_close_reverses_the_actions_from_open)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    PTP_CLEANUP_GROUP test_cleanup_group;
    PTP_CALLBACK_ENVIRON cbe;
    PTP_IO test_ptp_io;

    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG))
        .CaptureArgumentValue_pcbe(&cbe);
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup())
        .CaptureReturn(&test_cleanup_group);
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_WaitForThreadpoolIoCallbacks(test_ptp_io, FALSE));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolCleanupGroup(test_cleanup_group));
    STRICT_EXPECTED_CALL(mocked_DestroyThreadpoolEnvironment(cbe));

    // act
    async_socket_close(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_022: [ If async_socket is not OPEN, async_socket_close shall return. ]*/
TEST_FUNCTION(async_socket_close_when_not_open_returns)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    umock_c_reset_all_calls();

    // act
    async_socket_close(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_022: [ If async_socket is not OPEN, async_socket_close shall return. ]*/
TEST_FUNCTION(async_socket_close_after_close_returns)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    async_socket_close(async_socket);
    umock_c_reset_all_calls();

    // act
    async_socket_close(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* async_socket_send_async */

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_024: [ If async_socket is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_NULL_async_socket_fails)
{
    // arrange
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    // act
    result = async_socket_send_async(NULL, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_025: [ If buffers is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_NULL_payload_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, NULL, 1, test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_085: [ If buffer_count is 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_0_payload_count_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, 0, test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_089: [ If any of the buffers in payload has buffer set to NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_first_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = NULL;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_089: [ If any of the buffers in payload has buffer set to NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_second_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = NULL;
    payload_buffers[1].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_090: [ If any of the buffers in payload has length set to 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_first_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = 0;
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_090: [ If any of the buffers in payload has length set to 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_second_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 0;
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_103: [ If the amount of memory needed to allocate the context and the WSABUF items is exceeding UINT32_MAX, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_UINT32_MAX_buffers_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 0;
    umock_c_reset_all_calls();

    // act
    // UINT32_MAX will for sure make us want more memory than UINT32_MAX bytes
    result = async_socket_send_async(async_socket, payload_buffers, UINT32_MAX, test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_026: [ If on_send_complete is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_on_send_complete_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), NULL, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_097: [ If async_socket is not OPEN, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]*/
TEST_FUNCTION(async_socket_send_async_when_not_open_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ABANDONED, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_097: [ If async_socket is not OPEN, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]*/
TEST_FUNCTION(async_socket_send_async_after_close_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    (void)async_socket_close(async_socket);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ABANDONED, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_028: [ Otherwise async_socket_send_async shall create a context for the send where the payload, on_send_complete and on_send_complete_context shall be stored. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_050: [ The context shall also allocate enough memory to keep an array of buffer_count WSABUF items. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_056: [ async_socket_send_async shall set the WSABUF items to point to the memory/length of the buffers in payload. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_057: [ An event to be used for the OVERLAPPED structure passed to WSASend shall be created and stored in the context. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_060: [ An asynchronous IO shall be started by calling StartThreadpoolIo. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_061: [ The WSABUF array associated with the context shall be sent by calling WSASend and passing to it the OVERLAPPED structure with the event that was just created, dwFlags set to 0, lpNumberOfBytesSent set to NULL and lpCompletionRoutine set to NULL. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_045: [ On success, async_socket_send_async shall return ASYNC_SOCKET_SEND_SYNC_OK. ]*/
TEST_FUNCTION(async_socket_send_async_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped);

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, result);

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_027: [ on_send_complete_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(async_socket_send_async_with_NULL_on_send_complete_context_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped);

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, result);

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_029: [ If any error occurs, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_062: [ If WSASend fails, async_socket_send_async shall call WSAGetLastError. ]*/
TEST_FUNCTION(when_underlying_calls_fail_async_socket_send_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    size_t i;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSAEINVAL)
        .CallCannotFail();

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

            // assert
            ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result, "On failed call %zu", i);
        }
    }

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_062: [ If WSASend fails, async_socket_send_async shall call WSAGetLastError. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_053: [ If WSAGetLastError returns WSA_IO_PENDING, it shall be not treated as an error. ]*/
TEST_FUNCTION(when_get_last_error_for_send_returns_WSA_IO_PENDING_it_is_treated_as_successfull)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;

    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_100: [ If WSAGetLastError returns any other error, async_socket_send_async shall call CancelThreadpoolIo. ]*/
TEST_FUNCTION(when_get_last_error_for_send_returns_an_error_then_async_socket_send_async_cancels_the_IO)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    HANDLE overlapped_event;

    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&overlapped_event);
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSAENOBUFS);
    STRICT_EXPECTED_CALL(mocked_CancelThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG))
        .ValidateArgumentValue_hObject(&overlapped_event);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_42_002: [ If WSAGetLastError returns WSAECONNRESET, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]*/
TEST_FUNCTION(when_get_last_error_for_send_returns_WSAGetLastError_then_async_socket_send_async_cancels_the_IO_and_returns_ABANDONED)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    HANDLE overlapped_event;

    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&overlapped_event);
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSAECONNRESET);
    STRICT_EXPECTED_CALL(mocked_CancelThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG))
        .ValidateArgumentValue_hObject(&overlapped_event);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ABANDONED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_106: [ If WSASend fails with any other error, async_socket_send_async shall call CancelThreadpoolIo and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(when_WSASend_returns_an_error_different_than_SOCKET_ERROR_async_socket_send_async_cancels_the_IO)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    HANDLE overlapped_event;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&overlapped_event);
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_CancelThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG))
        .ValidateArgumentValue_hObject(&overlapped_event);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_101: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]*/
TEST_FUNCTION(async_socket_send_async_with_sum_of_buffer_lengths_exceeding_UINT32_MAX_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = UINT32_MAX;
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 1;
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* async_socket_receive_async */

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_073: [ If async_socket is NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_NULL_async_socket_fails)
{
    // arrange
    int result;
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    // act
    result = async_socket_receive_async(NULL, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_074: [ If buffers is NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_NULL_payload_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, NULL, 1, test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_086: [ If buffer_count is 0, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_0_payload_count_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, 0, test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_091: [ If any of the buffers in payload has buffer set to NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_first_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = NULL;
    payload_buffers[0].length = sizeof(payload_bytes_1);
    payload_buffers[1].buffer = payload_bytes_2;
    payload_buffers[1].length = sizeof(payload_bytes_2);
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_091: [ If any of the buffers in payload has buffer set to NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_second_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes_1;
    payload_buffers[0].length = sizeof(payload_bytes_1);
    payload_buffers[1].buffer = NULL;
    payload_buffers[1].length = sizeof(payload_bytes_2);
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_092: [ If any of the buffers in payload has length set to 0, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_first_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes_1;
    payload_buffers[0].length = 0;
    payload_buffers[1].buffer = payload_bytes_2;
    payload_buffers[1].length = sizeof(payload_bytes_2);
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_092: [ If any of the buffers in payload has length set to 0, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_second_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes_1;
    payload_buffers[0].length = sizeof(payload_bytes_1);
    payload_buffers[1].buffer = payload_bytes_2;
    payload_buffers[1].length = 0;
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_096: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_sum_of_buffer_lengths_exceeding_UINT32_MAX_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = UINT32_MAX;
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 1;
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_104: [ If the amount of memory needed to allocate the context and the WSABUF items is exceeding UINT32_MAX, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_UINT32_MAX_buffers_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 0;
    umock_c_reset_all_calls();

    // act
    // UINT32_MAX will for sure make us want more memory than UINT32_MAX bytes
    result = async_socket_receive_async(async_socket, payload_buffers, UINT32_MAX, test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_075: [ If on_receive_complete is NULL, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_with_on_send_complete_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), NULL, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_098: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_when_not_open_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];

    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_098: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_socket_receive_async_after_close_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];

    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    (void)async_socket_close(async_socket);
    umock_c_reset_all_calls();

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_077: [ Otherwise async_socket_receive_async shall create a context for the send where the payload, on_receive_complete and on_receive_complete_context shall be stored. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_078: [ The context shall also allocate enough memory to keep an array of buffer_count WSABUF items. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_079: [ async_socket_receive_async shall set the WSABUF items to point to the memory/length of the buffers in payload. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_080: [ An event to be used for the OVERLAPPED structure passed to WSARecv shall be created and stored in the context. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_081: [ An asynchronous IO shall be started by calling StartThreadpoolIo. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_082: [ A receive shall be started for the WSABUF array associated with the context calling WSARecv and passing to it the OVERLAPPED structure with the event that was just created, dwFlags set to 0, lpNumberOfBytesSent set to NULL and lpCompletionRoutine set to NULL. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_083: [ On success, async_socket_receive_async shall return 0. ]*/
TEST_FUNCTION(async_socket_receive_async_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;

    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped);

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_076: [ on_receive_complete_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(async_socket_receive_async_with_NULL_on_send_complete_context_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    DWORD expected_flags = 0;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .ValidateArgumentBuffer(5, &expected_flags, sizeof(expected_flags))
        .CaptureArgumentValue_lpOverlapped(&overlapped);

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_084: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_054: [ If WSARecv fails with SOCKET_ERROR, async_socket_receive_async shall call WSAGetLastError. ]*/
TEST_FUNCTION(when_underlying_calls_fail_async_socket_receive_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    size_t i;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSAEINVAL)
        .CallCannotFail();

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_054: [ If WSARecv fails with SOCKET_ERROR, async_socket_receive_async shall call WSAGetLastError. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_055: [ If WSAGetLastError returns IO_PENDING, it shall be not treated as an error. ]*/
TEST_FUNCTION(when_get_last_error_for_receive_returns_WSA_IO_PENDING_it_is_treated_as_successfull)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    DWORD expected_flags = 0;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .ValidateArgumentBuffer(5, &expected_flags, sizeof(expected_flags))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_105: [ If WSARecv fails with any other error, async_socket_receive_async shall call CancelThreadpoolIo and return a non-zero value. ]*/
TEST_FUNCTION(when_WSARecv_returns_an_error_different_than_SOCKET_ERROR_async_socket_receive_async_cancels_the_IO)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    DWORD expected_flags = 0;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    HANDLE overlapped_event;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&overlapped_event);
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .ValidateArgumentBuffer(5, &expected_flags, sizeof(expected_flags))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_CancelThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG))
        .ValidateArgumentValue_hObject(&overlapped_event);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_099: [ If WSAGetLastError returns any other error, async_socket_receive_async shall call CancelThreadpoolIo. ]*/
TEST_FUNCTION(when_get_last_error_for_receive_returns_an_error_then_async_socket_receive_async_cancels_the_IO)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    int result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    DWORD expected_flags = 0;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    HANDLE overlapped_event;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&overlapped_event);
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .ValidateArgumentBuffer(5, &expected_flags, sizeof(expected_flags))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSAECONNRESET);
    STRICT_EXPECTED_CALL(mocked_CancelThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG))
        .ValidateArgumentValue_hObject(&overlapped_event);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* on_io_complete */

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_063: [ If overlapped is NULL, on_io_complete shall return. ]*/
TEST_FUNCTION(on_io_complete_with_NULL_overlapped_for_send_returns)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped);

    (void)async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    // act
    test_on_io_complete(NULL, test_ptp_io_context, NULL, NO_ERROR, (ULONG_PTR)1, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_065: [ If the context of the IO indicates that a send has completed: ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_066: [ If io_result is NO_ERROR, the on_send_complete callback passed to async_socket_send_async shall be called with on_send_complete_context as argument and ASYNC_SOCKET_SEND_OK. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
TEST_FUNCTION(on_io_complete_with_NO_ERROR_indicates_the_send_as_complete_with_OK)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_send_complete((void*)0x4244, ASYNC_SOCKET_SEND_OK));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_065: [ If the context of the IO indicates that a send has completed: ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_067: [ If io_result is not NO_ERROR, the on_send_complete callback passed to async_socket_send_async shall be called with on_send_complete_context as argument and ASYNC_SOCKET_SEND_ERROR. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
TEST_FUNCTION(on_io_complete_with_error_indicates_the_send_as_complete_with_ERROR)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_send_complete((void*)0x4244, ASYNC_SOCKET_SEND_ERROR));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, WSAECANCELLED, (ULONG_PTR)1, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_102: [ If io_result is NO_ERROR, but the number of bytes send is different than the sum of all buffer sizes passed to async_socket_send_async, the on_send_complete callback passed to async_socket_send_async shall be called with on_send_complete_context as context and ASYNC_SOCKET_SEND_ERROR. ]*/
TEST_FUNCTION(on_io_complete_with_NO_ERROR_and_number_of_bytes_sent_less_than_expected_indicates_error)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[3] = { 0x42, 0x43, 0x44 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_send_complete((void*)0x4244, ASYNC_SOCKET_SEND_ERROR));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)2, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_102: [ If io_result is NO_ERROR, but the number of bytes send is different than the sum of all buffer sizes passed to async_socket_send_async, the on_send_complete callback passed to async_socket_send_async shall be called with on_send_complete_context as context and ASYNC_SOCKET_SEND_ERROR. ]*/
TEST_FUNCTION(on_io_complete_with_NO_ERROR_and_number_of_bytes_sent_more_than_expected_indicates_error)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[3] = { 0x42, 0x43, 0x44 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSASend((SOCKET)test_socket, IGNORED_ARG, 1, NULL, 0, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_send_complete((void*)0x4244, ASYNC_SOCKET_SEND_ERROR));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)4, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_063: [ If overlapped is NULL, on_io_complete shall return. ]*/
TEST_FUNCTION(on_io_complete_with_NULL_overlapped_for_receive_returns)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped);
    (void)async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    // act
    test_on_io_complete(NULL, test_ptp_io_context, NULL, NO_ERROR, (ULONG_PTR)1, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_071: [ If the context of the IO indicates that a receive has completed: ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_069: [ If io_result is NO_ERROR, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_OK as result and number_of_bytes_transferred as bytes_received. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
TEST_FUNCTION(on_io_complete_with_NO_ERROR_indicates_the_receive_as_complete_with_OK)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_receive_complete((void*)0x4244, ASYNC_SOCKET_RECEIVE_OK, 1));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)1, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_071: [ If the context of the IO indicates that a receive has completed: ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_069: [ If io_result is NO_ERROR, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_OK as result and number_of_bytes_transferred as bytes_received. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
TEST_FUNCTION(on_io_complete_with_NO_ERROR_indicates_the_receive_as_complete_with_OK_with_2_bytes_received)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[2];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_receive_complete((void*)0x4244, ASYNC_SOCKET_RECEIVE_OK, 2));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)2, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_095: [If io_result is NO_ERROR, but the number of bytes received is greater than the sum of all buffer sizes passed to async_socket_receive_async, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ERROR as result and number_of_bytes_transferred for bytes_received. ]*/
TEST_FUNCTION(on_io_complete_with_NO_ERROR_indicates_the_receive_as_complete_with_OK_with_2_bytes_received_when_buffers_had_only_1_byte)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_receive_complete((void*)0x4244, ASYNC_SOCKET_RECEIVE_ERROR, 2));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, NO_ERROR, (ULONG_PTR)2, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_071: [ If the context of the IO indicates that a receive has completed: ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_070: [ If io_result is not NO_ERROR, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ERROR as result and 0 for bytes_received. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
TEST_FUNCTION(on_io_complete_with_error_indicates_the_receive_as_complete_with_ERROR)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_receive_complete((void*)0x4244, ASYNC_SOCKET_RECEIVE_ERROR, 0));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, WSAECANCELLED, (ULONG_PTR)1, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

static void on_io_complete_with_error_indicates_the_receive_as_complete_with_ABANDONED(ULONG error_code)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    PTP_IO test_ptp_io;
    PTP_WIN32_IO_CALLBACK test_on_io_complete;
    PVOID test_ptp_io_context;
    LPOVERLAPPED overlapped;
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(mocked_InitializeThreadpoolEnvironment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolCallbackPool(IGNORED_ARG, test_pool));
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolCleanupGroup());
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolIo(test_socket, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureReturn(&test_ptp_io)
        .CaptureArgumentValue_pv(&test_ptp_io_context)
        .CaptureArgumentValue_pfnio(&test_on_io_complete);
    (void)async_socket_open_async(async_socket, test_on_open_complete, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_StartThreadpoolIo(test_ptp_io));
    STRICT_EXPECTED_CALL(mocked_WSARecv((SOCKET)test_socket, IGNORED_ARG, 1, NULL, IGNORED_ARG, IGNORED_ARG, NULL))
        .CaptureArgumentValue_lpOverlapped(&overlapped)
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(mocked_WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    (void)async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, (void*)0x4244);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_on_receive_complete((void*)0x4244, ASYNC_SOCKET_RECEIVE_ABANDONED, 0));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    test_on_io_complete(NULL, test_ptp_io_context, overlapped, error_code, (ULONG_PTR)1, test_ptp_io);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_071: [ If the context of the IO indicates that a receive has completed: ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_42_001: [ If io_result is ERROR_NETNAME_DELETED or ERROR_CONNECTION_ABORTED, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ABANDONED as result and 0 for bytes_received. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
TEST_FUNCTION(on_io_complete_with_ERROR_NETNAME_DELETED_indicates_the_receive_as_complete_with_ABANDONED)
{
    on_io_complete_with_error_indicates_the_receive_as_complete_with_ABANDONED(ERROR_NETNAME_DELETED);
}

/* Tests_SRS_ASYNC_SOCKET_WIN32_01_064: [ overlapped shall be used to determine the context of the IO. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_071: [ If the context of the IO indicates that a receive has completed: ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_42_001: [ If io_result is ERROR_NETNAME_DELETED or ERROR_CONNECTION_ABORTED, the on_receive_complete callback passed to async_socket_receive_async shall be called with on_receive_complete_context as context, ASYNC_SOCKET_RECEIVE_ABANDONED as result and 0 for bytes_received. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_068: [ on_io_complete shall close the event handle created in async_socket_send_async/async_socket_receive_async. ]*/
/* Tests_SRS_ASYNC_SOCKET_WIN32_01_072: [ on_io_complete shall free the IO context. ]*/
TEST_FUNCTION(on_io_complete_with_ERROR_CONNECTION_ABORTED_indicates_the_receive_as_complete_with_ABANDONED)
{
    on_io_complete_with_error_indicates_the_receive_as_complete_with_ABANDONED(ERROR_CONNECTION_ABORTED);
}

END_TEST_SUITE(async_socket_win32_unittests)

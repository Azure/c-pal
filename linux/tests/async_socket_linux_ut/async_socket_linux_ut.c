// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <inttypes.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>                          // for memset
#include <sys/types.h>                       // for ssize_t

#include "macro_utils/macro_utils.h"  // IWYU pragma: keep

#include "real_gballoc_ll.h"    // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"              // for IMPLEMENT_UMOCK_C_ENUM_TYPE
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/socket_handle.h"
#include "c_pal/threadapi.h"

#undef ENABLE_MOCKS

#include "real_interlocked.h"
#include "real_gballoc_hl.h" // IWYU pragma: keep

#include "c_pal/async_socket.h"

#define TEST_MAX_EVENTS_NUM     64

static TEST_MUTEX_HANDLE test_serialize_mutex;
static SOCKET_HANDLE test_socket = (SOCKET_HANDLE)0x4242;
static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4243;
static void* test_callback_ctx = (void*)0x4244;

static struct epoll_event g_events[2];
static uint32_t g_event_index = 0;

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

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

MOCK_FUNCTION_WITH_CODE(, int, mocked_epoll_create, int, __size)
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_epoll_ctl, int, epfd, int, op, int, fd, struct epoll_event*, event)
    if (event != NULL)
    {
        if (op == EPOLL_CTL_MOD)
        {
            g_events[g_event_index++].data.ptr = event->data.ptr;
        }
    }
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_epoll_wait, int, epfd, struct epoll_event*, events, int, maxevents, int, timeout)
    events = &g_events[0];
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_close, int, s)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, ssize_t, mocked_send, int, fd, const void*, buf, size_t, n, int, flags)
MOCK_FUNCTION_END(n)
MOCK_FUNCTION_WITH_CODE(, ssize_t, mocked_recv, int, fd, void*, buf, size_t, n, int, flags)
MOCK_FUNCTION_END(n)

MOCK_FUNCTION_WITH_CODE(, void, test_on_open_complete, void*, context, ASYNC_SOCKET_OPEN_RESULT, open_result)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, test_on_send_complete, void*, context, ASYNC_SOCKET_SEND_RESULT, send_result)
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, test_on_receive_complete, void*, context, ASYNC_SOCKET_RECEIVE_RESULT, receive_result, uint32_t, bytes_received)
MOCK_FUNCTION_END()

static void mock_deinitialize_global_thread_setup(void)
{
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
}

static void mock_internal_close_setup(void)
{
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_DEL, IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_close(test_socket));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
}

static void mock_async_socket_create_setup(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_epoll_create(TEST_MAX_EVENTS_NUM));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
}

static void setup_async_socket_receive_async_mocks(void)
{
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, sizeof(ASYNC_SOCKET_BUFFER)));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_MOD, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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

    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);

    REGISTER_GLOBAL_MOCK_RETURNS(mocked_epoll_create, 0, -1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_epoll_ctl, 0, -1);

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ssize_t, long);


    REGISTER_TYPE(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT);
    REGISTER_TYPE(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT);
    REGISTER_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT);
    REGISTER_TYPE(THREADAPI_RESULT, THREADAPI_RESULT);
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

    g_event_index = 0;
    memset(g_events, 0, sizeof(struct epoll_event)*2);
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

// async_socket_create

// Tests_SRS_ASYNC_SOCKET_LINUX_11_002: [ If execution_engine is NULL, async_socket_create shall fail and return NULL. ]
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

// Tests_SRS_ASYNC_SOCKET_LINUX_11_003: [ If socket_handle is INVALID_SOCKET, async_socket_create shall fail and return NULL. ]
TEST_FUNCTION(async_socket_create_with_INVALID_SOCKET_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket;

    // act
    async_socket = async_socket_create(test_execution_engine, INVALID_SOCKET);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_001: [ async_socket_create shall allocate a new async socket and on success shall return a non-NULL handle. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_004: [ async_socket_create shall increment the reference count on execution_engine. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_005: [ async_socket_create shall initialize the global thread. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_008: [ initialize_global_thread shall increment the global g_thread_access_cnt variable. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_009: [ If the g_thread_access_cnt count is 1, initialize_global_thread shall do the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_010: [ initialize_global_thread shall create the epoll variable by calling epoll_create. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_011: [ initialize_global_thread shall create the threads specified in THREAD_COUNT by calling ThreadAPI_Create. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_013: [ On success initialize_global_thread shall return the value returned by epoll_create. ]
TEST_FUNCTION(async_socket_create_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket;

    mock_async_socket_create_setup();

    // act
    async_socket = async_socket_create(test_execution_engine, test_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(async_socket);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_006: [ If any error occurs, async_socket_create shall fail and return NULL. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_012: [ If any error occurs initialize_global_thread shall fail and return -1. ]
TEST_FUNCTION(async_socket_create_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket;

    mock_async_socket_create_setup();

    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            // act
            async_socket = async_socket_create(test_execution_engine, test_socket);

            // assert
            ASSERT_IS_NULL(async_socket, "On failed call %zu", index);
        }
    }
}

// async_socket_destroy

// Tests_SRS_ASYNC_SOCKET_LINUX_11_019: [ If async_socket is NULL, async_socket_destroy shall return. ]
TEST_FUNCTION(async_socket_destroy_with_NULL_returns)
{
    // arrange

    // act
    async_socket_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_022: [ async_socket_destroy shall decrement the reference count on the execution engine. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_015: [ deinitialize_global_thread shall decrement the global g_thread_access_cnt variable. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_016: [ If the g_thread_access_cnt count is 0, deinitialize_global_thread shall do the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_017: [ deinitialize_global_thread shall call close on the global epoll variable. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_018: [ deinitialize_global_thread shall wait for the global threads to close by calling ThreadAPI_Join. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_023: [ async_socket_destroy shall free all resources associated with async_socket. ]
TEST_FUNCTION(async_socket_destroy_frees_resources)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // deinit
    mock_deinitialize_global_thread_setup();

    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    async_socket_destroy(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_020: [ While async_socket is OPENING or CLOSING, async_socket_destroy shall wait for the open to complete either successfully or with error. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_021: [ async_socket_destroy shall perform an implicit close if async_socket is OPEN. ]
TEST_FUNCTION(async_socket_destroy_closes_first_if_open)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    // close first
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    mock_internal_close_setup();
    mock_deinitialize_global_thread_setup();

    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    async_socket_destroy(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// async_socket_open_async

// Tests_SRS_ASYNC_SOCKET_LINUX_11_024: [ If async_socket is NULL, async_socket_open_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_open_async_with_NULL_async_socket_fails)
{
    // arrange
    int result;

    // act
    result = async_socket_open_async(NULL, test_on_open_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_025: [ If on_open_complete is NULL, async_socket_open_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_open_async_with_NULL_on_open_complete_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);

    int result;
    umock_c_reset_all_calls();

    // act
    result = async_socket_open_async(async_socket, NULL, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_027: [ Otherwise, async_socket_open_async shall switch the state to OPENING. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_028: [ On success, async_socket_open_async shall return 0. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_031: [ async_socket_open_async shall add the socket to the epoll system by calling epoll_ctl with EPOLL_CTL_ADD. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_032: [ async_socket_open_async shall set the state to OPEN. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_033: [ On success async_socket_open_async shall call on_open_complete_context with ASYNC_SOCKET_OPEN_OK. ]
TEST_FUNCTION(async_socket_open_async_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_ADD, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(test_on_open_complete(test_callback_ctx, ASYNC_SOCKET_OPEN_OK));

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_026: [ on_open_complete_context shall be allowed to be NULL. ]
TEST_FUNCTION(async_socket_open_async_succeeds_with_NULL_context)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_ADD, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(test_on_open_complete(NULL, ASYNC_SOCKET_OPEN_OK));

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_034: [ If any error occurs, async_socket_open_async shall fail and return a non-zero value. ]
TEST_FUNCTION(when_underlying_calls_fail_async_socket_open_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_ADD, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(test_on_open_complete(test_callback_ctx, ASYNC_SOCKET_OPEN_OK));

    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            // act
            result = async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", index);
        }
    }

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_029: [ If async_socket is already OPEN or OPENING, async_socket_open_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_open_async_after_async_socket_open_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_030: [ If async_socket has already closed the underlying socket handle then async_socket_open_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_open_async_after_close_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    async_socket_close(async_socket);
    int result;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    result = async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// async_socket_close

// Tests_SRS_ASYNC_SOCKET_LINUX_11_035: [ If async_socket is NULL, async_socket_close shall return. ]
TEST_FUNCTION(async_socket_close_with_NULL_returns)
{
    // arrange

    // act
    async_socket_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_036: [ Otherwise, async_socket_close shall switch the state to CLOSING. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_037: [ async_socket_close shall wait for all executing async_socket_send_async and async_socket_receive_async APIs. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_038: [ Then async_socket_close shall remove the underlying socket form the epoll by calling epoll_ctl with EPOLL_CTL_DEL. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_039: [ async_socket_close shall call close on the underlying socket. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_041: [ async_socket_close shall set the state to closed. ]
TEST_FUNCTION(async_socket_close_reverses_the_actions_from_open)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    mock_internal_close_setup();

    // act
    async_socket_close(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_042: [ If async_socket is not OPEN, async_socket_close shall return. ]
TEST_FUNCTION(async_socket_close_when_not_open_returns)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    async_socket_close(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_042: [ If async_socket is not OPEN, async_socket_close shall return. ]
TEST_FUNCTION(async_socket_close_after_close_returns)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    async_socket_close(async_socket);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    async_socket_close(async_socket);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

// async_socket_send_async

// Tests_SRS_ASYNC_SOCKET_LINUX_11_043: [ If async_socket is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
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

// Tests_SRS_ASYNC_SOCKET_LINUX_11_044: [ If buffers is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_NULL_payload_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, NULL, 1, test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_045: [ If buffer_count is 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_0_payload_count_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, 0, test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_046: [ If any of the buffers in payload has buffer set to NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_first_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = NULL;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_046: [ If any of the buffers in payload has buffer set to NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_second_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = NULL;
    payload_buffers[1].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_047: [ If any of the buffers in payload has length set to 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_first_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = 0;
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_047: [ If any of the buffers in payload has length set to 0, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_second_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 0;
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_048: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_UINT32_MAX_buffers_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
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
    result = async_socket_send_async(async_socket, payload_buffers, UINT32_MAX, test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_049: [ If on_send_complete is NULL, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(async_socket_send_async_with_on_send_complete_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), NULL, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_051: [ If async_socket is not OPEN, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]
TEST_FUNCTION(async_socket_send_async_when_not_open_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ABANDONED, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_051: [ If async_socket is not OPEN, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]
TEST_FUNCTION(async_socket_send_async_after_close_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    async_socket_close(async_socket);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ABANDONED, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_052: [ async_socket_send_async shall attempt to send the data by calling send with the MSG_NOSIGNAL flag to ensure an exception is not generated. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_053: [ async_socket_send_async shall continue to send the data until the payload length has been sent. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_062: [ On success, async_socket_send_async shall return ASYNC_SOCKET_SEND_SYNC_OK. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_061: [ If the send is successful, async_socket_send_async shall call the on_send_complete with on_send_complete_context and ASYNC_SOCKET_SEND_SYNC_OK. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_062: [ On success, async_socket_send_async shall return ASYNC_SOCKET_SEND_SYNC_OK. ]
TEST_FUNCTION(async_socket_send_async_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL));
    STRICT_EXPECTED_CALL(test_on_send_complete(test_callback_ctx, ASYNC_SOCKET_SEND_OK));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    // act
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_050: [ on_send_complete_context shall be allowed to be NULL. ]
TEST_FUNCTION(async_socket_send_async_with_NULL_on_send_complete_context_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL));
    STRICT_EXPECTED_CALL(test_on_send_complete(NULL, ASYNC_SOCKET_SEND_OK));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    // act
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_063: [ If any error occurs, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(when_underlying_calls_fail_async_socket_send_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    errno = EDESTADDRREQ;
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_054: [ If the send fails to send the data, async_socket_send_async shall do the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_055: [ If the errno value is EAGAIN or EWOULDBLOCK. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_056: [ async_socket_send_async shall create a context for the send where the payload, on_send_complete and on_send_complete_context shall be stored. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_057: [ The the context shall then be added to the epoll system by calling epoll_ctl with EPOLL_CTL_MOD. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_058: [ If the epoll_ctl call fails with ENOENT, async_socket_send_async shall call epoll_ctl again with EPOLL_CTL_ADD. ]
TEST_FUNCTION(when_errno_for_send_returns_EWOULDBLOCK_it_uses_epoll_treated_as_successfull)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_MOD, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    // act
    errno = EWOULDBLOCK;
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(g_events[0].data.ptr);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_059: [ If the errno value is ECONNRESET, ENOTCONN, or EPIPE shall fail and return ASYNC_SOCKET_SEND_SYNC_ABANDONED. ]
TEST_FUNCTION(when_errno_for_send_returns_ECONNRESET_async_socket_send_async_returns_ABANDONED)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    errno = ECONNRESET;

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ABANDONED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_060: [ If any other error is encountered, async_socket_send_async shall fail and return ASYNC_SOCKET_SEND_SYNC_ERROR. ]
TEST_FUNCTION(when_errno_for_send_returns_error_async_socket_send_async_returns_ERROR)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    errno = EAFNOSUPPORT;

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
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    ASYNC_SOCKET_SEND_SYNC_RESULT result;
    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = UINT32_MAX;
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 1;
    umock_c_reset_all_calls();

    // act
    result = async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_ERROR, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// async_socket_receive_async

// Tests_SRS_ASYNC_SOCKET_LINUX_11_064: [ If async_socket is NULL, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_NULL_async_socket_fails)
{
    // arrange
    int result;
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    // act
    result = async_socket_receive_async(NULL, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_065: [ If buffers is NULL, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_NULL_payload_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, NULL, 1, test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_066: [ If buffer_count is 0, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_0_payload_count_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    uint8_t payload_bytes[1];
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, 0, test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_067: [ If any of the buffers in payload has buffer set to NULL, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_first_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = NULL;
    payload_buffers[0].length = sizeof(payload_bytes_1);
    payload_buffers[1].buffer = payload_bytes_2;
    payload_buffers[1].length = sizeof(payload_bytes_2);
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_067: [ If any of the buffers in payload has buffer set to NULL, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_second_out_of_2_buffers_having_buffer_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes_1;
    payload_buffers[0].length = sizeof(payload_bytes_1);
    payload_buffers[1].buffer = NULL;
    payload_buffers[1].length = sizeof(payload_bytes_2);
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_068: [ If any of the buffers in payload has length set to 0, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_first_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes_1;
    payload_buffers[0].length = 0;
    payload_buffers[1].buffer = payload_bytes_2;
    payload_buffers[1].length = sizeof(payload_bytes_2);
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_068: [ If any of the buffers in payload has length set to 0, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_second_out_of_2_buffers_having_length_0_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes_1[1];
    uint8_t payload_bytes_2[1];
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes_1;
    payload_buffers[0].length = sizeof(payload_bytes_1);
    payload_buffers[1].buffer = payload_bytes_2;
    payload_buffers[1].length = 0;
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_069: [ If the sum of buffer lengths for all the buffers in payload is greater than UINT32_MAX, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_sum_of_buffer_lengths_exceeding_UINT32_MAX_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[2];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = UINT32_MAX;
    payload_buffers[1].buffer = payload_bytes;
    payload_buffers[1].length = 1;
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_070: [ If on_receive_complete is NULL, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_with_on_recv_complete_NULL_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), NULL, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_072: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_when_not_open_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];

    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_072: [ If async_socket is not OPEN, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(async_socket_receive_async_after_close_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    (void)async_socket_close(async_socket);

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_073: [ Otherwise async_socket_receive_async shall create a context for the send where the payload, on_receive_complete and on_receive_complete_context shall be stored. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_074: [ The context shall also allocate enough memory to keep an array of buffer_count items. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_075: [ async_socket_receive_async shall add the socket in the epoll system by calling epoll_ctl with EPOLL_CTL_MOD ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_077: [ On success, async_socket_receive_async shall return 0. ]
TEST_FUNCTION(async_socket_receive_async_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    setup_async_socket_receive_async_mocks();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, test_callback_ctx);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    free(g_events[0].data.ptr);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_071: [ on_receive_complete_context shall be allowed to be NULL. ]
TEST_FUNCTION(async_socket_receive_async_with_NULL_on_recv_complete_context_succeeds)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    setup_async_socket_receive_async_mocks();

    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    free(g_events[0].data.ptr);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_078: [ If any error occurs, async_socket_receive_async shall fail and return a non-zero value. ]
TEST_FUNCTION(when_underlying_calls_fail_async_socket_receive_async_fails)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    setup_async_socket_receive_async_mocks();

    umock_c_negative_tests_snapshot();
    errno = 0;

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            // act
            int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", index);
        }
    }

    // cleanup
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_076: [ If the epoll_ctl call fails with ENOENT, async_socket_send_async shall call epoll_ctl again with EPOLL_CTL_ADD. ]
TEST_FUNCTION(when_errno_for_epoll_returns_ENOENT_it_adds_socket_again_successfull)
{
    // arrange
    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, sizeof(ASYNC_SOCKET_BUFFER)));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_MOD, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(IGNORED_ARG, EPOLL_CTL_ADD, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    errno = ENOENT;
    // act
    int result = async_socket_receive_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_receive_complete, NULL);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    free(g_events[0].data.ptr);
    async_socket_destroy(async_socket);
}

// thread_worker_func

// Tests_SRS_ASYNC_SOCKET_LINUX_11_079: [ thread_worker_func shall call epoll_wait waiting for the epoll to become signaled. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_080: [ Onced signaled thread_worker_func shall loop through the signaled epolls. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_085: [ If the events value contains EPOLLIN (recv), thread_worker_func shall the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_086: [ thread_worker_func shall receive the ASYNC_SOCKET_RECV_CONTEXT value from the ptr variable from the epoll_event data ptr. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_087: [ Then thread_worker_func shall call recv and do the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_092: [ If the recv size > 0, if we have another buffer to fill then we will attempt another read, otherwise we shall call on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_OK ]
TEST_FUNCTION(thread_worker_func_epoll_wait_contains_EPOLLIN)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    uint32_t payload_size = sizeof(payload_buffers) / sizeof(payload_buffers[0]);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(async_socket, payload_buffers, payload_size, test_on_receive_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events |= EPOLLIN;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_receive_complete(test_callback_ctx, ASYNC_SOCKET_RECEIVE_OK, payload_size));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_089: [ If errno is EAGAIN or EWOULDBLOCK, then unlikely errors will continue ]
TEST_FUNCTION(thread_worker_func_recv_returns_EWOULDBLOCK)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    uint32_t payload_size = sizeof(payload_buffers) / sizeof(payload_buffers[0]);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(async_socket, payload_buffers, payload_size, test_on_receive_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLIN;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    errno = EWOULDBLOCK;
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_090: [ If errno is ECONNRESET, then thread_worker_func shall call the on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_ABANDONED ]
TEST_FUNCTION(thread_worker_func_recv_returns_ECONNRESET)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    uint32_t payload_size = sizeof(payload_buffers) / sizeof(payload_buffers[0]);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(async_socket, payload_buffers, payload_size, test_on_receive_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLIN;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(test_on_receive_complete(test_callback_ctx, ASYNC_SOCKET_RECEIVE_ABANDONED, 0));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    errno = ECONNRESET;
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_088: [ If the recv size < 0, then: ]
TEST_FUNCTION(thread_worker_func_recv_returns_any_random_error_no)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    uint32_t payload_size = sizeof(payload_buffers) / sizeof(payload_buffers[0]);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(async_socket, payload_buffers, payload_size, test_on_receive_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLIN;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(test_on_receive_complete(test_callback_ctx, ASYNC_SOCKET_RECEIVE_ERROR, 0));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    errno = EMSGSIZE;
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_091: [ If the recv size equal 0, then thread_worker_func shall call on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_OK ]
TEST_FUNCTION(thread_worker_func_recv_returns_0_bytes_success)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    uint32_t payload_size = sizeof(payload_buffers) / sizeof(payload_buffers[0]);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(async_socket, payload_buffers, payload_size, test_on_receive_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLIN;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mocked_recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(test_on_receive_complete(test_callback_ctx, ASYNC_SOCKET_RECEIVE_OK, 0));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_081: [ If the events value contains EPOLLRDHUP (hang up), thread_worker_func shall the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_082: [ thread_worker_func shall receive the ASYNC_SOCKET_RECV_CONTEXT value from the ptr variable from the epoll_event data ptr. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_084: [ Then call the on_receive_complete callback with the on_receive_complete_context and ASYNC_SOCKET_RECEIVE_ABANDONED. ]
TEST_FUNCTION(thread_worker_func_epoll_contains_EPOLLRDHUP_and_abandons_the_connection)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);
    uint32_t payload_size = sizeof(payload_buffers) / sizeof(payload_buffers[0]);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(async_socket, payload_buffers, payload_size, test_on_receive_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLRDHUP;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(test_on_receive_complete(test_callback_ctx, ASYNC_SOCKET_RECEIVE_ABANDONED, 0));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_094: [ If the events value contains EPOLLOUT (send), thread_worker_func shall the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_095: [ thread_worker_func shall receive the ASYNC_SOCKET_SEND_CONTEXT value from the ptr variable from the epoll_event data ptr. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_096: [ thread_worker_func shall loop through the total buffers and send the data. ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_097: [ if send returns value is < 0 thread_worker_func shall do the following: ]
// Tests_SRS_ASYNC_SOCKET_LINUX_11_100: [ If the thread_access_cnt variable is not 0, thread_worker_func continue, otherwise it shall exit ]
TEST_FUNCTION(thread_worker_func_epoll_contains_EPOLLOUT_and_sends_msg)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    errno = EWOULDBLOCK;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLOUT;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);

    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL));
    STRICT_EXPECTED_CALL(test_on_send_complete(test_callback_ctx, ASYNC_SOCKET_SEND_OK));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_098: [ if errno is ECONNRESET, then on_send_complete shall be called with ASYNC_SOCKET_SEND_ABANDONED. ]
TEST_FUNCTION(thread_worker_func_epoll_contains_EPOLLOUT_and_sends_ECONNRESET)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    errno = EWOULDBLOCK;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLOUT;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);

    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(test_on_send_complete(test_callback_ctx, ASYNC_SOCKET_SEND_ABANDONED));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    errno = ECONNRESET;
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

// Tests_SRS_ASYNC_SOCKET_LINUX_11_099: [ if errno is anything else, then on_send_complete shall be called with ASYNC_SOCKET_SEND_ERROR. ]
TEST_FUNCTION(thread_worker_func_epoll_contains_EPOLLOUT_and_send_error)
{
    // arrange
    volatile_atomic int32_t* threads_num;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CaptureArgumentValue_addend(&threads_num);

    ASYNC_SOCKET_HANDLE async_socket = async_socket_create(test_execution_engine, test_socket);
    ASSERT_IS_NOT_NULL(async_socket);
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(async_socket, test_on_open_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    uint8_t payload_bytes[] = { 0x42 };
    ASYNC_SOCKET_BUFFER payload_buffers[1];
    payload_buffers[0].buffer = payload_bytes;
    payload_buffers[0].length = sizeof(payload_bytes);

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    errno = EWOULDBLOCK;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(async_socket, payload_buffers, sizeof(payload_buffers) / sizeof(payload_buffers[0]), test_on_send_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLOUT;
    STRICT_EXPECTED_CALL(mocked_epoll_wait(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);

    STRICT_EXPECTED_CALL(mocked_send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, MSG_NOSIGNAL))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(test_on_send_complete(test_callback_ctx, ASYNC_SOCKET_SEND_ERROR));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // Set this so it stops the worker_thread_func loop from going
    // around twice
    *threads_num = 0;

    // act
    errno = EMSGSIZE;
    g_saved_worker_thread_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    *threads_num = 1;
    async_socket_close(async_socket);
    async_socket_destroy(async_socket);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

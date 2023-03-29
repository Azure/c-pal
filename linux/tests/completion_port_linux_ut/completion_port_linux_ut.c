// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>                          // for memset

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "real_gballoc_ll.h"    // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"        // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/refcount.h"  // IWYU pragma: keep
#include "c_pal/socket_handle.h"
#include "c_pal/s_list.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#undef ENABLE_MOCKS

#include "real_refcount.h"  // IWYU pragma: keep
#include "real_interlocked.h"
#include "real_gballoc_hl.h" // IWYU pragma: keep
#include "real_s_list.h" // IWYU pragma: keep

#include "c_pal/completion_port_linux.h"

#define TEST_MAX_EVENTS_NUM     64
#define EVENTS_TIMEOUT_MS       2*1000

static SOCKET_HANDLE test_socket = (SOCKET_HANDLE)0x4242;
static void* test_callback_ctx = (void*)0x4244;
static THREAD_HANDLE test_thread_handle = (THREAD_HANDLE)0x4200;
static THREAD_START_FUNC g_saved_worker_thread_func;
static void* g_saved_worker_thread_func_context;

static struct epoll_event g_events[2];
static uint32_t g_event_index = 0;
static int g_test_epoll = 2;

THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    g_saved_worker_thread_func = func;
    g_saved_worker_thread_func_context = arg;
    *threadHandle = test_thread_handle;
    return THREADAPI_OK;
}

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, int, mocked_epoll_create, int, __size)
MOCK_FUNCTION_END(g_test_epoll)
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
MOCK_FUNCTION_END(0)
MOCK_FUNCTION_WITH_CODE(, int, mocked_close, int, s)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, void, test_port_event_complete, void*, context, COMPLETION_PORT_EPOLL_ACTION, action)
MOCK_FUNCTION_END()

static void setup_completion_port_create_mocks(void)
{
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();

    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(s_list_initialize(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_create(TEST_MAX_EVENTS_NUM));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle));
}

static void setup_completion_port_add_mocks(void)
{
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetFailReturn(1);
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(s_list_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, test_socket, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types failed");

    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_S_LIST_GLOBAL_MOCK_HOOKS();

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_RETURNS(s_list_initialize, 0, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_RETURNS(s_list_add, 0, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ThreadAPI_Create, THREADAPI_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);

    REGISTER_GLOBAL_MOCK_RETURNS(mocked_epoll_create, g_test_epoll, -1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_epoll_ctl, 0, -1);

    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PS_LIST_ENTRY, void*);

    REGISTER_TYPE(THREADAPI_RESULT, THREADAPI_RESULT);
    REGISTER_TYPE(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
    errno = 0;
    g_event_index = 0;
    memset(g_events, 0, sizeof(struct epoll_event)*2);
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    umock_c_negative_tests_deinit();
}

// completion_port_create

// Tests_SRS_COMPLETION_PORT_LINUX_11_001: [ completion_port_create shall allocate memory for a completion port object. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_002: [ completion_port_create shall create the epoll instance by calling epoll_create. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_003: [ completion_port_create shall create a thread that runs epoll_worker_func to handle the epoll events. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_004: [ On success completion_port_create shall return the allocated COMPLETION_PORT_HANDLE. ]
TEST_FUNCTION(completion_port_create_success)
{
    //arrange
    setup_completion_port_create_mocks();

     //act
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(port_handle);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_005: [ If there are any errors then completion_port_create shall fail and return NULL. ]
TEST_FUNCTION(completion_port_create_fails)
{
    // arrange
    COMPLETION_PORT_HANDLE port_handle;

    setup_completion_port_create_mocks();

    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            // act
            port_handle = completion_port_create();

            // assert
            ASSERT_IS_NULL(port_handle, "On failed call %zu", index);
        }
    }
}

// completion_port_inc_ref

// Tests_SRS_COMPLETION_PORT_LINUX_11_006: [ If completion_port is NULL, completion_port_inc_ref shall return. ]
TEST_FUNCTION(completion_port_inc_ref_handle_NULL_fail)
{
    //arrange

    //act
    completion_port_inc_ref(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_007: [ Otherwise completion_port_inc_ref shall increment the internally maintained reference count. ]
TEST_FUNCTION(completion_port_inc_ref_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();

    //act
    completion_port_inc_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
    completion_port_dec_ref(port_handle);
}

// completion_port_dec_ref

// Tests_SRS_COMPLETION_PORT_LINUX_11_008: [ If completion_port is NULL, completion_port_dec_ref shall return. ]
TEST_FUNCTION(completion_port_dec_ref_handle_NULL_fail)
{
    //arrange

    //act
    completion_port_dec_ref(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_009: [ completion_port_dec_ref shall decrement the reference count for completion_port. ]
TEST_FUNCTION(completion_port_dec_ref_1_ref_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    completion_port_inc_ref(port_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    //act
    completion_port_dec_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_010: [ If the reference count reaches 0, completion_port_dec_ref shall do the following: ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_011: [ wait for the ongoing call count to reach zero. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_012: [ increment the flag signaling that the threads can complete. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_013: [ close the epoll object. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_014: [ close the thread by calling ThreadAPI_Join. ]
TEST_FUNCTION(completion_port_dec_ref_0_ref_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mocked_close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(s_list_remove_head(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    completion_port_dec_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_015: [ then the memory associated with completion_port shall be freed. ]
TEST_FUNCTION(completion_port_dec_ref_remove_item_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLOUT | EPOLLRDHUP, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mocked_close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(s_list_remove_head(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(s_list_remove_head(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    completion_port_dec_ref(port_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// completion_port_add

// Tests_SRS_COMPLETION_PORT_LINUX_11_016: [ If completion_port is NULL, completion_port_add shall return a non-NULL value. ]
TEST_FUNCTION(completion_port_add_handle_NULL_fail)
{
    //arrange

    //act
    int result = completion_port_add(NULL, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_018: [ If event_callback is NULL, completion_port_add shall return a non-NULL value. ]
TEST_FUNCTION(completion_port_add_event_callback_NULL_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    //act
    int result = completion_port_add(port_handle, EPOLLIN, test_socket, NULL, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_017: [ If socket is INVALID_SOCKET, completion_port_add shall return a non-NULL value. ]
TEST_FUNCTION(completion_port_add_socket_handle_INVALID_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    //act
    int result = completion_port_add(port_handle, EPOLLIN, INVALID_SOCKET, test_port_event_complete, test_callback_ctx);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_019: [ completion_port_add shall ensure the thread completion flag is not set. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_020: [ completion_port_add shall increment the ongoing call count value to prevent close. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_021: [ completion_port_add shall allocate a EPOLL_THREAD_DATA object to store thread data. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_022: [ completion_port_add shall add the EPOLL_THREAD_DATA object to a list for later removal. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_023: [ completion_port_add shall add the socket in the epoll system by calling epoll_ctl with EPOLL_CTL_MOD along with the epoll_op variable. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_025: [ completion_port_add shall decrement the ongoing call count value to unblock close. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_026: [ On success, completion_port_add shall return 0. ]
TEST_FUNCTION(completion_port_add_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    setup_completion_port_add_mocks();

    //act
    int result = completion_port_add(port_handle, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx);
    ASSERT_ARE_EQUAL(int, 0, result);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_027: [ If any error occurs, completion_port_add shall fail and return a non-zero value. ]
TEST_FUNCTION(completion_port_add_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    setup_completion_port_add_mocks();

    umock_c_negative_tests_snapshot();

    //act
    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            errno = EACCES;
            int result = completion_port_add(port_handle, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", index);
        }
    }

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_019: [ completion_port_add shall ensure the thread completion flag is not set. ]
TEST_FUNCTION(completion_port_add_dec_ref_running_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);

    //act
    int result = completion_port_add(port_handle, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx);
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_024: [ If the epoll_ctl call fails with ENOENT, completion_port_add shall call epoll_ctl again with EPOLL_CTL_ADD. ]
TEST_FUNCTION(completion_port_add_epoll_add_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(s_list_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_MOD, test_socket, IGNORED_ARG))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_ADD, test_socket, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    //act
    errno = ENOENT;
    int result = completion_port_add(port_handle, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx);
    ASSERT_ARE_EQUAL(int, 0, result);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// completion_port_remove

// Tests_SRS_COMPLETION_PORT_LINUX_11_028: [ If completion_port is NULL, completion_port_remove shall return. ]
TEST_FUNCTION(completion_port_remove_completion_port_NULL_fail)
{
    //arrange

    //act
    completion_port_remove(NULL, test_socket);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_029: [ If socket is INVALID_SOCKET, completion_port_remove shall return. ]
TEST_FUNCTION(completion_port_remove_socket_INVALID_fail)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    //act
    completion_port_remove(port_handle, INVALID_SOCKET);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_030: [ completion_port_remove shall remove the underlying socket from the epoll by calling epoll_ctl with EPOLL_CTL_DEL. ]
TEST_FUNCTION(completion_port_remove_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_DEL, test_socket, NULL));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    //act
    completion_port_remove(port_handle, test_socket);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_037: [ completion_port_remove shall loop through the list of EPOLL_THREAD_DATA object and call the event_callback with COMPLETION_PORT_EPOLL_ABANDONED ]
TEST_FUNCTION(completion_port_remove_port_event_callback_called_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLOUT | EPOLLRDHUP, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_epoll_ctl(g_test_epoll, EPOLL_CTL_DEL, test_socket, NULL));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_ABANDONED));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));

    //act
    completion_port_remove(port_handle, test_socket);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_031: [ If parameter is NULL, epoll_worker_func shall do nothing. ]
TEST_FUNCTION(epoll_worker_func_parameter_NULL_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLIN | EPOLLRDHUP, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    //act
    g_saved_worker_thread_func(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_032: [ epoll_worker_func shall call epoll_wait to wait for an epoll event to become signaled with a timeout of 2 Seconds. ]
TEST_FUNCTION(epoll_worker_func_epoll_wait_timeout_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLIN | EPOLLRDHUP, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_035: [ epoll_worker_func shall call the event_callback with the specified COMPLETION_PORT_EPOLL_ACTION that was returned. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_036: [ Then epoll_worker_func shall remove the EPOLL_THREAD_DATA from the list and free the object. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_033: [ On a epoll_wait timeout epoll_worker_func shall ensure it should not exit and issue another epoll_wait. ]
TEST_FUNCTION(epoll_worker_func_epoll_wait_EPOLLRDHUP_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLIN | EPOLLRDHUP, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLRDHUP;

    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLRDHUP));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(s_list_remove(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_035: [ epoll_worker_func shall call the event_callback with the specified COMPLETION_PORT_EPOLL_ACTION that was returned. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_036: [ Then epoll_worker_func shall remove the EPOLL_THREAD_DATA from the list and free the object. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_033: [ On a epoll_wait timeout epoll_worker_func shall ensure it should not exit and issue another epoll_wait. ]
TEST_FUNCTION(epoll_worker_func_epoll_wait_EPOLLIN_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLIN, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLIN;

    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLIN));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(s_list_remove(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_035: [ epoll_worker_func shall call the event_callback with the specified COMPLETION_PORT_EPOLL_ACTION that was returned. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_036: [ Then epoll_worker_func shall remove the EPOLL_THREAD_DATA from the list and free the object. ]
// Tests_SRS_COMPLETION_PORT_LINUX_11_033: [ On a epoll_wait timeout epoll_worker_func shall ensure it should not exit and issue another epoll_wait. ]
TEST_FUNCTION(epoll_worker_func_epoll_wait_EPOLLOUT_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLOUT, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLOUT;

    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(s_list_remove(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

// Tests_SRS_COMPLETION_PORT_LINUX_11_034: [ epoll_worker_func shall loop through the num of descriptors that was returned. ]
TEST_FUNCTION(epoll_worker_func_epoll_wait_multiple_values_success)
{
    //arrange
    COMPLETION_PORT_HANDLE port_handle = completion_port_create();
    ASSERT_IS_NOT_NULL(port_handle);
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLOUT, test_socket, test_port_event_complete, test_callback_ctx));
    ASSERT_ARE_EQUAL(int, 0, completion_port_add(port_handle, EPOLLOUT, test_socket, test_port_event_complete, test_callback_ctx));
    umock_c_reset_all_calls();

    g_events[0].events = 0;
    g_events[0].events |= EPOLLOUT;
    g_events[1].events = 0;
    g_events[1].events |= EPOLLRDHUP;

    STRICT_EXPECTED_CALL(mocked_epoll_wait(g_test_epoll, IGNORED_ARG, TEST_MAX_EVENTS_NUM, EVENTS_TIMEOUT_MS))
        .CopyOutArgumentBuffer_events(&g_events, sizeof(struct epoll_event)*2)
        .SetReturn(2);
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLOUT));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(s_list_remove(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(test_port_event_complete(test_callback_ctx, COMPLETION_PORT_EPOLL_EPOLLRDHUP));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(s_list_remove(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(wake_by_address_single(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0))
        .SetReturn(1);

    //act
    g_saved_worker_thread_func(g_saved_worker_thread_func_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    completion_port_dec_ref(port_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

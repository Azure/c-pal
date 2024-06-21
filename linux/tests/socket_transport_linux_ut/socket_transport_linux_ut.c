// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "socket_mocked.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/socket_handle.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/sm.h"
#include "c_pal/sync.h"
#include "c_pal/string_utils.h"

#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"
#include "real_gballoc_hl.h"
#include "../reals/real_sm.h"

#include "c_pal/socket_transport.h"

#define XTEST_FUNCTION(x) void x(void)

#define MAX_SOCKET_ARRAY            10

static SOCKET_HANDLE* g_test_socket_array[MAX_SOCKET_ARRAY];
static SOCKET_HANDLE test_socket = 0;
static struct addrinfo g_addrInfo = { 0 };

#define TEST_HOSTNAME               "test_hostname"
#define TEST_INCOMING_HOSTNAME      "incoming_hostname"
#define TEST_PORT                   1234
#define TEST_INCOMING_PORT          5678
#define TEST_CONNECTION_TIMEOUT     1000
#define TEST_FLAGS                  123

MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static SOCKET_HANDLE my_socket(int af, int type, int protocol)
{
    (void)af;
    (void)type;
    (void)protocol;

    ASSERT_IS_TRUE(test_socket < MAX_SOCKET_ARRAY);

    g_test_socket_array[test_socket++] = real_gballoc_ll_malloc(sizeof(SOCKET_HANDLE));
    return test_socket;
}

static SOCKET_HANDLE my_accept(SOCKET_HANDLE s, struct sockaddr* addr, socklen_t* addrlen)
{
    (void)s;
    (void)addr;
    (void)addrlen;
    g_test_socket_array[test_socket++] = real_gballoc_ll_malloc(sizeof(SOCKET_HANDLE));
    return test_socket;
}

static int my_close(int s)
{
    real_gballoc_ll_free((void*)g_test_socket_array[s]);
    return 0;
}

static int my_getaddrinfo(const char* pNodeName, const char* pServiceName, const struct addrinfo* pHints, struct addrinfo** ppResult)
{
    (void)pNodeName;
    (void)pServiceName;
    (void)pHints;
    *ppResult = &g_addrInfo;
    return 0;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types failed");

    REGISTER_SM_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_gballoc_ll_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(free, real_gballoc_ll_free);

    REGISTER_GLOBAL_MOCK_HOOK(socket, my_socket);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(socket, INVALID_SOCKET);
    REGISTER_GLOBAL_MOCK_HOOK(close, my_close);
    REGISTER_GLOBAL_MOCK_RETURN(bind, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(bind, -1);
    REGISTER_GLOBAL_MOCK_RETURN(listen, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(listen, -1);
    REGISTER_GLOBAL_MOCK_HOOK(accept, my_accept);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(accept, INVALID_SOCKET);
    REGISTER_GLOBAL_MOCK_HOOK(getaddrinfo, my_getaddrinfo);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(getaddrinfo, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connect, MU_FAILURE);

    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SOCKET_HANDLE, int);

    g_addrInfo.ai_canonname = "ai_canonname";
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    umock_c_negative_tests_deinit();

}

// Tests_SOCKET_TRANSPORT_LINUX_11_001: [ socket_transport_create shall ensure type is either SOCKET_CLIENT, or SOCKET_SERVER. ]
TEST_FUNCTION(socket_transport_create_invalid_type_fail)
{
    //arrange

    //act
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create((SOCKET_TYPE)555);

    //assert
    ASSERT_IS_NULL(socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_002: [ socket_transport_create shall allocate a new SOCKET_TRANSPORT object. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_003: [ socket_transport_create shall call sm_create to create a sm object. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_005: [ On success socket_transport_create shall return SOCKET_TRANSPORT_HANDLE. ]
TEST_FUNCTION(socket_transport_create_succeed)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);

    //assert
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_004: [ On any failure socket_transport_create shall return NULL. ]
TEST_FUNCTION(socket_transport_create_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);

            //assert
            ASSERT_IS_NULL(socket_handle);
        }
    }

    //cleanup
}

// Tests_SOCKET_TRANSPORT_LINUX_11_006: [ If socket_transport is NULL socket_transport_destroy shall return. ]
TEST_FUNCTION(socket_transport_destroy_socket_NULL_fail)
{
    //arrange

    //act
    socket_transport_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_LINUX_11_007: [ socket_transport_destroy shall call sm_destroy to destroy the sm object. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_008: [ socket_transport_destroy shall free the SOCKET_TRANSPORT_HANDLE object. ]
TEST_FUNCTION(socket_transport_destroy_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    socket_transport_destroy(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_LINUX_11_009: [ If socket_transport is NULL, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_socket_transport_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    //act
    int result = socket_transport_connect(NULL, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_010: [ If hostname is NULL, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_hostname_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    //act
    int result = socket_transport_connect(socket_handle, NULL, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_011: [ If port is 0, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_port_0_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, 0, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_012: [ If the socket_transport is not SOCKET_CLIENT, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_invalid_client_type_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_013: [ socket_transport_connect shall call sm_open_begin to begin the open. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_015: [ socket_transport_connect shall call socket with the params AF_INET, SOCK_STREAM and 0. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_017: [ socket_transport_connect shall set the socket to non-blocking by calling fcntl with O_NONBLOCK. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_018: [ If successful socket_transport_connect shall call sm_open_end with true. ]
TEST_FUNCTION(socket_transport_connect_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_014: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_connect shall fail and return a non-zero value. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_019: [ If any failure is encountered, socket_transport_connect shall call sm_open_end with false, fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

        //act
        int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_020: [ If socket_transport is NULL, socket_transport_disconnect shall fail and return. ]
TEST_FUNCTION(socket_transport_disconnect_socket_transport_NULL_fail)
{
    //arrange

    //act
    socket_transport_disconnect(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(socket_transport_disconnect_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

XTEST_FUNCTION(socket_transport_send_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// socket_transport_receive

XTEST_FUNCTION(socket_transport_receive_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, &payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// socket_transport_listen

XTEST_FUNCTION(socket_transport_listen_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(htons(TEST_PORT));
    STRICT_EXPECTED_CALL(bind(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(listen(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    //act
    int result = socket_transport_listen(socket_handle, TEST_PORT);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// socket_transport_accept

XTEST_FUNCTION(socket_transport_accept_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NOT_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(accept_socket_handle);
    socket_transport_destroy(accept_socket_handle);
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// socket_transport_get_underlying_socket

XTEST_FUNCTION(socket_transport_get_underlying_socket_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_HANDLE underying_socket = socket_transport_get_underlying_socket(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(int, test_socket, underying_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

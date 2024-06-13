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
#include "umock_c/umocktypes_windows.h"

#define ENABLE_MOCKS

#include "winsock_mocked.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/socket_handle.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"
#include "c_pal/interlocked.h"
#include "c_pal/sm.h"
#include "c_pal/sync.h"
#include "c_pal/string_utils.h"

#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"
#include "real_gballoc_hl.h"
#include "../reals/real_sm.h"

#include "c_pal/socket_transport.h"

static const uint32_t TEST_PROC_COUNT = 4;
static SOCKET_HANDLE test_socket = (SOCKET_HANDLE)123;
static ADDRINFO g_addrInfo = { 0 };
static uint32_t g_select_timeout_usec;

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

static SOCKET my_socket(int af, int type, int protocol)
{
    (void)af;
    (void)type;
    (void)protocol;
    return (SOCKET)real_gballoc_ll_malloc(1);
}

static SOCKET my_accept(SOCKET s, struct sockaddr* addr, int* addrlen)
{
    (void)s;
    (void)addr;
    (void)addrlen;
    return (SOCKET)real_gballoc_ll_malloc(sizeof(SOCKET));
}

static int my_getaddrinfo(const char* pNodeName, const char* pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult)
{
    (void)pNodeName;
    (void)pServiceName;
    (void)pHints;
    *ppResult = &g_addrInfo;
    return 0;
}

static int my_close(int_fast16_t s)
{
    real_gballoc_ll_free((void*)s);
    return 0;
}

#if 0
static PCSTR my_inet_ntop(INT Family, const VOID* pAddr, PSTR pStringBuf, size_t StringBufSize)
{
    (void)Family;
    (void)StringBufSize;

    struct sockaddr_in* cli_addr = (struct sockaddr_in*)pAddr;
    cli_addr->sin_port = TEST_INCOMING_PORT;
    (void)sprintf(pStringBuf, "%s", TEST_INCOMING_HOSTNAME);

    return pStringBuf;
}
#endif

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());

    REGISTER_SM_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_gballoc_ll_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(free, real_gballoc_ll_free);

    REGISTER_UMOCK_ALIAS_TYPE(WORD, uint16_t);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, uint32_t);

    REGISTER_GLOBAL_MOCK_HOOK(socket, my_socket);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(socket, INVALID_SOCKET);
    REGISTER_GLOBAL_MOCK_HOOK(closesocket, my_closesocket);
    REGISTER_GLOBAL_MOCK_RETURN(bind, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(bind, SOCKET_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(ioctlsocket, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ioctlsocket, SOCKET_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(listen, 0);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(listen, SOCKET_ERROR);
    REGISTER_GLOBAL_MOCK_RETURN(select, 1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(select, SOCKET_ERROR);
    REGISTER_GLOBAL_MOCK_HOOK(accept, my_accept);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(accept, INVALID_SOCKET);
    REGISTER_GLOBAL_MOCK_HOOK(getaddrinfo, my_getaddrinfo);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(getaddrinfo, MU_FAILURE);

    //REGISTER_GLOBAL_MOCK_HOOK(inet_ntop, my_inet_ntop);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(select, SOCKET_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(WSAGetLastError, WSAEALREADY, WSAEFAULT);

    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    /*REGISTER_UMOCK_ALIAS_TYPE(SOCKET, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PADDRINFOA, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSABUF, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPDWORD, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED_COMPLETION_ROUTINE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED, void*);

    REGISTER_UMOCK_ALIAS_TYPE(INT, int);
    REGISTER_UMOCK_ALIAS_TYPE(PSTR, char*);*/

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

TEST_FUNCTION(socket_transport_destroy_socket_NULL_fail)
{
    //arrange

    //act
    socket_transport_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

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

TEST_FUNCTION(socket_transport_connect_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
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

TEST_FUNCTION(socket_transport_disconnect_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

TEST_FUNCTION(socket_transport_send_succeed)
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
    STRICT_EXPECTED_CALL(WSASend(IGNORED_ARG, (LPWSABUF)&payload, 1, IGNORED_ARG, TEST_FLAGS, NULL, NULL));
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

TEST_FUNCTION(socket_transport_receive_succeed)
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
    STRICT_EXPECTED_CALL(WSARecv(IGNORED_ARG, (LPWSABUF)&payload, 1, IGNORED_ARG, IGNORED_ARG, NULL, NULL));
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

TEST_FUNCTION(socket_transport_listen_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(htons(TEST_PORT));
    STRICT_EXPECTED_CALL(bind(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
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

TEST_FUNCTION(socket_transport_accept_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
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

TEST_FUNCTION(socket_transport_get_underlying_socket_succeed)
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
    ASSERT_IS_NOT_NULL(underying_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

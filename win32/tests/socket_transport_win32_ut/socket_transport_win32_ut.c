// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stdlib.h>

#include "winsock2.h"
#include "ws2tcpip.h"

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
#include "../../win32/src/string_utils.c"
#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"
#include "real_gballoc_hl.h"
#include "../reals/real_sm.h"

#include "c_pal/socket_transport.h"

static const uint32_t TEST_PROC_COUNT = 4;
static SOCKET test_socket = (SOCKET)123;
static ADDRINFO g_addrInfo = { 0 };
static uint32_t g_select_timeout_usec;

#define TEST_HOSTNAME               "test_hostname"
#define TEST_INCOMING_HOSTNAME      "incoming_hostname"
#define TEST_PORT                   1234
#define TEST_INCOMING_PORT          5678
#define TEST_CONNECTION_TIMEOUT     1000
#define TEST_FLAGS                  123
#define TEST_BYTES_SENT             10
#define TEST_BYTES_RECV             10

MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES);


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

static int my_closesocket(SOCKET s)
{
    real_gballoc_ll_free((void*)s);
    return 0;
}

static PCSTR my_inet_ntop(INT Family, const VOID* pAddr, PSTR pStringBuf, size_t StringBufSize)
{
    (void)Family;
    (void)StringBufSize;

    struct sockaddr_in* cli_addr = (struct sockaddr_in*)pAddr;
    cli_addr->sin_port = TEST_INCOMING_PORT;
    (void)sprintf(pStringBuf, "%s", TEST_INCOMING_HOSTNAME);

    return pStringBuf;
}

static int my_shutdown(SOCKET s, int how)
{
    (void)s;
    (void)how;
    return 0;
}

static int my_gethostname(char* name, int namelen)
{
    (void)namelen;
    strcpy(name, TEST_INCOMING_HOSTNAME);
    return 0;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());

    REGISTER_SM_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_gballoc_ll_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(malloc_2, real_gballoc_ll_malloc_2);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
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
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connect, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ioctlsocket, 1);
    REGISTER_GLOBAL_MOCK_HOOK(shutdown, my_shutdown);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(shutdown, 1);

    REGISTER_GLOBAL_MOCK_HOOK(inet_ntop, my_inet_ntop);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(inet_ntop, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gethostname, my_gethostname);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gethostname, SOCKET_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gethostbyname, NULL);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(select, SOCKET_ERROR);
    REGISTER_GLOBAL_MOCK_RETURNS(WSAGetLastError, WSAEALREADY, WSAEFAULT);

    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SOCKET, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PADDRINFOA, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSABUF, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPDWORD, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED_COMPLETION_ROUTINE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSAOVERLAPPED, void*);

    REGISTER_UMOCK_ALIAS_TYPE(INT, int);
    REGISTER_UMOCK_ALIAS_TYPE(PSTR, char*);

    REGISTER_TYPE(SM_RESULT, SM_RESULT);

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

// Tests_SOCKET_TRANSPORT_WIN32_09_004: [ On any failure socket_transport_create_client shall return NULL. ]
TEST_FUNCTION(socket_transport_create_client_fail)
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
            SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();

            //assert
            ASSERT_IS_NULL(socket_handle);
        }
    }

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_005: [ On success socket_transport_create_client shall return SOCKET_TRANSPORT_HANDLE. ]
TEST_FUNCTION(socket_transport_create_client_succeed)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();

    //assert
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_089: [ On any failure socket_transport_create_server shall return NULL. ]
TEST_FUNCTION(socket_transport_create_server_fail)
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
            SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();

            //assert
            ASSERT_IS_NULL(socket_handle);
        }
    }

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_090: [ On success socket_transport_create_server shall return SOCKET_TRANSPORT_HANDLE. ]
TEST_FUNCTION(socket_transport_create_server_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();

    //assert
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_006: [ If socket_transport is NULL socket_transport_destroy shall return. ]
TEST_FUNCTION(socket_transport_destroy_socket_NULL_fail)
{
    //arrange

    //act
    socket_transport_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_007: [ socket_transport_destroy shall call sm_destroy to destroy the sm object. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_008: [ socket_transport_destroy shall free the SOCKET_TRANSPORT_HANDLE object. ]
TEST_FUNCTION(socket_transport_destroy_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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

// connect_to_endpoint

// Tests_SOCKET_TRANSPORT_WIN32_09_026: [ If any error is encountered connect_to_endpoint shall return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_winsock_connect_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAETOOMANYREFS);
    STRICT_EXPECTED_CALL(freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));


    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_022: [ If the return is SOCKET_ERROR, this indicates a failure and connect_to_endpoint shall fail. ]
TEST_FUNCTION(socket_transport_connect_connecting_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAEWOULDBLOCK);
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(WSAGetLastError());
    STRICT_EXPECTED_CALL(freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));


    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_023: [ If the return value is 0, this indicates a timeout and connect_to_endpoint shall fail. ]
TEST_FUNCTION(socket_transport_connect_timeout_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAEWOULDBLOCK);
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));


    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_024: [ Any other value this indicates a possible success and connect_to_endpoint shall test if the socket is writable by calling FD_ISSET. ]
TEST_FUNCTION(socket_transport_connect_fd_isset_fails)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAEWOULDBLOCK);
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(FD_ISSET(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(WSAGetLastError());
    STRICT_EXPECTED_CALL(freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));


    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}


// Tests_SOCKET_TRANSPORT_WIN32_09_009: [ If socket_transport is NULL, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_socket_transport_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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

// Tests_SOCKET_TRANSPORT_WIN32_09_010: [If hostname is NULL, socket_transport_connect shall fail and return a non - zero value.]
TEST_FUNCTION(socket_transport_connect_invalid_arguments_hostname)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    //act
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    int result = socket_transport_connect(socket_handle, NULL, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);

}

// Tests_SOCKET_TRANSPORT_WIN32_09_011 : [If port is 0, socket_transport_connect shall fail and return a non - zero value.]
TEST_FUNCTION(socket_transport_connect_invalid_arguments_fail_port)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    //act
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, 0, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);

}

// Tests_SOCKET_TRANSPORT_WIN32_09_012: [ If the socket_transport type is not SOCKET_CLIENT, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_fail_sockettransporttype)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    umock_c_negative_tests_snapshot();

    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}


// Tests_SOCKET_TRANSPORT_WIN32_09_014: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_sm_open_begin_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);
    umock_c_negative_tests_snapshot();

    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_018: [ If any failure is encountered, socket_transport_connect shall call sm_open_end with false, fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(freeaddrinfo(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));
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

// Tests_SOCKET_TRANSPORT_WIN32_09_024: [ Any other value this indicates a possible success and connect_to_endpoint shall test if the socket is writable by calling FD_ISSET. ]
TEST_FUNCTION(socket_transport_connect_fd_isset_succeeds)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(getaddrinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(connect(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAEWOULDBLOCK);
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(FD_ISSET(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(true);
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

// Tests_SOCKET_TRANSPORT_WIN32_09_013: [ socket_transport_connect shall call sm_open_begin to begin the open. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_017: [ If successful socket_transport_connect shall call sm_open_end with true. ]
TEST_FUNCTION(socket_transport_connect_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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

// Tests_SOCKET_TRANSPORT_WIN32_09_027: [ If socket_transport is NULL, socket_transport_disconnect shall fail and return. ]
TEST_FUNCTION(socket_transport_disconnect_invalid_arguments)
{
    //arrange
    umock_c_negative_tests_snapshot();

    //act
    socket_transport_disconnect(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup

}

// Tests_SOCKET_TRANSPORT_WIN32_09_083: [ If shutdown does not return 0, the socket is not valid therefore socket_transport_disconnect shall not call close ]
TEST_FUNCTION(socket_transport_disconnect_shutdown_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    int dummy_socket = 100;
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(dummy_socket);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(shutdown(IGNORED_ARG, 2))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_030: [ socket_transport_disconnect shall call closesocket to disconnect the connected socket. ]
TEST_FUNCTION(socket_transport_disconnect_failure_closesocket)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    int dummy_socket = 100;
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(dummy_socket);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(shutdown(IGNORED_ARG, SD_BOTH));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_083: [ If shutdown does not return 0 on a socket that is not a binding socket, the socket is not valid therefore socket_transport_disconnect shall not call close ]
TEST_FUNCTION(socket_transport_disconnect_binding_socket_closesocket)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    int dummy_socket = 100;
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(dummy_socket);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_9_029: [ If sm_close_begin does not return SM_EXEC_GRANTED, socket_transport_disconnect shall fail and return. ]
TEST_FUNCTION(socket_transport_disconnect_sm_close_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_030: [ socket_transport_disconnect shall call closesocket to disconnect the connected socket. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_031: [ socket_transport_disconnect shall call sm_close_end. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_029: [ If sm_close_begin does not return SM_EXEC_GRANTED, socket_transport_disconnect shall fail and return. ]
TEST_FUNCTION(socket_transport_disconnect_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(shutdown(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// socket_transport_send
// Tests_SOCKET_TRANSPORT_WIN32_09_032: [ If socket_transport is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_fail_socket_transport_NULL)
{
    //arrange
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(NULL, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_033: [ If payload is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_fail_payload_NULL)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;


    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, NULL, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_034: [ If buffer_count is 0, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_fail_buffercount_zero)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;


    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 0, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_036: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_send shall fail and return SOCKET_SEND_ERROR. ]
TEST_FUNCTION(socket_transport_send_fail_sm_exec_begin)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_039: [ Otherwise socket_transport_send shall return SOCKET_SEND_FAILED. ]
TEST_FUNCTION(socket_transport_send_fail_WSASend)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(WSASend(IGNORED_ARG, (LPWSABUF)&payload, 1, IGNORED_ARG, TEST_FLAGS, NULL, NULL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_FAILED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}


// Tests_SOCKET_TRANSPORT_WIN32_09_035: [ socket_transport_send shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_037: [ socket_transport_send shall call WSASend to send data with flags and the overlapped_data. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_040: [ socket_transport_send shall call sm_exec_end. ]
TEST_FUNCTION(socket_transport_send_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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

// Tests_SOCKET_TRANSPORT_WIN32_09_041: [ If socket_transport is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
TEST_FUNCTION(socket_transport_receive_socket_transport_NULL_fail)
{
    //arrange
    uint32_t bytes_recv;
    SOCKET_BUFFER payload = { 0 };

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(NULL, &payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_042: [ If payload is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
TEST_FUNCTION(socket_transport_receive_payload_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, NULL, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_043: [ If buffer_count is 0, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
TEST_FUNCTION(socket_transport_receive_buffer_count_0_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload = { TEST_BYTES_RECV, buffer };

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, &payload, 0, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_045: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
TEST_FUNCTION(socket_transport_receive_sm_exec_begin_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[] = {
        { TEST_BYTES_RECV, buffer }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_051: [ If WSAGetLastError returns WSA_IO_PENDING, and bytes_recv is not NULL, socket_transport_receive shall set bytes_recv to 0. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_049: [ socket_transport_receive shall return SOCKET_RECEIVE_OK. ]
TEST_FUNCTION(socket_transport_receive_socket_receive_would_block)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(WSARecv(IGNORED_ARG, (LPWSABUF)&payload, 1, IGNORED_ARG, IGNORED_ARG, NULL, NULL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSA_IO_PENDING);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, &payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_WOULD_BLOCK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_053: [ If WSAGetLastError does not returns WSA_IO_PENDING socket_transport_receive shall return SOCKET_RECEIVE_ERROR. ]
TEST_FUNCTION(socket_transport_receive_socket_receive_error)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = 10;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(WSARecv(IGNORED_ARG, (LPWSABUF)&payload, 1, IGNORED_ARG, IGNORED_ARG, NULL, NULL))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(WSAGetLastError());
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, &payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_044: [ socket_transport_receive shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_046: [ socket_transport_receive shall call WSARecv with the payload, flags and the data which is used as overlapped object. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_047: [ If WSARecv return 0, socket_transport_receive shall do the following: ]
// Tests_SOCKET_TRANSPORT_WIN32_09_048: [ If bytes_recv is not NULL, socket_transport_receive shall copy the number of bytes into bytes_recv. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_052: [socket_transport_receive shall return SOCKET_RECEIVE_WOULD_BLOCK.]
// Tests_SOCKET_TRANSPORT_WIN32_09_054: [ socket_transport_receive shall call sm_exec_end. ]
TEST_FUNCTION(socket_transport_receive_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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

// Tests_SOCKET_TRANSPORT_WIN32_09_058: [ socket_transport_listen shall call sm_open_begin to begin the open. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_060: [ socket_transport_listen shall call socket with the params AF_INET, SOCK_STREAM and IPPROTO_TCP. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_061: [ socket_transport_listen shall bind to the socket by calling bind. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_062: [ socket_transport_listen shall start listening to incoming connection by calling listen. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_063: [ If successful socket_transport_listen shall call sm_open_end with true. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_066: [ socket_transport_listen shall call closesocket. ]
TEST_FUNCTION(socket_transport_listen_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
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

// Tests_SOCKET_TRANSPORT_WIN32_09_055: [ If socket_transport is NULL, socket_transport_listen shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_socket_transport_NULL_fail)
{
    //arrange

    //act
    int result = socket_transport_listen(NULL, TEST_PORT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_056: [ If port is 0, socket_transport_listen shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_port_0_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    //act
    int result = socket_transport_listen(socket_handle, 0);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_057: [ If the transport type is not SOCKET_BINDING, socket_transport_listen shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_invalid_socket_type_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    //act
    int result = socket_transport_listen(socket_handle, TEST_PORT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_059: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_listen shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_sm_open_not_granted_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG))
        .SetReturn(SM_ERROR);

    //act
    int result = socket_transport_listen(socket_handle, TEST_PORT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_060: [ socket_transport_listen shall call socket with the params AF_INET, SOCK_STREAM and IPPROTO_TCP. ]
TEST_FUNCTION(socket_transport_listen_socket_create_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(INVALID_SOCKET);

    //act
    int result = socket_transport_listen(socket_handle, TEST_PORT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_061: [ socket_transport_listen shall bind to the socket by calling bind. ]
TEST_FUNCTION(socket_transport_listen_socket_bind_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(htons(TEST_PORT));
    STRICT_EXPECTED_CALL(bind(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    //act
    int result = socket_transport_listen(socket_handle, TEST_PORT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_065: [ sock_transport_listen shall set listening socket in non-blocking mode by calling ioctlsocket. ]
TEST_FUNCTION(socket_transport_listen_socket_ioctlsocket_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(htons(TEST_PORT));
    STRICT_EXPECTED_CALL(bind(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    //act
    int result = socket_transport_listen(socket_handle, TEST_PORT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_062: [ socket_transport_listen shall start listening to incoming connection by calling listen. ]
TEST_FUNCTION(socket_transport_listen_socket_listen_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(htons(TEST_PORT));
    STRICT_EXPECTED_CALL(bind(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(ioctlsocket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(listen(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    //act
    int result = socket_transport_listen(socket_handle, TEST_PORT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_09_064: [ If any failure is encountered, socket_transport_listen shall call sm_open_end with false, fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(htons(TEST_PORT));
    STRICT_EXPECTED_CALL(bind(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(listen(IGNORED_ARG, IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            int result = socket_transport_listen(socket_handle, TEST_PORT);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }
    //cleanup
    socket_transport_destroy(socket_handle);
}

// socket_transport_accept

// Tests_SOCKET_TRANSPORT_WIN32_09_069: [ socket_transport_accept shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_071: [ socket_transport_accept shall call select to determine if the socket is ready to be read passing connection_timeout_ms. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_072: [ socket_transport_accept shall call accept to accept the incoming socket connection. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_074: [ socket_transport_accept shall allocate a SOCKET_TRANSPORT for the incoming connection and call sm_create and sm_open on the connection. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_075: [ If successful socket_transport_accept shall return SOCKET_ACCEPT_OK. ]
TEST_FUNCTION(socket_transport_accept_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
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
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_OK, accept_result);
    ASSERT_IS_NOT_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(accept_socket_handle);
    socket_transport_destroy(accept_socket_handle);
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_067: [ If socket_transport is NULL, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_null_input_fail)
{
    //arrange
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(NULL, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SOCKET_TRANSPORT_WIN32_09_068: [ If the transport type is not SOCKET_BINDING, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_sockettransport_type_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_070: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_smexecbegin_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_002: [ If select returns SOCKET_ERROR and WSAGetLastError does not return WSAEINPROGRESS, socket_transport_accept shall return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_select_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(WSAGetLastError());
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_001: [ If select returns SOCKET_ERROR and WSAGetLastError return WSAEINPROGRESS, socket_transport_accept shall return SOCKET_ACCEPT_INPROGRESS. ]
TEST_FUNCTION(socket_transport_accept_select_in_progress_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAEINPROGRESS);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_INPROGRESS, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_091: [ If select returns zero, socket_transport_accept shall set accepted_socket to NULL and return SOCKET_ACCEPT_NO_CONNECTION. ]
TEST_FUNCTION(socket_transport_accept_select_returns_zero)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_NO_CONNECTION, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_073: [ If accept returns an INVALID_SOCKET and WSAGetLastError does not return WSAEWOULDBLOCK, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_accept_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn((SOCKET)SOCKET_ERROR);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAENOTSOCK);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_003: [ If accept returns an INVALID_SOCKET and WSAGetLastError returns WSAENOBUFS, socket_transport_accept shall fail and return SOCKET_ACCEPT_PORT_EXHAUSTION. ]
TEST_FUNCTION(socket_transport_accept_accept_no_buffer_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn((SOCKET)SOCKET_ERROR);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAENOBUFS);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_PORT_EXHAUSTION, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_004: [ If accept returns an INVALID_SOCKET and WSAGetLastError returns WSAEWOULDBLOCK, socket_transport_accept shall fail and return SOCKET_ACCEPT_NO_CONNECTION. ]
TEST_FUNCTION(socket_transport_accept_accept_would_block_no_connection)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn((SOCKET)SOCKET_ERROR);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(WSAEWOULDBLOCK);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_NO_CONNECTION, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_084: [ If malloc fails, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_malloc_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    int dummy_socket = 100;
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(dummy_socket);
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_085: [ If sm_create fails, socket_transport_accept shall close the incoming socket, fail, and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_sm_create_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_086: [ If sm_open_begin fails, socket_transport_accept shall close the incoming socket, fail, and return SOCKET_ACCEPT_ERROR ]
TEST_FUNCTION(socket_transport_accept_sm_open_begin_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_076: [ If any failure is encountered, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-2);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// socket_transport_get_underlying_socket

// Tests_SOCKET_TRANSPORT_WIN32_09_079: [ socket_transport_get_underlying_socket shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_081: [ socket_transport_get_underlying_socket shall return the SOCKET_TRANSPORT socket value. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_082: [ socket_transport_get_underlying_socket shall call sm_exec_end. ]
TEST_FUNCTION(socket_transport_get_underlying_socket_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
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

// Tests_SOCKET_TRANSPORT_WIN32_09_078: [ If socket_transport is NULL, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
TEST_FUNCTION(socket_transport_get_underlying_socket_smexecbegin_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    //act
    SOCKET_HANDLE underlying_socket = socket_transport_get_underlying_socket(socket_handle);

    //assert
    ASSERT_IS_NOT_NULL(underlying_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_078: [ If socket_transport is NULL, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
TEST_FUNCTION(socket_transport_get_underlying_socket_NULL_input_fail)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    SOCKET_HANDLE underying_socket = socket_transport_get_underlying_socket(NULL);

    //assert
    ASSERT_IS_NOT_NULL(underying_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_098: [ socket_transport_create_from_socket shall call sm_create to create a sm_object with the type set to SOCKET_CLIENT. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_097: [ socket_transport_create_from_socket shall allocate a new SOCKET_TRANSPORT object. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_099: [ socket_transport_create_from_socket shall assign the socket_handle to the new allocated socket transport. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_101: [ On success socket_transport_create_from_socket shall return SOCKET_TRANSPORT_HANDLE. ]
TEST_FUNCTION(socket_transport_create_from_socket_succeeds)
{
    //arrange
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    //act
    SOCKET_TRANSPORT_HANDLE socket_transport = socket_transport_create_from_socket((SOCKET_HANDLE)test_socket);

    //assert
    ASSERT_IS_NOT_NULL(socket_transport);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_transport);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_096: [ If socket_handle is an INVALID_SOCKET, socket_transport_create_from_socket shall fail and return NULL. ]
TEST_FUNCTION(socket_transport_create_from_socket_invalid_input)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    SOCKET_TRANSPORT_HANDLE socket_transport = socket_transport_create_from_socket((SOCKET_HANDLE)INVALID_SOCKET);

    //assert
    ASSERT_IS_NULL(socket_transport);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SOCKET_TRANSPORT_WIN32_09_014: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_create_from_socket shall fail and return NULL. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_102: [ On any failure socket_transport_create_from_socket shall return NULL. ]
TEST_FUNCTION(socket_transport_create_from_socket_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_from_socket((SOCKET_HANDLE)test_socket);

            //assert
            ASSERT_IS_NULL(socket_handle);

            //cleanup
            socket_transport_destroy(socket_handle);
        }
    }
}

// Tests_SOCKET_TRANSPORT_WIN32_09_093: [ If socket_transport_handle is NULL, socket_transport_is_valid_socket shall fail and return false. ]
TEST_FUNCTION(socket_transport_is_valid_socket_NULL_input)
{
    //arrange
    umock_c_reset_all_calls();

    //act
    bool result = socket_transport_is_valid_socket(NULL);

    //assert
    ASSERT_IS_TRUE(!result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SOCKET_TRANSPORT_WIN32_09_095: [ On success, socket_transport_is_valid_socket shall return true.
TEST_FUNCTION(socket_transport_is_valid_socket_succeeds)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE test_socket_transport = socket_transport_create_client();
    socket_transport_connect(test_socket_transport, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);
    umock_c_reset_all_calls();

    //act
    bool result = socket_transport_is_valid_socket(test_socket_transport);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(test_socket_transport);
    socket_transport_destroy(test_socket_transport);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_094: [ If the socket inside socket_transport_handle is an INVALID_SOCKET, socket_transport_is_valid_socket shall fail and return false. ]
TEST_FUNCTION(socket_transport_is_valid_socket_INVALID_SOCKET)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE test_socket_transport = socket_transport_create_client();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(INVALID_SOCKET);
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    //act
    socket_transport_connect(test_socket_transport, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);
    bool result = socket_transport_is_valid_socket(test_socket_transport);

    //assert
    ASSERT_IS_TRUE(!result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(test_socket_transport);
    socket_transport_destroy(test_socket_transport);
}

// socket_transport_get_local_address

// Tests_SOCKET_TRANSPORT_WIN32_11_001: [ If socket_transport is NULL, socket_transport_get_local_address shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_get_local_address_handle_NULL_fail)
{
    //arrange
    char hostname[MAX_GET_HOST_NAME_LEN];
    LOCAL_ADDRESS* local_address_list;
    uint32_t address_count;

    //act
    int result = socket_transport_get_local_address(NULL, hostname, &local_address_list, &address_count);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_11_002: [ If hostname is NULL, socket_transport_get_local_address shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_get_local_address_hostname_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    LOCAL_ADDRESS* local_address_list;
    uint32_t address_count;

    //act
    int result = socket_transport_get_local_address(socket_handle, NULL, &local_address_list, &address_count);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Test_SOCKET_TRANSPORT_WIN32_11_003: [ If local_address_list is not NULL and address_count is NULL, socket_transport_get_local_address shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_get_local_address_address_count_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    char hostname[MAX_GET_HOST_NAME_LEN];
    LOCAL_ADDRESS* local_address_list;

    //act
    int result = socket_transport_get_local_address(socket_handle, hostname, &local_address_list, NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_004: [ socket_transport_get_local_address shall call sm_exec_begin ]
// Tests_SOCKET_TRANSPORT_WIN32_11_005: [ socket_transport_get_local_address shall call get the hostname by calling gethostname. ]
// Tests_SOCKET_TRANSPORT_WIN32_11_006: [ If local_address_list is not NULL, socket_transport_get_local_address shall call gethostbyname to get the addresses in a hostent object. ]
// Tests_SOCKET_TRANSPORT_WIN32_11_007: [ socket_transport_get_local_address shall allocate the LOCAL_ADDRESS array. ]
// Tests_SOCKET_TRANSPORT_WIN32_11_008: [ For each IP in the hostent object, socket_transport_get_local_address shall copy the value into the LOCAL_ADDRESS address by calling inet_ntop. ]
// Tests_SOCKET_TRANSPORT_WIN32_11_009: [ socket_transport_get_local_address shall call sm_exec_end. ]
// Tests_SOCKET_TRANSPORT_WIN32_11_010: [ On success socket_transport_get_local_address shall return 0. ]
TEST_FUNCTION(socket_transport_get_local_address_success)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    char hostname[MAX_GET_HOST_NAME_LEN];
    LOCAL_ADDRESS* local_address_list;
    uint32_t address_count;

    uint32_t NUM_OF_ADDRESSES = 2;
    const char* ip_address_list[] = {
        "10.0.0.1",
        "10.0.0.50",
        NULL
    };

    struct hostent test_host_info = {0};
    test_host_info.h_addrtype = AF_INET;
    test_host_info.h_addr_list = ip_address_list;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(gethostname(IGNORED_ARG, MAX_GET_HOST_NAME_LEN));
    STRICT_EXPECTED_CALL(gethostbyname(IGNORED_ARG))
        .SetReturn(&test_host_info);
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, 2));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    int result = socket_transport_get_local_address(socket_handle, hostname, &local_address_list, &address_count);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, NUM_OF_ADDRESSES, address_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
    free(local_address_list);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_006: [ If local_address_list is not NULL, socket_transport_get_local_address shall call gethostbyname to get the addresses in a hostent object. ]
TEST_FUNCTION(socket_transport_get_local_address_invalid_address_list_success)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    char hostname[MAX_GET_HOST_NAME_LEN];
    const char* ip_address_list[] = {
        "10.0.0.1",
        NULL
    };

    struct hostent test_host_info = { 0 };
    test_host_info.h_addrtype = 1;
    test_host_info.h_addr_list = ip_address_list;
    LOCAL_ADDRESS* local_address_list;
    uint32_t address_count;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(gethostname(IGNORED_ARG, MAX_GET_HOST_NAME_LEN));
    STRICT_EXPECTED_CALL(gethostbyname(IGNORED_ARG))
        .SetReturn(&test_host_info);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    int result = socket_transport_get_local_address(socket_handle, hostname, &local_address_list, &address_count);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_006: [ If local_address_list is not NULL, socket_transport_get_local_address shall call gethostbyname to get the addresses in a hostent object. ]
TEST_FUNCTION(socket_transport_get_local_address_no_address_list_success)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    char hostname[MAX_GET_HOST_NAME_LEN];

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(gethostname(IGNORED_ARG, MAX_GET_HOST_NAME_LEN));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    int result = socket_transport_get_local_address(socket_handle, hostname, NULL, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_11_010: [ On success socket_transport_get_local_address shall return 0. ]
TEST_FUNCTION(socket_transport_get_local_address_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    char hostname[MAX_GET_HOST_NAME_LEN];
    LOCAL_ADDRESS* local_address_list;
    uint32_t address_count;

    const char* ip_address_list[] = {
        "10.0.0.1",
        "10.0.0.50",
        NULL
    };

    struct hostent test_host_info = { 0 };
    test_host_info.h_addrtype = AF_INET;
    test_host_info.h_addr_list = ip_address_list;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(gethostname(IGNORED_ARG, MAX_GET_HOST_NAME_LEN));
    STRICT_EXPECTED_CALL(gethostbyname(IGNORED_ARG))
        .SetReturn(&test_host_info);
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, 2));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            int result = socket_transport_get_local_address(socket_handle, hostname, &local_address_list, &address_count);

            //assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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
#include "c_pal/string_utils.h"

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
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connect, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(ioctlsocket, 1);

    REGISTER_GLOBAL_MOCK_HOOK(inet_ntop, my_inet_ntop);

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

// Tests_SOCKET_TRANSPORT_WIN32_09_004: [ On any failure socket_transport_create shall return NULL. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_09_001: [ socket_transport_create shall ensure type is either SOCKET_CLIENT, or SOCKET_SERVER. ]
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

// Tests_SOCKET_TRANSPORT_WIN32_09_005: [ On success socket_transport_create shall return SOCKET_TRANSPORT_HANDLE. ]
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

// connect_to_endpoint

// Tests_SOCKET_TRANSPORT_WIN32_09_026: [ If any error is encountered connect_to_endpoint shall return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_winsock_connect_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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

// Tests_SOCKET_TRANSPORT_WIN32_09_010: [If hostname is NULL, socket_transport_connect shall fail and return a non - zero value.]
TEST_FUNCTION(socket_transport_connect_invalid_arguments_hostname)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    //act
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    int result = socket_transport_connect(socket_handle, NULL, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, 0, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);

}

// Tests_SOCKET_TRANSPORT_WIN32_09_012: [ If the socket_transport type is not SOCKET_CLIENT, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_fail_sockettransporttype)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    umock_c_negative_tests_snapshot();

    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}


// Tests_SOCKET_TRANSPORT_WIN32_09_014: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_sm_open_begin_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);
    umock_c_negative_tests_snapshot();

    //act
    int result = socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_018: [ If any failure is encountered, socket_transport_connect shall call sm_open_end with false, fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_fail)
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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

// Tests_SOCKET_TRANSPORT_WIN32_09_030: [ socket_transport_disconnect shall call closesocket to disconnect the connected socket. ]
TEST_FUNCTION(socket_transport_disconnect_failure_closesocket)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
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
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_9_029: [ If sm_close_begin does not return SM_EXEC_GRANTED, socket_transport_disconnect shall fail and return. ]
TEST_FUNCTION(socket_transport_disconnect_close_fail_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG))
        .SetReturn(SM_ERROR);

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
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_WIN32_09_033: [ If payload is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_fail_payload_NULL)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_034: [ If buffer_count is 0, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_fail_buffercount_zero)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_036: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_send shall fail and return SOCKET_SEND_ERROR. ]
TEST_FUNCTION(socket_transport_send_fail_sm_exec_begin)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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

TEST_FUNCTION(socket_transport_receive_socket_transport_NULL_fail)
{
    //arrange
    uint32_t bytes_recv;
    SOCKET_BUFFER payload = { 0 };

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(NULL, &payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

TEST_FUNCTION(socket_transport_receive_payload_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, NULL, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

TEST_FUNCTION(socket_transport_receive_buffer_count_0_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload = { TEST_BYTES_RECV, buffer };

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, &payload, 0, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

TEST_FUNCTION(socket_transport_receive_not_connected_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    socket_transport_destroy(socket_handle);
}

TEST_FUNCTION(socket_transport_receive_socket_receive_would_block)
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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

// Tests_SOCKET_TRANSPORT_WIN32_09_058: [ socket_transport_listen shall call sm_open_begin to begin the open. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_060: [ socket_transport_listen shall call socket with the params AF_INET, SOCK_STREAM and IPPROTO_TCP. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_061: [ socket_transport_listen shall bind to the socket by calling bind. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_062: [ socket_transport_listen shall start listening to incoming connection by calling listen. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_063: [ If successful socket_transport_listen shall call sm_open_end with true. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_066: [ socket_transport_listen shall call closesocket. ]
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
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

// Tests_SOCKET_TRANSPORT_WIN32_09_057: [ If the transport type is not SOCKET_SERVER, socket_transport_listen shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_invalid_socket_type_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
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
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_061: [ socket_transport_listen shall bind to the socket by calling bind. ]
TEST_FUNCTION(socket_transport_listen_socket_bind_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
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
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_065: [ sock_transport_listen shall set listening socket in non-blocking mode by calling ioctlsocket. ]
TEST_FUNCTION(socket_transport_listen_socket_ioctlsocket_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
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
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_062: [ socket_transport_listen shall start listening to incoming connection by calling listen. ]
TEST_FUNCTION(socket_transport_listen_socket_listen_fail)
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
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_09_064: [ If any failure is encountered, socket_transport_listen shall call sm_open_end with false, fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_fail)
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

// Tests_SOCKET_TRANSPORT_WIN32_09_067: [ If socket_transport is NULL, socket_transport_accept shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_accept_null_input_fail)
{
    //arrange
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(NULL);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SOCKET_TRANSPORT_WIN32_09_068: [ If the transport type is not SOCKET_SERVER, socket_transport_accept shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_accept_sockettransport_type_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_070: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_accept shall fail and return SOCKET_SEND_ERROR. ]
TEST_FUNCTION(socket_transport_accept_smexecbegin_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_071: [ socket_transport_accept shall call select determine if the socket is ready to be read passing a timeout of 10 milliseconds. ]
TEST_FUNCTION(socket_transport_accept_select_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(SOCKET_ERROR);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_073: [ If accept returns an INVALID_SOCKET, socket_transport_accept shall fail and return Null. ]
TEST_FUNCTION(socket_transport_accept_accept_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn((SOCKET)SOCKET_ERROR);
    STRICT_EXPECTED_CALL(WSAGetLastError())
        .SetReturn(0);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

TEST_FUNCTION(socket_transport_accept_malloc_fail)
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
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(accept_socket_handle);
    socket_transport_destroy(accept_socket_handle);
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}


TEST_FUNCTION(socket_transport_accept_sm_create_fail)
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
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(accept_socket_handle);
    socket_transport_destroy(accept_socket_handle);
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}


TEST_FUNCTION(socket_transport_accept_sm_open_begin_fail)
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
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);
    STRICT_EXPECTED_CALL(closesocket(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(accept_socket_handle);
    socket_transport_destroy(accept_socket_handle);
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_076: [ If any failure is encountered, socket_transport_accept shall fail and return NULL. ]
TEST_FUNCTION(socket_transport_accept_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(select(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-2);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle = socket_transport_accept(socket_handle);

    //assert
    ASSERT_IS_NULL(accept_socket_handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(accept_socket_handle);
    socket_transport_destroy(accept_socket_handle);
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_WIN32_09_069: [ socket_transport_accept shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_071: [ socket_transport_accept shall call select determine if the socket is ready to be read passing a timeout of 10 milliseconds. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_072: [ socket_transport_accept shall call accept to accept the incoming socket connection. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_074: [ socket_transport_accept shall allocate a SOCKET_TRANSPORT for the incoming connection and call sm_create and sm_open on the connection. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_075: [ If successful socket_transport_accept shall return the allocated SOCKET_TRANSPORT. ]
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

// Tests_SOCKET_TRANSPORT_WIN32_09_079: [ socket_transport_get_underlying_socket shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_081: [ socket_transport_get_underlying_socket shall return the SOCKET_TRANSPORT socket value. ]
// Tests_SOCKET_TRANSPORT_WIN32_09_082: [ socket_transport_get_underlying_socket shall call sm_exec_end. ]
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

// Tests_SOCKET_TRANSPORT_WIN32_09_078: [ If socket_transport is NULL, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
TEST_FUNCTION(socket_transport_get_underlying_socket_smexecbegin_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create(SOCKET_SERVER);
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

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

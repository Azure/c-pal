// Copyright(C) Microsoft Corporation.All rights reserved.


#include "socket_transport_linux_ut_pch.h"

static SOCKET_HANDLE* g_test_socket_array[MAX_SOCKET_ARRAY];
static SOCKET_HANDLE test_socket = 0;
static struct addrinfo g_addrInfo = { 0 };

#define TEST_HOSTNAME               "test_hostname"
#define TEST_INCOMING_HOSTNAME      "incoming_hostname"
#define TEST_PORT                   1234
#define TEST_INCOMING_PORT          5678
#define TEST_CONNECTION_TIMEOUT     1000
#define TEST_FLAGS                  123
#define TEST_BYTES_SENT             10
#define TEST_BYTES_RECV             10

static unsigned char recv_buff[TEST_BYTES_RECV];

TEST_DEFINE_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

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
    return test_socket-1;
}

static SOCKET_HANDLE my_accept(SOCKET_HANDLE s, struct sockaddr* addr, socklen_t* addrlen)
{
    (void)s;
    (void)addrlen;

    struct sockaddr_in* cli_addr = (struct sockaddr_in*)addr;
    cli_addr->sin_port = TEST_INCOMING_PORT;
    g_test_socket_array[test_socket++] = real_gballoc_ll_malloc(sizeof(SOCKET_HANDLE));
    return test_socket-1;
}

static int my_close(int s)
{
    test_socket--;
    real_gballoc_ll_free((void*)g_test_socket_array[s]);
    return 0;
}

static const char* my_inet_ntop(int af, const void* cp, char* buf, socklen_t len)
{
    (void)af;
    (void)cp;
    (void)len;

    strcpy(buf, TEST_INCOMING_HOSTNAME);
    return buf;
}

static int my_getaddrinfo(const char* pNodeName, const char* pServiceName, const struct addrinfo* pHints, struct addrinfo** ppResult)
{
    (void)pNodeName;
    (void)pServiceName;
    (void)pHints;
    *ppResult = &g_addrInfo;
    return 0;
}

static ssize_t my_send(SOCKET_HANDLE sockfd, const void* buf, size_t len, int flags)
{
    (void)sockfd;
    (void)buf;
    (void)flags;
    return len;
}

static ssize_t my_recv(SOCKET_HANDLE sockfd, void* buf, size_t len, int flags)
{
    (void)sockfd;
    (void)flags;

    memcpy(buf, recv_buff, len);
    return TEST_BYTES_RECV;
}

static int my_gethostname(char* name, size_t namelen)
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

    REGISTER_SM_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(malloc, real_gballoc_ll_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(malloc_2, real_gballoc_ll_malloc_2);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
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
    REGISTER_GLOBAL_MOCK_HOOK(inet_ntop, my_inet_ntop);
    REGISTER_GLOBAL_MOCK_HOOK(getaddrinfo, my_getaddrinfo);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(getaddrinfo, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(connect, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(send, my_send);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(send, INVALID_SOCKET);
    REGISTER_GLOBAL_MOCK_HOOK(recv, my_recv);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(recv, INVALID_SOCKET);
    REGISTER_GLOBAL_MOCK_HOOK(gethostname, my_gethostname);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gethostname, -1);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(getnameinfo, EAI_SYSTEM);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(getifaddrs, -1);

    REGISTER_TYPE(SM_RESULT, SM_RESULT);

    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SOCKET_HANDLE, int);
    REGISTER_UMOCK_ALIAS_TYPE(ssize_t, long);
    REGISTER_UMOCK_ALIAS_TYPE(socklen_t, int);

    g_addrInfo.ai_canonname = "ai_canonname";
    for (size_t index = 0; index < TEST_BYTES_RECV; index++)
    {
        recv_buff[index] = 'a' + index;
    }
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

// Tests_SOCKET_TRANSPORT_LINUX_11_002: [ socket_transport_create_client shall allocate a new SOCKET_TRANSPORT object. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_003: [ socket_transport_create_client shall call sm_create to create a sm object with the type set to SOCKET_CLIENT. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_005: [ On success socket_transport_create_client shall return SOCKET_TRANSPORT_HANDLE. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_004: [ On any failure socket_transport_create_client shall return NULL. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_079: [ socket_transport_create_server shall allocate a new SOCKET_TRANSPORT object. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_080: [ socket_transport_create_server shall call sm_create to create a sm object to create a sm object with the type set to SOCKET_BINDING. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_082: [ On success socket_transport_create_server shall return SOCKET_TRANSPORT_HANDLE. ]
TEST_FUNCTION(socket_transport_create_server_succeed)
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

// Tests_SOCKET_TRANSPORT_LINUX_11_081: [ On any failure socket_transport_create_server shall return NULL. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_009: [ If socket_transport is NULL, socket_transport_connect shall fail and return a non-zero value. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_010: [ If hostname is NULL, socket_transport_connect shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_connect_hostname_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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

// Tests_SOCKET_TRANSPORT_LINUX_11_021: [ socket_transport_disconnect shall call sm_close_begin to begin the closing process. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_025: [ socket_transport_disconnect shall call shutdown to stop both the transmit and reception of the connected socket. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_023: [ socket_transport_disconnect shall call close to disconnect the connected socket. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_024: [ socket_transport_disconnect shall call sm_close_end. ]
TEST_FUNCTION(socket_transport_disconnect_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(shutdown(IGNORED_ARG, 2));
    STRICT_EXPECTED_CALL(close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    //act
    socket_transport_disconnect(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_022: [ If sm_close_begin does not return SM_EXEC_GRANTED, socket_transport_disconnect shall fail and return. ]
TEST_FUNCTION(socket_transport_disconnect_close_fail_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
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

// Tests_SOCKET_TRANSPORT_LINUX_11_026: [ If shutdown does not return 0, the socket is not valid therefore socket_transport_disconnect shall not call 'close' ]
TEST_FUNCTION(socket_transport_disconnect_shutdown_fail_succeed)
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

// socket_transport_send

// Tests_SOCKET_TRANSPORT_LINUX_11_027: [ If socket_transport is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_socket_transport_NULL_fail)
{
    //arrange

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = TEST_BYTES_SENT;

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(NULL, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_LINUX_11_028: [ If payload is NULL, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_payload_NULL_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_written;

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, NULL, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_029: [ If buffer_count is 0, socket_transport_send shall fail and return SOCKET_SEND_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_send_count_0_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = TEST_BYTES_SENT;

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 0, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_030: [ socket_transport_send shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_032: [ For each buffer count in payload socket_transport_send shall call send to send data with flags as a parameter. ]
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
    payload.length = TEST_BYTES_SENT;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, payload.length, bytes_written);

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_035: [ Otherwise socket_transport_send shall continue calling send until the SOCKET_BUFFER length is reached. ]
TEST_FUNCTION(socket_transport_send_multiple_sends_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = TEST_BYTES_SENT;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(TEST_BYTES_SENT/2);
    STRICT_EXPECTED_CALL(send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(TEST_BYTES_SENT/2);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, payload.length, bytes_written);

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_031: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_send shall fail and return SOCKET_SEND_ERROR. ]
TEST_FUNCTION(socket_transport_send_not_connected_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = TEST_BYTES_SENT;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_destroy(socket_handle);
}

// Codes_SOCKET_TRANSPORT_LINUX_11_034: [ If the errno is equal to ECONNRESET, socket_transport_send shall return SOCKET_SEND_SHUTDOWN. ]
TEST_FUNCTION(socket_transport_send_shutsdown_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = TEST_BYTES_SENT;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = ECONNRESET;

    //act
    SOCKET_SEND_RESULT result = socket_transport_send(socket_handle, &payload, 1, &bytes_written, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_SHUTDOWN, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_033: [ If send returns a value less then 0, socket_transport_send shall stop sending and return SOCKET_SEND_FAILED. ]
TEST_FUNCTION(socket_transport_send_fails)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_written;
    SOCKET_BUFFER payload;
    payload.buffer = (void*)0xABC;
    payload.length = TEST_BYTES_SENT;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(send(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(-1);
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

// socket_transport_receive

// Tests_SOCKET_TRANSPORT_LINUX_11_038: [ If socket_transport is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_INVALID_ARG. ]
TEST_FUNCTION(socket_transport_receive_socket_transport_NULL_fail)
{
    //arrange
    uint32_t bytes_recv;
    SOCKET_BUFFER payload = {0};

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(NULL, &payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_LINUX_11_039: [ If payload is NULL, socket_transport_receive shall fail and return SOCKET_RECEIVE_INVALID_ARG. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_040: [ If buffer_count is 0, socket_transport_receive shall fail and return SOCKET_RECEIVE_INVALID_ARG. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_041: [ socket_transport_receive shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_043: [ For each buffer count in payload socket_transport_receive shall call recv with the flags parameter. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_049: [ Else socket_transport_receive shall do the following: ]
// Tests_SOCKET_TRANSPORT_LINUX_11_050: [ socket_transport_receive shall test that the total recv size will not overflow. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_051: [ socket_transport_receive shall store the received byte size. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_053: [ socket_transport_receive shall call sm_exec_end. ]
TEST_FUNCTION(socket_transport_receive_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[] = {
        { TEST_BYTES_RECV, buffer }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, payload[0].length, bytes_recv);
    ASSERT_ARE_EQUAL(int, 0, memcmp(payload[0].buffer, recv_buff, payload[0].length));

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_050: [ socket_transport_receive shall test that the total recv size will not overflow. ]
TEST_FUNCTION(socket_transport_receive_overflow_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer_1[TEST_BYTES_RECV];
    unsigned char buffer_2[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[2] = {
        { TEST_BYTES_RECV, buffer_1 },
        { TEST_BYTES_RECV, buffer_2 }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(TEST_BYTES_RECV);
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(UINT32_MAX-1);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 2, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_043: [ For each buffer count in payload socket_transport_receive shall call recv with the flags parameter. ]
TEST_FUNCTION(socket_transport_receive_multiple_recv_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer_1[TEST_BYTES_RECV];
    unsigned char buffer_2[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[2] = {
        { TEST_BYTES_RECV, buffer_1 },
        { TEST_BYTES_RECV, buffer_2 }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 2, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, payload[0].length, TEST_BYTES_RECV);
    ASSERT_ARE_EQUAL(int, 0, memcmp(payload[0].buffer, recv_buff, payload[0].length));
    ASSERT_ARE_EQUAL(uint32_t, payload[1].length, TEST_BYTES_RECV);
    ASSERT_ARE_EQUAL(int, 0, memcmp(payload[1].buffer, recv_buff, payload[1].length));

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_042: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_receive shall fail and return SOCKET_RECEIVE_ERROR. ]
TEST_FUNCTION(socket_transport_receive_not_connected_fail)
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
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_044: [ If recv a value less then 0, socket_transport_receive shall do the following: ]
// Tests_SOCKET_TRANSPORT_LINUX_11_045: [ If errno is EAGAIN or EWOULDBLOCK, socket_transport_receive shall break out of loop and return SOCKET_RECEIVE_WOULD_BLOCK. ]
TEST_FUNCTION(socket_transport_receive_recv_returns_WOULDBLOCK_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[] = {
        { TEST_BYTES_RECV, buffer }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = EWOULDBLOCK;

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_WOULD_BLOCK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_044: [ If recv a value less then 0, socket_transport_receive shall do the following: ]
// Tests_SOCKET_TRANSPORT_LINUX_11_045: [ If errno is EAGAIN or EWOULDBLOCK, socket_transport_receive shall break out of loop and return SOCKET_RECEIVE_WOULD_BLOCK. ]
TEST_FUNCTION(socket_transport_receive_recv_returns_EAGAIN_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[] = {
        { TEST_BYTES_RECV, buffer }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = EAGAIN;

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_WOULD_BLOCK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_046: [ If errno is ECONNRESET, socket_transport_receive shall break out of the loop and return SOCKET_RECEIVE_SHUTDOWN. ]
TEST_FUNCTION(socket_transport_receive_recv_returns_ECONNRESET_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[] = {
        { TEST_BYTES_RECV, buffer }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = ECONNRESET;

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_SHUTDOWN, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_047: [ else socket_transport_receive shall break out of the looop and return SOCKET_RECEIVE_ERROR. ]
TEST_FUNCTION(socket_transport_receive_recv_returns_other_error_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[] = {
        { TEST_BYTES_RECV, buffer }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(-1);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = ENOBUFS;

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_048: [ If recv returns a 0 value, socket_transport_receive shall break and return SOCKET_RECEIVE_SHUTDOWN. ]
TEST_FUNCTION(socket_transport_receive_recv_returns_0_shutsdowns)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(socket_handle, TEST_HOSTNAME, TEST_PORT, TEST_CONNECTION_TIMEOUT));
    umock_c_reset_all_calls();

    uint32_t bytes_recv;
    unsigned char buffer[TEST_BYTES_RECV];
    SOCKET_BUFFER payload[] = {
        { TEST_BYTES_RECV, buffer }
    };

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(recv(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, TEST_FLAGS))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = ENOBUFS;

    //act
    SOCKET_RECEIVE_RESULT result = socket_transport_receive(socket_handle, payload, 1, &bytes_recv, TEST_FLAGS, NULL);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_SHUTDOWN, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// socket_transport_listen

// Tests_SOCKET_TRANSPORT_LINUX_11_054: [ If socket_transport is NULL, socket_transport_listen shall fail and return a non-zero value. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_055: [ If port is 0, socket_transport_listen shall fail and return a non-zero value. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_056: [ If the transport type is not SOCKET_BINDING, socket_transport_listen shall fail and return a non-zero value. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_057: [ socket_transport_listen shall call sm_open_begin to begin the open. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_059: [ socket_transport_listen shall call socket with the params AF_INET, SOCK_STREAM and IPPROTO_TCP. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_083: [ socket_transport_listen shall set the SO_REUSEADDR option on the socket. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_060: [ socket_transport_listen shall bind to the socket by calling bind. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_061: [ socket_transport_listen shall start listening to incoming connection by calling listen. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_062: [ If successful socket_transport_listen shall call sm_open_end with true. ]
TEST_FUNCTION(socket_transport_listen_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(setsockopt(IGNORED_ARG, SOL_SOCKET, SO_REUSEADDR, IGNORED_ARG, sizeof(int)));
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

// Tests_SOCKET_TRANSPORT_LINUX_11_063: [ If any failure is encountered, socket_transport_listen shall call sm_open_end with false, fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_listen_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(socket(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(setsockopt(IGNORED_ARG, SOL_SOCKET, SO_REUSEADDR, IGNORED_ARG, sizeof(int)))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(htons(TEST_PORT))
        .CallCannotFail();
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

// Tests_SOCKET_TRANSPORT_LINUX_11_058: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_listen shall fail and return a non-zero value. ]
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

// socket_transport_accept

// Tests_SOCKET_TRANSPORT_LINUX_11_069: [ If socket_transport is NULL, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_socket_transport_NULL_fail)
{
    //arrange

    //act
    SOCKET_TRANSPORT_HANDLE accept_socket_handle;
    SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(NULL, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

    //assert
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_LINUX_11_070: [ If the transport type is not SOCKET_BINDING, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_invalid_type_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

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

// Tests_SOCKET_TRANSPORT_LINUX_11_072: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_not_listening_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

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

// Tests_SOCKET_TRANSPORT_LINUX_11_084: [ If errno is EAGAIN or EWOULDBLOCK, socket_transport_accept shall return SOCKET_ACCEPT_NO_CONNECTION. ]
TEST_FUNCTION(socket_transport_accept_accept_returns_EAGAIN)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(INVALID_SOCKET);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = EAGAIN;

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

// Tests_SOCKET_TRANSPORT_LINUX_11_084: [ If errno is EAGAIN or EWOULDBLOCK, socket_transport_accept shall return SOCKET_ACCEPT_NO_CONNECTION. ]
TEST_FUNCTION(socket_transport_accept_accept_returns_EWOULDBLOCK)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(INVALID_SOCKET);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    errno = EWOULDBLOCK;

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

// Tests_SOCKET_TRANSPORT_LINUX_11_071: [ socket_transport_accept shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_073: [ socket_transport_accept shall call accept to accept the incoming socket connection. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_074: [ socket_transport_accept shall set the incoming socket to non-blocking. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_075: [ socket_transport_accept shall allocate a SOCKET_TRANSPORT for the incoming connection and call sm_create and sm_open on the connection. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_076: [ If successful socket_transport_accept shall assign accepted_socket to be the allocated incoming SOCKET_TRANSPORT and return SOCKET_ACCEPT_OK. ]
TEST_FUNCTION(socket_transport_accept_succeed)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
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

// Tests_SOCKET_TRANSPORT_LINUX_11_077: [ If any failure is encountered, socket_transport_accept shall fail and return SOCKET_ACCEPT_ERROR. ]
TEST_FUNCTION(socket_transport_accept_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(socket_handle, TEST_PORT));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(accept(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(inet_ntop(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    umock_c_negative_tests_snapshot();

    for (size_t index = 0; index < umock_c_negative_tests_call_count(); index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            //act
            SOCKET_TRANSPORT_HANDLE accept_socket_handle;
            SOCKET_ACCEPT_RESULT accept_result = socket_transport_accept(socket_handle, &accept_socket_handle, TEST_CONNECTION_TIMEOUT);

            //assert
            ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_ERROR, accept_result);
        }
    }

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// socket_transport_get_underlying_socket

// Tests_SOCKET_TRANSPORT_LINUX_11_064: [ If socket_transport is NULL, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
TEST_FUNCTION(socket_transport_get_underlying_socket_socket_transport_NULL_fail)
{
    //arrange

    //act
    SOCKET_HANDLE underying_socket = socket_transport_get_underlying_socket(NULL);

    //assert
    ASSERT_ARE_EQUAL(int, INVALID_SOCKET, underying_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
}

// Tests_SOCKET_TRANSPORT_LINUX_11_065: [ socket_transport_get_underlying_socket shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_067: [ socket_transport_get_underlying_socket shall return the SOCKET_HANDLE socket value. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_068: [ socket_transport_get_underlying_socket shall call sm_exec_end. ]
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
    ASSERT_ARE_EQUAL(int, test_socket-1, underying_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_066: [ If sm_exec_begin does not return SM_EXEC_GRANTED, socket_transport_get_underlying_socket shall fail and return INVALID_SOCKET. ]
TEST_FUNCTION(socket_transport_get_underlying_socket_not_open_fail)
{
    //arrange
    SOCKET_TRANSPORT_HANDLE socket_handle = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(socket_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    //act
    SOCKET_HANDLE underying_socket = socket_transport_get_underlying_socket(socket_handle);

    //assert
    ASSERT_ARE_EQUAL(int, INVALID_SOCKET, underying_socket);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_087: [ socket_transport_create_from_socket shall allocate a new SOCKET_TRANSPORT object. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_088: [ socket_transport_create_from_socket shall call sm_create to create a sm_object with the type set to SOCKET_CLIENT. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_096: [ socket_transport_create_from_socket shall assign the socket_handle to the new allocated socket transport. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_091: [ On success socket_transport_create_from_socket shall return SOCKET_TRANSPORT_HANDLE. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_086: [ If socket_handle is an INVALID_SOCKET, socket_transport_create_from_socket shall fail and return NULL. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_097: [ If sm_open_begin does not return SM_EXEC_GRANTED, socket_transport_create_from_socket shall fail and return NULL. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_090: [ On any failure socket_transport_create_from_socket shall return NULL. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_093: [ If socket_transport_handle is NULL, socket_transport_is_valid_socket shall fail and return false. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_095: [ On success, socket_transport_is_valid_socket shall return true. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_094: [ If the socket inside socket_transport_handle is an INVALID_SOCKET, socket_transport_is_valid_socket shall fail and return false. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_098: [ If socket_transport is NULL, socket_transport_get_local_address shall fail and return a non-zero value. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_099: [ If hostname is NULL, socket_transport_get_local_address shall fail and return a non-zero value. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_100: [ If local_address_list is not NULL and address_count is NULL, socket_transport_get_local_address shall fail and return a non-zero value. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_101: [ socket_transport_get_local_address shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_102: [ socket_transport_get_local_address shall get the hostname by calling gethostname. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_103: [ If local_address_list is not NULL, socket_transport_get_local_address shall call getifaddrs to get the link list of ifaddrs. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_104: [ socket_transport_get_local_address shall allocate the LOCAL_ADDRESS array for each ifaddrs with a sa_family of AF_INET and the interface is up and running. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_105: [ For each IP in the ifaddr object if the sa_family is AF_INET and the interface is up and running and it's not a loopback, socket_transport_get_local_address shall retrieve the name of the address by calling getnameinfo. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_106: [ socket_transport_get_local_address shall call sm_exec_end. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_107: [ On success socket_transport_get_local_address shall return 0. ]
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

    struct ifaddrs address_info_1 = {0};
    struct sockaddr ifa_addr_1 = {0};
    struct ifaddrs address_info_2;

    ifa_addr_1.sa_family = AF_INET;

    address_info_1.ifa_next = &address_info_2;
    address_info_1.ifa_name = "lo";
    address_info_1.ifa_flags = IFF_LOOPBACK | IFF_UP | IFF_RUNNING | IFF_POINTOPOINT;
    address_info_1.ifa_addr = &ifa_addr_1;

    address_info_2.ifa_next = NULL;
    address_info_2.ifa_name = "enp0s31f6";
    address_info_2.ifa_flags = IFF_UP | IFF_RUNNING | IFF_POINTOPOINT;
    address_info_2.ifa_addr = &ifa_addr_1;
    const char* actual_address = "192.168.1.21";

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(gethostname(IGNORED_ARG, MAX_GET_HOST_NAME_LEN));
    STRICT_EXPECTED_CALL(getifaddrs(IGNORED_ARG))
        .CopyOutArgumentBuffer_ifap(&address_info_1, sizeof(struct ifaddrs));
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(getnameinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, NULL, 0, IGNORED_ARG))
        .CopyOutArgumentBuffer_host(actual_address, sizeof(actual_address));

    STRICT_EXPECTED_CALL(freeifaddrs(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //act
    int result = socket_transport_get_local_address(socket_handle, hostname, &local_address_list, &address_count);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, 1, address_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
    free(local_address_list);
}

// Tests_SOCKET_TRANSPORT_LINUX_11_108: [ If any failure is encountered, socket_transport_get_local_address shall fail and return a non-zero value. ]
TEST_FUNCTION(socket_transport_get_local_address_invalid_address_list_fail)
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
    (void)ip_address_list;

    LOCAL_ADDRESS* local_address_list;
    uint32_t address_count;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(gethostname(IGNORED_ARG, MAX_GET_HOST_NAME_LEN));
    STRICT_EXPECTED_CALL(getifaddrs(IGNORED_ARG))
        .SetReturn(-1);
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

// Tests_SOCKET_TRANSPORT_LINUX_11_101: [ socket_transport_get_local_address shall call sm_exec_begin. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_102: [ socket_transport_get_local_address shall get the hostname by calling gethostname. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_106: [ socket_transport_get_local_address shall call sm_exec_end. ]
// Tests_SOCKET_TRANSPORT_LINUX_11_107: [ On success socket_transport_get_local_address shall return 0. ]
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

// Tests_SOCKET_TRANSPORT_LINUX_11_108: [ If any failure is encountered, socket_transport_get_local_address shall fail and return a non-zero value. ]
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

    struct ifaddrs address_info_1 = {0};
    struct sockaddr ifa_addr_1 = {0};
    struct ifaddrs address_info_2;

    ifa_addr_1.sa_family = AF_INET;

    address_info_1.ifa_next = &address_info_2;
    address_info_1.ifa_name = "lo";
    address_info_1.ifa_flags = IFF_LOOPBACK | IFF_UP | IFF_RUNNING | IFF_POINTOPOINT;
    address_info_1.ifa_addr = &ifa_addr_1;

    address_info_2.ifa_next = NULL;
    address_info_2.ifa_name = "enp0s31f6";
    address_info_2.ifa_flags = IFF_UP | IFF_RUNNING | IFF_POINTOPOINT;
    address_info_2.ifa_addr = &ifa_addr_1;
    const char* actual_address = "192.168.1.21";

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(gethostname(IGNORED_ARG, MAX_GET_HOST_NAME_LEN));
    STRICT_EXPECTED_CALL(getifaddrs(IGNORED_ARG))
        .CopyOutArgumentBuffer_ifap(&address_info_1, sizeof(struct ifaddrs));
    STRICT_EXPECTED_CALL(malloc_2(IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(getnameinfo(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, NULL, 0, IGNORED_ARG))
        .CopyOutArgumentBuffer_host(actual_address, sizeof(actual_address));

    STRICT_EXPECTED_CALL(freeifaddrs(IGNORED_ARG));
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
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", index);
        }
    }

    // Cleanup
    socket_transport_disconnect(socket_handle);
    socket_transport_destroy(socket_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

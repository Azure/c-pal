// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"
#include "c_pal/async_socket.h"
#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/execution_engine_win32.h"

#define TEST_PORT 4266

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)

static void on_open_complete(void* context, ASYNC_SOCKET_OPEN_RESULT open_result)
{
    HANDLE* event = (HANDLE*)context;
    (void)SetEvent(*event);
    (void)open_result;
}

static void on_send_complete(void* context, ASYNC_SOCKET_SEND_RESULT send_result)
{
    HANDLE* event = (HANDLE*)context;
    (void)SetEvent(*event);
    (void)send_result;
}

static void on_receive_complete(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    HANDLE* event = (HANDLE*)context;
    (void)SetEvent(*event);
    (void)receive_result;
    (void)bytes_received;
}

static void on_receive_complete_with_error(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    HANDLE* event = (HANDLE*)context;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_ABANDONED, receive_result);
    (void)SetEvent(*event);
    (void)receive_result;
    (void)bytes_received;
}

static void setup_sockets(SOCKET* client_socket, SOCKET* server_socket, SOCKET* listen_socket)
{
    // create a server socket
    *listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT_ARE_NOT_EQUAL(SOCKET_ASSERT_TYPE, INVALID_SOCKET, listen_socket);

    struct sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons((u_short)TEST_PORT);

    // bind it
    ASSERT_ARE_EQUAL(int, 0, bind(*listen_socket, (SOCKADDR *)&service, sizeof(service)));

    // set it to async IO
    u_long mode = 1;
    ASSERT_ARE_EQUAL(int, 0, ioctlsocket(*listen_socket, FIONBIO, &mode));

    // start listening
    ASSERT_ARE_EQUAL(int, 0, listen(*listen_socket, SOMAXCONN));

    // create a client socket
    *client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT_ARE_NOT_EQUAL(SOCKET_ASSERT_TYPE, INVALID_SOCKET, *client_socket);

    char portString[16];
    ADDRINFO addrHint = { 0 };
    ADDRINFO* addrInfo = NULL;

    addrHint.ai_family = AF_INET;
    addrHint.ai_socktype = SOCK_STREAM;
    addrHint.ai_protocol = 0;

    ASSERT_IS_TRUE(sprintf(portString, "%u", TEST_PORT) >= 0);

    ASSERT_ARE_EQUAL(int, 0, getaddrinfo("localhost", portString, &addrHint, &addrInfo));

    // connect
    ASSERT_ARE_EQUAL(int, 0, connect(*client_socket, addrInfo->ai_addr, (int)addrInfo->ai_addrlen));

    // set async
    ASSERT_ARE_EQUAL(int, 0, ioctlsocket(*listen_socket, FIONBIO, &mode));

    // accept it on the server side
    fd_set read_fds;
    int select_result;
    struct timeval timeout;

    read_fds.fd_array[0] = *listen_socket;
    read_fds.fd_count = 1;
    timeout.tv_usec = 1000 * 100;
    timeout.tv_sec = 0;
    select_result = select(0, &read_fds, NULL, NULL, &timeout);
    ASSERT_ARE_NOT_EQUAL(int, SOCKET_ERROR, select_result);

    *server_socket = accept(*listen_socket, NULL, NULL);
    ASSERT_ARE_NOT_EQUAL(SOCKET_ASSERT_TYPE, INVALID_SOCKET, *server_socket);

    ASSERT_ARE_EQUAL(int, 0, ioctlsocket(*server_socket, FIONBIO, &mode));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    WSADATA wsaData;
    ASSERT_ARE_EQUAL(int, 0, WSAStartup(MAKEWORD(2, 2), &wsaData));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    (void)WSACleanup();

    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(send_and_receive_1_byte_succeeds)
{
    // arrange
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET client_socket;
    SOCKET server_socket;
    SOCKET listen_socket;
    setup_sockets(&client_socket, &server_socket, &listen_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);

    // open
    HANDLE server_open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(server_open_event);
    HANDLE client_open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(client_open_event);

    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(server_async_socket, (SOCKET_HANDLE)server_socket, on_open_complete, &server_open_event));
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(client_async_socket, (SOCKET_HANDLE)client_socket, on_open_complete, &client_open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(server_open_event, INFINITE) == WAIT_OBJECT_0);
    ASSERT_IS_TRUE(WaitForSingleObject(client_open_event, INFINITE) == WAIT_OBJECT_0);

    uint8_t send_payload[] = { 0x42, 0x43 };
    uint8_t receive_buffer[2];
    ASYNC_SOCKET_BUFFER send_payload_buffers[1];
    send_payload_buffers[0].buffer = send_payload;
    send_payload_buffers[0].length = sizeof(send_payload);
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer;
    receive_payload_buffers[0].length = sizeof(receive_buffer);

    HANDLE send_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(send_event);
    HANDLE receive_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(receive_event);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 1, on_send_complete, &send_event));
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_complete, &receive_event));

    // assert
    ASSERT_IS_TRUE(WaitForSingleObject(send_event, INFINITE) == WAIT_OBJECT_0);
    ASSERT_IS_TRUE(WaitForSingleObject(receive_event, INFINITE) == WAIT_OBJECT_0);

    // cleanup
    (void)CloseHandle(send_event);
    (void)CloseHandle(receive_event);
    (void)CloseHandle(client_open_event);
    (void)CloseHandle(server_open_event);
    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);
    closesocket(server_socket);
    closesocket(listen_socket);
    closesocket(client_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(receive_and_send_2_buffers_succeeds)
{
    // arrange
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET client_socket;
    SOCKET server_socket;
    SOCKET listen_socket;
    setup_sockets(&client_socket, &server_socket, &listen_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);

    // open
    HANDLE server_open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(server_open_event);
    HANDLE client_open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(client_open_event);

    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(server_async_socket, (SOCKET_HANDLE)server_socket, on_open_complete, &server_open_event));
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(client_async_socket, (SOCKET_HANDLE)client_socket, on_open_complete, &client_open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(server_open_event, INFINITE) == WAIT_OBJECT_0);
    ASSERT_IS_TRUE(WaitForSingleObject(client_open_event, INFINITE) == WAIT_OBJECT_0);

    uint8_t send_payload_1[] = { 0x42, 0x43 };
    uint8_t send_payload_2[] = { 0x02 };
    uint8_t receive_buffer_1[1];
    uint8_t receive_buffer_2[2];
    ASYNC_SOCKET_BUFFER send_payload_buffers[2];
    send_payload_buffers[0].buffer = send_payload_1;
    send_payload_buffers[0].length = sizeof(send_payload_1);
    send_payload_buffers[1].buffer = send_payload_2;
    send_payload_buffers[1].length = sizeof(send_payload_2);
    ASYNC_SOCKET_BUFFER receive_payload_buffers[2];
    receive_payload_buffers[0].buffer = receive_buffer_1;
    receive_payload_buffers[0].length = sizeof(receive_buffer_1);
    receive_payload_buffers[1].buffer = receive_buffer_2;
    receive_payload_buffers[1].length = sizeof(receive_buffer_2);

    HANDLE send_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(send_event);
    HANDLE receive_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(receive_event);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 2, on_receive_complete, &receive_event));
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 2, on_send_complete, &send_event));

    // assert
    ASSERT_IS_TRUE(WaitForSingleObject(send_event, INFINITE) == WAIT_OBJECT_0);
    ASSERT_IS_TRUE(WaitForSingleObject(receive_event, INFINITE) == WAIT_OBJECT_0);

    // cleanup
    (void)CloseHandle(send_event);
    (void)CloseHandle(receive_event);
    (void)CloseHandle(client_open_event);
    (void)CloseHandle(server_open_event);
    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);
    closesocket(server_socket);
    closesocket(listen_socket);
    closesocket(client_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(when_server_socket_is_closed_receive_errors_on_client_side)
{
    // arrange
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET client_socket;
    SOCKET server_socket;
    SOCKET listen_socket;
    setup_sockets(&client_socket, &server_socket, &listen_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);

    // open
    HANDLE client_open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(client_open_event);

    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(client_async_socket, (SOCKET_HANDLE)client_socket, on_open_complete, &client_open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(client_open_event, INFINITE) == WAIT_OBJECT_0);

    uint8_t receive_buffer_1[1];
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer_1;
    receive_payload_buffers[0].length = sizeof(receive_buffer_1);

    HANDLE receive_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(receive_event);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_complete_with_error, &receive_event));

    closesocket(client_socket);

    // assert
    ASSERT_IS_TRUE(WaitForSingleObject(receive_event, INFINITE) == WAIT_OBJECT_0);

    // cleanup
    (void)CloseHandle(receive_event);
    (void)CloseHandle(client_open_event);
    async_socket_close(client_async_socket);
    closesocket(server_socket);
    closesocket(listen_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

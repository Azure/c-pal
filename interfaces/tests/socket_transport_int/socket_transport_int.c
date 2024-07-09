// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"  // IWYU pragma: keep

#include "c_logging/logger.h"

#include "c_pal/platform.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/socket_transport.h"

#ifndef WIN32
    #include <sys/socket.h>
#endif // !WIN32

#define XTEST_FUNCTION(x) void x(void)

#define TEST_PORT           4466
#define TEST_CONN_TIMEOUT   10000

static uint16_t g_port_num = TEST_PORT;

TEST_DEFINE_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, platform_init());

    // Need to see what port we can use because on linux there are 3 iteration of this
    // test: normal, valgrind, and helgrind, so we need to incrment the port number
    SOCKET_TRANSPORT_HANDLE test_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(test_socket);

    for (size_t index = 0; index < 10; index++)
    {
        int socket_result = socket_transport_listen(test_socket, g_port_num);
        if (socket_result == 0)
        {
            LogInfo("Socket_transport_listen success %" PRIu16 "", g_port_num);
            break;
        }
        g_port_num++;
        LogInfo("Socket_transport_listen failed %" PRIu16 "", g_port_num);
    }
    socket_transport_disconnect(test_socket);
    socket_transport_destroy(test_socket);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    (void)platform_deinit();

    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    g_port_num++;
}

TEST_FUNCTION(send_and_receive_2_buffer_of_2_byte_succeeds)
{
    // assert
    SOCKET_TRANSPORT_HANDLE listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    // create the async socket object
    SOCKET_TRANSPORT_HANDLE client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, 10000));

    SOCKET_TRANSPORT_HANDLE incoming_socket = socket_transport_accept(listen_socket);

    uint8_t send_payload_1[] = { 0x42, 0x43 };
    uint8_t send_payload_2[] = { 0x44, 0x45 };

    uint8_t recv_buffer_1[2];
    uint8_t recv_buffer_2[2];

    SOCKET_BUFFER send_data[2];
    send_data[0].buffer = send_payload_1;
    send_data[0].length = sizeof(send_payload_1);
    send_data[1].buffer = send_payload_2;
    send_data[1].length = sizeof(send_payload_2);

    uint32_t total_bytes_sent = send_data[0].length + send_data[1].length;

    SOCKET_BUFFER recv_data[2];
    recv_data[0].buffer = recv_buffer_1;
    recv_data[0].length = sizeof(recv_buffer_1);
    recv_data[1].buffer = recv_buffer_2;
    recv_data[1].length = sizeof(recv_buffer_2);

    uint32_t bytes_recv;
    uint32_t bytes_written;

    uint32_t flag = 0;
#ifndef WIN32
    flag = MSG_NOSIGNAL;
#endif // !WIN32

    // Send data back and forth
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_OK, socket_transport_send(client_socket, send_data, 2, &bytes_written, flag, NULL));
    ASSERT_ARE_EQUAL(int, total_bytes_sent, bytes_written);

    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_OK, socket_transport_receive(incoming_socket, recv_data, 2, &bytes_recv, 0, NULL));
    ASSERT_ARE_EQUAL(int, total_bytes_sent, bytes_recv);

    ASSERT_ARE_EQUAL(int, 0, memcmp(send_payload_1, recv_buffer_1, send_data[0].length));
    ASSERT_ARE_EQUAL(int, 0, memcmp(send_payload_2, recv_buffer_2, send_data[1].length));

    socket_transport_disconnect(incoming_socket);
    socket_transport_destroy(incoming_socket);

    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);

    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);
}

static uint32_t make_send_recv_buffer(uint8_t item_count, uint32_t data_size, SOCKET_BUFFER** outgoing_send_data, SOCKET_BUFFER** outgoing_recv_data)
{
    uint32_t result = 0;

    SOCKET_BUFFER* send_data = malloc(sizeof(SOCKET_BUFFER) * item_count);
    ASSERT_IS_NOT_NULL(send_data);

    SOCKET_BUFFER* recv_data = malloc(sizeof(SOCKET_BUFFER) * item_count);
    ASSERT_IS_NOT_NULL(recv_data);

    unsigned char send_char = 0x41;
    for (uint32_t inner = 0; inner < item_count; inner++)
    {
        send_data[inner].length = recv_data[inner].length = data_size;

        send_data[inner].buffer = malloc(sizeof(unsigned char)* data_size);
        ASSERT_IS_NOT_NULL(send_data[inner].buffer);
        recv_data[inner].buffer = malloc(sizeof(unsigned char) * data_size);
        ASSERT_IS_NOT_NULL(recv_data[inner].buffer);

        result += data_size;
        (void)memset(send_data[inner].buffer, send_char, data_size);
        (void)memset(recv_data[inner].buffer, 0, data_size);

        if (send_char == 0x7A)
        {
            send_char = 0x41;
        }
        else
        {
            send_char++;
        }
    }

    *outgoing_send_data = send_data;
    *outgoing_recv_data = recv_data;
    return result;
}

// This test does the following:
/* 1. Sets up a sending and listening socket */
/* 2. */
TEST_FUNCTION(send_and_receive_random_buffer_of_random_byte_succeeds)
{
    // assert
    SOCKET_TRANSPORT_HANDLE listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num));

    // create the async socket object
    SOCKET_TRANSPORT_HANDLE client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, 10000));

    SOCKET_TRANSPORT_HANDLE incoming_socket = socket_transport_accept(listen_socket);

    uint8_t buffer_count = 8;

    SOCKET_BUFFER* send_data;
    SOCKET_BUFFER* recv_data;

    uint32_t data_size = 64;

    uint32_t total_bytes = make_send_recv_buffer(buffer_count, data_size, &send_data, &recv_data);

    uint32_t bytes_recv;
    uint32_t bytes_sent;
    uint32_t flag = 0;

    // Send data back and forth
    ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_OK, socket_transport_send(client_socket, send_data, buffer_count, &bytes_sent, flag, NULL));
    ASSERT_ARE_EQUAL(int, total_bytes, bytes_sent);

    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_OK, socket_transport_receive(incoming_socket, recv_data, buffer_count, &bytes_recv, 0, NULL));
    ASSERT_ARE_EQUAL(int, total_bytes, bytes_recv);

    for (uint32_t inner = 0; inner < buffer_count; inner++)
    {
        ASSERT_ARE_EQUAL(int, 0, memcmp(send_data[inner].buffer, recv_data[inner].buffer, send_data[inner].length), "mismatch data from array index %" PRIu32 "", inner);

        free(send_data[inner].buffer);
        free(recv_data[inner].buffer);
    }
    free(send_data);
    free(recv_data);

    socket_transport_disconnect(incoming_socket);
    socket_transport_destroy(incoming_socket);

    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);

    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

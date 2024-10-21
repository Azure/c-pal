// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "testrunnerswitcher.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"

#include "macro_utils/macro_utils.h"  // IWYU pragma: keep

#include "c_logging/logger.h"  // IWYU pragma: keep

#include "c_pal/platform.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl.h"
#include "c_pal/socket_transport.h"
#include "c_pal/timer.h" // IWYU pragma: keep

#ifdef WIN32
#define SOCKET_SEND_FLAG     0
#else
#include <sys/socket.h>
#define SOCKET_SEND_FLAG     MSG_NOSIGNAL

#endif // !WIN32

#define XTEST_FUNCTION(x) void x(void)

#define TEST_PORT           4466
#define TEST_CONN_TIMEOUT   10000

static uint16_t g_port_num = TEST_PORT;

#define CHAOS_THREAD_COUNT 4

typedef struct CHAOS_TEST_SOCKETS_TAG
{
    SOCKET_TRANSPORT_HANDLE client_socket_handles[CHAOS_THREAD_COUNT];
    SOCKET_TRANSPORT_HANDLE incoming_socket_handles[CHAOS_THREAD_COUNT];
    SOCKET_TRANSPORT_HANDLE listen_socket;
    volatile_atomic int32_t API_create_result;
    int failed_server_num;

} CHAOS_TEST_SOCKETS;

TEST_DEFINE_ENUM_TYPE(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, platform_init());
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
}

#if WIN32
TEST_FUNCTION(connect_to_endpoint_timesout)
{
    // assert;

    // create the async socket object
    SOCKET_TRANSPORT_HANDLE client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);


    double start_time = timer_global_get_elapsed_ms();
    ASSERT_ARE_NOT_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));
    double time_elapsed = timer_global_get_elapsed_ms() - start_time;

    // within 500 ms of the timeout since the connection timeout is
    // not exact
    time_elapsed += 500;

    // Ensure that the elapsed time is greater than or equal to the connection timeout
    ASSERT_IS_TRUE(time_elapsed >= TEST_CONN_TIMEOUT, "Connection did not time out correctly: expected timeout: %" PRId32 " actual time: %f", TEST_CONN_TIMEOUT, time_elapsed);

    // cleanup

    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);

}

TEST_FUNCTION(socket_transport_accept_timesout)
{
    SOCKET_TRANSPORT_HANDLE listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    //uint16_t g_port_num_fail = -1;

    socket_transport_listen(listen_socket, (uint16_t) - 1);

    // assert

    SOCKET_TRANSPORT_HANDLE incoming_socket;
    double start_time = timer_global_get_elapsed_ms();
    ASSERT_ARE_NOT_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_OK, socket_transport_accept(listen_socket, &incoming_socket, TEST_CONN_TIMEOUT));
    double time_elapsed = timer_global_get_elapsed_ms() - start_time;

    time_elapsed += 500;

    ASSERT_IS_TRUE(time_elapsed >= TEST_CONN_TIMEOUT, "Connection did not time out correctly: expected timeout: %" PRId32 " actual time: %f", TEST_CONN_TIMEOUT, time_elapsed);

    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);

}
#endif

TEST_FUNCTION(send_and_receive_2_buffer_of_2_byte_succeeds)
{
    // assert
    SOCKET_TRANSPORT_HANDLE listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    // create the async socket object
    SOCKET_TRANSPORT_HANDLE client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));

    SOCKET_TRANSPORT_HANDLE incoming_socket;
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_OK, socket_transport_accept(listen_socket, &incoming_socket, TEST_CONN_TIMEOUT));
    ASSERT_IS_NOT_NULL(incoming_socket);

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

    uint32_t flag = SOCKET_SEND_FLAG;

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

        send_data[inner].buffer = malloc(sizeof(unsigned char) * data_size);
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

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));

    SOCKET_TRANSPORT_HANDLE incoming_socket;
    ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_OK, socket_transport_accept(listen_socket, &incoming_socket, TEST_CONN_TIMEOUT));
    ASSERT_IS_NOT_NULL(incoming_socket);

    uint8_t buffer_count = 8;

    SOCKET_BUFFER* send_data;
    SOCKET_BUFFER* recv_data;

    uint32_t data_size = 64;

    uint32_t total_bytes = make_send_recv_buffer(buffer_count, data_size, &send_data, &recv_data);

    uint32_t bytes_recv;
    uint32_t bytes_sent;
    uint32_t flag = SOCKET_SEND_FLAG;

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


static int connect_and_listen_func(void* parameter)
{
    size_t i;
    CHAOS_TEST_SOCKETS* chaos_knight_test = parameter;

    chaos_knight_test->listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(chaos_knight_test->listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(chaos_knight_test->listen_socket, g_port_num));

    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(chaos_knight_test->client_socket_handles[i], "localhost", g_port_num, TEST_CONN_TIMEOUT));

        ASSERT_ARE_EQUAL(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_OK, socket_transport_accept(chaos_knight_test->listen_socket, &chaos_knight_test->incoming_socket_handles[i], TEST_CONN_TIMEOUT));
    }

    return 0;
}

/* chaos test
* This test will do the following:
* create a bunch of client sockets to connect to binding socket every second
* create a thread to listen and connect
* send and receive random buffers of data per client
* Disconnect a random incoming socket
* Test if the disconnected socket receives the data from the client socket
*/
TEST_FUNCTION(socket_transport_chaos_knight_test)
{
    CHAOS_TEST_SOCKETS chaos_knight_test;
    chaos_knight_test.failed_server_num = rand() % CHAOS_THREAD_COUNT + 0;

    for (size_t i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        chaos_knight_test.client_socket_handles[i] = socket_transport_create_client();

        ASSERT_IS_NOT_NULL(chaos_knight_test.client_socket_handles[i]);

    }

    connect_and_listen_func(&chaos_knight_test);

    // disconnect random server on second run
    for (size_t runs = 0; runs < 2; runs++)
    {
        if (runs == 1)
        {
            socket_transport_disconnect(chaos_knight_test.incoming_socket_handles[chaos_knight_test.failed_server_num]);
        }

        for (size_t i = 0; i < CHAOS_THREAD_COUNT; i++)
        {
            uint8_t buffer_count = 8;

            SOCKET_BUFFER* send_data;
            SOCKET_BUFFER* recv_data;

            uint32_t data_size = 64;

            uint32_t total_bytes = make_send_recv_buffer(buffer_count, data_size, &send_data, &recv_data);

            uint32_t bytes_recv;
            uint32_t bytes_sent;
            uint32_t flag = SOCKET_SEND_FLAG;

            if (runs == 0) {
                ASSERT_ARE_EQUAL(SOCKET_SEND_RESULT, SOCKET_SEND_OK, socket_transport_send(chaos_knight_test.client_socket_handles[i], send_data, buffer_count, &bytes_sent, flag, NULL));
                ASSERT_ARE_EQUAL(int, total_bytes, bytes_sent);

                ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_OK, socket_transport_receive(chaos_knight_test.incoming_socket_handles[i], recv_data, buffer_count, &bytes_recv, 0, NULL));
                ASSERT_ARE_EQUAL(int, total_bytes, bytes_recv);
            }
            else
            {
                SOCKET_SEND_RESULT send_result = socket_transport_send(chaos_knight_test.client_socket_handles[i], send_data, buffer_count, &bytes_sent, flag, NULL);
                if (send_result == SOCKET_SEND_OK)
                {
                    ASSERT_ARE_EQUAL(int, total_bytes, bytes_sent);
                }

                if (i == chaos_knight_test.failed_server_num)
                {
                    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_ERROR, socket_transport_receive(chaos_knight_test.incoming_socket_handles[i], recv_data, buffer_count, &bytes_recv, 0, NULL));
                }
                else
                {
                    ASSERT_ARE_EQUAL(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_OK, socket_transport_receive(chaos_knight_test.incoming_socket_handles[i], recv_data, buffer_count, &bytes_recv, 0, NULL));
                    ASSERT_ARE_EQUAL(int, total_bytes, bytes_recv);
                }
            }

            for (uint32_t inner = 0; inner < buffer_count; inner++)
            {
                if (runs == 0)
                {
                    ASSERT_ARE_EQUAL(int, 0, memcmp(send_data[inner].buffer, recv_data[inner].buffer, send_data[inner].length), "mismatch data from array index %" PRIu32 "", inner);
                }

                free(send_data[inner].buffer);
                free(recv_data[inner].buffer);
            }
            free(send_data);
            free(recv_data);
        }

    }

    // cleanup
    for (size_t i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        socket_transport_disconnect(chaos_knight_test.client_socket_handles[i]);
        socket_transport_destroy(chaos_knight_test.client_socket_handles[i]);

        if (i != chaos_knight_test.failed_server_num)
        {
            socket_transport_disconnect(chaos_knight_test.incoming_socket_handles[i]);
        }
        socket_transport_destroy(chaos_knight_test.incoming_socket_handles[i]);
    }

    socket_transport_disconnect(chaos_knight_test.listen_socket);
    socket_transport_destroy(chaos_knight_test.listen_socket);

}

TEST_FUNCTION(get_local_socket_address_test)
{
    // assert
    SOCKET_TRANSPORT_HANDLE listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num));

    // create the async socket object
    SOCKET_TRANSPORT_HANDLE client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    char client_hostname[MAX_GET_HOST_NAME_LEN] = { 0 };
    char server_hostname[MAX_GET_HOST_NAME_LEN] = { 0 };

    LOCAL_ADDRESS* server_local_address_list;
    LOCAL_ADDRESS* client_local_address_list;
    uint32_t server_list_count;
    uint32_t client_list_count;
    ASSERT_ARE_EQUAL(int, 0, socket_transport_get_local_address(listen_socket, server_hostname, &server_local_address_list, &server_list_count));
    ASSERT_ARE_EQUAL(int, 0, socket_transport_get_local_address(client_socket, client_hostname, &client_local_address_list, &client_list_count));

    ASSERT_ARE_EQUAL(char_ptr, server_hostname, client_hostname);
    ASSERT_ARE_EQUAL(uint32_t, server_list_count, client_list_count);

    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);

    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);

    free(server_local_address_list);
    free(client_local_address_list);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

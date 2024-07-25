// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "c_pal/async_socket.h"
#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/socket_handle.h"
#include "c_pal/platform.h"
#include "c_pal/socket_transport.h"

#define SOCKET_SEND_FLAG MSG_NOSIGNAL
#define TEST_PORT           4466

#define TEST_CONN_TIMEOUT   10000

static int g_port_num = TEST_PORT;

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)

static void on_open_complete(void* context, ASYNC_SOCKET_OPEN_RESULT open_result)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)context;

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_OK, open_result);
    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static void on_send_complete(void* context, ASYNC_SOCKET_SEND_RESULT send_result)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)context;

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_OK, send_result);
    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static void on_receive_complete(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)context;

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_OK, receive_result);
    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static void on_receive_abandoned_complete(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_ABANDONED, receive_result);
}

static void on_receive_and_accumulate_complete(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    volatile_atomic int32_t* accumulator = (volatile_atomic int32_t*)context;

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_OK, receive_result);
    (void)interlocked_add(accumulator, bytes_received);
    wake_by_address_single(accumulator);
}

static void on_receive_complete_with_error(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    volatile_atomic int32_t* thread_counter = (volatile_atomic int32_t*)context;

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_ABANDONED, receive_result);
    (void)interlocked_increment(thread_counter);
    wake_by_address_single(thread_counter);
}

static void set_nonblocking(SOCKET_HANDLE socket)
{
    int opts = fcntl(socket, F_GETFL);
    ASSERT_IS_TRUE(opts >= 0, "Failure getting socket option");

    opts = fcntl(socket, F_SETFL, opts|O_NONBLOCK);
    ASSERT_IS_TRUE(opts >= 0, "Failure setting socket option");
}

static void wait_for_value(volatile_atomic int32_t* counter, int32_t target_value)
{
    int32_t value;
    while ((value = interlocked_add(counter, 0)) != target_value)
    {
        (void)wait_on_address(counter, value, UINT32_MAX);
    }
}

static void open_async_handle(ASYNC_SOCKET_HANDLE handle, SOCKET_TRANSPORT_HANDLE socket)
{
    volatile_atomic int32_t counter = 0;

    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(handle, socket, on_open_complete, (void*)&counter), "Failure opening async socket");

    wait_for_value(&counter, 1);
}

static void dump_bytes(const char* msg, uint8_t data_payload[], uint32_t length)
{
    printf("%s %" PRIu32 " data: ", msg, length);
    for (uint32_t index = 0; index < length; index++)
    {
        printf("0x%" PRIu8 " ", data_payload[index]);
    }
    printf("\n");
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, platform_init());
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    platform_deinit();

    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    g_port_num++;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(connect_no_send_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_TRANSPORT_HANDLE client_socket;
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;

    listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    // create the async socket object
    client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));

    server_socket = socket_transport_accept(listen_socket);
    ASSERT_IS_NOT_NULL(server_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // wait for open to complete
    open_async_handle(server_async_socket, server_socket);
    open_async_handle(client_async_socket, client_socket);

    uint8_t receive_buffer[2];
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer;
    receive_payload_buffers[0].length = sizeof(receive_buffer);

    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_abandoned_complete, NULL));
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(server_async_socket, receive_payload_buffers, 1, on_receive_abandoned_complete, NULL));

    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);
    socket_transport_disconnect(server_socket);
    socket_transport_destroy(server_socket);
    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);
    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(send_and_receive_1_byte_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_TRANSPORT_HANDLE client_socket;
    // server socket is same as accept socket
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;

    listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    // create the async socket object
    client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));

    server_socket = socket_transport_accept(listen_socket);
    ASSERT_IS_NOT_NULL(server_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(client_async_socket);
    
    // wait for open to complete
    open_async_handle(server_async_socket, server_socket);
    open_async_handle(client_async_socket, client_socket);

    uint8_t data_payload[] = { 0x42, 0x43 };
    uint8_t receive_buffer[2];
    ASYNC_SOCKET_BUFFER send_payload_buffers[1];
    send_payload_buffers[0].buffer = data_payload;
    send_payload_buffers[0].length = sizeof(data_payload);
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer;
    receive_payload_buffers[0].length = sizeof(receive_buffer);

    volatile_atomic int32_t send_counter;
    volatile_atomic int32_t recv_counter;

    interlocked_exchange(&send_counter, 0);
    interlocked_exchange(&recv_counter, 0);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 1, on_send_complete, (void*)&send_counter), "Failure sending socket");
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_complete, (void*)&recv_counter));

    // assert
    wait_for_value(&send_counter, 1);
    wait_for_value(&recv_counter, 1);

    ASSERT_ARE_EQUAL(int, 0, memcmp(receive_payload_buffers[0].buffer, send_payload_buffers[0].buffer, sizeof(data_payload)));

    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);
    socket_transport_disconnect(server_socket);
    socket_transport_destroy(server_socket);
    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);
    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(receive_and_send_2_buffers_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_TRANSPORT_HANDLE client_socket;
    // server socket is same as accept socket
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;

    listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    // create the async socket object
    client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));

    server_socket = socket_transport_accept(listen_socket);
    ASSERT_IS_NOT_NULL(server_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // wait for open to complete
    open_async_handle(server_async_socket, server_socket);
    open_async_handle(client_async_socket, client_socket);

    uint8_t data_payload_1[] = { 0x42, 0x43 };
    uint8_t data_payload_2[] = { 0x02 };
    uint8_t receive_buffer_1[1];
    uint8_t receive_buffer_2[2];
    ASYNC_SOCKET_BUFFER send_payload_buffers[2];
    send_payload_buffers[0].buffer = data_payload_1;
    send_payload_buffers[0].length = sizeof(data_payload_1);
    send_payload_buffers[1].buffer = data_payload_2;
    send_payload_buffers[1].length = sizeof(data_payload_2);
    ASYNC_SOCKET_BUFFER receive_payload_buffers[2];
    receive_payload_buffers[0].buffer = receive_buffer_1;
    receive_payload_buffers[0].length = sizeof(receive_buffer_1);
    receive_payload_buffers[1].buffer = receive_buffer_2;
    receive_payload_buffers[1].length = sizeof(receive_buffer_2);

    volatile_atomic int32_t send_counter;
    volatile_atomic int32_t recv_counter;

    interlocked_exchange(&send_counter, 0);
    interlocked_exchange(&recv_counter, 0);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 2, on_send_complete, (void*)&send_counter));
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 2, on_receive_complete, (void*)&recv_counter));

    // assert
    wait_for_value(&send_counter, 1);
    wait_for_value(&recv_counter, 1);

    // cleanup
    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);
    socket_transport_disconnect(server_socket);
    socket_transport_destroy(server_socket);
    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);
    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(when_server_socket_is_closed_receive_errors_on_client_side)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_TRANSPORT_HANDLE client_socket;
    // server socket is same as accept socket
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;

    listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    // create the async socket object
    client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));

    server_socket = socket_transport_accept(listen_socket);
    ASSERT_IS_NOT_NULL(server_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // open and wait for open to complete
    open_async_handle(client_async_socket, client_socket);

    uint8_t receive_buffer_1[1];
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer_1;
    receive_payload_buffers[0].length = sizeof(receive_buffer_1);

    volatile_atomic int32_t recv_counter;
    interlocked_exchange(&recv_counter, 0);

    //close(accept_socket);
    socket_transport_disconnect(server_socket);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_complete_with_error, (void*)&recv_counter));

    // assert
    wait_for_value(&recv_counter, 1);

    // cleanup
    async_socket_close(client_async_socket);
    socket_transport_destroy(server_socket);
    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);
    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(multiple_sends_and_receives_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_TRANSPORT_HANDLE client_socket;
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;

    listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    // create the async socket object
    client_socket = socket_transport_create_client();
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket, "localhost", g_port_num, TEST_CONN_TIMEOUT));

    server_socket = socket_transport_accept(listen_socket);
    ASSERT_IS_NOT_NULL(server_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // wait for open to complete
    open_async_handle(server_async_socket, server_socket);
    open_async_handle(client_async_socket, client_socket);

    uint8_t data_payload[] = { 0x42, 0x43, 0x44, 0x45 };
    uint8_t receive_buffer[4];
    ASYNC_SOCKET_BUFFER send_payload_buffers[1];
    send_payload_buffers[0].buffer = data_payload;
    send_payload_buffers[0].length = sizeof(data_payload);
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer;
    receive_payload_buffers[0].length = sizeof(receive_buffer);

    int32_t expected_recv_size = sizeof(data_payload)*3;

    volatile_atomic int32_t send_counter;
    volatile_atomic int32_t recv_counter;
    interlocked_exchange(&send_counter, 0);
    interlocked_exchange(&recv_counter, 0);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 1, on_send_complete, (void*)&send_counter), "Failure sending socket 1");
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_and_accumulate_complete, (void*)&recv_counter));
    wait_for_value(&recv_counter, sizeof(data_payload));

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 1, on_send_complete, (void*)&send_counter), "Failure sending socket 2");

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 1, on_send_complete, (void*)&send_counter), "Failure sending socket 3");
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_and_accumulate_complete, (void*)&recv_counter));
    wait_for_value(&recv_counter, sizeof(data_payload)*2);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_and_accumulate_complete, (void*)&recv_counter));

    // assert
    wait_for_value(&send_counter, 3);
    wait_for_value(&recv_counter, expected_recv_size);

    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);
    socket_transport_disconnect(server_socket);
    socket_transport_destroy(server_socket);
    socket_transport_disconnect(client_socket);
    socket_transport_destroy(client_socket);
    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

#define N_WORK_ITEMS 100

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _sockets_items))
{
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_TRANSPORT_HANDLE listen_socket;
    listen_socket = socket_transport_create_server();
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(listen_socket, g_port_num), "failed listening on port %" PRIu16 "", g_port_num);

    ASYNC_SOCKET_HANDLE server_async_socket[N_WORK_ITEMS];
    ASYNC_SOCKET_HANDLE client_async_socket[N_WORK_ITEMS];

    SOCKET_TRANSPORT_HANDLE client_socket[N_WORK_ITEMS];
    SOCKET_TRANSPORT_HANDLE server_socket[N_WORK_ITEMS];
    
    uint32_t socket_count = N_WORK_ITEMS;

    for (uint32_t index = 0; index < socket_count; index++)
    {

        client_socket[index] = socket_transport_create_client();
        ASSERT_IS_NOT_NULL(client_socket[index]);

        ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(client_socket[index], "localhost", g_port_num, TEST_CONN_TIMEOUT));

        server_socket[index] = socket_transport_accept(listen_socket);
        ASSERT_IS_NOT_NULL(server_socket[index]);

        // create the async socket object
        ASSERT_IS_NOT_NULL(server_async_socket[index] = async_socket_create(execution_engine));
        ASSERT_IS_NOT_NULL(client_async_socket[index] = async_socket_create(execution_engine));

        // wait for open to complete
        open_async_handle(server_async_socket[index], server_socket[index]);
        open_async_handle(client_async_socket[index], client_socket[index]);

    }

    uint8_t data_payload[] = { 0x42, 0x43, 0x44, 0x45 };
    uint8_t receive_buffer[8];
    ASYNC_SOCKET_BUFFER send_payload_buffers[1];
    send_payload_buffers[0].buffer = data_payload;
    send_payload_buffers[0].length = sizeof(data_payload);
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer;
    receive_payload_buffers[0].length = sizeof(receive_buffer);

    uint32_t expected_recv_size = 0;

    volatile_atomic int32_t send_counter;
    volatile_atomic int32_t recv_size;
    interlocked_exchange(&send_counter, 0);
    interlocked_exchange(&recv_size, 0);

    for (uint32_t index = 0; index < socket_count; index++)
    {
        // act (send bytes and receive it)
        ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket[index], send_payload_buffers, 1, on_send_complete, (void*)&send_counter), "Failure sending socket 1");
        expected_recv_size += send_payload_buffers[0].length;
        ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket[index], receive_payload_buffers, 1, on_receive_and_accumulate_complete, (void*)&recv_size));
    }

    // assert
    wait_for_value(&send_counter, socket_count);
    wait_for_value(&recv_size, expected_recv_size);

    socket_transport_disconnect(listen_socket);
    socket_transport_destroy(listen_socket);

    for (uint32_t index = 0; index < socket_count; index++)
    {
        async_socket_close(server_async_socket[index]);
        async_socket_close(client_async_socket[index]);
        async_socket_destroy(server_async_socket[index]);
        async_socket_destroy(client_async_socket[index]);
        socket_transport_disconnect(client_socket[index]);
        socket_transport_destroy(client_socket[index]);
        socket_transport_disconnect(server_socket[index]);
        socket_transport_destroy(server_socket[index]);
    }
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

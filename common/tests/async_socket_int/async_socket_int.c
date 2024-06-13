// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/async_socket.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/execution_engine_win32.h"
#include "c_pal/platform.h"

#define TEST_PORT 4266

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)

static void on_open_complete(void* context, ASYNC_SOCKET_OPEN_RESULT open_result)
{
    int32_t* interlocked_val = (int32_t*)context;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_OK, open_result);
    (void)InterlockedHL_SetAndWake(interlocked_val, 1);
}

static void on_send_complete(void* context, ASYNC_SOCKET_SEND_RESULT send_result)
{
    int32_t* interlocked_val = (int32_t*)context;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_OK, send_result);
    (void)InterlockedHL_SetAndWake(interlocked_val, 1);
}

static void on_receive_complete(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    int32_t* interlocked_val = (int32_t*)context;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_OK, receive_result);
    (void)InterlockedHL_SetAndWake(interlocked_val, 1);
    (void)bytes_received;
}

static void on_receive_complete_with_error(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    int32_t* interlocked_val = (int32_t*)context;
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_ABANDONED, receive_result);
    (void)InterlockedHL_SetAndWake(interlocked_val, 1);
    (void)bytes_received;
}

static void setup_sockets(SOCKET_TRANSPORT_HANDLE* client_socket, SOCKET_TRANSPORT_HANDLE* server_socket, SOCKET_TRANSPORT_HANDLE* listen_socket)
{
    // create a server socket
    *listen_socket = socket_transport_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(*listen_socket);
    ASSERT_ARE_EQUAL(int, 0, socket_transport_listen(*listen_socket, TEST_PORT));

    // create a client socket
    *client_socket = socket_transport_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(*client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_transport_connect(*client_socket, "localhost", TEST_PORT, 1000));

    // accept it on the server side
    *server_socket = socket_transport_accept(*listen_socket);
    ASSERT_IS_NOT_NULL(*server_socket);
}

static void open_communication(EXECUTION_ENGINE_HANDLE execution_engine, ASYNC_SOCKET_HANDLE* client_async_socket, ASYNC_SOCKET_HANDLE* server_async_socket, SOCKET_TRANSPORT_HANDLE client_socket, SOCKET_TRANSPORT_HANDLE server_socket)
{
    // create the async socket object
    *server_async_socket = async_socket_create(execution_engine);
    *client_async_socket = async_socket_create(execution_engine);

    // open
    volatile_atomic int32_t server_open;
    (void)interlocked_exchange(&server_open, 0);
    volatile_atomic int32_t client_open;
    (void)interlocked_exchange(&client_open, 0);

    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(*server_async_socket, server_socket, on_open_complete, (void*)&server_open));
    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(*client_async_socket, client_socket, on_open_complete, (void*)&client_open));

    // wait for open to complete
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&server_open, 1, UINT32_MAX));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&client_open, 1, UINT32_MAX));
}

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

TEST_FUNCTION(send_and_receive_1_byte_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_TRANSPORT_HANDLE client_socket;
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;
    setup_sockets(&client_socket, &server_socket, &listen_socket);

    ASYNC_SOCKET_HANDLE server_async_socket;
    ASYNC_SOCKET_HANDLE client_async_socket;
    open_communication(execution_engine, &client_async_socket, &server_async_socket, client_socket, server_socket);

    uint8_t send_payload[] = { 0x42, 0x43 };
    uint8_t receive_buffer[2];
    ASYNC_SOCKET_BUFFER send_payload_buffers[1];
    send_payload_buffers[0].buffer = send_payload;
    send_payload_buffers[0].length = sizeof(send_payload);
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer;
    receive_payload_buffers[0].length = sizeof(receive_buffer);

    volatile_atomic int32_t send_complete;
    (void)interlocked_exchange(&send_complete, 0);
    volatile_atomic int32_t recv_complete;
    (void)interlocked_exchange(&recv_complete, 0);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 1, on_send_complete, (void*)&send_complete));
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_complete, (void*)&recv_complete));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&send_complete, 1, UINT32_MAX));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&recv_complete, 1, UINT32_MAX));

    // cleanup
    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);

    socket_transport_disconnect(listen_socket);

    socket_transport_destroy(server_socket);
    socket_transport_destroy(listen_socket);
    socket_transport_destroy(client_socket);

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
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;
    setup_sockets(&client_socket, &server_socket, &listen_socket);

    ASYNC_SOCKET_HANDLE server_async_socket;
    ASYNC_SOCKET_HANDLE client_async_socket;
    open_communication(execution_engine, &client_async_socket, &server_async_socket, client_socket, server_socket);

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

    volatile_atomic int32_t send_complete;
    (void)interlocked_exchange(&send_complete, 0);
    volatile_atomic int32_t recv_complete;
    (void)interlocked_exchange(&recv_complete, 0);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 2, on_receive_complete, (void*)&recv_complete));
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(server_async_socket, send_payload_buffers, 2, on_send_complete, (void*)&send_complete));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&send_complete, 1, UINT32_MAX));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&recv_complete, 1, UINT32_MAX));

    // cleanup
    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);

    socket_transport_disconnect(listen_socket);

    socket_transport_destroy(server_socket);
    socket_transport_destroy(listen_socket);
    socket_transport_destroy(client_socket);

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
    SOCKET_TRANSPORT_HANDLE server_socket;
    SOCKET_TRANSPORT_HANDLE listen_socket;
    setup_sockets(&client_socket, &server_socket, &listen_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);

    // open
    volatile_atomic int32_t client_open;
    (void)interlocked_exchange(&client_open, 0);

    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(client_async_socket, client_socket, on_open_complete, (void*)&client_open));

    // wait for open to complete
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&client_open, 1, UINT32_MAX));

    uint8_t receive_buffer_1[1];
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer_1;
    receive_payload_buffers[0].length = sizeof(receive_buffer_1);

    volatile_atomic int32_t recv_complete;
    (void)interlocked_exchange(&recv_complete, 0);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_complete_with_error, (void*)&recv_complete));

    socket_transport_disconnect(client_socket);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&recv_complete, 1, UINT32_MAX));

    // cleanup
    async_socket_close(client_async_socket);

    socket_transport_disconnect(server_socket);
    socket_transport_disconnect(listen_socket);

    socket_transport_destroy(server_socket);
    socket_transport_destroy(listen_socket);
    socket_transport_destroy(client_socket);

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

    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE accept_socket;
    SOCKET_HANDLE listen_socket;

    setup_server_socket(&listen_socket);
    setup_test_socket(g_port_num, &client_socket, &listen_socket, &accept_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // wait for open to complete
    open_async_handle(server_async_socket, accept_socket);
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
    close(listen_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

#define N_WORK_ITEMS 1000

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _sockets_items))
{
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE listen_socket;
    ASYNC_SOCKET_HANDLE server_async_socket[N_WORK_ITEMS];
    ASYNC_SOCKET_HANDLE client_async_socket[N_WORK_ITEMS];

    setup_server_socket(&listen_socket);

    uint32_t socket_count = N_WORK_ITEMS;

    for (uint32_t index = 0; index < socket_count; index++)
    {
        SOCKET_HANDLE accept_socket;
        SOCKET_HANDLE client_socket;
        setup_test_socket(g_port_num, &client_socket, &listen_socket, &accept_socket);

        // create the async socket object
        ASSERT_IS_NOT_NULL(server_async_socket[index] = async_socket_create(execution_engine));
        ASSERT_IS_NOT_NULL(client_async_socket[index] = async_socket_create(execution_engine));

        // wait for open to complete
        open_async_handle(server_async_socket[index], accept_socket);
        open_async_handle(client_async_socket[index], client_socket);
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

    close(listen_socket);
    for (uint32_t index = 0; index < socket_count; index++)
    {
        async_socket_close(server_async_socket[index]);
        async_socket_close(client_async_socket[index]);
        async_socket_destroy(server_async_socket[index]);
        async_socket_destroy(client_async_socket[index]);
    }
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

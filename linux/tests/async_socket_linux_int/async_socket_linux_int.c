// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "c_pal/async_socket.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/socket_handle.h"
#include "c_pal/platform.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

#define TEST_PORT 2244
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

static void setup_server_socket(SOCKET_HANDLE* listen_socket)
{
    // create a listening socket
    *listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_ARE_NOT_EQUAL(int, INVALID_SOCKET, *listen_socket);

    const int enable = 1;
    ASSERT_ARE_EQUAL(int, 0, setsockopt(*listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)));

    int bind_res;
    uint32_t counter = 0;
    // The gate machines fails on bind due to 3 different
    // Process running, so we have to figure out the proper
    // address to bind to by going through ports looking for an open one
    do
    {
        struct sockaddr_in service;

        service.sin_family = AF_INET;
        service.sin_port = htons(g_port_num);
        service.sin_addr.s_addr = htonl(INADDR_ANY);

        bind_res = bind(*listen_socket, (struct sockaddr*)&service, sizeof(service));
        if (bind_res != 0)
        {
            g_port_num++;
        }
        counter++;
    } while (bind_res != 0 && counter < 10);
    ASSERT_ARE_EQUAL(int, 0, bind_res, "Failure attempting to bind (%d) to socket %d error (%d): %s", bind_res, g_port_num, errno, strerror(errno));

    // set it to async IO
    set_nonblocking(*listen_socket);

    // start listening
    ASSERT_ARE_EQUAL(int, 0, listen(*listen_socket, SOMAXCONN), "Failure on listen socket");
}

static void setup_test_socket(int port_num, SOCKET_HANDLE* client_socket, SOCKET_HANDLE* listen_socket, SOCKET_HANDLE* accept_socket)
{
    // create a client socket
    *client_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_ARE_NOT_EQUAL(int, INVALID_SOCKET, *client_socket);

    struct sockaddr_in  serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(port_num);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_ARE_NOT_EQUAL(int, INVALID_SOCKET, connect(*client_socket, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr)), "Unable to connect client");

    // set it to async IO
    set_nonblocking(*client_socket);

    do
    {
        *accept_socket = accept(*listen_socket, NULL, NULL);
    } while (*accept_socket == INVALID_SOCKET && errno == 11);
    ASSERT_ARE_NOT_EQUAL(int, INVALID_SOCKET, *accept_socket, "Failure accepting socket.  Error No: %s", strerror(errno));

    set_nonblocking(*accept_socket);
}

static void wait_for_value(volatile_atomic int32_t* counter, int32_t target_value)
{
    int32_t value;
    while ((value = interlocked_add(counter, 0)) != target_value)
    {
        (void)wait_on_address(counter, value, UINT32_MAX);
    }
}

static void open_async_handle(ASYNC_SOCKET_HANDLE handle)
{
    volatile_atomic int32_t counter = 0;

    ASSERT_ARE_EQUAL(int, 0, async_socket_open_async(handle, on_open_complete, (void*)&counter), "Failure opening async socket");

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

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);

    platform_deinit();

    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
    g_port_num++;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

TEST_FUNCTION(send_and_receive_1_byte_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS_LINUX execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE accept_socket;
    SOCKET_HANDLE listen_socket;

    setup_server_socket(&listen_socket);
    setup_test_socket(g_port_num, &client_socket, &listen_socket, &accept_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine, accept_socket);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine, client_socket);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // wait for open to complete
    open_async_handle(server_async_socket);
    open_async_handle(client_async_socket);

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
    close(listen_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(receive_and_send_2_buffers_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS_LINUX execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE accept_socket;
    SOCKET_HANDLE listen_socket;

    setup_server_socket(&listen_socket);
    setup_test_socket(g_port_num, &client_socket, &listen_socket, &accept_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine, accept_socket);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine, client_socket);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // wait for open to complete
    open_async_handle(server_async_socket);
    open_async_handle(client_async_socket);

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
    close(listen_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(when_server_socket_is_closed_receive_errors_on_client_side)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS_LINUX execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE accept_socket;
    SOCKET_HANDLE listen_socket;

    setup_server_socket(&listen_socket);
    setup_test_socket(g_port_num, &client_socket, &listen_socket, &accept_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine, client_socket);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // open and wait for open to complete
    open_async_handle(client_async_socket);

    uint8_t receive_buffer_1[1];
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer_1;
    receive_payload_buffers[0].length = sizeof(receive_buffer_1);

    volatile_atomic int32_t recv_counter;
    interlocked_exchange(&recv_counter, 0);

    close(accept_socket);

    // act (send one byte and receive it)
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_complete_with_error, (void*)&recv_counter));

    // assert
    wait_for_value(&recv_counter, 1);

    // cleanup
    async_socket_close(client_async_socket);
    close(listen_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(multiple_sends_and_receives_succeeds)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_PARAMETERS_LINUX execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE accept_socket;
    SOCKET_HANDLE listen_socket;

    setup_server_socket(&listen_socket);
    setup_test_socket(g_port_num, &client_socket, &listen_socket, &accept_socket);

    // create the async socket object
    ASYNC_SOCKET_HANDLE server_async_socket = async_socket_create(execution_engine, accept_socket);
    ASSERT_IS_NOT_NULL(server_async_socket);
    ASYNC_SOCKET_HANDLE client_async_socket = async_socket_create(execution_engine, client_socket);
    ASSERT_IS_NOT_NULL(client_async_socket);

    // wait for open to complete
    open_async_handle(server_async_socket);
    open_async_handle(client_async_socket);

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
    EXECUTION_ENGINE_PARAMETERS_LINUX execution_engine_parameters = { 4, 0 };
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
        ASSERT_IS_NOT_NULL(server_async_socket[index] = async_socket_create(execution_engine, accept_socket));
        ASSERT_IS_NOT_NULL(client_async_socket[index] = async_socket_create(execution_engine, client_socket));

        // wait for open to complete
        open_async_handle(server_async_socket[index]);
        open_async_handle(client_async_socket[index]);
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

    for (uint32_t index = 0; index < socket_count; index++)
    {
        async_socket_close(server_async_socket[index]);
        async_socket_close(client_async_socket[index]);
        async_socket_destroy(server_async_socket[index]);
        async_socket_destroy(client_async_socket[index]);
    }
    close(listen_socket);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

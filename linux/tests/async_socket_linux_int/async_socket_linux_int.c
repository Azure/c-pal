// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "c_pal/async_socket.h"
#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/socket_handle.h"
#include "c_pal/platform.h"
#include "c_pal/threadapi.h"

#define TEST_SMALL_SOCKET_BUFFER_SIZE   4096
#define TEST_FILL_CHUNK_SIZE            4096
#define TEST_FILL_SETTLE_TIME_MS        100
#define TEST_FILL_BYTE                  0xAA
#define TEST_PENDING_SEND_SIZE          64
#define TEST_PEER_PAYLOAD_SIZE          8
#define TEST_RECEIVE_BUFFER_SIZE        64
#define TEST_IO_COMPLETE_TIMEOUT_MS     5000

static int g_port_num;

TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

typedef struct SEND_COMPLETE_CONTEXT_TAG
{
    volatile_atomic int32_t complete_count;
    volatile_atomic int32_t send_result;
} SEND_COMPLETE_CONTEXT;

typedef struct RECEIVE_COMPLETE_CONTEXT_TAG
{
    volatile_atomic int32_t complete_count;
    volatile_atomic int32_t receive_result;
    volatile_atomic int32_t bytes_received;
} RECEIVE_COMPLETE_CONTEXT;

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

static void on_send_complete_record_result(void* context, ASYNC_SOCKET_SEND_RESULT send_result)
{
    SEND_COMPLETE_CONTEXT* send_complete_context = context;

    (void)interlocked_exchange(&send_complete_context->send_result, send_result);
    (void)interlocked_increment(&send_complete_context->complete_count);
    wake_by_address_single(&send_complete_context->complete_count);
}

static void on_receive_complete_record_result(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received)
{
    RECEIVE_COMPLETE_CONTEXT* receive_complete_context = context;

    (void)interlocked_exchange(&receive_complete_context->receive_result, receive_result);
    (void)interlocked_exchange(&receive_complete_context->bytes_received, (int32_t)bytes_received);
    (void)interlocked_increment(&receive_complete_context->complete_count);
    wake_by_address_single(&receive_complete_context->complete_count);
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

    // Bind to port 0 - let the kernel choose an available port
    struct sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(0);
    service.sin_addr.s_addr = htonl(INADDR_ANY);

    ASSERT_ARE_EQUAL(int, 0, bind(*listen_socket, (struct sockaddr*)&service, sizeof(service)), "Failure attempting to bind error (%d): %s", errno, strerror(errno));

    // Get the actual port assigned by the kernel
    struct sockaddr_in actual_addr;
    socklen_t addr_len = sizeof(actual_addr);
    ASSERT_ARE_EQUAL(int, 0, getsockname(*listen_socket, (struct sockaddr*)&actual_addr, &addr_len), "Failure getting socket name error (%d): %s", errno, strerror(errno));
    g_port_num = ntohs(actual_addr.sin_port);

    // set it to async IO
    set_nonblocking(*listen_socket);

    // start listening
    ASSERT_ARE_EQUAL(int, 0, listen(*listen_socket, SOMAXCONN), "Failure on listen socket error (%d): port: %d", errno, g_port_num);
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

static void open_async_handle(ASYNC_SOCKET_HANDLE handle, SOCKET_HANDLE socket)
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

static void set_socket_buffer_size(SOCKET_HANDLE socket, int option_name, int buffer_size)
{
    ASSERT_ARE_EQUAL(int, 0, setsockopt(socket, SOL_SOCKET, option_name, &buffer_size, sizeof(buffer_size)), "Failure setting socket buffer size error (%d): %s", errno, strerror(errno));
}

static uint32_t send_until_eagain(SOCKET_HANDLE socket, size_t chunk_size)
{
    uint8_t chunk[TEST_FILL_CHUNK_SIZE];
    uint32_t total_bytes_sent = 0;
    ssize_t bytes_sent;

    (void)memset(chunk, TEST_FILL_BYTE, sizeof(chunk));
    while ((bytes_sent = send(socket, chunk, chunk_size, MSG_NOSIGNAL)) > 0)
    {
        total_bytes_sent += (uint32_t)bytes_sent;
    }
    ASSERT_IS_TRUE((errno == EAGAIN) || (errno == EWOULDBLOCK), "Unexpected send error (%d): %s", errno, strerror(errno));

    return total_bytes_sent;
}

// Writes to socket while the peer does not read, until the kernel refuses even a single byte (EAGAIN).
// Ending with 1 byte sends guarantees that any later send on socket also gets EAGAIN until the peer reads.
static uint32_t fill_socket_send_path(SOCKET_HANDLE socket)
{
    uint32_t total_bytes_sent = 0;
    uint32_t bytes_sent;

    do
    {
        bytes_sent = send_until_eagain(socket, TEST_FILL_CHUNK_SIZE);
        bytes_sent += send_until_eagain(socket, 1);
        total_bytes_sent += bytes_sent;

        // let in-flight segments be acknowledged, then check that no more space was freed
        ThreadAPI_Sleep(TEST_FILL_SETTLE_TIME_MS);
    } while (bytes_sent > 0);

    return total_bytes_sent;
}

static uint32_t receive_with_timeout(SOCKET_HANDLE socket, uint8_t* buffer, uint32_t length)
{
    uint32_t total_bytes_received = 0;

    while (total_bytes_received < length)
    {
        struct pollfd poll_fd;
        poll_fd.fd = socket;
        poll_fd.events = POLLIN;
        poll_fd.revents = 0;

        if (poll(&poll_fd, 1, TEST_IO_COMPLETE_TIMEOUT_MS) <= 0)
        {
            break;
        }
        else
        {
            ssize_t bytes_received = recv(socket, buffer + total_bytes_received, length - total_bytes_received, 0);
            if (bytes_received > 0)
            {
                total_bytes_received += (uint32_t)bytes_received;
            }
            else if ((bytes_received < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
            {
                /* do nothing, poll again */
            }
            else
            {
                break;
            }
        }
    }

    return total_bytes_received;
}

// Reads and discards byte_count bytes, waiting at most TEST_IO_COMPLETE_TIMEOUT_MS for each chunk
static uint32_t discard_with_timeout(SOCKET_HANDLE socket, uint32_t byte_count)
{
    uint8_t chunk[TEST_FILL_CHUNK_SIZE];
    uint32_t total_bytes_discarded = 0;

    while (total_bytes_discarded < byte_count)
    {
        uint32_t remaining_bytes = byte_count - total_bytes_discarded;
        uint32_t bytes_to_read = (remaining_bytes < sizeof(chunk)) ? remaining_bytes : (uint32_t)sizeof(chunk);
        uint32_t bytes_read = receive_with_timeout(socket, chunk, bytes_to_read);
        total_bytes_discarded += bytes_read;
        if (bytes_read < bytes_to_read)
        {
            break;
        }
        else
        {
            /* do nothing, read the next chunk */
        }
    }

    return total_bytes_discarded;
}

static void create_connected_client_async_socket_with_small_buffers(EXECUTION_ENGINE_HANDLE execution_engine, SOCKET_HANDLE* listen_socket, SOCKET_HANDLE* client_socket, SOCKET_HANDLE* peer_socket, ASYNC_SOCKET_HANDLE* client_async_socket)
{
    setup_server_socket(listen_socket);
    setup_test_socket(g_port_num, client_socket, listen_socket, peer_socket);

    // small buffers so that a send hits EAGAIN after a few KB while the peer is not reading
    set_socket_buffer_size(*client_socket, SO_SNDBUF, TEST_SMALL_SOCKET_BUFFER_SIZE);
    set_socket_buffer_size(*peer_socket, SO_RCVBUF, TEST_SMALL_SOCKET_BUFFER_SIZE);

    *client_async_socket = async_socket_create(execution_engine);
    ASSERT_IS_NOT_NULL(*client_async_socket);
    open_async_handle(*client_async_socket, *client_socket);
}

static uint32_t start_send_that_hits_eagain(ASYNC_SOCKET_HANDLE client_async_socket, SOCKET_HANDLE client_socket, ASYNC_SOCKET_BUFFER* send_payload_buffer, SEND_COMPLETE_CONTEXT* send_complete_context)
{
    uint32_t fill_byte_count = fill_socket_send_path(client_socket);
    ASSERT_IS_TRUE(fill_byte_count > 0, "could not fill the socket send path");

    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_OK, async_socket_send_async(client_async_socket, send_payload_buffer, 1, on_send_complete_record_result, send_complete_context));

    // a synchronous completion would mean the send did not hit EAGAIN and is not waiting for EPOLLOUT
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&send_complete_context->complete_count, 0), "send completed synchronously, it did not hit EAGAIN");

    return fill_byte_count;
}

static void drain_peer_and_assert_send_completes(SOCKET_HANDLE peer_socket, uint32_t fill_byte_count, const uint8_t* expected_payload, SEND_COMPLETE_CONTEXT* send_complete_context)
{
    uint8_t received_payload[TEST_PENDING_SEND_SIZE];

    ASSERT_ARE_EQUAL(uint32_t, fill_byte_count, discard_with_timeout(peer_socket, fill_byte_count), "peer did not receive the bytes used to fill the socket");
    ASSERT_ARE_EQUAL(uint32_t, TEST_PENDING_SEND_SIZE, receive_with_timeout(peer_socket, received_payload, TEST_PENDING_SEND_SIZE), "the pending send never wrote its bytes");
    ASSERT_ARE_EQUAL(int, 0, memcmp(received_payload, expected_payload, TEST_PENDING_SEND_SIZE));

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&send_complete_context->complete_count, 1, TEST_IO_COMPLETE_TIMEOUT_MS), "send complete callback was not called");
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_OK, (ASYNC_SOCKET_SEND_RESULT)interlocked_add(&send_complete_context->send_result, 0));
}

static void write_from_peer_and_assert_receive_completes(SOCKET_HANDLE peer_socket, const uint8_t* receive_buffer, RECEIVE_COMPLETE_CONTEXT* receive_complete_context)
{
    uint8_t peer_payload[TEST_PEER_PAYLOAD_SIZE] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 };

    ASSERT_ARE_EQUAL(int, TEST_PEER_PAYLOAD_SIZE, (int)send(peer_socket, peer_payload, sizeof(peer_payload), MSG_NOSIGNAL), "peer send failed error (%d): %s", errno, strerror(errno));

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&receive_complete_context->complete_count, 1, TEST_IO_COMPLETE_TIMEOUT_MS), "receive complete callback was not called");
    ASSERT_ARE_EQUAL(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_OK, (ASYNC_SOCKET_RECEIVE_RESULT)interlocked_add(&receive_complete_context->receive_result, 0));
    ASSERT_ARE_EQUAL(int32_t, TEST_PEER_PAYLOAD_SIZE, interlocked_add(&receive_complete_context->bytes_received, 0));
    ASSERT_ARE_EQUAL(int, 0, memcmp(receive_buffer, peer_payload, sizeof(peer_payload)));
}

static void initialize_pending_send_payload(uint8_t* payload, ASYNC_SOCKET_BUFFER* send_payload_buffer)
{
    for (uint32_t index = 0; index < TEST_PENDING_SEND_SIZE; index++)
    {
        payload[index] = (uint8_t)index;
    }
    send_payload_buffer->buffer = payload;
    send_payload_buffer->length = TEST_PENDING_SEND_SIZE;
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

    uint8_t receive_buffer[2];
    ASYNC_SOCKET_BUFFER receive_payload_buffers[1];
    receive_payload_buffers[0].buffer = receive_buffer;
    receive_payload_buffers[0].length = sizeof(receive_buffer);

    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, receive_payload_buffers, 1, on_receive_abandoned_complete, NULL));
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(server_async_socket, receive_payload_buffers, 1, on_receive_abandoned_complete, NULL));

    async_socket_close(server_async_socket);
    async_socket_close(client_async_socket);
    async_socket_destroy(server_async_socket);
    async_socket_destroy(client_async_socket);
    execution_engine_dec_ref(execution_engine);
    close(listen_socket);
}

TEST_FUNCTION(send_and_receive_1_byte_succeeds)
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
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE accept_socket;
    SOCKET_HANDLE listen_socket;

    setup_server_socket(&listen_socket);
    setup_test_socket(g_port_num, &client_socket, &listen_socket, &accept_socket);

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

TEST_FUNCTION(when_send_hits_EAGAIN_while_receive_is_pending_then_both_complete)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE listen_socket;
    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE peer_socket;
    ASYNC_SOCKET_HANDLE client_async_socket;
    create_connected_client_async_socket_with_small_buffers(execution_engine, &listen_socket, &client_socket, &peer_socket, &client_async_socket);

    uint8_t receive_buffer[TEST_RECEIVE_BUFFER_SIZE];
    ASYNC_SOCKET_BUFFER receive_payload_buffer;
    receive_payload_buffer.buffer = receive_buffer;
    receive_payload_buffer.length = sizeof(receive_buffer);
    RECEIVE_COMPLETE_CONTEXT receive_complete_context;
    (void)interlocked_exchange(&receive_complete_context.complete_count, 0);
    (void)interlocked_exchange(&receive_complete_context.receive_result, ASYNC_SOCKET_RECEIVE_ERROR);
    (void)interlocked_exchange(&receive_complete_context.bytes_received, 0);

    uint8_t send_payload[TEST_PENDING_SEND_SIZE];
    ASYNC_SOCKET_BUFFER send_payload_buffer;
    initialize_pending_send_payload(send_payload, &send_payload_buffer);
    SEND_COMPLETE_CONTEXT send_complete_context;
    (void)interlocked_exchange(&send_complete_context.complete_count, 0);
    (void)interlocked_exchange(&send_complete_context.send_result, ASYNC_SOCKET_SEND_ERROR);

    // act
    // receive is pending first, then a send hits EAGAIN and waits for EPOLLOUT on the same socket
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, &receive_payload_buffer, 1, on_receive_complete_record_result, &receive_complete_context));
    uint32_t fill_byte_count = start_send_that_hits_eagain(client_async_socket, client_socket, &send_payload_buffer, &send_complete_context);

    // assert
    write_from_peer_and_assert_receive_completes(peer_socket, receive_buffer, &receive_complete_context);
    drain_peer_and_assert_send_completes(peer_socket, fill_byte_count, send_payload, &send_complete_context);

    // cleanup
    async_socket_close(client_async_socket);
    async_socket_destroy(client_async_socket);
    (void)close(peer_socket);
    (void)close(listen_socket);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(when_receive_is_started_while_send_waits_after_EAGAIN_then_both_complete)
{
    // arrange
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    SOCKET_HANDLE listen_socket;
    SOCKET_HANDLE client_socket;
    SOCKET_HANDLE peer_socket;
    ASYNC_SOCKET_HANDLE client_async_socket;
    create_connected_client_async_socket_with_small_buffers(execution_engine, &listen_socket, &client_socket, &peer_socket, &client_async_socket);

    uint8_t receive_buffer[TEST_RECEIVE_BUFFER_SIZE];
    ASYNC_SOCKET_BUFFER receive_payload_buffer;
    receive_payload_buffer.buffer = receive_buffer;
    receive_payload_buffer.length = sizeof(receive_buffer);
    RECEIVE_COMPLETE_CONTEXT receive_complete_context;
    (void)interlocked_exchange(&receive_complete_context.complete_count, 0);
    (void)interlocked_exchange(&receive_complete_context.receive_result, ASYNC_SOCKET_RECEIVE_ERROR);
    (void)interlocked_exchange(&receive_complete_context.bytes_received, 0);

    uint8_t send_payload[TEST_PENDING_SEND_SIZE];
    ASYNC_SOCKET_BUFFER send_payload_buffer;
    initialize_pending_send_payload(send_payload, &send_payload_buffer);
    SEND_COMPLETE_CONTEXT send_complete_context;
    (void)interlocked_exchange(&send_complete_context.complete_count, 0);
    (void)interlocked_exchange(&send_complete_context.send_result, ASYNC_SOCKET_SEND_ERROR);

    // act
    // a send hits EAGAIN and waits for EPOLLOUT first, then a receive is started on the same socket
    uint32_t fill_byte_count = start_send_that_hits_eagain(client_async_socket, client_socket, &send_payload_buffer, &send_complete_context);
    ASSERT_ARE_EQUAL(int, 0, async_socket_receive_async(client_async_socket, &receive_payload_buffer, 1, on_receive_complete_record_result, &receive_complete_context));

    // assert
    drain_peer_and_assert_send_completes(peer_socket, fill_byte_count, send_payload, &send_complete_context);
    write_from_peer_and_assert_receive_completes(peer_socket, receive_buffer, &receive_complete_context);

    // cleanup
    async_socket_close(client_async_socket);
    async_socket_destroy(client_async_socket);
    (void)close(peer_socket);
    (void)close(listen_socket);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/platform.h"
#include "c_pal/socket_mgr.h"
#include "c_pal/gballoc_hl.h"

#define TEST_PORT 4366
#define TEST_CONN_TIMEOUT 10000

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

TEST_FUNCTION(send_and_receive_2_buffer_of_2_byte_succeeds)
{
    // assert
    SOCKET_MGR_HANDLE listen_socket = socket_mgr_create(SOCKET_SERVER);
    ASSERT_IS_NOT_NULL(listen_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_mgr_listen(listen_socket, TEST_PORT));

    // create the async socket object
    SOCKET_MGR_HANDLE client_socket = socket_mgr_create(SOCKET_CLIENT);
    ASSERT_IS_NOT_NULL(client_socket);

    ASSERT_ARE_EQUAL(int, 0, socket_mgr_connect(client_socket, "localhost", TEST_PORT, 10000));

    SOCKET_MGR_HANDLE incoming_socket = socket_mgr_accept(listen_socket);

    uint8_t send_payload_1[] = {0x42, 0x43};
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

    // Send data back and forth
    ASSERT_ARE_EQUAL(int, 0, socket_mgr_send(client_socket, send_data, 2, &bytes_written, 0, NULL));
    ASSERT_ARE_EQUAL(int, total_bytes_sent, bytes_written);

    ASSERT_ARE_EQUAL(int, 0, socket_mgr_receive(incoming_socket, recv_data, 2, &bytes_recv, 0, NULL));
    ASSERT_ARE_EQUAL(int, total_bytes_sent, bytes_recv);

    ASSERT_ARE_EQUAL(int, 0, memcmp(send_payload_1, recv_buffer_1, send_data[0].length));
    ASSERT_ARE_EQUAL(int, 0, memcmp(send_payload_2, recv_buffer_2, send_data[1].length));

    socket_mgr_disconnect(incoming_socket);
    socket_mgr_destroy(incoming_socket);

    socket_mgr_disconnect(client_socket);
    socket_mgr_destroy(client_socket);

    socket_mgr_disconnect(listen_socket);
    socket_mgr_destroy(listen_socket);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

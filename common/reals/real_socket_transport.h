// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_SOCKET_TRANSPORT_H
#define REAL_SOCKET_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SOCKET_TRANSPORT_GLOBAL_MOCK_HOOK()        \
    MU_FOR_EACH_1(R2,                                       \
        socket_transport_create_client,                \
        socket_transport_create_server,                \
        socket_transport_create_from_socket,           \
        socket_transport_destroy,                      \
        socket_transport_connect,                      \
        socket_transport_listen,                       \
        socket_transport_disconnect,                   \
        socket_transport_accept,                       \
        socket_transport_send,                         \
        socket_transport_receive,                      \
        socket_transport_get_underlying_socket,        \
        socket_transport_is_valid_socket,              \
        socket_transport_get_local_address             \
    )

#ifdef __cplusplus
extern "C" {
#endif

SOCKET_TRANSPORT_HANDLE real_socket_transport_create_client(void);
SOCKET_TRANSPORT_HANDLE real_socket_transport_create_server(void);
SOCKET_TRANSPORT_HANDLE real_socket_transport_create_from_socket(SOCKET_HANDLE socket_handle);
void real_socket_transport_destroy(SOCKET_TRANSPORT_HANDLE socket_transport);
int real_socket_transport_connect(SOCKET_TRANSPORT_HANDLE socket_transport, const char* hostname, uint16_t port, uint32_t connection_timeout_ms);
int real_socket_transport_listen(SOCKET_TRANSPORT_HANDLE socket_transport, uint16_t port);
void real_socket_transport_disconnect(SOCKET_TRANSPORT_HANDLE socket_transport);
SOCKET_ACCEPT_RESULT real_socket_transport_accept(SOCKET_TRANSPORT_HANDLE socket_transport, SOCKET_TRANSPORT_HANDLE* accepted_socket, uint32_t connection_timeout_ms);
SOCKET_SEND_RESULT real_socket_transport_send(SOCKET_TRANSPORT_HANDLE socket_transport, const SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_sent, uint32_t flags, void* data);
SOCKET_RECEIVE_RESULT real_socket_transport_receive(SOCKET_TRANSPORT_HANDLE socket_transport, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_recv, uint32_t flags, void* data);
SOCKET_HANDLE real_socket_transport_get_underlying_socket(SOCKET_TRANSPORT_HANDLE socket_transport);
bool real_socket_transport_is_valid_socket(SOCKET_TRANSPORT_HANDLE socket_transport_handle);
int real_socket_transport_get_local_address(SOCKET_TRANSPORT_HANDLE socket_transport, char hostname[MAX_GET_HOST_NAME_LEN], LOCAL_ADDRESS** local_address_list, uint32_t* address_count);

#ifdef __cplusplus
}
#endif

#endif //REAL_SOCKET_TRANSPORT_H

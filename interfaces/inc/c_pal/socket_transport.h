// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SOCKET_TRANSPORT_H
#define SOCKET_TRANSPORT_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdbool.h>
#include <stdint.h>
#endif
#include "macro_utils/macro_utils.h"

#include "socket_handle.h"


#define MAX_GET_HOST_NAME_LEN       256
#define MAX_LOCAL_ADDRESS_LEN       22

typedef struct SOCKET_TRANSPORT_TAG* SOCKET_TRANSPORT_HANDLE;

#define SOCKET_SEND_RESULT_VALUES \
    SOCKET_SEND_INVALID_ARG, \
    SOCKET_SEND_OK, \
    SOCKET_SEND_ERROR, \
    SOCKET_SEND_FAILED, \
    SOCKET_SEND_SHUTDOWN

MU_DEFINE_ENUM(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)

#define SOCKET_RECEIVE_RESULT_VALUES \
    SOCKET_RECEIVE_INVALID_ARG, \
    SOCKET_RECEIVE_OK, \
    SOCKET_RECEIVE_WOULD_BLOCK, \
    SOCKET_RECEIVE_ERROR, \
    SOCKET_RECEIVE_SHUTDOWN

MU_DEFINE_ENUM(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)

#define SOCKET_ACCEPT_RESULT_VALUES \
    SOCKET_ACCEPT_OK,               \
    SOCKET_ACCEPT_ERROR,            \
    SOCKET_ACCEPT_PORT_EXHAUSTION,  \
    SOCKET_ACCEPT_INPROGRESS,       \
    SOCKET_ACCEPT_NO_CONNECTION

MU_DEFINE_ENUM(SOCKET_ACCEPT_RESULT, SOCKET_ACCEPT_RESULT_VALUES)

#define SOCKET_TYPE_VALUES \
    SOCKET_BINDING, \
    SOCKET_CLIENT

MU_DEFINE_ENUM(SOCKET_TYPE, SOCKET_TYPE_VALUES)

#define ADDRESS_TYPE_VALUES \
    ADDRESS_INET, \
    ADDRESS_INET_6, \
    ADDRESS_NETBIOS

MU_DEFINE_ENUM(ADDRESS_TYPE, ADDRESS_TYPE_VALUES)

typedef struct SOCKET_BUFFER_TAG
{
    uint32_t length;
    unsigned char* buffer;
} SOCKET_BUFFER;

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct LOCAL_ADDRESS_TAG
{
    ADDRESS_TYPE address_type;
    char address[MAX_LOCAL_ADDRESS_LEN];
} LOCAL_ADDRESS;

MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_client);
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_server);
MOCKABLE_FUNCTION(, SOCKET_TRANSPORT_HANDLE, socket_transport_create_from_socket, SOCKET_HANDLE, socket_handle);
MOCKABLE_FUNCTION(, void, socket_transport_destroy, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, int, socket_transport_connect, SOCKET_TRANSPORT_HANDLE, socket_transport, const char*, hostname, uint16_t, port, uint32_t, connection_timeout_ms);
MOCKABLE_FUNCTION(, int, socket_transport_listen, SOCKET_TRANSPORT_HANDLE, socket_transport, uint16_t, port);
MOCKABLE_FUNCTION(, void, socket_transport_disconnect, SOCKET_TRANSPORT_HANDLE, socket_transport);

MOCKABLE_FUNCTION(, SOCKET_ACCEPT_RESULT, socket_transport_accept, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_TRANSPORT_HANDLE*, accepted_socket, uint32_t, connection_timeout_ms);

MOCKABLE_FUNCTION(, SOCKET_SEND_RESULT, socket_transport_send, SOCKET_TRANSPORT_HANDLE, socket_transport, const SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_sent, uint32_t, flags, void*, data);
MOCKABLE_FUNCTION(, SOCKET_RECEIVE_RESULT, socket_transport_receive, SOCKET_TRANSPORT_HANDLE, socket_transport, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);

MOCKABLE_FUNCTION(, SOCKET_HANDLE, socket_transport_get_underlying_socket, SOCKET_TRANSPORT_HANDLE, socket_transport);
MOCKABLE_FUNCTION(, bool, socket_transport_is_valid_socket, SOCKET_TRANSPORT_HANDLE, socket_transport_handle);
MOCKABLE_FUNCTION(, int, socket_transport_get_local_address, SOCKET_TRANSPORT_HANDLE, socket_transport, char, hostname[MAX_GET_HOST_NAME_LEN], LOCAL_ADDRESS**, local_address_list, uint32_t*, address_count);

#ifdef __cplusplus
}
#endif

#endif // SOCKET_TRANSPORT_H

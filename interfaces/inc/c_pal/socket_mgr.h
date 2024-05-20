// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SOCKET_MGR_H
#define SOCKET_MGR_H

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#include "socket_handle.h"

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

typedef struct SOCKET_MGR_TAG* SOCKET_MGR_HANDLE;

#define SOCKET_SEND_RESULT_VALUES \
    SOCKET_SEND_OK, \
    SOCKET_SEND_ERROR, \
    SOCKET_SEND_ABANDONED

MU_DEFINE_ENUM(SOCKET_SEND_RESULT, SOCKET_SEND_RESULT_VALUES)

#define SOCKET_RECEIVE_RESULT_VALUES \
    SOCKET_RECEIVE_OK, \
    SOCKET_RECEIVE_ERROR, \
    SOCKET_RECEIVE_ABANDONED

MU_DEFINE_ENUM(SOCKET_RECEIVE_RESULT, SOCKET_RECEIVE_RESULT_VALUES)

#define SOCKET_TYPE_VALUES \
    SOCKET_CLIENT, \
    SOCKET_SERVER

MU_DEFINE_ENUM(SOCKET_TYPE, SOCKET_TYPE_VALUES)

typedef struct SOCKET_BUFFER_TAG
{
    uint32_t length;
    void* buffer;
} SOCKET_BUFFER;

MOCKABLE_FUNCTION(, SOCKET_MGR_HANDLE, socket_mgr_create, SOCKET_TYPE, type);
MOCKABLE_FUNCTION(, void, socket_mgr_destroy, SOCKET_MGR_HANDLE, socket);

MOCKABLE_FUNCTION(, int, socket_mgr_connect, SOCKET_MGR_HANDLE, socket, const char*, hostname, uint16_t, port, uint32_t, connection_timeout);
MOCKABLE_FUNCTION(, void, socket_mgr_disconnect, SOCKET_MGR_HANDLE, socket);

MOCKABLE_FUNCTION(, int, socket_mgr_listen, SOCKET_MGR_HANDLE, socket, uint16_t, port);
MOCKABLE_FUNCTION(, SOCKET_MGR_HANDLE, socket_mgr_accept, SOCKET_MGR_HANDLE, socket);

MOCKABLE_FUNCTION(, int, socket_mgr_send, SOCKET_MGR_HANDLE, socket, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_written, uint32_t, flags, void*, data);
MOCKABLE_FUNCTION(, int, socket_mgr_receive, SOCKET_MGR_HANDLE, socket, SOCKET_BUFFER*, payload, uint32_t, buffer_count, uint32_t*, bytes_recv, uint32_t, flags, void*, data);


#ifdef __cplusplus
}
#endif

#endif // SOCKET_MGR_H

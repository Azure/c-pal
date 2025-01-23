// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef ASYNC_SOCKET_H
#define ASYNC_SOCKET_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/execution_engine.h"
#include "c_pal/socket_transport.h"

#include "umock_c/umock_c_prod.h"

/* Note : At some point this API should be extended with connect/listen in order to fully encapsulate the underlying SOCKET,
so that no other module cares how the underlying SOCKET is obtained and what its lifetime is. */

typedef struct ASYNC_SOCKET_TAG* ASYNC_SOCKET_HANDLE;

#define ASYNC_SOCKET_OPEN_RESULT_VALUES \
    ASYNC_SOCKET_OPEN_OK, \
    ASYNC_SOCKET_OPEN_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES \
    ASYNC_SOCKET_SEND_SYNC_OK, \
    ASYNC_SOCKET_SEND_SYNC_ERROR, \
    ASYNC_SOCKET_SEND_SYNC_NOT_OPEN

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_RESULT_VALUES \
    ASYNC_SOCKET_SEND_OK, \
    ASYNC_SOCKET_SEND_ERROR, \
    ASYNC_SOCKET_SEND_ABANDONED

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)

#define ASYNC_SOCKET_RECEIVE_RESULT_VALUES \
    ASYNC_SOCKET_RECEIVE_OK, \
    ASYNC_SOCKET_RECEIVE_ERROR, \
    ASYNC_SOCKET_RECEIVE_ABANDONED

MU_DEFINE_ENUM(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)

#define ASYNC_SOCKET_NOTIFY_IO_TYPE_VALUES \
    ASYNC_SOCKET_NOTIFY_IO_TYPE_IN, \
    ASYNC_SOCKET_NOTIFY_IO_TYPE_OUT

MU_DEFINE_ENUM(ASYNC_SOCKET_NOTIFY_IO_TYPE, ASYNC_SOCKET_NOTIFY_IO_TYPE_VALUES)

#define ASYNC_SOCKET_NOTIFY_IO_RESULT_VALUES \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_IN, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_OUT, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_ABANDONED, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_NOTIFY_IO_RESULT, ASYNC_SOCKET_NOTIFY_IO_RESULT_VALUES)

typedef void (*ON_ASYNC_SOCKET_OPEN_COMPLETE)(void* context, ASYNC_SOCKET_OPEN_RESULT open_result);
typedef void (*ON_ASYNC_SOCKET_SEND_COMPLETE)(void* context, ASYNC_SOCKET_SEND_RESULT send_result);
typedef void (*ON_ASYNC_SOCKET_RECEIVE_COMPLETE)(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received);
typedef void (*ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE)(void* context, ASYNC_SOCKET_NOTIFY_IO_RESULT notify_io_result);
typedef int (*ON_ASYNC_SOCKET_SEND)(void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len);
typedef int (*ON_ASYNC_SOCKET_RECV)(void* context, ASYNC_SOCKET_HANDLE async_socket, void* buf, size_t len);

typedef struct ASYNC_SOCKET_BUFFER_TAG
{
    void* buffer;
    uint32_t length;
} ASYNC_SOCKET_BUFFER;

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create_with_transport, EXECUTION_ENGINE_HANDLE, execution_engine, ON_ASYNC_SOCKET_SEND, on_send, void*, on_send_context, ON_ASYNC_SOCKET_RECV, on_recv, void*, on_recv_context);
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);

MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, SOCKET_TRANSPORT_HANDLE, socket_transport, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
MOCKABLE_FUNCTION(, ASYNC_SOCKET_SEND_SYNC_RESULT, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_notify_io_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE, io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE, on_notify_io_complete, void*, on_notify_io_complete_context);

#ifdef __cplusplus
}
#endif

#endif // ASYNC_SOCKET_H

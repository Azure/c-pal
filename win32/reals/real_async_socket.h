// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_ASYNC_SOCKET_H
#define REAL_ASYNC_SOCKET_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_ASYNC_SOCKET_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        async_socket_create,                                      \
        async_socket_create_with_transport, \
        async_socket_destroy, \
        async_socket_open_async, \
        async_socket_close, \
        async_socket_send_async, \
        async_socket_receive_async, \
        async_socket_notify_io \
    )

#ifdef __cplusplus
extern "C" {
#endif

    ASYNC_SOCKET_HANDLE real_async_socket_create(EXECUTION_ENGINE_HANDLE execution_engine, SOCKET_HANDLE socket_handle);
    ASYNC_SOCKET_HANDLE real_async_socket_create_with_transport(EXECUTION_ENGINE_HANDLE execution_engine, SOCKET_HANDLE socket_handle, ON_ASYNC_SOCKET_SEND on_send, void* on_send_context, ON_ASYNC_SOCKET_RECV on_recv, void* on_recv_context);
    void real_async_socket_destroy(ASYNC_SOCKET_HANDLE async_socket);

    int real_async_socket_open_async(ASYNC_SOCKET_HANDLE async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE on_open_complete, void* on_open_complete_context);
    void real_async_socket_close(ASYNC_SOCKET_HANDLE async_socket);
    ASYNC_SOCKET_SEND_SYNC_RESULT real_async_socket_send_async(ASYNC_SOCKET_HANDLE async_socket, const ASYNC_SOCKET_BUFFER* payload, uint32_t buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE on_send_complete, void* on_send_complete_context);
    int real_async_socket_receive_async(ASYNC_SOCKET_HANDLE async_socket, ASYNC_SOCKET_BUFFER* payload, uint32_t buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE on_receive_complete, void* on_receive_complete_context);
    int async_socket_notify_io(ASYNC_SOCKET_HANDLE async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE on_notify_io_complete, void* on_notify_io_complete_context);
#ifdef __cplusplus
}
#endif

#endif //REAL_ASYNC_SOCKET_H

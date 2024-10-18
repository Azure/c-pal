// Copyright (c) Microsoft. All rights reserved.

#define async_socket_create                   real_async_socket_create
#define async_socket_create_with_transport    real_async_socket_create_with_transport
#define async_socket_destroy                  real_async_socket_destroy
#define async_socket_open_async               real_async_socket_open_async
#define async_socket_close                    real_async_socket_close
#define async_socket_send_async               real_async_socket_send_async
#define async_socket_receive_async            real_async_socket_receive_async
#define async_socket_notify_io_async          real_async_socket_notify_io_async

#define ASYNC_SOCKET_SEND_SYNC_RESULT         real_ASYNC_SOCKET_SEND_SYNC_RESULT
#define ASYNC_SOCKET_NOTIFY_IO_TYPE           real_ASYNC_SOCKET_NOTIFY_IO_TYPE
#define ASYNC_SOCKET_NOTIFY_IO_RESULT         real_ASYNC_SOCKET_NOTIFY_IO_RESULT
#define ASYNC_SOCKET_SEND_RESULT              real_ASYNC_SOCKET_SEND_RESULT
#define ASYNC_SOCKET_OPEN_RESULT              real_ASYNC_SOCKET_OPEN_RESULT
#define ASYNC_SOCKET_WIN32_STATE              real_ASYNC_SOCKET_WIN32_STATE
#define ASYNC_SOCKET_IO_TYPE                  real_ASYNC_SOCKET_IO_TYPE
#define ASYNC_SOCKET_SEND_RESULT              real_ASYNC_SOCKET_SEND_RESULT

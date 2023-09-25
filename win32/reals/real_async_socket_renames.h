// Copyright (c) Microsoft. All rights reserved.

#define async_socket_create                   real_async_socket_create
#define async_socket_create_with_transport    real_async_socket_create_with_transport
#define async_socket_destroy                  real_async_socket_destroy
#define async_socket_open_async               real_async_socket_open_async
#define async_socket_close                    real_async_socket_close
#define async_socket_send_async               real_async_socket_send_async
#define async_socket_receive_async            real_async_socket_receive_async
#define async_socket_notify_io                real_async_socket_notify_io

#define ASYNC_SOCKET_SEND_SYNC_RESULT   real_ASYNC_SOCKET_SEND_SYNC_RESULT

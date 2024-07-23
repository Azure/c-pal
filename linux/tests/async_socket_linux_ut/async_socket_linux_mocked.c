// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>     // for size_t
#include <sys/types.h>  // for ssize_t
#include <stdint.h>     // for uint32_t
#include "c_pal/socket_transport.h"
#include "c_pal/socket_handle.h"

#define close                                           mocked_close
#define send                                            mocked_send
#define recv                                            mocked_recv
#define socket_transport_get_underlying_socket          mocked_socket_transport_get_underlying_socket
#define socket_transport_send                           mocked_socket_transport_send
#define socket_transport_receive                        mocked_socket_transport_receive

int mocked_close(int s);
ssize_t mocked_send(int fd, const void* buf, size_t n, int flags);
ssize_t mocked_recv(int fd, void* buf, size_t n, int flags);
SOCKET_HANDLE mocked_socket_transport_get_underlying_socket(SOCKET_TRANSPORT_HANDLE socket_transport);
SOCKET_SEND_RESULT mocked_socket_transport_send(SOCKET_TRANSPORT_HANDLE socket_transport, const SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_sent, uint32_t flags, void* overlapped_data);
SOCKET_RECEIVE_RESULT mocked_socket_transport_receive(SOCKET_TRANSPORT_HANDLE socket_transport, SOCKET_BUFFER* payload, uint32_t buffer_count, uint32_t* bytes_recv, uint32_t flags, void* data);

#include "../../src/async_socket_linux.c"

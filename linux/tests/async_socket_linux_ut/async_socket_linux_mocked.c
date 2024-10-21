// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>     // for size_t
#include <sys/types.h>  // for ssize_t

#define close           mocked_close
#define send            mocked_send
#define recv            mocked_recv

int mocked_close(int s);
ssize_t mocked_send(int fd, const void* buf, size_t n, int flags);
ssize_t mocked_recv(int fd, void* buf, size_t n, int flags);

#include "../../src/async_socket_linux.c"

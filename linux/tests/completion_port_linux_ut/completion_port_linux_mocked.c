// Copyright (c) Microsoft. All rights reserved.

#include <sys/epoll.h>  // IWYU pragma: keep

#define epoll_create    mocked_epoll_create
#define epoll_ctl       mocked_epoll_ctl
#define epoll_wait      mocked_epoll_wait
#define close           mocked_close

int mocked_epoll_create(int __size);
int mocked_epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
int mocked_epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
int mocked_close(int s);

#include "../../src/completion_port_linux.c"

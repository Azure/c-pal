// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef COMPLETION_PORT_LINUX
#define COMPLETION_PORT_LINUX

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "c_pal/socket_handle.h"

typedef struct COMPLETION_PORT_TAG* COMPLETION_PORT_HANDLE;

#define COMPLETION_PORT_EPOLL_ACTION_VALUES \
    COMPLETION_PORT_EPOLL_EPOLLRDHUP, \
    COMPLETION_PORT_EPOLL_EPOLLIN, \
    COMPLETION_PORT_EPOLL_EPOLLOUT, \
    COMPLETION_PORT_EPOLL_EPOLL_ABANDONED, \
    COMPLETION_PORT_EPOLL_EPOLL_ERROR

MU_DEFINE_ENUM(COMPLETION_PORT_EPOLL_ACTION, COMPLETION_PORT_EPOLL_ACTION_VALUES)

typedef void (*ON_COMPLETION_PORT_EVENT_COMPLETE)(void* context, COMPLETION_PORT_EPOLL_ACTION action);

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, COMPLETION_PORT_HANDLE, completion_port_create);
MOCKABLE_FUNCTION(, void, completion_port_inc_ref, COMPLETION_PORT_HANDLE, completion_port);
MOCKABLE_FUNCTION(, void, completion_port_dec_ref, COMPLETION_PORT_HANDLE, completion_port);
MOCKABLE_FUNCTION(, int, completion_port_add, COMPLETION_PORT_HANDLE, completion_port, int, epoll_op, SOCKET_HANDLE, socket,
    ON_COMPLETION_PORT_EVENT_COMPLETE, event_callback, void*, event_callback_ctx);
MOCKABLE_FUNCTION(, void, completion_port_remove, COMPLETION_PORT_HANDLE, completion_port, SOCKET_HANDLE, socket);

#ifdef __cplusplus
}
#endif

#endif // COMPLETION_PORT_LINUX

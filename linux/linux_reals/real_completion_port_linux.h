// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_UUID_H
#define REAL_UUID_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_COMPLETION_PORT_LINUX_GLOBAL_MOCK_HOOK()   \
    MU_FOR_EACH_1(R2,                                       \
        completion_port_create,                             \
        completion_port_inc_ref,                            \
        completion_port_dec_ref,                            \
        completion_port_add,                                \
        completion_port_remove                              \
    )

#ifdef __cplusplus
extern "C" {
#endif

    COMPLETION_PORT_HANDLE real_completion_port_create(void);
    void real_completion_port_inc_ref(COMPLETION_PORT_HANDLE completion_port);
    void real_completion_port_dec_ref(COMPLETION_PORT_HANDLE completion_port);
    int real_completion_port_add(COMPLETION_PORT_HANDLE completion_port, int epoll_op, SOCKET_HANDLE socket, ON_COMPLETION_PORT_EVENT_COMPLETE event_callback, void* event_callback_ctx);
    void real_completion_port_remove(COMPLETION_PORT_HANDLE completion_port, SOCKET_HANDLE socket);

#ifdef __cplusplus
}
#endif

#endif //REAL_UUID_H

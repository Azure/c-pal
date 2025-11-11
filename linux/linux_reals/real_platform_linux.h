// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_PLATFORM_LINUX_H
#define REAL_PLATFORM_LINUX_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_PLATFORM_LINUX_GLOBAL_MOCK_HOOK()   \
    MU_FOR_EACH_1(R2,                                \
        platform_get_completion_port                 \
    )

#ifdef __cplusplus
extern "C" {
#endif

    COMPLETION_PORT_HANDLE real_platform_get_completion_port(void);

#ifdef __cplusplus
}
#endif

#endif //REAL_PLATFORM_LINUX_H

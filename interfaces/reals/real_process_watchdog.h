// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_PROCESS_WATCHDOG_H
#define REAL_PROCESS_WATCHDOG_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_PROCESS_WATCHDOG_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2,                               \
        process_watchdog_init,                       \
        process_watchdog_deinit                      \
    )

#ifdef __cplusplus
extern "C" {
#endif

    int real_process_watchdog_init(uint32_t timeout_ms);
    void real_process_watchdog_deinit(void);

#ifdef __cplusplus
}
#endif

#endif //REAL_PROCESS_WATCHDOG_H

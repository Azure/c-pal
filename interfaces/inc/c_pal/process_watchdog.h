// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef PROCESS_WATCHDOG_H
#define PROCESS_WATCHDOG_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

    int process_watchdog_init(uint32_t timeout_ms);
    void process_watchdog_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // PROCESS_WATCHDOG_H

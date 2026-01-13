// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TEST_WATCHDOG_H
#define TEST_WATCHDOG_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

    int test_watchdog_init(uint32_t timeout_ms);
    void test_watchdog_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_WATCHDOG_H

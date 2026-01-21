// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TIMED_TEST_SUITE_H
#define TIMED_TEST_SUITE_H

#include "testrunnerswitcher.h"
#include "c_pal/process_watchdog.h"

// Default timeout: 10 minutes
#define TIMED_TEST_DEFAULT_TIMEOUT_MS 600000

// Timed test suite initialize - timeout_ms is required parameter
// Watchdog init fixture runs BEFORE user init code (first in fixture list)
#define TIMED_TEST_SUITE_INITIALIZE(name, timeout_ms, ...) \
    static void MU_C2(timed_test_watchdog_init_, name)(void) { (void)process_watchdog_init(timeout_ms); } \
    TEST_SUITE_INITIALIZE(name, MU_C2(timed_test_watchdog_init_, name), ##__VA_ARGS__)

// Cleanup - stops the watchdog after user cleanup code runs
// Watchdog deinit fixture runs AFTER user cleanup code (last in fixture list)
#define TIMED_TEST_SUITE_CLEANUP(name, ...) \
    static void MU_C2(timed_test_watchdog_deinit_, name)(void) { process_watchdog_deinit(); } \
    TEST_SUITE_CLEANUP(name, ##__VA_ARGS__, MU_C2(timed_test_watchdog_deinit_, name))

#endif // TIMED_TEST_SUITE_H

// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TIMED_TEST_SUITE_H
#define TIMED_TEST_SUITE_H

#include "testrunnerswitcher.h"
#include "c_pal/process_watchdog.h"

// Default timeout: 10 minutes
#define TIMED_TEST_DEFAULT_TIMEOUT_MS 600000

// Timed test suite initialize - timeout_ms is required parameter
// Watchdog init fixture runs BEFORE user init code (first in fixture list)
/*Codes_SRS_TIMED_TEST_SUITE_43_002: [ TIMED_TEST_SUITE_INITIALIZE shall create a static fixture function that calls process_watchdog_init with timeout_ms. ]*/
/*Codes_SRS_TIMED_TEST_SUITE_43_003: [ TIMED_TEST_SUITE_INITIALIZE shall call TEST_SUITE_INITIALIZE with the watchdog init fixture as the first fixture, followed by any additional fixtures passed via variadic arguments. ]*/
/*Codes_SRS_TIMED_TEST_SUITE_43_004: [ The watchdog init fixture shall execute before the user's initialization code. ]*/
#define TIMED_TEST_SUITE_INITIALIZE(name, timeout_ms, ...) \
    static void MU_C2(timed_test_watchdog_init_, name)(void) { (void)process_watchdog_init(timeout_ms); } \
    TEST_SUITE_INITIALIZE(name, MU_C2(timed_test_watchdog_init_, name), ##__VA_ARGS__)

// Cleanup - stops the watchdog after user cleanup code runs
// Watchdog deinit fixture runs AFTER user cleanup code (last in fixture list)
/*Codes_SRS_TIMED_TEST_SUITE_43_005: [ TIMED_TEST_SUITE_CLEANUP shall create a static fixture function that calls process_watchdog_deinit. ]*/
/*Codes_SRS_TIMED_TEST_SUITE_43_006: [ TIMED_TEST_SUITE_CLEANUP shall call TEST_SUITE_CLEANUP with any additional fixtures passed via variadic arguments, followed by the watchdog deinit fixture as the last fixture. ]*/
/*Codes_SRS_TIMED_TEST_SUITE_43_007: [ The watchdog deinit fixture shall execute after the user's cleanup code. ]*/
#define TIMED_TEST_SUITE_CLEANUP(name, ...) \
    static void MU_C2(timed_test_watchdog_deinit_, name)(void) { process_watchdog_deinit(); } \
    TEST_SUITE_CLEANUP(name, ##__VA_ARGS__, MU_C2(timed_test_watchdog_deinit_, name))

#endif // TIMED_TEST_SUITE_H

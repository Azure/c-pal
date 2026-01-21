// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdio.h>
#include <stdint.h>

#include "ctest.h"
#include "macro_utils/macro_utils.h"

#include "timed_test_suite_ut.h"

// Counters to track call ordering
static int watchdog_init_call_order = 0;
static int watchdog_deinit_call_order = 0;
static int user_init_call_order = 0;
static int user_cleanup_call_order = 0;
static int call_counter = 0;

// Mock process_watchdog functions
int process_watchdog_init(uint32_t timeout_ms)
{
    (void)timeout_ms;
    watchdog_init_call_order = ++call_counter;
    return 0;
}

void process_watchdog_deinit(void)
{
    watchdog_deinit_call_order = ++call_counter;
}

// Include timed_test_suite.h AFTER the mock definitions
// This ensures the macros use our mock functions
#include "c_pal/timed_test_suite.h"

CTEST_BEGIN_TEST_SUITE(timed_test_suite_ut)

TIMED_TEST_SUITE_INITIALIZE(TestInit, TIMED_TEST_DEFAULT_TIMEOUT_MS)
{
    user_init_call_order = ++call_counter;

    // Verify watchdog was initialized BEFORE user init code
    CTEST_ASSERT_IS_TRUE(watchdog_init_call_order < user_init_call_order,
        "watchdog_init (%d) should be called before user_init (%d)",
        watchdog_init_call_order, user_init_call_order);

    // Verify watchdog was initialized as first call
    CTEST_ASSERT_ARE_EQUAL(int, 1, watchdog_init_call_order,
        "watchdog_init should be called first");
}

TIMED_TEST_SUITE_CLEANUP(TestCleanup)
{
    user_cleanup_call_order = ++call_counter;

    // Verify watchdog deinit has not yet been called
    CTEST_ASSERT_ARE_EQUAL(int, 0, watchdog_deinit_call_order,
        "watchdog_deinit should not be called yet during user cleanup");
}

CTEST_FUNCTION(timed_test_suite_watchdog_init_called_before_user_init)
{
    // arrange/act is done by suite initialization

    // assert
    CTEST_ASSERT_ARE_EQUAL(int, 1, watchdog_init_call_order,
        "watchdog_init should be call #1");
    CTEST_ASSERT_ARE_EQUAL(int, 2, user_init_call_order,
        "user_init should be call #2");
}

CTEST_FUNCTION(timed_test_suite_watchdog_is_running_during_tests)
{
    // arrange/act

    // assert - verify watchdog deinit has not been called yet
    CTEST_ASSERT_ARE_EQUAL(int, 0, watchdog_deinit_call_order,
        "watchdog_deinit should not be called during test execution");
}

CTEST_END_TEST_SUITE(timed_test_suite_ut)

// Validation function called from main after test suite completes
int timed_test_suite_ut_succeeded(void)
{
    int result;

    // Verify watchdog deinit was called AFTER user cleanup
    if (watchdog_deinit_call_order <= user_cleanup_call_order)
    {
        (void)printf("ERROR: watchdog_deinit_call_order (%d) should be greater than user_cleanup_call_order (%d)\r\n",
            watchdog_deinit_call_order, user_cleanup_call_order);
        result = MU_FAILURE;
    }
    else if (watchdog_deinit_call_order == 0)
    {
        (void)printf("ERROR: watchdog_deinit was never called\r\n");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

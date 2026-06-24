// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "c_logging/logger.h"

#include "testrunnerswitcher.h"
#include "macro_utils/macro_utils.h"

// Counters to track call ordering
static int watchdog_init_call_order = 0;
static int watchdog_deinit_call_order = 0;
static int user_init_call_order = 0;
static int user_cleanup_call_order = 0;
static int additional_init_fixture_call_order = 0;
static int additional_cleanup_fixture_call_order = 0;
static int call_counter = 0;

// Additional fixture functions to test variadic argument passing
static void additional_init_fixture(void)
{
    LogInfo("additional_init_fixture called");
    additional_init_fixture_call_order = ++call_counter;

    // Verify watchdog init has already run (it should be first)
    ASSERT_ARE_NOT_EQUAL(int, 0, watchdog_init_call_order,
        "additional_init_fixture called but watchdog_init has not run yet");
    ASSERT_IS_TRUE(additional_init_fixture_call_order > watchdog_init_call_order,
        "additional_init_fixture (%d) should be called after watchdog_init (%d)",
        additional_init_fixture_call_order, watchdog_init_call_order);
}

static void additional_cleanup_fixture(void)
{
    LogInfo("additional_cleanup_fixture called");
    additional_cleanup_fixture_call_order = ++call_counter;

    // Verify user cleanup has already run
    ASSERT_ARE_NOT_EQUAL(int, 0, user_cleanup_call_order,
        "additional_cleanup_fixture called but user_cleanup has not run yet");

    // Verify watchdog deinit has NOT yet run (it should be last)
    ASSERT_ARE_EQUAL(int, 0, watchdog_deinit_call_order,
        "additional_cleanup_fixture should be called before watchdog_deinit");
}

// Expected timeout value (must match TIMED_TEST_DEFAULT_TIMEOUT_MS from timed_test_suite.h)
#define EXPECTED_TIMEOUT_MS 600000

// Mock process_watchdog functions - these are called by the TIMED_TEST_SUITE_INITIALIZE
// and TIMED_TEST_SUITE_CLEANUP macros from timed_test_suite.h. The macros generate fixture
// functions that call process_watchdog_init/deinit, so by defining these mocks before
// including timed_test_suite.h, we can track when they are called to verify fixture ordering.

int process_watchdog_init(uint32_t timeout_ms)
{
    LogInfo("process_watchdog_init called with timeout_ms=%" PRIu32, timeout_ms);

    // Verify the timeout value passed matches what was specified in the macro
    ASSERT_ARE_EQUAL(uint32_t, EXPECTED_TIMEOUT_MS, timeout_ms,
        "Expected timeout_ms to be EXPECTED_TIMEOUT_MS (%" PRIu32 "), but got %" PRIu32,
        (uint32_t)EXPECTED_TIMEOUT_MS, timeout_ms);

    watchdog_init_call_order = ++call_counter;
    return 0;
}

void process_watchdog_deinit(void)
{
    LogInfo("process_watchdog_deinit called");
    watchdog_deinit_call_order = ++call_counter;

    // Validate that user cleanup has already run (proves correct ordering)
    ASSERT_ARE_NOT_EQUAL(int, 0, user_cleanup_call_order,
        "watchdog_deinit called but user_cleanup has not run yet");

    // Validate that this call came after user cleanup
    ASSERT_IS_TRUE(watchdog_deinit_call_order > user_cleanup_call_order,
        "watchdog_deinit (%d) should be called after user_cleanup (%d)",
        watchdog_deinit_call_order, user_cleanup_call_order);
}

// Include timed_test_suite.h AFTER the mock definitions
// This ensures the macros use our mock functions
#include "c_pal/timed_test_suite.h"

BEGIN_TEST_SUITE(timed_test_suite_ut)

TIMED_TEST_SUITE_INITIALIZE(TestInit, TIMED_TEST_DEFAULT_TIMEOUT_MS, additional_init_fixture)
{
    LogInfo("user init code called");
    user_init_call_order = ++call_counter;

    // Verify watchdog was initialized BEFORE user init code
    ASSERT_IS_TRUE(watchdog_init_call_order < user_init_call_order,
        "watchdog_init (%d) should be called before user_init (%d)",
        watchdog_init_call_order, user_init_call_order);

    // Verify watchdog was initialized as first call
    ASSERT_ARE_EQUAL(int, 1, watchdog_init_call_order,
        "watchdog_init should be called first");

    // Verify additional init fixture ran after watchdog but before user init
    ASSERT_ARE_EQUAL(int, 2, additional_init_fixture_call_order,
        "additional_init_fixture should be call #2");
    ASSERT_ARE_EQUAL(int, 3, user_init_call_order,
        "user_init should be call #3");
}

TIMED_TEST_SUITE_CLEANUP(TestCleanup, additional_cleanup_fixture)
{
    LogInfo("user cleanup code called");
    user_cleanup_call_order = ++call_counter;

    // Verify watchdog deinit has not yet been called
    ASSERT_ARE_EQUAL(int, 0, watchdog_deinit_call_order,
        "watchdog_deinit should not be called yet during user cleanup");

    // Verify additional cleanup fixture has not yet been called (it runs after user cleanup)
    ASSERT_ARE_EQUAL(int, 0, additional_cleanup_fixture_call_order,
        "additional_cleanup_fixture should not be called yet during user cleanup");
}

/*Tests_SRS_TIMED_TEST_SUITE_43_002: [ TIMED_TEST_SUITE_INITIALIZE shall create a static fixture function that calls process_watchdog_init with timeout_ms. ]*/
/*Tests_SRS_TIMED_TEST_SUITE_43_003: [ TIMED_TEST_SUITE_INITIALIZE shall call TEST_SUITE_INITIALIZE with the watchdog init fixture as the first fixture, followed by any additional fixtures passed via variadic arguments. ]*/
/*Tests_SRS_TIMED_TEST_SUITE_43_004: [ The watchdog init fixture shall execute before the user's initialization code. ]*/
TEST_FUNCTION(timed_test_suite_watchdog_init_called_before_user_init)
{
    // arrange
    // act is done by suite initialization

    // assert - verify fixture ordering: watchdog_init (1) -> additional_init_fixture (2) -> user_init (3)
    ASSERT_ARE_EQUAL(int, 1, watchdog_init_call_order,
        "watchdog_init should be call #1");
    ASSERT_ARE_EQUAL(int, 2, additional_init_fixture_call_order,
        "additional_init_fixture should be call #2");
    ASSERT_ARE_EQUAL(int, 3, user_init_call_order,
        "user_init should be call #3");
}

/*Tests_SRS_TIMED_TEST_SUITE_43_005: [ TIMED_TEST_SUITE_CLEANUP shall create a static fixture function that calls process_watchdog_deinit. ]*/
/*Tests_SRS_TIMED_TEST_SUITE_43_006: [ TIMED_TEST_SUITE_CLEANUP shall call TEST_SUITE_CLEANUP with any additional fixtures passed via variadic arguments, followed by the watchdog deinit fixture as the last fixture. ]*/
/*Tests_SRS_TIMED_TEST_SUITE_43_007: [ The watchdog deinit fixture shall execute after the user's cleanup code. ]*/
TEST_FUNCTION(timed_test_suite_watchdog_is_running_during_tests)
{
    // arrange
    // act is done by test suite (watchdog running during tests)

    // assert - verify watchdog deinit has not been called yet
    ASSERT_ARE_EQUAL(int, 0, watchdog_deinit_call_order,
        "watchdog_deinit should not be called during test execution");
}

END_TEST_SUITE(timed_test_suite_ut)

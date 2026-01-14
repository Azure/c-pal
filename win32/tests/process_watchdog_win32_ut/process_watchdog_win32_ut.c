// Copyright (c) Microsoft. All rights reserved.

#include "process_watchdog_win32_ut_pch.h"

// Test timer handle
static PTP_TIMER g_test_timer = (PTP_TIMER)0x1234;
static PTP_TIMER_CALLBACK g_captured_timer_callback = NULL;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static PTP_TIMER hook_CreateThreadpoolTimer(PTP_TIMER_CALLBACK pfnti, PVOID pv, PTP_CALLBACK_ENVIRON pcbe)
{
    (void)pv;
    (void)pcbe;
    g_captured_timer_callback = pfnti;
    return g_test_timer;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_UMOCK_ALIAS_TYPE(PTP_TIMER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_TIMER_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PTP_CALLBACK_ENVIRON, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PVOID, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PFILETIME, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, unsigned long);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, int);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_CreateThreadpoolTimer, hook_CreateThreadpoolTimer);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    g_captured_timer_callback = NULL;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    // Reset watchdog state between tests
    process_watchdog_deinit();
}

// Tests_SRS_PROCESS_WATCHDOG_43_001: [ process_watchdog_init shall call interlocked_compare_exchange to atomically check if the watchdog is already initialized. ]
// Tests_SRS_PROCESS_WATCHDOG_43_002: [ If the watchdog is already initialized, process_watchdog_init shall fail and return a non-zero value. ]
TEST_FUNCTION(process_watchdog_init_when_already_initialized_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    (void)process_watchdog_init(1000);
    umock_c_reset_all_calls();

    // act
    int result = process_watchdog_init(1000);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    process_watchdog_deinit();
}

// Tests_SRS_PROCESS_WATCHDOG_43_003: [ process_watchdog_init shall create a timer that expires after timeout_ms milliseconds and calls on_timer_expired. ]
// Tests_SRS_PROCESS_WATCHDOG_WIN32_43_001: [ process_watchdog_init shall call CreateThreadpoolTimer to create a timer with on_timer_expired as the callback. ]
// Tests_SRS_PROCESS_WATCHDOG_WIN32_43_003: [ process_watchdog_init shall call SetThreadpoolTimer to start the timer with the specified timeout_ms. ]
// Tests_SRS_PROCESS_WATCHDOG_43_005: [ On success, process_watchdog_init shall return zero. ]
TEST_FUNCTION(process_watchdog_init_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    int result = process_watchdog_init(60000);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    process_watchdog_deinit();
}

// Tests_SRS_PROCESS_WATCHDOG_WIN32_43_002: [ If CreateThreadpoolTimer fails, process_watchdog_init shall fail and return a non-zero value. ]
// Tests_SRS_PROCESS_WATCHDOG_43_004: [ If creating the timer fails, process_watchdog_init shall call interlocked_exchange to atomically mark the watchdog as not initialized and return a non-zero value. ]
TEST_FUNCTION(process_watchdog_init_when_CreateThreadpoolTimer_fails_returns_error)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL);

    // act
    int result = process_watchdog_init(60000);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_PROCESS_WATCHDOG_43_006: [ on_timer_expired shall call LogCriticalAndTerminate to terminate the process. ]
// Note: This test verifies the callback is captured correctly. Actually calling it would terminate the process.
TEST_FUNCTION(process_watchdog_init_captures_timer_callback)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    int result = process_watchdog_init(60000);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(g_captured_timer_callback);

    // cleanup
    process_watchdog_deinit();
}

// Tests_SRS_PROCESS_WATCHDOG_43_007: [ process_watchdog_deinit shall call interlocked_compare_exchange to atomically check if the watchdog is initialized and mark it as not initialized. ]
// Tests_SRS_PROCESS_WATCHDOG_43_008: [ If the watchdog is not initialized, process_watchdog_deinit shall return. ]
TEST_FUNCTION(process_watchdog_deinit_when_not_initialized_returns)
{
    // arrange

    // act
    process_watchdog_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_PROCESS_WATCHDOG_43_009: [ process_watchdog_deinit shall cancel and delete the timer. ]
// Tests_SRS_PROCESS_WATCHDOG_WIN32_43_004: [ process_watchdog_deinit shall call SetThreadpoolTimer with NULL to stop the timer. ]
// Tests_SRS_PROCESS_WATCHDOG_WIN32_43_005: [ process_watchdog_deinit shall call WaitForThreadpoolTimerCallbacks to wait for any pending callbacks to complete. ]
// Tests_SRS_PROCESS_WATCHDOG_WIN32_43_006: [ process_watchdog_deinit shall call CloseThreadpoolTimer to delete the timer. ]
TEST_FUNCTION(process_watchdog_deinit_stops_and_closes_timer)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_CreateThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    (void)process_watchdog_init(60000);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_SetThreadpoolTimer(g_test_timer, NULL, 0, 0));
    STRICT_EXPECTED_CALL(mocked_WaitForThreadpoolTimerCallbacks(g_test_timer, TRUE));
    STRICT_EXPECTED_CALL(mocked_CloseThreadpoolTimer(g_test_timer));

    // act
    process_watchdog_deinit();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

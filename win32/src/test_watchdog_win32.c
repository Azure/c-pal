// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "windows.h"

#include "c_logging/logger.h"

#include "c_pal/log_critical_and_terminate.h"

#include "c_pal/test_watchdog.h"

static PTP_TIMER g_watchdog_timer = NULL;
static uint32_t g_timeout_ms = 0;

static VOID CALLBACK on_watchdog_timeout(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer)
{
    (void)instance;
    (void)context;
    (void)timer;

    /*Codes_SRS_TEST_WATCHDOG_43_005: [ on_timer_expired shall call LogCriticalAndTerminate to terminate the process. ]*/
    LogCriticalAndTerminate("Test watchdog timeout after %" PRIu32 " ms", g_timeout_ms);
}

int test_watchdog_init(uint32_t timeout_ms)
{
    int result;

    /*Codes_SRS_TEST_WATCHDOG_43_001: [ If the watchdog is already initialized, test_watchdog_init shall fail and return a non-zero value. ]*/
    if (g_watchdog_timer != NULL)
    {
        LogError("Watchdog already initialized");
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_TEST_WATCHDOG_43_002: [ test_watchdog_init shall create a timer that expires after timeout_ms milliseconds and calls on_timer_expired. ]*/
        g_watchdog_timer = CreateThreadpoolTimer(on_watchdog_timeout, NULL, NULL);
        if (g_watchdog_timer == NULL)
        {
            /*Codes_SRS_TEST_WATCHDOG_43_003: [ If creating the timer fails, test_watchdog_init shall fail and return a non-zero value. ]*/
            LogError("CreateThreadpoolTimer failed");
            result = MU_FAILURE;
        }
        else
        {
            g_timeout_ms = timeout_ms;

            /* Convert timeout to 100-nanosecond intervals (negative for relative time) */
            ULARGE_INTEGER ularge_due_time;
            ularge_due_time.QuadPart = (ULONGLONG)-((int64_t)timeout_ms * 10000);
            FILETIME filetime_due_time;
            filetime_due_time.dwHighDateTime = ularge_due_time.HighPart;
            filetime_due_time.dwLowDateTime = ularge_due_time.LowPart;

            SetThreadpoolTimer(g_watchdog_timer, &filetime_due_time, 0, 0);

            /*Codes_SRS_TEST_WATCHDOG_43_004: [ On success, test_watchdog_init shall return zero. ]*/
            result = 0;
        }
    }

    return result;
}

void test_watchdog_deinit(void)
{
    /*Codes_SRS_TEST_WATCHDOG_43_006: [ If the watchdog is not initialized, test_watchdog_deinit shall return. ]*/
    if (g_watchdog_timer == NULL)
    {
        /* not initialized, nothing to do */
    }
    else
    {
        /*Codes_SRS_TEST_WATCHDOG_43_007: [ test_watchdog_deinit shall cancel and delete the timer. ]*/
        /* Cancel the timer by setting due time to NULL */
        SetThreadpoolTimer(g_watchdog_timer, NULL, 0, 0);
        WaitForThreadpoolTimerCallbacks(g_watchdog_timer, TRUE);
        CloseThreadpoolTimer(g_watchdog_timer);

        /*Codes_SRS_TEST_WATCHDOG_43_008: [ test_watchdog_deinit shall mark the watchdog as not initialized. ]*/
        g_watchdog_timer = NULL;
        g_timeout_ms = 0;
    }
}

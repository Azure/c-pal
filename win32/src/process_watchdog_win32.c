// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "windows.h"

#include "c_logging/logger.h"

#include "c_pal/interlocked.h"
#include "c_pal/log_critical_and_terminate.h"

#include "c_pal/process_watchdog.h"

#define WATCHDOG_STATE_NOT_INITIALIZED 0
#define WATCHDOG_STATE_INITIALIZED 1

static volatile_atomic int32_t g_watchdog_state = WATCHDOG_STATE_NOT_INITIALIZED;
static PTP_TIMER g_watchdog_timer = NULL;
static uint32_t g_timeout_ms = 0;

static VOID CALLBACK on_watchdog_timeout(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer)
{
    (void)instance;
    (void)context;
    (void)timer;

    /*Codes_SRS_PROCESS_WATCHDOG_43_006: [ on_timer_expired shall call LogCriticalAndTerminate to terminate the process. ]*/
    LogCriticalAndTerminate("Process watchdog timeout after %" PRIu32 " ms", g_timeout_ms);
}

int process_watchdog_init(uint32_t timeout_ms)
{
    int result;

    /*Codes_SRS_PROCESS_WATCHDOG_43_001: [ process_watchdog_init shall call interlocked_compare_exchange to atomically check if the watchdog is already initialized. ]*/
    int32_t previous_state = interlocked_compare_exchange(&g_watchdog_state, WATCHDOG_STATE_INITIALIZED, WATCHDOG_STATE_NOT_INITIALIZED);

    /*Codes_SRS_PROCESS_WATCHDOG_43_002: [ If the watchdog is already initialized, process_watchdog_init shall fail and return a non-zero value. ]*/
    if (previous_state != WATCHDOG_STATE_NOT_INITIALIZED)
    {
        LogError("Watchdog already initialized");
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_PROCESS_WATCHDOG_43_003: [ process_watchdog_init shall create a timer that expires after timeout_ms milliseconds and calls on_timer_expired. ]*/
        /*Codes_SRS_PROCESS_WATCHDOG_WIN32_43_001: [ process_watchdog_init shall call CreateThreadpoolTimer to create a timer with on_timer_expired as the callback. ]*/
        g_watchdog_timer = CreateThreadpoolTimer(on_watchdog_timeout, NULL, NULL);

        /*Codes_SRS_PROCESS_WATCHDOG_WIN32_43_002: [ If CreateThreadpoolTimer fails, process_watchdog_init shall fail and return a non-zero value. ]*/
        if (g_watchdog_timer == NULL)
        {
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

            /*Codes_SRS_PROCESS_WATCHDOG_WIN32_43_003: [ process_watchdog_init shall call SetThreadpoolTimer to start the timer with the specified timeout_ms. ]*/
            SetThreadpoolTimer(g_watchdog_timer, &filetime_due_time, 0, 0);

            /*Codes_SRS_PROCESS_WATCHDOG_43_005: [ On success, process_watchdog_init shall return zero. ]*/
            result = 0;
            goto all_ok;
        }

        /*Codes_SRS_PROCESS_WATCHDOG_43_004: [ If creating the timer fails, process_watchdog_init shall call interlocked_exchange to atomically mark the watchdog as not initialized and return a non-zero value. ]*/
        (void)interlocked_exchange(&g_watchdog_state, WATCHDOG_STATE_NOT_INITIALIZED);
    }

all_ok:
    return result;
}

void process_watchdog_deinit(void)
{
    /*Codes_SRS_PROCESS_WATCHDOG_43_007: [ process_watchdog_deinit shall call interlocked_compare_exchange to atomically check if the watchdog is initialized and mark it as not initialized. ]*/
    int32_t previous_state = interlocked_compare_exchange(&g_watchdog_state, WATCHDOG_STATE_NOT_INITIALIZED, WATCHDOG_STATE_INITIALIZED);

    /*Codes_SRS_PROCESS_WATCHDOG_43_008: [ If the watchdog is not initialized, process_watchdog_deinit shall return. ]*/
    if (previous_state != WATCHDOG_STATE_INITIALIZED)
    {
        /* not initialized, nothing to do */
    }
    else
    {
        /*Codes_SRS_PROCESS_WATCHDOG_43_009: [ process_watchdog_deinit shall cancel and delete the timer. ]*/
        /*Codes_SRS_PROCESS_WATCHDOG_WIN32_43_004: [ process_watchdog_deinit shall call SetThreadpoolTimer with NULL to stop the timer. ]*/
        SetThreadpoolTimer(g_watchdog_timer, NULL, 0, 0);

        /*Codes_SRS_PROCESS_WATCHDOG_WIN32_43_005: [ process_watchdog_deinit shall call WaitForThreadpoolTimerCallbacks to wait for any pending callbacks to complete. ]*/
        WaitForThreadpoolTimerCallbacks(g_watchdog_timer, TRUE);

        /*Codes_SRS_PROCESS_WATCHDOG_WIN32_43_006: [ process_watchdog_deinit shall call CloseThreadpoolTimer to delete the timer. ]*/
        CloseThreadpoolTimer(g_watchdog_timer);

        g_watchdog_timer = NULL;
        g_timeout_ms = 0;
    }
}

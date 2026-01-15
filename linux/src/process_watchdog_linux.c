// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <signal.h>
#include <time.h>

#include "c_logging/logger.h"

#include "c_pal/interlocked.h"
#include "c_pal/log_critical_and_terminate.h"

#include "c_pal/process_watchdog.h"

#define WATCHDOG_STATE_NOT_INITIALIZED 0
#define WATCHDOG_STATE_INITIALIZED 1

static volatile_atomic int32_t g_watchdog_state = WATCHDOG_STATE_NOT_INITIALIZED;
static timer_t g_watchdog_timer = NULL;
static uint32_t g_timeout_ms = 0;

static void on_watchdog_timeout(union sigval timer_data)
{
    (void)timer_data;

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
        LogError("Watchdog already initialized, uint32_t timeout_ms=%" PRIu32, timeout_ms);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_PROCESS_WATCHDOG_43_003: [ process_watchdog_init shall create a timer that expires after timeout_ms milliseconds and calls on_timer_expired. ]*/
        /*Codes_SRS_PROCESS_WATCHDOG_LINUX_43_001: [ process_watchdog_init shall call timer_create with CLOCK_MONOTONIC and SIGEV_THREAD notification to create a timer with on_timer_expired as the callback. ]*/
        struct sigevent sigev = {0};
        sigev.sigev_notify = SIGEV_THREAD;
        sigev.sigev_notify_function = on_watchdog_timeout;
        sigev.sigev_value.sival_ptr = NULL;

        if (timer_create(CLOCK_MONOTONIC, &sigev, &g_watchdog_timer) != 0)
        {
            /*Codes_SRS_PROCESS_WATCHDOG_LINUX_43_002: [ If timer_create fails, process_watchdog_init shall fail and return a non-zero value. ]*/
            LogError("timer_create failed, uint32_t timeout_ms=%" PRIu32, timeout_ms);
            result = MU_FAILURE;
        }
        else
        {
            g_timeout_ms = timeout_ms;

            struct itimerspec its = {0};
            its.it_value.tv_sec = timeout_ms / 1000;
            its.it_value.tv_nsec = (timeout_ms % 1000) * 1000000;
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 0;

            /*Codes_SRS_PROCESS_WATCHDOG_LINUX_43_003: [ process_watchdog_init shall call timer_settime to start the timer with the specified timeout_ms. ]*/
            if (timer_settime(g_watchdog_timer, 0, &its, NULL) != 0)
            {
                LogError("timer_settime failed, uint32_t timeout_ms=%" PRIu32, timeout_ms);
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_PROCESS_WATCHDOG_43_005: [ On success, process_watchdog_init shall return zero. ]*/
                result = 0;
                goto all_ok;
            }

            /*Codes_SRS_PROCESS_WATCHDOG_LINUX_43_004: [ If timer_settime fails, process_watchdog_init shall call timer_delete to clean up and return a non-zero value. ]*/
            (void)timer_delete(g_watchdog_timer);
            g_watchdog_timer = NULL;
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
        /*Codes_SRS_PROCESS_WATCHDOG_LINUX_43_005: [ process_watchdog_deinit shall call timer_delete to stop and delete the timer. ]*/
        (void)timer_delete(g_watchdog_timer);

        g_watchdog_timer = NULL;
        g_timeout_ms = 0;
    }
}

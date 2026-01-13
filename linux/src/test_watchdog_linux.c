// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <signal.h>
#include <time.h>

#include "c_logging/logger.h"

#include "c_pal/log_critical_and_terminate.h"

#include "c_pal/test_watchdog.h"

static timer_t g_watchdog_timer = NULL;
static int g_timer_initialized = 0;
static uint32_t g_timeout_ms = 0;

static void on_watchdog_timeout(union sigval timer_data)
{
    (void)timer_data;

    /*Codes_SRS_TEST_WATCHDOG_43_005: [ on_timer_expired shall call LogCriticalAndTerminate to terminate the process. ]*/
    LogCriticalAndTerminate("Test watchdog timeout after %" PRIu32 " ms", g_timeout_ms);
}

int test_watchdog_init(uint32_t timeout_ms)
{
    int result;

    /*Codes_SRS_TEST_WATCHDOG_43_001: [ If the watchdog is already initialized, test_watchdog_init shall fail and return a non-zero value. ]*/
    if (g_timer_initialized != 0)
    {
        LogError("Watchdog already initialized, uint32_t timeout_ms=%" PRIu32, timeout_ms);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_TEST_WATCHDOG_43_002: [ test_watchdog_init shall create a timer that expires after timeout_ms milliseconds and calls on_timer_expired. ]*/
        struct sigevent sigev = {0};
        sigev.sigev_notify = SIGEV_THREAD;
        sigev.sigev_notify_function = on_watchdog_timeout;
        sigev.sigev_value.sival_ptr = NULL;

        if (timer_create(CLOCK_REALTIME, &sigev, &g_watchdog_timer) != 0)
        {
            /*Codes_SRS_TEST_WATCHDOG_43_003: [ If creating the timer fails, test_watchdog_init shall fail and return a non-zero value. ]*/
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

            if (timer_settime(g_watchdog_timer, 0, &its, NULL) != 0)
            {
                LogError("timer_settime failed, uint32_t timeout_ms=%" PRIu32, timeout_ms);
            }
            else
            {
                g_timer_initialized = 1;

                /*Codes_SRS_TEST_WATCHDOG_43_004: [ On success, test_watchdog_init shall return zero. ]*/
                result = 0;
                goto all_ok;
            }

            (void)timer_delete(g_watchdog_timer);
            g_watchdog_timer = NULL;
            result = MU_FAILURE;
        }
    }

all_ok:
    return result;
}

void test_watchdog_deinit(void)
{
    /*Codes_SRS_TEST_WATCHDOG_43_006: [ If the watchdog is not initialized, test_watchdog_deinit shall return. ]*/
    if (g_timer_initialized == 0)
    {
        /* not initialized, nothing to do */
    }
    else
    {
        /*Codes_SRS_TEST_WATCHDOG_43_007: [ test_watchdog_deinit shall cancel and delete the timer. ]*/
        /* Cancel the timer by setting to zero */
        struct itimerspec its_cancel = {0};
        (void)timer_settime(g_watchdog_timer, 0, &its_cancel, NULL);
        timer_delete(g_watchdog_timer);

        /*Codes_SRS_TEST_WATCHDOG_43_008: [ test_watchdog_deinit shall mark the watchdog as not initialized. ]*/
        g_watchdog_timer = NULL;
        g_timer_initialized = 0;
        g_timeout_ms = 0;
    }
}

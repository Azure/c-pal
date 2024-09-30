// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <time.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"
#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

#include "c_pal/timer.h"

typedef struct TIMER_HANDLE_DATA_TAG
{
    struct timespec start_time;
}TIMER_HANDLE_DATA;

TIMER_HANDLE timer_create_new(void)
{
    TIMER_HANDLE result;

    /* Codes_SRS_TIMER_LINUX_01_001: [ timer_create_new shall allocate memory for a new timer. ]*/
    result = malloc(sizeof(TIMER_HANDLE_DATA));
    if (result == NULL)
    {
        /* Codes_SRS_TIMER_LINUX_01_003: [ If any error occurs, timer_create_new shall return NULL. ]*/
        LogError("failure in malloc(sizeof(TIMER_HANDLE_DATA)=%zu);", sizeof(TIMER_HANDLE_DATA));
        /*return as is*/
    }
    else
    {
        /* Codes_SRS_TIMER_01_001: [ timer_create_new shall create a new timer and on success return a non-NULL handle to it. ]*/
        /* Codes_SRS_TIMER_LINUX_01_002: [ timer_create_new shall call clock_gettime with CLOCK_MONOTONIC to obtain the initial timer value. ]*/
        if (clock_gettime(CLOCK_MONOTONIC, &result->start_time) != 0)
        {
            /* Codes_SRS_TIMER_LINUX_01_003: [ If any error occurs, timer_create_new shall return NULL. ]*/
            LogError("clock_gettime failed");
        }
        else
        {
            goto all_ok;
        }

        free(result);
    }

    result = NULL;

all_ok:
    return result;
}

void timer_destroy(TIMER_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_TIMER_01_002: [ If timer is NULL, timer_destroy shall return. ]*/
        /* Codes_SRS_TIMER_LINUX_01_004: [ If timer is NULL, timer_destroy shall return. ]*/
        LogError("Invalid arguments: TIMER_HANDLE timer=%p", timer);
    }
    else
    {
        /* Codes_SRS_TIMER_01_003: [ Otherwise, timer_destroy shall free the memory associated with timer. ]*/
        /* Codes_SRS_TIMER_LINUX_01_005: [ Otherwise, timer_destroy shall free the memory associated with timer. ]*/
        free(timer);
    }
}

int timer_start(TIMER_HANDLE timer)
{
    int result;

    if (timer == NULL)
    {
        /* Codes_SRS_TIMER_01_004: [ If timer is NULL, timer_start shall fail and return a non-zero value. ]*/
        /* Codes_SRS_TIMER_LINUX_01_007: [ If timer is NULL, timer_start shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: TIMER_HANDLE timer=%p", timer);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_TIMER_01_005: [ Otherwise, timer_start shall record the start time (used for computing the elapsed time). ]*/
        /* Codes_SRS_TIMER_LINUX_01_006: [ timer_start shall call clock_gettime with CLOCK_MONOTONIC to obtain the start timer value. ]*/
        if (clock_gettime(CLOCK_MONOTONIC, &timer->start_time) != 0)
        {
            /* Codes_SRS_TIMER_LINUX_01_022: [ If any error occurs, timer_start shall return a non-zero value. ]*/
            LogError("clock_gettime failed");
            result = MU_FAILURE;
        }
        else
        {
            /* all ok */
            result = 0;
        }
    }

    return result;
}

double timer_get_elapsed(TIMER_HANDLE timer)
{
    double result;
    if (timer == NULL)
    {
        /* Codes_SRS_TIMER_01_006: [ If timer is NULL, timer_get_elapsed shall return -1. ]*/
        /* Codes_SRS_TIMER_LINUX_01_008: [ If timer is NULL, timer_get_elapsed shall return -1. ]*/
        LogError("Invalid arguments: TIMER_HANDLE timer=%p", timer);
        result = -1.0;
    }
    else
    {
        struct timespec stop_time;

        /* Codes_SRS_TIMER_01_007: [ Otherwise timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
        /* Codes_SRS_TIMER_LINUX_01_009: [ Otherwise timer_get_elapsed shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
        if (clock_gettime(CLOCK_MONOTONIC, &stop_time) < 0)
        {
            /* Codes_SRS_TIMER_LINUX_01_020: [ If any error occurs, timer_get_elapsed shall fail and return -1. ]*/
            LogError("clock_gettime failed");
            result = -1.0;
        }
        else
        {
            /* all ok, compute the diff */
            /* Codes_SRS_TIMER_LINUX_01_010: [ timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
            if (stop_time.tv_nsec < timer->start_time.tv_sec)
            {
                result = (double)(stop_time.tv_sec - timer->start_time.tv_sec - 1);
                result += (double)(stop_time.tv_nsec - timer->start_time.tv_nsec + 1000000000) / 1000000000;
            }
            else
            {
                result = (double)(stop_time.tv_sec - timer->start_time.tv_sec);
                result += (double)(stop_time.tv_nsec - timer->start_time.tv_nsec) / 1000000000;
            }
        }
    }
    return result;
}

double timer_get_elapsed_ms(TIMER_HANDLE timer)
{
    double result;
    if (timer == NULL)
    {
        /* Codes_SRS_TIMER_01_008: [ if timer is NULL, timer_get_elapsed_ms shall return -1. ]*/
        /* Codes_SRS_TIMER_LINUX_01_011: [ If timer is NULL, timer_get_elapsed_ms shall return -1. ]*/
        LogError("Invalid arguments: TIMER_HANDLE timer=%p", timer);
        result = -1.0;
    }
    else
    {
        struct timespec stop_time;

        /* Codes_SRS_TIMER_01_009: [ Otherwise timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
        /* Codes_SRS_TIMER_LINUX_01_012: [ Otherwise timer_get_elapsed_ms shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
        if (clock_gettime(CLOCK_MONOTONIC, &stop_time) < 0)
        {
            /* Codes_SRS_TIMER_LINUX_01_021: [ If any error occurs, timer_get_elapsed_ms shall fail and return -1. ]*/
            LogError("clock_gettime failed");
            result = -1.0;
        }
        else
        {
            /* all ok, compute the diff */
            /* Codes_SRS_TIMER_LINUX_01_013: [ timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
            if (stop_time.tv_nsec < timer->start_time.tv_sec)
            {
                result = (double)(stop_time.tv_sec - timer->start_time.tv_sec - 1) * 1000;
                result += (double)(stop_time.tv_nsec - timer->start_time.tv_nsec + 1000000000) / 1000000;
            }
            else
            {
                result = (double)(stop_time.tv_sec - timer->start_time.tv_sec) * 1000;
                result += (double)(stop_time.tv_nsec - timer->start_time.tv_nsec) / 1000000;
            }
        }
    }
    return result;
}

double timer_global_get_elapsed_s(void)
{
    double result;
    struct timespec elapsed_time;

    /* Codes_SRS_TIMER_27_001: [ timer_global_get_elapsed_s shall return the elapsed time in seconds from a start time in the past. ]*/
    /* Codes_SRS_TIMER_LINUX_27_001: [ timer_global_get_elapsed_s shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
    if (clock_gettime(CLOCK_MONOTONIC, &elapsed_time) < 0)
    {
        /* Codes_SRS_TIMER_LINUX_27_003: [ If any error occurs, timer_global_get_elapsed_s shall return -1. ]*/
        LogError("clock_gettime failed");
        result = -1.0;
    }
    else
    {
        /* Codes_SRS_TIMER_LINUX_27_002: [ timer_global_get_elapsed_s shall return the elapsed time in seconds (as returned by clock_gettime). ]*/
        result = (double)elapsed_time.tv_sec;
        result += (double)elapsed_time.tv_nsec / 1000000000;
    }
    return result;
}

double timer_global_get_elapsed_ms(void)
{
    double result;
    struct timespec elapsed_time;

    /* Codes_SRS_TIMER_01_010: [ timer_global_get_elapsed_ms shall return the elapsed time in milliseconds from a start time in the past. ]*/
    /* Codes_SRS_TIMER_LINUX_01_014: [ timer_global_get_elapsed_ms shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value. ]*/
    if (clock_gettime(CLOCK_MONOTONIC, &elapsed_time) < 0)
    {
        /* Codes_SRS_TIMER_LINUX_01_018: [ If any error occurs, timer_global_get_elapsed_ms shall return -1. ]*/
        LogError("clock_gettime failed");
        result = -1.0;
    }
    else
    {
        /* Codes_SRS_TIMER_LINUX_01_015: [ timer_global_get_elapsed_ms shall return the elapsed time in milliseconds (as returned by clock_gettime). ]*/
        result = (double)elapsed_time.tv_sec * 1000;
        result += (double)elapsed_time.tv_nsec / 1000000;
    }
    return result;
}

double timer_global_get_elapsed_us(void)
{
    double result;
    struct timespec elapsed_time;

    /* Codes_SRS_TIMER_01_011: [ timer_global_get_elapsed_us shall return the elapsed time in microseconds from a start time in the past. ]*/
    /* Codes_SRS_TIMER_LINUX_01_016: [ timer_global_get_elapsed_us shall call clock_gettime with CLOCK_MONOTONIC to obtain the current timer value.  ]*/
    if (clock_gettime(CLOCK_MONOTONIC, &elapsed_time) < 0)
    {
        /* Codes_SRS_TIMER_LINUX_01_019: [ If any error occurs, timer_global_get_elapsed_us shall return -1. ]*/
        LogError("clock_gettime failed");
        result = -1.0;
    }
    else
    {
        /* Codes_SRS_TIMER_LINUX_01_017: [ timer_global_get_elapsed_us shall return the elapsed time in microseconds (as returned by clock_gettime). ]*/
        result = (double)elapsed_time.tv_sec * 1000000;
        result += (double)elapsed_time.tv_nsec / 1000;
    }
    return result;
}

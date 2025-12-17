// Copyright (C) Microsoft Corporation. All rights reserved.

#include "windows.h"

#include "c_logging/logger.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/timer.h"

typedef struct TIMER_HANDLE_DATA_TAG
{
    LARGE_INTEGER freq;
    LARGE_INTEGER startTime;
}TIMER_HANDLE_DATA;

TIMER_HANDLE timer_create_new(void)
{
    TIMER_HANDLE result;
    /*Codes_SRS_TIMER_WIN32_88_001: [ timer_create_new shall allocate memory for a new timer handle. ]*/
    result = malloc(sizeof(TIMER_HANDLE_DATA));
    /*Codes_SRS_TIMER_WIN32_88_002: [ If memory allocation fails, timer_create_new shall return NULL. ]*/
    if (result == NULL)
    {
        LogError("failure in malloc(sizeof(TIMER_HANDLE_DATA)=%zu);", sizeof(TIMER_HANDLE_DATA));
        /*return as is*/
    }
    else
    {
        /* Codes_SRS_TIMER_01_001: [ timer_create_new shall create a new timer and on success return a non-NULL handle to it. ]*/
        /*Codes_SRS_TIMER_WIN32_88_003: [ timer_create_new shall call QueryPerformanceFrequency to obtain the timer frequency. ]*/
        (void)QueryPerformanceFrequency(&(result->freq)); /* from MSDN:  On systems that run Windows XP or later, the function will always succeed and will thus never return zero.*/
                                                          /*return as is*/
        /*Codes_SRS_TIMER_WIN32_88_004: [ timer_create_new shall call QueryPerformanceCounter to record the start time. ]*/
        (void)QueryPerformanceCounter(&(result->startTime)); /*from MSDN:  On systems that run Windows XP or later, the function will always succeed and will thus never return zero.*/
    }
    return result;
}

int timer_start(TIMER_HANDLE timer)
{
    int result;
    if (timer == NULL)
    {
        /* Codes_SRS_TIMER_01_004: [ If timer is NULL, timer_start shall fail and return a non-zero value. ]*/
        LogError("invalid arg TIMER_HANDLE timer=%p", timer);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_TIMER_01_005: [ Otherwise, timer_start shall record the start time (used for computing the elapsed time). ]*/
        /*Codes_SRS_TIMER_WIN32_88_005: [ timer_start shall call QueryPerformanceCounter to record the current time as the start time. ]*/
        (void)QueryPerformanceCounter(&(timer->startTime)); /*from MSDN:  On systems that run Windows XP or later, the function will always succeed and will thus never return zero.*/
        result = 0;
    }
    return result;
}

double timer_get_elapsed(TIMER_HANDLE timer)
{
    double result;
    if (timer == NULL)
    {
        /* Codes_SRS_TIMER_01_006: [ If timer is NULL, timer_get_elapsed shall return -1. ]*/
        /* Codes_SRS_TIMER_01_008: [ if timer is NULL, timer_get_elapsed_ms shall return -1. ]*/
        LogError("invalid arg TIMER_HANDLE timer=%p", timer);
        result = -1.0;
    }
    else
    {
        /* Codes_SRS_TIMER_01_007: [ Otherwise timer_get_elapsed shall return the time difference in seconds between the current time and the start time of the timer. ]*/
        /*Codes_SRS_TIMER_WIN32_88_006: [ timer_get_elapsed shall call QueryPerformanceCounter to get the current time. ]*/
        LARGE_INTEGER stopTime;
        (void)QueryPerformanceCounter(&stopTime);
        /*Codes_SRS_TIMER_WIN32_88_007: [ timer_get_elapsed shall compute and return the elapsed time in seconds. ]*/
        result = ((double)(stopTime.QuadPart - timer->startTime.QuadPart) / (double)timer->freq.QuadPart);
    }
    return result;
}

double timer_get_elapsed_ms(TIMER_HANDLE timer)
{
    double result = timer_get_elapsed(timer);
    /* Codes_SRS_TIMER_01_009: [ Otherwise timer_get_elapsed_ms shall return the time difference in milliseconds between the current time and the start time of the timer. ]*/
    /*Codes_SRS_TIMER_WIN32_88_008: [ timer_get_elapsed_ms shall return the elapsed time in milliseconds by multiplying the result of timer_get_elapsed by 1000. ]*/
    return result < 0 ? result : result * 1000;
}

void timer_destroy(TIMER_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_TIMER_01_002: [ If timer is NULL, timer_destroy shall return. ]*/
        LogError("invalid arg TIMER_HANDLE timer=%p", timer);
    }
    else
    {
        /* Codes_SRS_TIMER_01_003: [ Otherwise, timer_destroy shall free the memory associated with timer. ]*/
        /*Codes_SRS_TIMER_WIN32_88_009: [ timer_destroy shall free the memory allocated for the timer handle. ]*/
        free(timer);
    }
}

static LARGE_INTEGER g_freq;
static volatile LONG g_timer_state = 0; /*0 - not "created", 1 - "created", "2" - creating*/

void global_timer_state_reset(void)
{
    (void)InterlockedExchange(&g_timer_state, 0);
}

/*returns a time in seconds since "some" start.*/
double timer_global_get_elapsed_s(void)
{
    while (InterlockedCompareExchange(&g_timer_state, 2, 0) != 1)
    {
        (void)QueryPerformanceFrequency(&g_freq); /*from MSDN:  On systems that run Windows XP or later, the function will always succeed and will thus never return zero.*/
        (void)InterlockedExchange(&g_timer_state, 1);
    }

    /* Codes_SRS_TIMER_27_001: [ timer_global_get_elapsed_s shall return the elapsed time in seconds from a start time in the past. ]*/
    /*Codes_SRS_TIMER_WIN32_88_010: [ timer_global_get_elapsed_s shall call QueryPerformanceFrequency and QueryPerformanceCounter to compute the elapsed time in seconds. ]*/
    LARGE_INTEGER now;
    (void)QueryPerformanceCounter(&now);
    return (double)now.QuadPart/ (double)g_freq.QuadPart;
}

/*returns a time in ms since "some" start.*/
double timer_global_get_elapsed_ms(void)
{
    while (InterlockedCompareExchange(&g_timer_state, 2, 0) != 1)
    {
        (void)QueryPerformanceFrequency(&g_freq); /*from MSDN:  On systems that run Windows XP or later, the function will always succeed and will thus never return zero.*/
        (void)InterlockedExchange(&g_timer_state, 1);
    }

    /* Codes_SRS_TIMER_01_010: [ timer_global_get_elapsed_ms shall return the elapsed time in milliseconds from a start time in the past. ]*/
    /*Codes_SRS_TIMER_WIN32_88_011: [ timer_global_get_elapsed_ms shall call QueryPerformanceFrequency and QueryPerformanceCounter to compute the elapsed time in milliseconds. ]*/
    LARGE_INTEGER now;
    (void)QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1000.0 / (double)g_freq.QuadPart;
}

/*returns a time in us since "some" start.*/
double timer_global_get_elapsed_us(void)
{
    while (InterlockedCompareExchange(&g_timer_state, 2, 0) != 1)
    {
        (void)QueryPerformanceFrequency(&g_freq); /*from MSDN:  On systems that run Windows XP or later, the function will always succeed and will thus never return zero.*/
        (void)InterlockedExchange(&g_timer_state, 1);
    }

    /* Codes_SRS_TIMER_01_011: [ timer_global_get_elapsed_us shall return the elapsed time in microseconds from a start time in the past. ]*/
    /*Codes_SRS_TIMER_WIN32_88_012: [ timer_global_get_elapsed_us shall call QueryPerformanceFrequency and QueryPerformanceCounter to compute the elapsed time in microseconds. ]*/
    LARGE_INTEGER now;
    (void)QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1000000.0 / (double)g_freq.QuadPart;
}

// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <stdlib.h>

#include "windows.h"

#include "c_logging/logger.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"

typedef struct WORK_ITEM_CONTEXT_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
} WORK_ITEM_CONTEXT;

typedef struct THREADPOOL_WORK_ITEM_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    PTP_WORK ptp_work;
} THREADPOOL_WORK_ITEM, *THREADPOOL_WORK_ITEM_HANDLE;

typedef struct TIMER_INSTANCE_TAG
{
    PTP_TIMER timer;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
} TIMER_INSTANCE;

typedef struct THREADPOOL_TAG
{
    EXECUTION_ENGINE_HANDLE execution_engine;
    PTP_POOL pool;
    TP_CALLBACK_ENVIRON tp_environment;
    PTP_CLEANUP_GROUP tp_cleanup_group;
} THREADPOOL;

THANDLE_TYPE_DEFINE(THREADPOOL);

static VOID NTAPI on_io_cancelled(PVOID ObjectContext, PVOID CleanupContext)
{
    (void)ObjectContext;
    (void)CleanupContext;
}

static VOID CALLBACK on_work_callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work)
{
    if (context == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_035: [ If context is NULL, on_work_callback shall return. ]*/
        LogError("Invalid arguments: PTP_CALLBACK_INSTANCE instance=%p, PVOID context=%p, PTP_WORK work=%p",
            instance, context, work);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_036: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
        WORK_ITEM_CONTEXT* work_item_context = (WORK_ITEM_CONTEXT*)context;

        /* Codes_SRS_THREADPOOL_WIN32_01_037: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
        work_item_context->work_function(work_item_context->work_function_context);

        /* Codes_SRS_THREADPOOL_WIN32_01_038: [ on_work_callback shall call CloseThreadpoolWork. ]*/
        CloseThreadpoolWork(work);

        /* Codes_SRS_THREADPOOL_WIN32_01_039: [ on_work_callback shall free the context allocated in threadpool_schedule_work. ]*/
        free(context);
    }
}

static VOID CALLBACK on_work_callback_v2(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work)
{
    if (context == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_05_001: [ If context is NULL, on_work_callback_v2 shall Log Message with severity CRITICAL and terminate. ]*/
        LogCriticalAndTerminate("Invalid arguments: PTP_CALLBACK_INSTANCE instance=%p, PVOID context=%p, PTP_WORK work=%p",
            instance, context, work);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_05_002: [ Otherwise context shall be used as the context created in threadpool_create_work_item. ]*/
        THREADPOOL_WORK_ITEM_HANDLE work_item_context = (THREADPOOL_WORK_ITEM_HANDLE)context;
        /* Codes_SRS_THREADPOOL_WIN32_05_003: [ The work_function callback passed to threadpool_create_work_item shall be called with the work_function_context as an argument. work_function_context was set inside the threadpool_create_work_item as an argument to CreateThreadpoolContext. ]*/
        work_item_context->work_function(work_item_context->work_function_context);
    }
}

static void threadpool_dispose(THREADPOOL* threadpool)
{
    /* Codes_SRS_THREADPOOL_WIN32_01_030: [ threadpool_close shall wait for any executing callbacks by calling CloseThreadpoolCleanupGroupMembers, passing FALSE as fCancelPendingCallbacks. ]*/
    CloseThreadpoolCleanupGroupMembers(threadpool->tp_cleanup_group, FALSE, NULL);

    /* Codes_SRS_THREADPOOL_WIN32_01_032: [ threadpool_close shall close the threadpool cleanup group by calling CloseThreadpoolCleanupGroup. ]*/
    CloseThreadpoolCleanupGroup(threadpool->tp_cleanup_group);

    /* Codes_SRS_THREADPOOL_WIN32_01_033: [ threadpool_close shall destroy the thread pool environment created in threadpool_create. ]*/
    DestroyThreadpoolEnvironment(&threadpool->tp_environment);

    /* Codes_SRS_THREADPOOL_WIN32_42_028: [ threadpool_dispose shall decrement the reference count on the execution_engine. ]*/
    execution_engine_dec_ref(threadpool->execution_engine);
}

THANDLE(THREADPOOL) threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THREADPOOL* result = NULL;
    /* Codes_SRS_THREADPOOL_WIN32_01_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
    if (execution_engine == NULL)
    {
        LogError("EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_001: [ threadpool_create shall allocate a new threadpool object and on success shall return a non-NULL handle. ]*/
        result = THANDLE_MALLOC(THREADPOOL)(threadpool_dispose);
        if (result == NULL)
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_003: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC(THREADPOOL) failed");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_025: [ threadpool_create shall obtain the PTP_POOL from the execution engine by calling execution_engine_win32_get_threadpool. ]*/
            result->pool = execution_engine_win32_get_threadpool(execution_engine);
            if (result->pool == NULL)
            {
                /* Codes_SRS_THREADPOOL_WIN32_01_003: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                LogError("execution_engine_win32_get_threadpool failed");
            }
            else
            {
                /* Codes_SRS_THREADPOOL_WIN32_01_026: [ threadpool_create shall initialize a thread pool environment by calling InitializeThreadpoolEnvironment. ]*/
                InitializeThreadpoolEnvironment(&result->tp_environment);

                /* Codes_SRS_THREADPOOL_WIN32_01_027: [ threadpool_create shall set the thread pool for the environment to the pool obtained from the execution engine by calling SetThreadpoolCallbackPool. ]*/
                SetThreadpoolCallbackPool(&result->tp_environment, result->pool);

                /* Codes_SRS_THREADPOOL_WIN32_01_028: [ threadpool_create shall create a threadpool cleanup group by calling CreateThreadpoolCleanupGroup. ]*/
                result->tp_cleanup_group = CreateThreadpoolCleanupGroup();
                if (result->tp_cleanup_group == NULL)
                {
                    /* Codes_SRS_THREADPOOL_WIN32_01_003: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                    LogLastError("CreateThreadpoolCleanupGroup failed");
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_WIN32_01_029: [ threadpool_create shall associate the cleanup group with the just created environment by calling SetThreadpoolCallbackCleanupGroup. ]*/
                    SetThreadpoolCallbackCleanupGroup(&result->tp_environment, result->tp_cleanup_group, on_io_cancelled);

                    /* Codes_SRS_THREADPOOL_WIN32_42_027: [ threadpool_create shall increment the reference count on the execution_engine. ]*/
                    execution_engine_inc_ref(execution_engine);
                    result->execution_engine = execution_engine;

                    goto all_ok;
                }
                DestroyThreadpoolEnvironment(&result->tp_environment);
            }
            THANDLE_FREE(THREADPOOL)(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

int threadpool_open(THANDLE(THREADPOOL) threadpool)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_WIN32_01_008: [ If threadpool is NULL, threadpool_open shall fail and return a non-zero value. ]*/
        threadpool == NULL
    )
    {
        LogError("THREADPOOL_HANDLE threadpool=%p", threadpool);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_012: [ On success, threadpool_open shall return 0. ]*/
        result = 0;
    }
    return result;
}

void threadpool_close(THANDLE(THREADPOOL) threadpool)
{
    if (threadpool == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_05_019: [ If threadpool is NULL, threadpool_close shall return. ]*/
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool=%p", threadpool);
    }
    else
    {
        // do_nothing.
    }
}

THREADPOOL_WORK_ITEM_HANDLE threadpool_create_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, PVOID work_function_context)
{
    THREADPOOL_WORK_ITEM_HANDLE work_item_context = NULL;

    /* Codes_SRS_THREADPOOL_WIN32_01_022: [ work_function_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_THREADPOOL_WIN32_05_004: [ If threadpool is NULL, threadpool_create_work_item shall fail and return a NULL value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_WIN32_05_005: [ If work_function is NULL, threadpool_create_work_item shall fail and return a NULL value. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p, THREADPOOL_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            threadpool, work_function, work_function_context);
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        /* Codes_SRS_THREADPOOL_WIN32_05_006: [ Otherwise threadpool_create_work_item shall allocate a context work_item_context of type THREADPOOL_WORK_ITEM_HANDLE where work_function, work_function_context, and ptp_work shall be saved. ]*/
        work_item_context = malloc(sizeof(THREADPOOL_WORK_ITEM));
        if (work_item_context == NULL)
        {
            /* Codes_SRS_THREADPOOL_WIN32_05_007: [ If any error occurs, threadpool_create_work_item shall fail and return a NULL value. ]*/
            LogError("malloc failed");
        }
        else
        {
            work_item_context->work_function = work_function;
            work_item_context->work_function_context = work_function_context;

            /* Codes_SRS_THREADPOOL_WIN32_05_008: [ threadpool_create_work_item shall create work_item_context member variable ptp_work of type PTP_WORK by calling CreateThreadpoolWork to set the callback function as on_work_callback_v2. ]*/
            work_item_context->ptp_work = CreateThreadpoolWork(on_work_callback_v2, work_item_context, &threadpool_ptr->tp_environment);
            /* Codes_SRS_THREADPOOL_WIN32_05_009: [ If there are no errors then this work_item_context of type THREADPOOL_WORK_ITEM_HANDLE would be returned indicating a succcess to the caller. ]*/
            if (work_item_context->ptp_work == NULL)
            {
                /* Codes_SRS_THREADPOOL_WIN32_05_010: [ If any error occurs, threadpool_create_work_item shall fail, free the newly created context and return a NULL value. ]*/
                LogError("CreateThreadpoolWork failed");
                free(work_item_context);
                work_item_context = NULL;
            }
        }
    }
    return work_item_context;
}

int threadpool_schedule_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_ITEM_HANDLE work_item_context)
{
    int result = MU_FAILURE;

    if (
        /* Codes_SRS_THREADPOOL_WIN32_05_011: [ If threadpool is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_WIN32_05_012: [ If work_item_context is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
        (work_item_context == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p", threadpool);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_05_013: [ threadpool_schedule_work_item shall call SubmitThreadpoolWork to submit the work item for execution. ]*/
        SubmitThreadpoolWork(work_item_context->ptp_work);
        result = 0;
    }
    return result;
}

void threadpool_destroy_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_ITEM_HANDLE work_item_context)
{
    if (
        /* Codes_SRS_THREADPOOL_WIN32_05_014: [ If threadpool is NULL, threadpool_destroy_work_item shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_WIN32_05_015: [ If work_item_context is NULL, threadpool_destroy_work_item shall fail and not do anything before returning. ]*/
        (work_item_context == NULL)
        )
    {
        LogError("Invalid arguments: Work Item Context is NULL.");
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_05_016: [ threadpool_destroy_work_item shall call WaitForThreadpoolWorkCallbacks to wait on all outstanding tasks being scheduled on this ptp_work. ]*/
        WaitForThreadpoolWorkCallbacks(work_item_context->ptp_work, false);
        /* Codes_SRS_THREADPOOL_WIN32_05_017: [ threadpool_destroy_work_item shall call CloseThreadpoolWork to close ptp_work. ]*/
        CloseThreadpoolWork(work_item_context->ptp_work);
        /* Codes_SRS_THREADPOOL_WIN32_05_018: [ threadpool_destroy_work_item shall free the work_item_context. ]*/
        free(work_item_context);
    }
}

int threadpool_schedule_work(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    int result;

    /* Codes_SRS_THREADPOOL_WIN32_01_022: [ work_function_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_THREADPOOL_WIN32_01_020: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_WIN32_01_021: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p, THREADPOOL_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            threadpool, work_function, work_function_context);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        /* Codes_SRS_THREADPOOL_WIN32_01_023: [ Otherwise threadpool_schedule_work shall allocate a context where work_function and context shall be saved. ]*/
        WORK_ITEM_CONTEXT* work_item_context = malloc(sizeof(WORK_ITEM_CONTEXT));
        if (work_item_context == NULL)
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_024: [ If any error occurs, threadpool_schedule_work shall fail and return a non-zero value. ]*/
            LogError("malloc failed");
            result = MU_FAILURE;
        }
        else
        {
            work_item_context->work_function = work_function;
            work_item_context->work_function_context = work_function_context;

            /* Codes_SRS_THREADPOOL_WIN32_01_034: [ threadpool_schedule_work shall call CreateThreadpoolWork to schedule execution the callback while passing to it the on_work_callback function and the newly created context. ]*/
            PTP_WORK ptp_work = CreateThreadpoolWork(on_work_callback, work_item_context, &threadpool_ptr->tp_environment);
            if (ptp_work == NULL)
            {
                /* Codes_SRS_THREADPOOL_WIN32_01_024: [ If any error occurs, threadpool_schedule_work shall fail and return a non-zero value. ]*/
                LogError("CreateThreadpoolWork failed");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_THREADPOOL_WIN32_01_041: [ threadpool_schedule_work shall call SubmitThreadpoolWork to submit the work item for execution. ]*/
                SubmitThreadpoolWork(ptp_work);
                result = 0;
                goto all_ok;
            }

            free(work_item_context);
        }
    }

all_ok:
    return result;
}

static VOID CALLBACK on_timer_callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer)
{
    if (context == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_42_016: [ If context is NULL, on_work_callback shall return. ]*/
        LogError("Invalid args: PTP_CALLBACK_INSTANCE instance = %p, PVOID context = %p, PTP_TIMER timer = %p",
            instance, context, timer);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_42_017: [ Otherwise context shall be used as the context created in threadpool_schedule_work. ]*/
        TIMER_INSTANCE_HANDLE timer_instance = context;

        /* Codes_SRS_THREADPOOL_WIN32_42_018: [ The work_function callback passed to threadpool_schedule_work shall be called, passing to it the work_function_context argument passed to threadpool_schedule_work. ]*/
        timer_instance->work_function(timer_instance->work_function_context);
    }
}

static void threadpool_internal_set_timer(PTP_TIMER tp_timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    ULARGE_INTEGER ularge_due_time;
    ularge_due_time.QuadPart = (ULONGLONG)-((int64_t)start_delay_ms * 10000);
    FILETIME filetime_due_time;
    filetime_due_time.dwHighDateTime = ularge_due_time.HighPart;
    filetime_due_time.dwLowDateTime = ularge_due_time.LowPart;
    SetThreadpoolTimer(tp_timer, &filetime_due_time, timer_period_ms, 0);
}

static void threadpool_internal_cancel_timer_and_wait(PTP_TIMER tp_timer)
{
    SetThreadpoolTimer(tp_timer, NULL, 0, 0);
    WaitForThreadpoolTimerCallbacks(tp_timer, TRUE);
}

int threadpool_timer_start(THANDLE(THREADPOOL) threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context, TIMER_INSTANCE_HANDLE* timer_handle)
{
    int result;

    /* Codes_SRS_THREADPOOL_WIN32_42_004: [ work_function_context shall be allowed to be NULL. ]*/
    if (
        /* Codes_SRS_THREADPOOL_WIN32_42_001: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        threadpool == NULL ||
        /* Codes_SRS_THREADPOOL_WIN32_42_002: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        work_function == NULL ||
        /* Codes_SRS_THREADPOOL_WIN32_42_003: [ If timer_handle is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        timer_handle == NULL
        )
    {
        LogError("Invalid args: THANDLE(THREADPOOL) threadpool = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 ", THREADPOOL_WORK_FUNCTION work_function = %p, void* work_function_context = %p, TIMER_INSTANCE_HANDLE* timer_handle = %p",
            threadpool, start_delay_ms, timer_period_ms, work_function, work_function_context, timer_handle);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        /* Codes_SRS_THREADPOOL_WIN32_42_005: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_context in it. ]*/
        TIMER_INSTANCE_HANDLE timer_temp = malloc(sizeof(TIMER_INSTANCE));

        if (timer_temp == NULL)
        {
            /* Codes_SRS_THREADPOOL_WIN32_42_008: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
            LogError("malloc(%zu) failed for TIMER_INSTANCE_HANDLE", sizeof(TIMER_INSTANCE));
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_WIN32_42_006: [ threadpool_timer_start shall call CreateThreadpoolTimer to schedule execution the callback while passing to it the on_timer_callback function and the newly created context. ]*/
            PTP_TIMER tp_timer = CreateThreadpoolTimer(on_timer_callback, timer_temp, &threadpool_ptr->tp_environment);
            if (tp_timer == NULL)
            {
                /* Codes_SRS_THREADPOOL_WIN32_42_008: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
                LogError("CreateThreadpoolTimer failed");
                result = MU_FAILURE;
            }
            else
            {
                timer_temp->timer = tp_timer;
                timer_temp->work_function = work_function;
                timer_temp->work_function_context = work_function_context;

                /* Codes_SRS_THREADPOOL_WIN32_42_007: [ threadpool_timer_start shall call SetThreadpoolTimer, passing negative start_delay_ms as pftDueTime, timer_period_ms as msPeriod, and 0 as msWindowLength. ]*/
                threadpool_internal_set_timer(tp_timer, start_delay_ms, timer_period_ms);

                /* Codes_SRS_THREADPOOL_WIN32_42_009: [ threadpool_timer_start shall return the allocated handle in timer_handle. ]*/
                *timer_handle = timer_temp;
                timer_temp = NULL;

                /* Codes_SRS_THREADPOOL_WIN32_42_010: [ threadpool_timer_start shall succeed and return 0. ]*/
                result = 0;
            }

            if (timer_temp != NULL)
            {
                free(timer_temp);
            }
        }
    }

    return result;
}

int threadpool_timer_restart(TIMER_INSTANCE_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    int result;

    if (
        /* Codes_SRS_THREADPOOL_WIN32_42_019: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
        timer == NULL
        )
    {
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 "",
            timer, start_delay_ms, timer_period_ms);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_42_022: [ threadpool_timer_restart shall call SetThreadpoolTimer, passing negative start_delay_ms as pftDueTime, timer_period_ms as msPeriod, and 0 as msWindowLength. ]*/
        threadpool_internal_set_timer(timer->timer, start_delay_ms, timer_period_ms);

        /* Codes_SRS_THREADPOOL_WIN32_42_023: [ threadpool_timer_restart shall succeed and return 0. ]*/
        result = 0;
    }

    return result;
}

void threadpool_timer_cancel(TIMER_INSTANCE_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_42_024: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p", timer);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_42_025: [ threadpool_timer_cancel shall call SetThreadpoolTimer with NULL for pftDueTime and 0 for msPeriod and msWindowLength to cancel ongoing timers. ]*/
        /* Codes_SRS_THREADPOOL_WIN32_42_026: [ threadpool_timer_cancel shall call WaitForThreadpoolTimerCallbacks. ]*/
        threadpool_internal_cancel_timer_and_wait(timer->timer);
    }
}

void threadpool_timer_destroy(TIMER_INSTANCE_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_42_011: [ If timer is NULL, threadpool_timer_destroy shall fail and return. ]*/
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p", timer);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_42_012: [ threadpool_timer_destroy shall call SetThreadpoolTimer with NULL for pftDueTime and 0 for msPeriod and msWindowLength to cancel ongoing timers. ]*/
        /* Codes_SRS_THREADPOOL_WIN32_42_013: [ threadpool_timer_destroy shall call WaitForThreadpoolTimerCallbacks. ]*/
        threadpool_internal_cancel_timer_and_wait(timer->timer);

        /* Codes_SRS_THREADPOOL_WIN32_42_014: [ threadpool_timer_destroy shall call CloseThreadpoolTimer. ]*/
        CloseThreadpoolTimer(timer->timer);

        /* Codes_SRS_THREADPOOL_WIN32_42_015: [ threadpool_timer_destroy shall free all resources in timer. ]*/
        free(timer);
    }
}

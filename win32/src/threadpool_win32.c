// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <stdlib.h>

#include "windows.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadpool.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"

#define THREADPOOL_WIN32_STATE_VALUES \
    THREADPOOL_WIN32_STATE_CLOSED, \
    THREADPOOL_WIN32_STATE_OPENING, \
    THREADPOOL_WIN32_STATE_OPEN, \
    THREADPOOL_WIN32_STATE_CLOSING

MU_DEFINE_ENUM(THREADPOOL_WIN32_STATE, THREADPOOL_WIN32_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(THREADPOOL_WIN32_STATE, THREADPOOL_WIN32_STATE_VALUES)


typedef struct WORK_ITEM_CONTEXT_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
} WORK_ITEM_CONTEXT;

typedef struct TIMER_INSTANCE_TAG
{
    PTP_TIMER timer;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
} TIMER_INSTANCE;

typedef struct THREADPOOL_TAG
{
    EXECUTION_ENGINE_HANDLE execution_engine;
    volatile LONG state;
    PTP_POOL pool;
    TP_CALLBACK_ENVIRON tp_environment;
    PTP_CLEANUP_GROUP tp_cleanup_group;
    volatile LONG pending_api_calls;
} THREADPOOL;

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

static void internal_close(THREADPOOL_HANDLE threadpool)
{
    do
    {
        LONG current_pending_api_calls = InterlockedAdd(&threadpool->pending_api_calls, 0);
        if (current_pending_api_calls == 0)
        {
            break;
        }

        (void)WaitOnAddress(&threadpool->pending_api_calls, &current_pending_api_calls, sizeof(current_pending_api_calls), INFINITE);
    } while (1);

    /* Codes_SRS_THREADPOOL_WIN32_01_030: [ threadpool_close shall wait for any executing callbacks by calling CloseThreadpoolCleanupGroupMembers, passing FALSE as fCancelPendingCallbacks. ]*/
    CloseThreadpoolCleanupGroupMembers(threadpool->tp_cleanup_group, FALSE, NULL);

    /* Codes_SRS_THREADPOOL_WIN32_01_032: [ threadpool_close shall close the threadpool cleanup group by calling CloseThreadpoolCleanupGroup. ]*/
    CloseThreadpoolCleanupGroup(threadpool->tp_cleanup_group);

    /* Codes_SRS_THREADPOOL_WIN32_01_033: [ threadpool_close shall destroy the thread pool environment created in threadpool_open_async. ]*/
    DestroyThreadpoolEnvironment(&threadpool->tp_environment);

    (void)InterlockedExchange(&threadpool->state, (LONG)THREADPOOL_WIN32_STATE_CLOSED);
    WakeByAddressSingle((PVOID)&threadpool->state);
}

THREADPOOL_HANDLE threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THREADPOOL_HANDLE result;

    /* Codes_SRS_THREADPOOL_WIN32_01_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
    if (execution_engine == NULL)
    {
        LogError("EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_001: [ threadpool_create shall allocate a new threadpool object and on success shall return a non-NULL handle. ]*/
        result = malloc(sizeof(THREADPOOL));
        if (result == NULL)
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_003: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
            LogError("malloc failed");
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
                /* Codes_SRS_THREADPOOL_WIN32_42_027: [ threadpool_create shall increment the reference count on the execution_engine. ]*/
                execution_engine_inc_ref(execution_engine);
                result->execution_engine = execution_engine;

                (void)InterlockedExchange(&result->pending_api_calls, 0);
                (void)InterlockedExchange(&result->state, (LONG)THREADPOOL_WIN32_STATE_CLOSED);

                goto all_ok;
            }

            free(result);
        }
    }

    result = NULL;

all_ok:
    return result;
}

void threadpool_destroy(THREADPOOL_HANDLE threadpool)
{
    if (threadpool == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_004: [ If threadpool is NULL, threadpool_destroy shall return. ]*/
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool=%p", threadpool);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_006: [ While threadpool is OPENING or CLOSING, threadpool_destroy shall wait for the open to complete either successfully or with error. ]*/
        do
        {
            LONG current_state = InterlockedCompareExchange(&threadpool->state, (LONG)THREADPOOL_WIN32_STATE_CLOSING, (LONG)THREADPOOL_WIN32_STATE_OPEN);

            if (current_state == (LONG)THREADPOOL_WIN32_STATE_OPEN)
            {
                /* Codes_SRS_THREADPOOL_WIN32_01_007: [ threadpool_destroy shall perform an implicit close if threadpool is OPEN. ]*/
                internal_close(threadpool);
                break;
            }
            else if (current_state == (LONG)THREADPOOL_WIN32_STATE_CLOSED)
            {
                break;
            }

            (void)WaitOnAddress(&threadpool->state, &current_state, sizeof(current_state), INFINITE);
        } while (1);

        /* Codes_SRS_THREADPOOL_WIN32_42_028: [ threadpool_destroy shall decrement the reference count on the execution_engine. ]*/
        execution_engine_dec_ref(threadpool->execution_engine);

        /* Codes_SRS_THREADPOOL_WIN32_01_005: [ Otherwise, threadpool_destroy shall free all resources associated with threadpool. ]*/
        free(threadpool);
    }
}

int threadpool_open(THREADPOOL_HANDLE threadpool)
{
    int result;

    if (
        /* Codes_SRS_THREADPOOL_WIN32_01_008: [ If threadpool is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
        threadpool == NULL
    )
    {
        LogError("THREADPOOL_HANDLE threadpool=%p", threadpool);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_011: [ Otherwise, threadpool_open_async shall switch the state to OPENING. ]*/
        LONG current_state = InterlockedCompareExchange(&threadpool->state, THREADPOOL_WIN32_STATE_OPENING, THREADPOOL_WIN32_STATE_CLOSED);
        if (current_state != THREADPOOL_WIN32_STATE_CLOSED)
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_013: [ If threadpool is already OPEN or OPENING, threadpool_open_async shall fail and return a non-zero value. ]*/
            LogError("Open called in state %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_WIN32_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_026: [ threadpool_open_async shall initialize a thread pool environment by calling InitializeThreadpoolEnvironment. ]*/
            InitializeThreadpoolEnvironment(&threadpool->tp_environment);

            /* Codes_SRS_THREADPOOL_WIN32_01_027: [ threadpool_open_async shall set the thread pool for the environment to the pool obtained from the execution engine by calling SetThreadpoolCallbackPool. ]*/
            SetThreadpoolCallbackPool(&threadpool->tp_environment, threadpool->pool);

            /* Codes_SRS_THREADPOOL_WIN32_01_028: [ threadpool_open_async shall create a threadpool cleanup group by calling CreateThreadpoolCleanupGroup. ]*/
            threadpool->tp_cleanup_group = CreateThreadpoolCleanupGroup();
            if (threadpool->tp_cleanup_group == NULL)
            {
                /* Codes_SRS_THREADPOOL_WIN32_01_040: [ If any error occurrs, threadpool_open_async shall fail and return a non-zero value. ]*/
                LogLastError("CreateThreadpoolCleanupGroup failed");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_THREADPOOL_WIN32_01_029: [ threadpool_open_async shall associate the cleanup group with the just created environment by calling SetThreadpoolCallbackCleanupGroup. ]*/
                SetThreadpoolCallbackCleanupGroup(&threadpool->tp_environment, threadpool->tp_cleanup_group, on_io_cancelled);

                /* Codes_SRS_THREADPOOL_WIN32_01_015: [ threadpool_open_async shall set the state to OPEN. ]*/
                (void)InterlockedExchange(&threadpool->state, (LONG)THREADPOOL_WIN32_STATE_OPEN);
                WakeByAddressSingle((PVOID)&threadpool->state);

                /* Codes_SRS_THREADPOOL_WIN32_01_012: [ On success, threadpool_open_async shall return 0. ]*/
                result = 0;

                goto all_ok;
            }

            DestroyThreadpoolEnvironment(&threadpool->tp_environment);

            (void)InterlockedExchange(&threadpool->state, (LONG)THREADPOOL_WIN32_STATE_CLOSED);
            WakeByAddressSingle((PVOID)&threadpool->state);
        }
    }

all_ok:
    return result;
}

void threadpool_close(THREADPOOL_HANDLE threadpool)
{
    if (threadpool == NULL)
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_016: [ If threadpool is NULL, threadpool_close shall return. ]*/
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool=%p", threadpool);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_017: [ Otherwise, threadpool_close shall switch the state to CLOSING. ]*/
        THREADPOOL_WIN32_STATE current_state;
        if ((current_state = InterlockedCompareExchange(&threadpool->state, (LONG)THREADPOOL_WIN32_STATE_CLOSING, (LONG)THREADPOOL_WIN32_STATE_OPEN)) != (LONG)THREADPOOL_WIN32_STATE_OPEN)
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_019: [ If threadpool is not OPEN, threadpool_close shall return. ]*/
            LogWarning("Not open, current state = %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_WIN32_STATE, current_state));
        }
        else
        {
            internal_close(threadpool);
        }
    }
}

int threadpool_schedule_work(THREADPOOL_HANDLE threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
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
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool=%p, THREADPOOL_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            threadpool, work_function, work_function_context);
        result = MU_FAILURE;
    }
    else
    {
        (void)InterlockedIncrement(&threadpool->pending_api_calls);

        THREADPOOL_WIN32_STATE state = InterlockedAdd(&threadpool->state, 0);
        if (state != (LONG)THREADPOOL_WIN32_STATE_OPEN)
        {
            LogWarning("Bad state: %" PRI_MU_ENUM, MU_ENUM_VALUE(THREADPOOL_WIN32_STATE, state));
            result = MU_FAILURE;
        }
        else
        {
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
                PTP_WORK ptp_work = CreateThreadpoolWork(on_work_callback, work_item_context, &threadpool->tp_environment);
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

                    (void)InterlockedDecrement(&threadpool->pending_api_calls);
                    WakeByAddressSingle((PVOID)&threadpool->pending_api_calls);

                    result = 0;

                    goto all_ok;
                }

                free(work_item_context);
            }
        }

        (void)InterlockedDecrement(&threadpool->pending_api_calls);
        WakeByAddressSingle((PVOID)&threadpool->pending_api_calls);
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

int threadpool_timer_start(THREADPOOL_HANDLE threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context, TIMER_INSTANCE_HANDLE* timer_handle)
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
        LogError("Invalid args: THREADPOOL_HANDLE threadpool = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 ", THREADPOOL_WORK_FUNCTION work_function = %p, void* work_function_context = %p, TIMER_INSTANCE_HANDLE* timer_handle = %p",
            threadpool, start_delay_ms, timer_period_ms, work_function, work_function_context, timer_handle);
        result = MU_FAILURE;
    }
    else
    {
        (void)InterlockedIncrement(&threadpool->pending_api_calls);

        THREADPOOL_WIN32_STATE state = InterlockedAdd(&threadpool->state, 0);
        if (state != (LONG)THREADPOOL_WIN32_STATE_OPEN)
        {
            LogWarning("Bad state: %" PRI_MU_ENUM, MU_ENUM_VALUE(THREADPOOL_WIN32_STATE, state));
            result = MU_FAILURE;
        }
        else
        {
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
                PTP_TIMER tp_timer = CreateThreadpoolTimer(on_timer_callback, timer_temp, &threadpool->tp_environment);

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

        (void)InterlockedDecrement(&threadpool->pending_api_calls);
        WakeByAddressSingle((PVOID)&threadpool->pending_api_calls);
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

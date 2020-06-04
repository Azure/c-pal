// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include "windows.h"
#include "azure_c_logging/xlogging.h"
#include "threadpool.h"
#include "execution_engine.h"
#include "execution_engine_win32.h"

#define THREADPOOL_WIN32_STATE_VALUES \
    THREADPOOL_WIN32_STATE_CLOSED, \
    THREADPOOL_WIN32_STATE_OPENING, \
    THREADPOOL_WIN32_STATE_OPEN, \
    THREADPOOL_WIN32_STATE_CLOSING

MU_DEFINE_ENUM(THREADPOOL_WIN32_STATE, THREADPOOL_WIN32_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(THREADPOOL_WIN32_STATE, THREADPOOL_WIN32_STATE_VALUES)

MU_DEFINE_ENUM_STRINGS(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES)

typedef struct WORK_ITEM_CONTEXT_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
} WORK_ITEM_CONTEXT;

typedef struct THREADPOOL_TAG
{
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
        /* Codes_SRS_THREADPOOL_WIN32_01_006: [ While threadpool is OPENING or CLOSING, threadpool_destroy shall wait for the open to complete either succesfully or with error. ]*/
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

        /* Codes_SRS_THREADPOOL_WIN32_01_005: [ Otherwise, threadpool_destroy shall free all resources associated with threadpool. ]*/
        free(threadpool);
    }
}

int threadpool_open_async(THREADPOOL_HANDLE threadpool, ON_THREADPOOL_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;

    /* Codes_SRS_THREADPOOL_WIN32_01_010: [ on_open_complete_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_THREADPOOL_WIN32_01_008: [ If threadpool is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_WIN32_01_009: [ If on_open_complete is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
        (on_open_complete == NULL))
    {
        LogError("THREADPOOL_HANDLE threadpool=%p, ON_THREADPOOL_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            threadpool, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_WIN32_01_011: [ Otherwise, threadpool_open_async shall switch the state to OPENING. ]*/
        LONG current_state = InterlockedCompareExchange(&threadpool->state, (LONG)THREADPOOL_WIN32_STATE_OPENING, (LONG)THREADPOOL_WIN32_STATE_CLOSED);
        if (current_state != (LONG)THREADPOOL_WIN32_STATE_CLOSED)
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

                /* Codes_SRS_THREADPOOL_WIN32_01_014: [ On success, threadpool_open_async shall call on_open_complete_context shall with THREADPOOL_OPEN_OK. ]*/
                on_open_complete(on_open_complete_context, THREADPOOL_OPEN_OK);

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
        if (InterlockedCompareExchange(&threadpool->state, (LONG)THREADPOOL_WIN32_STATE_CLOSING, (LONG)THREADPOOL_WIN32_STATE_OPEN) != (LONG)THREADPOOL_WIN32_STATE_OPEN)
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_019: [ If threadpool is not OPEN, threadpool_close shall return. ]*/
            LogWarning("Not open");
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

        if (InterlockedAdd(&threadpool->state, 0) != (LONG)THREADPOOL_WIN32_STATE_OPEN)
        {
            LogWarning("Not open");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_WIN32_01_023: [ Otherwise threadpool_schedule_work shall allocate a context where work_function and context shall be saved. ]*/
            WORK_ITEM_CONTEXT* work_item_context = (WORK_ITEM_CONTEXT*)malloc(sizeof(WORK_ITEM_CONTEXT));
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

                /* Codes_SRS_THREADPOOL_WIN32_01_034: [ threadpool_schedule_work shall call CreateThreadpoolWork to schedule executiong the callback while apssing to it the on_work_callback function and the newly created context. ]*/
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

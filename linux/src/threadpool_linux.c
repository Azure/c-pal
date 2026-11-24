// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/sync.h"
#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#include "c_pal/threadpool.h"

typedef struct THREADPOOL_TAG
{
    EXECUTION_ENGINE_HANDLE execution_engine;
    volatile_atomic int32_t outstanding_callback_count;
} THREADPOOL;

THANDLE_TYPE_DEFINE(THREADPOOL);

typedef struct THREADPOOL_WORK_ITEM_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work;
} THREADPOOL_WORK_ITEM;

THANDLE_TYPE_DEFINE(THREADPOOL_WORK_ITEM);

typedef struct THREADPOOL_TIMER_TAG
{
    THREADPOOL* threadpool;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer;
    SRW_LOCK_LL timer_lock;
} THREADPOOL_TIMER;

THANDLE_TYPE_DEFINE(THREADPOOL_TIMER);

typedef struct SCHEDULED_WORK_CONTEXT_TAG
{
    THREADPOOL* threadpool;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work;
} SCHEDULED_WORK_CONTEXT;

static void threadpool_dispose(THREADPOOL* threadpool)
{
    /* Codes_SRS_THREADPOOL_LINUX_42_006: [ threadpool_dispose shall wait for the count of outstanding callbacks to reach 0 by calling InterlockedHL_WaitForValue. ]*/
    if (InterlockedHL_WaitForValue(&threadpool->outstanding_callback_count, 0, UINT32_MAX) != INTERLOCKED_HL_OK)
    {
        LogError("InterlockedHL_WaitForValue failed");
    }
    else
    {
        // no callback of the threadpool is outstanding
    }

    /* Codes_SRS_THREADPOOL_LINUX_42_007: [ threadpool_dispose shall decrement the reference count on the execution engine by calling execution_engine_dec_ref. ]*/
    execution_engine_dec_ref(threadpool->execution_engine);
}

THANDLE(THREADPOOL) threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THREADPOOL* result;

    if (execution_engine == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_001: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_002: [ threadpool_create shall allocate a new threadpool object with threadpool_dispose as dispose function. ]*/
        result = THANDLE_MALLOC(THREADPOOL)(threadpool_dispose);
        if (result == NULL)
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_005: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC(THREADPOOL) failed");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_003: [ threadpool_create shall increment the reference count on execution_engine by calling execution_engine_inc_ref. ]*/
            execution_engine_inc_ref(execution_engine);

            /* Codes_SRS_THREADPOOL_LINUX_42_104: [ threadpool_create shall store execution_engine in the threadpool object. ]*/
            result->execution_engine = execution_engine;

            /* Codes_SRS_THREADPOOL_LINUX_42_004: [ threadpool_create shall set the count of outstanding callbacks to 0. ]*/
            (void)interlocked_exchange(&result->outstanding_callback_count, 0);

            /* Codes_SRS_THREADPOOL_LINUX_42_105: [ threadpool_create shall succeed and return the threadpool object. ]*/
        }
    }

    return result;
}

static void on_work_item_callback(void* context)
{
    THREADPOOL_WORK_ITEM* threadpool_work_item = context;

    /* Codes_SRS_THREADPOOL_LINUX_42_014: [ on_work_item_callback shall call the work_function of the work item with its work_function_context. ]*/
    threadpool_work_item->work_function(threadpool_work_item->work_function_context);
}

static void threadpool_dispose_work_item(THREADPOOL_WORK_ITEM* threadpool_work_item)
{
    /* Codes_SRS_THREADPOOL_LINUX_42_015: [ threadpool_dispose_work_item shall wait for the pending and running callbacks of the work item by calling execution_engine_linux_wait_for_work with cancel_pending set to false. ]*/
    execution_engine_linux_wait_for_work(threadpool_work_item->work, false);

    /* Codes_SRS_THREADPOOL_LINUX_42_016: [ threadpool_dispose_work_item shall close the work object by calling execution_engine_linux_close_work. ]*/
    execution_engine_linux_close_work(threadpool_work_item->work);
}

THANDLE(THREADPOOL_WORK_ITEM) threadpool_create_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    THREADPOOL_WORK_ITEM* result;

    if (
        /* Codes_SRS_THREADPOOL_LINUX_42_008: [ If threadpool is NULL, threadpool_create_work_item shall fail and return NULL. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_42_009: [ If work_function is NULL, threadpool_create_work_item shall fail and return NULL. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p, THREADPOOL_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            threadpool, work_function, work_function_context);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_010: [ threadpool_create_work_item shall allocate a THANDLE(THREADPOOL_WORK_ITEM) with threadpool_dispose_work_item as dispose function. ]*/
        result = THANDLE_MALLOC(THREADPOOL_WORK_ITEM)(threadpool_dispose_work_item);
        if (result == NULL)
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_013: [ If any error occurs, threadpool_create_work_item shall fail, free the work item and return NULL. ]*/
            LogError("THANDLE_MALLOC(THREADPOOL_WORK_ITEM) failed");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_106: [ threadpool_create_work_item shall store work_function and work_function_context in the work item. ]*/
            result->work_function = work_function;
            result->work_function_context = work_function_context;

            /* Codes_SRS_THREADPOOL_LINUX_42_011: [ threadpool_create_work_item shall create a work object by calling execution_engine_linux_create_work with on_work_item_callback and the work item as context. ]*/
            result->work = execution_engine_linux_create_work(threadpool->execution_engine, on_work_item_callback, result);
            if (result->work == NULL)
            {
                /* Codes_SRS_THREADPOOL_LINUX_42_013: [ If any error occurs, threadpool_create_work_item shall fail, free the work item and return NULL. ]*/
                LogError("execution_engine_linux_create_work failed");
                THANDLE_FREE(THREADPOOL_WORK_ITEM)(result);
                result = NULL;
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_42_012: [ threadpool_create_work_item shall succeed and return the work item. ]*/
            }
        }
    }

    return result;
}

int threadpool_schedule_work_item(THANDLE(THREADPOOL) threadpool, THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item)
{
    int result;

    if (
        /* Codes_SRS_THREADPOOL_LINUX_42_017: [ If threadpool is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_42_018: [ If threadpool_work_item is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
        (threadpool_work_item == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p, THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item=%p", threadpool, threadpool_work_item);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_019: [ threadpool_schedule_work_item shall submit the work object of threadpool_work_item by calling execution_engine_linux_submit_work. ]*/
        execution_engine_linux_submit_work(threadpool_work_item->work);

        /* Codes_SRS_THREADPOOL_LINUX_42_020: [ threadpool_schedule_work_item shall succeed and return 0. ]*/
        result = 0;
    }

    return result;
}

static void on_scheduled_work_callback(void* context)
{
    SCHEDULED_WORK_CONTEXT* scheduled_work_context = context;
    THREADPOOL* threadpool = scheduled_work_context->threadpool;

    /* Codes_SRS_THREADPOOL_LINUX_42_029: [ on_scheduled_work_callback shall call work_function with work_function_context. ]*/
    scheduled_work_context->work_function(scheduled_work_context->work_function_context);

    /* Codes_SRS_THREADPOOL_LINUX_42_030: [ on_scheduled_work_callback shall close the work object by calling execution_engine_linux_close_work. ]*/
    execution_engine_linux_close_work(scheduled_work_context->work);

    /* Codes_SRS_THREADPOOL_LINUX_42_031: [ on_scheduled_work_callback shall free the context. ]*/
    free(scheduled_work_context);

    /* Codes_SRS_THREADPOOL_LINUX_42_032: [ on_scheduled_work_callback shall decrement the count of outstanding callbacks of the threadpool by calling interlocked_decrement. ]*/
    if (interlocked_decrement(&threadpool->outstanding_callback_count) == 0)
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_109: [ If the count of outstanding callbacks reached 0, on_scheduled_work_callback shall wake the threads waiting for it by calling wake_by_address_all. ]*/
        wake_by_address_all(&threadpool->outstanding_callback_count);
    }
    else
    {
        // other callbacks of the threadpool are outstanding
    }
}

int threadpool_schedule_work(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    int result;

    /* Codes_SRS_THREADPOOL_LINUX_42_023: [ work_function_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_THREADPOOL_LINUX_42_021: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_42_022: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
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

        /* Codes_SRS_THREADPOOL_LINUX_42_024: [ threadpool_schedule_work shall allocate a context. ]*/
        SCHEDULED_WORK_CONTEXT* scheduled_work_context = malloc(sizeof(SCHEDULED_WORK_CONTEXT));
        if (scheduled_work_context == NULL)
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_028: [ If any error occurs, threadpool_schedule_work shall free the context, fail and return a non-zero value. ]*/
            LogError("malloc(sizeof(SCHEDULED_WORK_CONTEXT)=%zu) failed", sizeof(SCHEDULED_WORK_CONTEXT));
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_107: [ threadpool_schedule_work shall store the threadpool, work_function and work_function_context in the context. ]*/
            scheduled_work_context->threadpool = threadpool_ptr;
            scheduled_work_context->work_function = work_function;
            scheduled_work_context->work_function_context = work_function_context;

            /* Codes_SRS_THREADPOOL_LINUX_42_025: [ threadpool_schedule_work shall create a work object by calling execution_engine_linux_create_work with on_scheduled_work_callback and the context. ]*/
            scheduled_work_context->work = execution_engine_linux_create_work(threadpool_ptr->execution_engine, on_scheduled_work_callback, scheduled_work_context);
            if (scheduled_work_context->work == NULL)
            {
                /* Codes_SRS_THREADPOOL_LINUX_42_028: [ If any error occurs, threadpool_schedule_work shall free the context, fail and return a non-zero value. ]*/
                LogError("execution_engine_linux_create_work failed");
                free(scheduled_work_context);
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_42_026: [ threadpool_schedule_work shall increment the count of outstanding callbacks by calling interlocked_increment. ]*/
                (void)interlocked_increment(&threadpool_ptr->outstanding_callback_count);

                /* Codes_SRS_THREADPOOL_LINUX_42_027: [ threadpool_schedule_work shall submit the work object by calling execution_engine_linux_submit_work. ]*/
                execution_engine_linux_submit_work(scheduled_work_context->work);

                /* Codes_SRS_THREADPOOL_LINUX_42_108: [ threadpool_schedule_work shall succeed and return 0. ]*/
                result = 0;
            }
        }
    }

    return result;
}

static void on_timer_callback(void* context)
{
    THREADPOOL_TIMER* timer = context;
    THREADPOOL* threadpool = timer->threadpool;

    /* Codes_SRS_THREADPOOL_LINUX_42_100: [ on_timer_callback shall increment the count of outstanding callbacks of the threadpool by calling interlocked_increment. ]*/
    (void)interlocked_increment(&threadpool->outstanding_callback_count);

    /* Codes_SRS_THREADPOOL_LINUX_42_042: [ on_timer_callback shall call the work_function of the timer with its work_function_context. ]*/
    timer->work_function(timer->work_function_context);

    /* Codes_SRS_THREADPOOL_LINUX_42_101: [ After work_function returns, on_timer_callback shall decrement the count of outstanding callbacks of the threadpool by calling interlocked_decrement, without accessing the timer, which work_function can release. ]*/
    if (interlocked_decrement(&threadpool->outstanding_callback_count) == 0)
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_111: [ If the count of outstanding callbacks reached 0, on_timer_callback shall wake the threads waiting for it by calling wake_by_address_all. ]*/
        wake_by_address_all(&threadpool->outstanding_callback_count);
    }
    else
    {
        // other callbacks of the threadpool are outstanding
    }
}

static void threadpool_timer_dispose(THREADPOOL_TIMER* timer)
{
    /* Codes_SRS_THREADPOOL_LINUX_42_102: [ threadpool_timer_dispose shall call execution_engine_linux_disassociate_current_thread_from_timer, so that it does not wait for the timer callback running on the current thread. ]*/
    execution_engine_linux_disassociate_current_thread_from_timer(timer->timer);

    /* Codes_SRS_THREADPOOL_LINUX_42_053: [ threadpool_timer_dispose shall disarm the timer by calling execution_engine_linux_cancel_timer. ]*/
    execution_engine_linux_cancel_timer(timer->timer);

    /* Codes_SRS_THREADPOOL_LINUX_42_054: [ threadpool_timer_dispose shall drop the callbacks that have not started and wait for the running callback by calling execution_engine_linux_wait_for_timer with cancel_pending set to true. ]*/
    execution_engine_linux_wait_for_timer(timer->timer, true);

    /* Codes_SRS_THREADPOOL_LINUX_42_055: [ threadpool_timer_dispose shall close the timer by calling execution_engine_linux_close_timer. ]*/
    execution_engine_linux_close_timer(timer->timer);

    /* Codes_SRS_THREADPOOL_LINUX_42_056: [ threadpool_timer_dispose shall de-initialize the timer lock by calling srw_lock_ll_deinit. ]*/
    srw_lock_ll_deinit(&timer->timer_lock);
}

THANDLE(THREADPOOL_TIMER) threadpool_timer_start(THANDLE(THREADPOOL) threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    THREADPOOL_TIMER* result;

    /* Codes_SRS_THREADPOOL_LINUX_42_035: [ work_function_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_THREADPOOL_LINUX_42_033: [ If threadpool is NULL, threadpool_timer_start shall fail and return NULL. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_42_034: [ If work_function is NULL, threadpool_timer_start shall fail and return NULL. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p, uint32_t start_delay_ms=%" PRIu32 ", uint32_t timer_period_ms=%" PRIu32 ", THREADPOOL_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            threadpool, start_delay_ms, timer_period_ms, work_function, work_function_context);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_036: [ threadpool_timer_start shall allocate a THANDLE(THREADPOOL_TIMER) with threadpool_timer_dispose as dispose function. ]*/
        result = THANDLE_MALLOC(THREADPOOL_TIMER)(threadpool_timer_dispose);
        if (result == NULL)
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_041: [ If any error occurs, threadpool_timer_start shall free the resources it allocated, fail and return NULL. ]*/
            LogError("THANDLE_MALLOC(THREADPOOL_TIMER) failed");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_42_110: [ threadpool_timer_start shall store the threadpool, work_function and work_function_context in the timer. ]*/
            result->threadpool = THANDLE_GET_T(THREADPOOL)(threadpool);
            result->work_function = work_function;
            result->work_function_context = work_function_context;

            /* Codes_SRS_THREADPOOL_LINUX_42_037: [ threadpool_timer_start shall initialize the lock guarding the timer state by calling srw_lock_ll_init. ]*/
            if (srw_lock_ll_init(&result->timer_lock) != 0)
            {
                /* Codes_SRS_THREADPOOL_LINUX_42_041: [ If any error occurs, threadpool_timer_start shall free the resources it allocated, fail and return NULL. ]*/
                LogError("srw_lock_ll_init failed");
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_42_038: [ threadpool_timer_start shall create the timer by calling execution_engine_linux_create_timer with on_timer_callback and the timer as context. ]*/
                result->timer = execution_engine_linux_create_timer(threadpool->execution_engine, on_timer_callback, result);
                if (result->timer == NULL)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_42_041: [ If any error occurs, threadpool_timer_start shall free the resources it allocated, fail and return NULL. ]*/
                    LogError("execution_engine_linux_create_timer failed");
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_LINUX_42_039: [ threadpool_timer_start shall arm the timer by calling execution_engine_linux_set_timer with start_delay_ms and timer_period_ms. ]*/
                    execution_engine_linux_set_timer(result->timer, start_delay_ms, timer_period_ms);

                    /* Codes_SRS_THREADPOOL_LINUX_42_040: [ threadpool_timer_start shall succeed and return the timer. ]*/
                    goto all_ok;
                }
                srw_lock_ll_deinit(&result->timer_lock);
            }
            THANDLE_FREE(THREADPOOL_TIMER)(result);
            result = NULL;
        }
    }

all_ok:
    return result;
}

int threadpool_timer_restart(THANDLE(THREADPOOL_TIMER) timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    int result;

    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_043: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: THANDLE(THREADPOOL_TIMER) timer=%p, uint32_t start_delay_ms=%" PRIu32 ", uint32_t timer_period_ms=%" PRIu32 "",
            timer, start_delay_ms, timer_period_ms);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL_TIMER* timer_content = THANDLE_GET_T(THREADPOOL_TIMER)(timer);

        /* Codes_SRS_THREADPOOL_LINUX_42_044: [ threadpool_timer_restart shall acquire the timer lock exclusively by calling srw_lock_ll_acquire_exclusive. ]*/
        srw_lock_ll_acquire_exclusive(&timer_content->timer_lock);

        /* Codes_SRS_THREADPOOL_LINUX_42_045: [ threadpool_timer_restart shall re-arm the timer by calling execution_engine_linux_set_timer with start_delay_ms and timer_period_ms. ]*/
        execution_engine_linux_set_timer(timer_content->timer, start_delay_ms, timer_period_ms);

        /* Codes_SRS_THREADPOOL_LINUX_42_046: [ threadpool_timer_restart shall release the timer lock by calling srw_lock_ll_release_exclusive. ]*/
        srw_lock_ll_release_exclusive(&timer_content->timer_lock);

        /* Codes_SRS_THREADPOOL_LINUX_42_047: [ threadpool_timer_restart shall succeed and return 0. ]*/
        result = 0;
    }

    return result;
}

void threadpool_timer_cancel(THANDLE(THREADPOOL_TIMER) timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_42_048: [ If timer is NULL, threadpool_timer_cancel shall return. ]*/
        LogError("Invalid arguments: THANDLE(THREADPOOL_TIMER) timer=%p", timer);
    }
    else
    {
        THREADPOOL_TIMER* timer_content = THANDLE_GET_T(THREADPOOL_TIMER)(timer);

        /* Codes_SRS_THREADPOOL_LINUX_42_103: [ Before acquiring the timer lock, threadpool_timer_cancel shall call execution_engine_linux_disassociate_current_thread_from_timer, so that it does not wait for the timer callback running on the current thread. ]*/
        // Before taking the lock: a cancel on another thread can hold it while waiting for this callback
        execution_engine_linux_disassociate_current_thread_from_timer(timer_content->timer);

        /* Codes_SRS_THREADPOOL_LINUX_42_049: [ threadpool_timer_cancel shall acquire the timer lock exclusively by calling srw_lock_ll_acquire_exclusive. ]*/
        srw_lock_ll_acquire_exclusive(&timer_content->timer_lock);

        /* Codes_SRS_THREADPOOL_LINUX_42_050: [ threadpool_timer_cancel shall disarm the timer by calling execution_engine_linux_cancel_timer. ]*/
        execution_engine_linux_cancel_timer(timer_content->timer);

        /* Codes_SRS_THREADPOOL_LINUX_42_051: [ threadpool_timer_cancel shall drop the callbacks that have not started and wait for the running callback by calling execution_engine_linux_wait_for_timer with cancel_pending set to true. ]*/
        execution_engine_linux_wait_for_timer(timer_content->timer, true);

        /* Codes_SRS_THREADPOOL_LINUX_42_052: [ threadpool_timer_cancel shall release the timer lock by calling srw_lock_ll_release_exclusive. ]*/
        srw_lock_ll_release_exclusive(&timer_content->timer_lock);
    }
}

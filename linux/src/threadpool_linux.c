// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/srw_lock.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#include "c_pal/threadpool.h"

#define TIMEOUT_MS      100
#define MAX_THREAD_COUNT    16

#define POOL_STATE_VALUES \
    POOL_STATE_UNINIT, \
    POOL_STATE_ACTIVE, \
    POOL_STATE_IDLE

MU_DEFINE_ENUM(POOL_STATE, POOL_STATE_VALUES)

typedef struct THREADPOOL_TASK_TAG
{
    THREADPOOL_WORK_FUNCTION task_func;
    void* task_param;
    struct THREADPOOL_TASK_TAG* next;
} THREADPOOL_TASK;

typedef struct THREADPOOL_TAG
{
    volatile_atomic int32_t thread_count;
    uint32_t max_thread_count;
    volatile_atomic int32_t pool_state;
    volatile_atomic int32_t quitting;
    volatile_atomic int32_t task_count;

    // Pool Queue
    THREADPOOL_TASK* task_list;
    THREADPOOL_TASK* task_list_last;

    SRW_LOCK_HANDLE srw_lock;

    THREAD_HANDLE* thread_handle_list;
    uint32_t list_index;
} THREADPOOL;

int threadpool_work_func(void* param)
{
    if (param == NULL)
    {
        LogCritical("Invalid args: param: %p", param);
    }
    else
    {
        THREADPOOL* threadpool = param;
        do
        {
            (void)interlocked_exchange(&threadpool->pool_state, POOL_STATE_IDLE);
            int32_t current_value = interlocked_add(&threadpool->task_count, 0);

            while (threadpool->task_list == NULL && interlocked_add(&threadpool->quitting, 0) != 1)
            {
                if (!wait_on_address(&threadpool->task_count, current_value, TIMEOUT_MS))
                {
                    // Timed out, loop through and try again
                }
                else
                {
                    // There was a new item added to the list, let's go
                    break;
                }
            }
            (void)interlocked_exchange(&threadpool->pool_state, POOL_STATE_ACTIVE);

            // Locking
            srw_lock_acquire_exclusive(threadpool->srw_lock);
            if (threadpool->task_list != NULL && interlocked_add(&threadpool->quitting, 0) != 1)
            {
                // Remove the task item from the list
                THREADPOOL_TASK* temp_item = threadpool->task_list;
                threadpool->task_list = temp_item->next;
                (void)interlocked_decrement(&threadpool->task_count);

                srw_lock_release_exclusive(threadpool->srw_lock);

                temp_item->task_func(temp_item->task_param);
                free(temp_item);

                srw_lock_acquire_exclusive(threadpool->srw_lock);
            }
            // Unlocking
            srw_lock_release_exclusive(threadpool->srw_lock);

        } while(interlocked_add(&threadpool->quitting, 0) != 1);
        (void)interlocked_decrement(&threadpool->thread_count);
    }
    return 0;
}

THREADPOOL_HANDLE threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THREADPOOL* result;
    if (execution_engine == 0)
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine: %p", execution_engine);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(THREADPOOL));
        if (result == NULL)
        {
            LogError("Failure allocating THREADPOOL structure");
        }
        else
        {
            const EXECUTION_ENGINE_PARAMETERS_LINUX* param = execution_engine_linux_get_parameters(execution_engine);

            result->max_thread_count = param->max_thread_count;
            if (result->max_thread_count == 0)
            {
                result->max_thread_count = MAX_THREAD_COUNT;
            }

            // Create a list of threads
            result->thread_handle_list = malloc(sizeof(THREAD_HANDLE)*result->max_thread_count);
            if (result->thread_handle_list == NULL)
            {
                LogError("Failure allocating THREAD array structure %zu", sizeof(THREAD_HANDLE)*result->max_thread_count);
            }
            else
            {
                result->list_index = 0;
                result->srw_lock = srw_lock_create(false, "");
                if (result->srw_lock == NULL)
                {
                    LogError("Failure srw_lock_create");
                }
                else
                {
                    result->thread_count = 0;
                    result->task_count = 0;
                    result->quitting = 0;
                    (void)interlocked_exchange(&result->pool_state, POOL_STATE_UNINIT);

                    result->task_list = NULL;
                    result->task_list_last = NULL;
                    goto all_ok;
                }
                free(result->thread_handle_list);
            }
            free(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

void threadpool_destroy(THREADPOOL_HANDLE threadpool)
{
    if (threadpool == NULL)
    {
        // do nothing
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool: %p", threadpool);
    }
    else
    {
        interlocked_exchange(&threadpool->quitting, 1);

        for (size_t index = 0; index < threadpool->list_index; index++)
        {
            int dont_care;
            ThreadAPI_Join(threadpool->thread_handle_list[index], &dont_care);
        }

        free(threadpool->thread_handle_list);

        srw_lock_destroy(threadpool->srw_lock);
        free(threadpool);
    }
}

int threadpool_schedule_work(THREADPOOL_HANDLE threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    int result;
    if (threadpool == NULL || work_function == NULL)
    {
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool: %p, THREADPOOL_TASK_FUNC work_function: %p", threadpool, work_function);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL_TASK* task_item = malloc(sizeof(THREADPOOL_TASK));
        if (task_item == NULL)
        {
            LogError("Failure allocating THREADPOOL_TASK structure");
            result = MU_FAILURE;
        }
        else
        {
            task_item->next = NULL;
            task_item->task_param = work_function_context;
            task_item->task_func = work_function;

            // Need to add a lock here
            srw_lock_acquire_exclusive(threadpool->srw_lock);

            if (threadpool->task_list == NULL)
            {
                threadpool->task_list = task_item;
            }
            else
            {
                threadpool->task_list_last->next = task_item;
            }
            threadpool->task_list_last = task_item;
            (void)interlocked_increment(&threadpool->task_count);

            srw_lock_release_exclusive(threadpool->srw_lock);

            // unlock

            if (interlocked_add(&threadpool->pool_state, 0) == POOL_STATE_IDLE)
            {
                // Idle signal that we have work
                wake_by_address_single(&threadpool->task_count);
                result = 0;
                goto all_ok;
            }
            else
            {
                // Check to see
                if (threadpool->thread_count < threadpool->max_thread_count)
                {
                    if (ThreadAPI_Create(&threadpool->thread_handle_list[threadpool->list_index], threadpool_work_func, threadpool) != THREADAPI_OK)
                    {
                        LogError("Failure creating thread");
                    }
                    else
                    {
                        (void)interlocked_increment(&threadpool->thread_count);
                        threadpool->list_index++;
                        result = 0;
                        goto all_ok;
                    }
                }
                else
                {
                    result = 0;
                    goto all_ok;
                }
            }
            free(task_item);
        }
        result = MU_FAILURE;
    }
all_ok:
    return result;
}

int threadpool_open_async(THREADPOOL_HANDLE threadpool, ON_THREADPOOL_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    (void)threadpool;
    (void)on_open_complete;
    (void)on_open_complete_context;
    return MU_FAILURE;
}

void threadpool_close(THREADPOOL_HANDLE threadpool)
{
    (void)threadpool;
}

int threadpool_timer_start(THREADPOOL_HANDLE threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context, TIMER_INSTANCE_HANDLE* timer_handle)
{
    (void)threadpool;
    (void)start_delay_ms;
    (void)timer_period_ms;
    (void)work_function;
    (void)work_function_context;
    (void)timer_handle;
    return MU_FAILURE;
}

int threadpool_timer_restart(TIMER_INSTANCE_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    (void)timer;
    (void)start_delay_ms;
    (void)timer_period_ms;
    return MU_FAILURE;
}

void threadpool_timer_cancel(TIMER_INSTANCE_HANDLE timer)
{
    (void)timer;
}

void threadpool_timer_destroy(TIMER_INSTANCE_HANDLE timer)
{
    (void)timer;
}

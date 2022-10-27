// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/srw_lock.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#include "c_pal/threadpool.h"

#define TIMEOUT_MS          100
#define MAX_THREAD_COUNT    1

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
    volatile_atomic int32_t quitting;
    volatile_atomic int32_t task_count;

    sem_t semaphore;

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
        struct timespec ts;
        THREADPOOL* threadpool = param;

        do
        {
            THREADPOOL_TASK* temp_item = NULL;
            // Locking
            srw_lock_acquire_exclusive(threadpool->srw_lock);
            if (threadpool->task_list != NULL && interlocked_add(&threadpool->quitting, 0) != 1)
            {
                // Remove the task item from the list
                temp_item = threadpool->task_list;
                threadpool->task_list = temp_item->next;
                (void)interlocked_decrement(&threadpool->task_count);
            }
            // Unlocking
            srw_lock_release_exclusive(threadpool->srw_lock);
            if (temp_item != NULL)
            {
                temp_item->task_func(temp_item->task_param);
                free(temp_item);
            }
            else
            {
                // No items, let's wait for new items
                do
                {
                    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
                    {
                        LogError("Failure getting time");
                    }
                    else
                    {
                        // Given number of seconds to wait
                        ts.tv_sec += 2;
                        if (sem_timedwait(&threadpool->semaphore, &ts) == 0)
                        {
                            break;
                        }
                    }
                } while (interlocked_add(&threadpool->quitting, 0) != 1);
            }
        } while (interlocked_add(&threadpool->quitting, 0) != 1);
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
            result->thread_handle_list = malloc_2(result->max_thread_count, sizeof(THREAD_HANDLE));
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
                    if (sem_init(&result->semaphore , 0 , 0) != 0)
                    {
                        LogError("Failure creating semi_init");
                    }
                    else
                    {
                        result->thread_count = 0;
                        result->task_count = 0;
                        (void)interlocked_exchange(&result->quitting, 0);

                        result->task_list = NULL;
                        result->task_list_last = NULL;
                        goto all_ok;
                    }
                    srw_lock_destroy(result->srw_lock);
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
            if (ThreadAPI_Join(threadpool->thread_handle_list[index], &dont_care) != THREADAPI_OK)
            {
                LogError("Failure joining thread");
            }
        }

        // Loop through
        while (threadpool->task_list != NULL)
        {
            THREADPOOL_TASK* temp_item = threadpool->task_list;
            threadpool->task_list = temp_item->next;
            free(temp_item);
        }

        free(threadpool->thread_handle_list);

        sem_destroy(&threadpool->semaphore);
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

            {
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
            }

            do
            {
                // Check to see if we should create another thread
                int32_t current_count = interlocked_add(&threadpool->thread_count, 0);
                if (current_count < threadpool->max_thread_count)
                {
                    if (interlocked_compare_exchange(&threadpool->thread_count, current_count+1, current_count) == current_count)
                    {
                        if (ThreadAPI_Create(&threadpool->thread_handle_list[threadpool->list_index], threadpool_work_func, threadpool) != THREADAPI_OK)
                        {
                            // Decrement the thread count since we didn't actually create the thread
                            interlocked_decrement(&threadpool->thread_count);
                            LogError("Failure creating thread");
                            break;
                        }
                        else
                        {
                            sem_post(&threadpool->semaphore);
                            threadpool->list_index++;
                            result = 0;
                            goto all_ok;
                        }
                    }
                }
                else
                {
                    sem_post(&threadpool->semaphore);
                    result = 0;
                    goto all_ok;
                }
            }
            while(1);
            free(task_item);
        }
        result = MU_FAILURE;
    }
all_ok:
    return result;
}

int threadpool_open_async(THREADPOOL_HANDLE threadpool, ON_THREADPOOL_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;
    if (
        (threadpool == NULL) ||
        (on_open_complete == NULL))
    {
        LogError("THREADPOOL_HANDLE threadpool=%p, ON_THREADPOOL_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            threadpool, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        on_open_complete(on_open_complete_context, THREADPOOL_OPEN_OK);
        result = 0;
    }
    return result;
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

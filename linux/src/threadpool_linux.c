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
#define THREAD_LIMIT        65536

#define TASK_RESULT_VALUES  \
    TASK_NOT_USED,          \
    TASK_WAITING,           \
    TASK_WORKING

MU_DEFINE_ENUM(TASK_RESULT, TASK_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(TASK_RESULT, TASK_RESULT_VALUES)

#define THREADPOOL_STATE_VALUES \
    THREADPOOL_STATE_NOT_OPEN, \
    THREADPOOL_STATE_OPEN

MU_DEFINE_ENUM(THREADPOOL_STATE, THREADPOOL_STATE_VALUES)

MU_DEFINE_ENUM_STRINGS(THREADPOOL_STATE, THREADPOOL_STATE_VALUES)

typedef struct THREADPOOL_TASK_TAG
{
    volatile_atomic int32_t task_state;
    THREADPOOL_WORK_FUNCTION task_func;
    void* task_param;
} THREADPOOL_TASK;

typedef struct THREADPOOL_TAG
{
    volatile_atomic int32_t state;
    volatile_atomic int32_t thread_count;
    uint32_t max_thread_count;
    volatile_atomic int32_t quitting;
    volatile_atomic int32_t task_count;

    sem_t semaphore;

    // Pool Array
    THREADPOOL_TASK* task_array;
    volatile_atomic int64_t insert_idx;
    volatile_atomic int64_t consume_idx;
    volatile_atomic int32_t task_array_count;

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
        int64_t current_index;

        do
        {
            do
            {
                if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
                {
                    LogError("Failure getting time from clock_gettime");
                }
                else
                {
                    // Setup the timeout for the semaphore to 2 sec
                    ts.tv_sec += 2;
                    if (sem_timedwait(&threadpool->semaphore, &ts) == 0)
                    {
                        break;
                    }
                }
            } while (interlocked_add(&threadpool->quitting, 0) != 1);

            if (interlocked_add(&threadpool->quitting, 0) != 1)
            {
                int32_t existing_count = interlocked_add(&threadpool->task_array_count, 0);

                // We have a new item
                current_index = interlocked_add_64(&threadpool->consume_idx, 0) % existing_count;

                // Is this index waiting for us to complete it
                srw_lock_acquire_shared(threadpool->srw_lock);
                TASK_RESULT curr_task_result = interlocked_compare_exchange(&threadpool->task_array[current_index].task_state, TASK_WORKING, TASK_WAITING);
                srw_lock_release_shared(threadpool->srw_lock);
                if (TASK_WAITING == curr_task_result)
                {
                    int64_t consume_pos = interlocked_compare_exchange_64(&threadpool->consume_idx, current_index+1, current_index);
                    if (consume_pos == current_index)
                    {
                        srw_lock_acquire_shared(threadpool->srw_lock);
                        THREADPOOL_TASK* temp_item = &threadpool->task_array[consume_pos];
                        srw_lock_release_shared(threadpool->srw_lock);

                        // Get the item and make sure it's valid
                        if (temp_item != NULL)
                        {
                            temp_item->task_func(temp_item->task_param);
                        }
                        // I'm done with this item mark it as complete
                        srw_lock_acquire_shared(threadpool->srw_lock);
                        interlocked_exchange(&threadpool->task_array[consume_pos].task_state, TASK_NOT_USED);
                        srw_lock_release_shared(threadpool->srw_lock);
                    }
                }
            }
        } while (interlocked_add(&threadpool->quitting, 0) != 1);
        (void)interlocked_decrement(&threadpool->thread_count);
    }
}

static int reallocate_threadpool_array(THREADPOOL* threadpool)
{
    int result;
    {
        // Lock the array here
        srw_lock_acquire_exclusive(threadpool->srw_lock);

        int32_t existing_count = interlocked_add(&threadpool->task_array_count, 0);

        // Double the items
        (void)interlocked_exchange(&threadpool->task_array_count, existing_count*2);

        // Reallocate here
        THREADPOOL_TASK* temp_array = realloc_2((void*)threadpool->task_array, existing_count*2, sizeof(THREADPOOL_TASK));
        if (temp_array == NULL)
        {
            LogError("Failure realloc_2(threadpool->task_array: %p, threadpool->task_array_count: %" PRId32", sizeof(THREADPOOL_TASK): %zu", threadpool->task_array, existing_count*2, sizeof(THREADPOOL_TASK));
            result = MU_FAILURE;
        }
        else
        {
            // Clear the newly allocated memory
            memset(temp_array+existing_count, 0, existing_count*sizeof(THREADPOOL_TASK));
            threadpool->task_array = temp_array;

            // Normalize the consumed index
            //(void)interlocked_exchange_64(&threadpool->consume_idx, threadpool->consume_idx % existing_count);
            // Make the insert index at the new items that were allocated
            (void)interlocked_exchange_64(&threadpool->insert_idx, existing_count+1);

            result = 0;
        }
        // Unlock the arrary here
        srw_lock_release_exclusive(threadpool->srw_lock);
    }
    return result;
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
                LogError("Failure malloc_2(result->max_thread_count: %" PRIu32 ", sizeof(THREAD_HANDLE)): %zu", result->max_thread_count, sizeof(THREAD_HANDLE));
            }
            else
            {
                result->task_array = malloc_2(THREAD_LIMIT, sizeof(THREADPOOL_TASK));
                if (result->task_array == NULL)
                {
                    LogError("Failure malloc_2(THREAD_LIMIT: %" PRIu32 ", sizeof(THREADPOOL_TASK)): %zu", THREAD_LIMIT, sizeof(THREADPOOL_TASK));
                }
                else
                {
                    // ensure array items are clear
                    memset(result->task_array, 0, THREAD_LIMIT*sizeof(THREADPOOL_TASK));
                    result->srw_lock = srw_lock_create(false, "threadpool_lock");
                    if (result->srw_lock == NULL)
                    {
                        LogError("Failure srw_lock_create");
                    }
                    else
                    {
                        result->task_array_count = THREAD_LIMIT;
                        result->list_index = 0;
                        if (sem_init(&result->semaphore, 0 , 0) != 0)
                        {
                            LogError("Failure creating semi_init");
                        }
                        else
                        {
                            (void)interlocked_exchange(&result->state, THREADPOOL_STATE_NOT_OPEN);
                            result->thread_count = 0;
                            result->task_count = 0;
                            (void)interlocked_exchange(&result->quitting, 0);

                            (void)interlocked_exchange_64(&result->insert_idx, 0);
                            (void)interlocked_exchange_64(&result->consume_idx, 0);

                            goto all_ok;
                        }
                        srw_lock_destroy(result->srw_lock);
                    }
                    free(result->task_array);
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

        free(threadpool->task_array);
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
        if (interlocked_add(&threadpool->state, 0) != (int32_t)THREADPOOL_STATE_OPEN)
        {
            LogWarning("Threadpool schedule work called on non-open pool");
            result = MU_FAILURE;
        }
        else
        {
            //int64_t insert_pos = (interlocked_increment_64(&threadpool->insert_idx) % threadpool->task_array_count) - 1;
            int64_t insert_pos = interlocked_exchange_64(&threadpool->insert_idx, threadpool->insert_idx+1) % threadpool->task_array_count ;

            // Do we need to reallocate or do we need to roll over
            int32_t task_state = interlocked_add(&threadpool->task_array[insert_pos].task_state, 0);
            if (task_state != TASK_NOT_USED && task_state != TASK_RESULT_INVALID)
            {
                // The first task is not complete, so we just need to reallocate and
                // go to the next item
                if (reallocate_threadpool_array(threadpool) != 0)
                {
                    LogError("Failure reallocating threadpool");
                    result = MU_FAILURE;
                }
                else
                {
                    insert_pos = interlocked_add_64(&threadpool->insert_idx, 0) - 1;
                    result = 0;
                }
            }
            else
            {
                result = 0;
            }

            if (result == 0)
            {
                srw_lock_acquire_shared(threadpool->srw_lock);
                THREADPOOL_TASK* task_item = &threadpool->task_array[insert_pos];
                task_item->task_param = work_function_context;
                task_item->task_func = work_function;
                (void)interlocked_exchange(&task_item->task_state, TASK_WAITING);
                srw_lock_release_shared(threadpool->srw_lock);

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
                result = MU_FAILURE;
            }
        }
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
        int32_t current_state = interlocked_compare_exchange(&threadpool->state, (int32_t)THREADPOOL_STATE_OPEN, (int32_t)THREADPOOL_STATE_NOT_OPEN);
        if (current_state != (int32_t)THREADPOOL_STATE_NOT_OPEN)
        {
            LogError("Not closed, cannot open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            on_open_complete(on_open_complete_context, THREADPOOL_OPEN_OK);
            result = 0;
        }
    }
    return result;
}

void threadpool_close(THREADPOOL_HANDLE threadpool)
{
    int result;
    if (threadpool == NULL)
    {
        LogError("THREADPOOL_HANDLE threadpool=%p", threadpool);
        result = MU_FAILURE;
    }
    else
    {
        int32_t current_state = interlocked_compare_exchange(&threadpool->state, (int32_t)THREADPOOL_STATE_NOT_OPEN, (int32_t)THREADPOOL_STATE_OPEN);
        if (current_state != (int32_t)THREADPOOL_STATE_NOT_OPEN)
        {
            LogError("Not closed, cannot open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_STATE, current_state));
            result = MU_FAILURE;
        }
    }
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

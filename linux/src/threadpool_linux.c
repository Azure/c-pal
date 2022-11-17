// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

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

#define DEFAULT_TASK_ARRAY_SIZE 2048

#define TASK_RESULT_VALUES  \
    TASK_NOT_USED,          \
    TASK_INITIALIZING,      \
    TASK_WAITING,           \
    TASK_WORKING

MU_DEFINE_ENUM(TASK_RESULT, TASK_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(TASK_RESULT, TASK_RESULT_VALUES)

#define THREADPOOL_STATE_VALUES \
    THREADPOOL_STATE_NOT_OPEN,  \
    THREADPOOL_STATE_OPENING,   \
    THREADPOOL_STATE_OPEN,      \
    THREADPOOL_STATE_CLOSING

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
    uint32_t max_thread_count;
    uint32_t min_thread_count;
    int32_t used_thread_count;
    volatile_atomic int32_t task_count;

    sem_t semaphore;

    // Pool Array
    THREADPOOL_TASK* task_array;
    volatile_atomic int32_t task_array_size;
    volatile_atomic int64_t insert_idx;
    volatile_atomic int64_t consume_idx;
    volatile_atomic int32_t pending_call_count;

    SRW_LOCK_HANDLE srw_lock;
    uint32_t list_index;

    THREAD_HANDLE* thread_handle_array;
} THREADPOOL;

static void internal_close(THREADPOOL_HANDLE threadpool)
{
    bool should_wait_for_transition = false;
    do
    {
        THREADPOOL_STATE current_state = interlocked_compare_exchange(&threadpool->state, THREADPOOL_STATE_CLOSING, THREADPOOL_STATE_OPEN);
        if (current_state == THREADPOOL_STATE_NOT_OPEN)
        {
            should_wait_for_transition = false;
        }
        else if (current_state == THREADPOOL_STATE_OPENING || current_state == THREADPOOL_STATE_CLOSING)
        {
            should_wait_for_transition = true;
        }
        else
        {
            int32_t value;
            while ((value = interlocked_add(&threadpool->pending_call_count, 0)) != 0)
            {
                (void)wait_on_address(&threadpool->pending_call_count, value, UINT32_MAX);
            }

            for (int32_t index = 0; index < threadpool->used_thread_count; index++)
            {
                int dont_care;
                if (ThreadAPI_Join(threadpool->thread_handle_array[index], &dont_care) != THREADAPI_OK)
                {
                    LogError("Failure joining thread number %" PRId32 "", index);
                }
            }
            (void)interlocked_exchange(&threadpool->state, THREADPOOL_STATE_NOT_OPEN);
            should_wait_for_transition = false;
        }
    } while (should_wait_for_transition);
}

static int threadpool_work_func(void* param)
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
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            {
                // Consider Log
                LogError("Failure getting time from clock_gettime");
            }
            else
            {
                // Setup the timeout for the semaphore to 2 sec
                ts.tv_sec += 2;
                if (sem_timedwait(&threadpool->semaphore, &ts) != 0)
                {
                    // Timed out
                }
                else
                {
                    THREADPOOL_WORK_FUNCTION task_func = NULL;
                    void* task_param = NULL;
                    // Is this index waiting for us to complete it
                    srw_lock_acquire_shared(threadpool->srw_lock);
                    {
                        int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);
                        // We have a new item
                        current_index = interlocked_increment_64(&threadpool->consume_idx) % existing_count;

                        TASK_RESULT curr_task_result = interlocked_compare_exchange(&threadpool->task_array[current_index].task_state, TASK_WORKING, TASK_WAITING);
                        if (TASK_WAITING == curr_task_result)
                        {
                            task_func = threadpool->task_array[current_index].task_func;
                            task_param = threadpool->task_array[current_index].task_param;
                            (void)interlocked_exchange(&threadpool->task_array[current_index].task_state, TASK_NOT_USED);
                        }
                        else
                        {
                            // Do nothing here, this should not happen, but log if it does
                            LogWarning("The impossible has happend, someone got the item %" PRId64 " task value %" PRI_MU_ENUM "", current_index, MU_ENUM_VALUE(TASK_RESULT, curr_task_result));
                        }
                    }
                    srw_lock_release_shared(threadpool->srw_lock);

                    if (task_func != NULL)
                    {
                        task_func(task_param);
                    }
                }
            }

        } while (interlocked_add(&threadpool->state, 0) != THREADPOOL_STATE_CLOSING);
    }
    return 0;
}

static int reallocate_threadpool_array(THREADPOOL* threadpool)
{
    int result;
    {
        // Lock the array here
        srw_lock_acquire_exclusive(threadpool->srw_lock);

        int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);
        int32_t new_task_array_size = existing_count*2;
        if (new_task_array_size < 0)
        {
            LogError("overflow in computation task_array_size: %" PRId32 "*2 (%" PRId32 ") > UINT32_MAX: %" PRId32 " - 1. ", existing_count, new_task_array_size, INT32_MAX);
            result = MU_FAILURE;
        }
        else
        {
            // Double the items
            (void)interlocked_exchange(&threadpool->task_array_size, new_task_array_size);

            // Reallocate here
            THREADPOOL_TASK* temp_array = realloc_2(threadpool->task_array, new_task_array_size, sizeof(THREADPOOL_TASK));
            if (temp_array == NULL)
            {
                LogError("Failure realloc_2(threadpool->task_array: %p, threadpool->task_array_size: %" PRId32", sizeof(THREADPOOL_TASK): %zu", threadpool->task_array, new_task_array_size, sizeof(THREADPOOL_TASK));
                result = MU_FAILURE;
            }
            else
            {
                // Clear the newly allocated memory
                for (int32_t index = existing_count; index < new_task_array_size; index++)
                {
                    temp_array[index].task_func = NULL;
                    temp_array[index].task_param = NULL;
                    (void)interlocked_exchange(&temp_array[index].task_state, TASK_NOT_USED);
                }
                threadpool->task_array = temp_array;

                // Ensure there are no gaps in the array
                // Consume = 2
                // Produce = 2
                // [x x x x]
                // Consume = 3
                // Produce = 2
                // [x x 0 x]
                // During resize we will have to memmove in the gap at 2
                // [x x 0 x 0 0 0 0]

                int32_t insert_pos = interlocked_add_64(&threadpool->insert_idx, 0);
                int32_t consume_pos = interlocked_add_64(&threadpool->consume_idx, 0);

                if (insert_pos == consume_pos)
                {
                    for (int32_t index = insert_pos; index < consume_pos; index++)
                    {
                        if (interlocked_add(&threadpool->task_array[index].task_state, 0) == TASK_NOT_USED)
                        {
                            // Move the place where we're putting the insert_index back because we
                            // compressing the list
                            existing_count--;
                        }
                    }
                    memmove(&threadpool->task_array[insert_pos], &threadpool->task_array[consume_pos+1], sizeof(THREADPOOL_TASK));
                }

                // Start back at zero (-1 because of the increment) for the consumed index which will
                // ensure that there are no left over items
                (void)interlocked_exchange_64(&threadpool->consume_idx, -1);
                // Make the insert index at the new items that were allocated
                (void)interlocked_exchange_64(&threadpool->insert_idx, existing_count-1);

                result = 0;
            }
        }
        // Unlock the array here
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

            result->min_thread_count = param->min_thread_count;
            result->max_thread_count = param->max_thread_count;

            // We will use min thread count and change this as we improve
            result->used_thread_count = result->min_thread_count;

            // Create a list of threads
            result->thread_handle_array = malloc_2(result->used_thread_count, sizeof(THREAD_HANDLE));
            if (result->thread_handle_array == NULL)
            {
                LogError("Failure malloc_2(result->used_thread_count: %" PRIu32 ", sizeof(THREAD_HANDLE)): %zu", result->used_thread_count, sizeof(THREAD_HANDLE));
            }
            else
            {
                result->task_array = malloc_2(DEFAULT_TASK_ARRAY_SIZE, sizeof(THREADPOOL_TASK));
                if (result->task_array == NULL)
                {
                    LogError("Failure malloc_2(DEFAULT_TASK_ARRAY_SIZE: %" PRIu32 ", sizeof(THREADPOOL_TASK)): %zu", DEFAULT_TASK_ARRAY_SIZE, sizeof(THREADPOOL_TASK));
                }
                else
                {
                    result->task_array_size = DEFAULT_TASK_ARRAY_SIZE;
                    // ensure array items are clear
                    for (int32_t index = 0; index < result->task_array_size; index++)
                    {
                        result->task_array[index].task_func = NULL;
                        result->task_array[index].task_param = NULL;
                        (void)interlocked_exchange(&result->task_array[index].task_state, TASK_NOT_USED);
                    }
                    result->srw_lock = srw_lock_create(false, "threadpool_lock");
                    if (result->srw_lock == NULL)
                    {
                        LogError("Failure srw_lock_create");
                    }
                    else
                    {
                        result->list_index = 0;
                        if (sem_init(&result->semaphore, 0 , 0) != 0)
                        {
                            LogError("Failure creating semi_init");
                        }
                        else
                        {
                            (void)interlocked_exchange(&result->state, THREADPOOL_STATE_NOT_OPEN);
                            (void)interlocked_exchange(&result->task_count, 0);
                            (void)interlocked_exchange(&result->pending_call_count, 0);

                            // Need to start the index at -1 so the first increment
                            // will start at zero
                            (void)interlocked_exchange_64(&result->insert_idx, -1);
                            (void)interlocked_exchange_64(&result->consume_idx, -1);

                            goto all_ok;
                        }
                        srw_lock_destroy(result->srw_lock);
                    }
                    free(result->task_array);
                }
                free(result->thread_handle_array);
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
        internal_close(threadpool);

        free(threadpool->task_array);
        free(threadpool->thread_handle_array);

        sem_destroy(&threadpool->semaphore);
        srw_lock_destroy(threadpool->srw_lock);
        free(threadpool);
    }
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
        int32_t current_state = interlocked_compare_exchange(&threadpool->state, THREADPOOL_STATE_OPENING, THREADPOOL_STATE_NOT_OPEN);
        if (current_state != THREADPOOL_STATE_NOT_OPEN)
        {
            LogError("Inconsistent state current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            int32_t index;
            // Create the thread for the pool here
            for (index = 0; index < threadpool->used_thread_count; index++)
            {
                if (ThreadAPI_Create(&threadpool->thread_handle_array[index], threadpool_work_func, threadpool) != THREADAPI_OK)
                {
                    LogError("Failure creating thread %" PRId32 "", index);
                    break;
                }
            }
            if (index < threadpool->used_thread_count)
            {
                // Stop the threads
                for (int32_t inner = 0; inner < index; inner++)
                {
                    int dont_care;
                    if (ThreadAPI_Join(threadpool->thread_handle_array[inner], &dont_care) != THREADAPI_OK)
                    {
                        LogError("Failure joining thread number %" PRId32 "", inner);
                    }
                }
                (void)interlocked_exchange(&threadpool->state, THREADPOOL_STATE_NOT_OPEN);
                on_open_complete(on_open_complete_context, THREADPOOL_OPEN_ERROR);
                result = MU_FAILURE;
            }
            else
            {
                (void)interlocked_exchange(&threadpool->state, THREADPOOL_STATE_OPEN);
                on_open_complete(on_open_complete_context, THREADPOOL_OPEN_OK);
                result = 0;
            }
        }
    }
    return result;
}

void threadpool_close(THREADPOOL_HANDLE threadpool)
{
    if (threadpool == NULL)
    {
        LogError("THREADPOOL_HANDLE threadpool=%p", threadpool);
    }
    else
    {
        internal_close(threadpool);
    }
}

int threadpool_schedule_work(THREADPOOL_HANDLE threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    int result;
    if (threadpool == NULL || work_function == NULL)
    {
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool: %p, THREADPOOL_WORK_FUNCTION work_function: %p, void* work_function_context: %p", threadpool, work_function, work_function_context);
        result = MU_FAILURE;
    }
    else
    {
        int32_t current_state = interlocked_add(&threadpool->state, 0);
        if (current_state != THREADPOOL_STATE_OPEN)
        {
            LogWarning("Inconsistent state current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            (void)interlocked_increment(&threadpool->pending_call_count);
            wake_by_address_single(&threadpool->pending_call_count);
            do
            {
                srw_lock_acquire_shared(threadpool->srw_lock);
                int64_t insert_pos = interlocked_increment_64(&threadpool->insert_idx) % threadpool->task_array_size;
                int32_t task_state = interlocked_compare_exchange(&threadpool->task_array[insert_pos].task_state, TASK_INITIALIZING, TASK_NOT_USED);
                if (task_state != TASK_NOT_USED)
                {
                    srw_lock_release_shared(threadpool->srw_lock);
                    // go to the next item
                    if (reallocate_threadpool_array(threadpool) != 0)
                    {
                        LogError("Failure reallocating threadpool");
                        result = MU_FAILURE;
                        break;
                    }
                    continue;
                }
                else
                {
                    THREADPOOL_TASK* task_item = &threadpool->task_array[insert_pos];
                    task_item->task_param = work_function_context;
                    task_item->task_func = work_function;
                    (void)interlocked_exchange(&task_item->task_state, TASK_WAITING);
                    srw_lock_release_shared(threadpool->srw_lock);

                    // The Sem_post unblocks 1 thread that is
                    // in the sem_wait call
                    sem_post(&threadpool->semaphore);
                    result = 0;
                    break;
                }
            } while (true);
            (void)interlocked_decrement(&threadpool->pending_call_count);
            wake_by_address_all(&threadpool->pending_call_count);
        }
    }
all_ok:
    return result;
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
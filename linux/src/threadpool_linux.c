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

#include "c_pal/threadpool.h"

#define TIMEOUT_MS      100

#define POOL_STATE_VALUES \
    POOL_STATE_UNINIT, \
    POOL_STATE_ACTIVE, \
    POOL_STATE_IDLE

MU_DEFINE_ENUM(POOL_STATE, POOL_STATE_VALUES)

typedef struct THREADPOOL_TASK_TAG
{
    THREADPOOL_TASK_FUNC task_func;
    void* task_param;
    THREAD_HANDLE thread_handle;
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
        LogError("Failure unknown user parameter encountered");
    }
    else
    {
        THREADPOOL* threadpool = (THREADPOOL*)param;
        do {
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

THREADPOOL_HANDLE threadpool_create(uint32_t thread_count)
{
    THREADPOOL* result;
    if (thread_count == 0)
    {
        LogError("Invalid arguments: uint32_t thread_count: %" PRIu32 "", thread_count);
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
            result->max_thread_count = thread_count;

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

void threadpool_destroy(THREADPOOL_HANDLE thread_handle)
{
    if (thread_handle == NULL)
    {
        // do nothing
        LogError("Invalid arguments: THREADPOOL_HANDLE thread_handle: %p", thread_handle);
    }
    else
    {
        interlocked_exchange(&thread_handle->quitting, 1);

        for (size_t index = 0; index < thread_handle->list_index; index++)
        {
            int dont_care;
            ThreadAPI_Join(thread_handle->thread_handle_list[index], &dont_care);
        }

        free(thread_handle->thread_handle_list);

        srw_lock_destroy(thread_handle->srw_lock);
        free(thread_handle);
    }
}

int threadpool_add_task(THREADPOOL_HANDLE thread_handle, THREADPOOL_TASK_FUNC task_func, void* task_arg)
{
    int result;
    if (thread_handle == NULL || task_func == NULL)
    {
        LogError("Invalid arguments: THREADPOOL_HANDLE thread_handle: %p, THREADPOOL_TASK_FUNC task_func: %p", thread_handle, task_func);
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
            task_item->task_param = task_arg;
            task_item->task_func = task_func;

            // Need to add a lock here
            srw_lock_acquire_exclusive(thread_handle->srw_lock);

            if (thread_handle->task_list == NULL)
            {
                thread_handle->task_list = task_item;
            }
            else
            {
                thread_handle->task_list_last->next = task_item;
            }
            thread_handle->task_list_last = task_item;
            (void)interlocked_increment(&thread_handle->task_count);

            srw_lock_release_exclusive(thread_handle->srw_lock);

            // unlock

            if (interlocked_add(&thread_handle->pool_state, 0) == POOL_STATE_IDLE)
            {
                // Idle signal that we have work
                wake_by_address_single(&thread_handle->task_count);
            }
            else
            {
                // Check to see
                if (thread_handle->thread_count < thread_handle->max_thread_count)
                {
                    if (ThreadAPI_Create(&thread_handle->thread_handle_list[thread_handle->list_index], threadpool_work_func, thread_handle) != THREADAPI_OK)
                    {
                        LogError("Failure creating thread");
                    }
                    else
                    {
                        (void)interlocked_increment(&thread_handle->thread_count);
                        thread_handle->list_index++;
                        result = 0;
                        goto all_ok;
                    }
                }
            }
            free(task_item);
        }
        result = MU_FAILURE;
    }
all_ok:
    return result;
}

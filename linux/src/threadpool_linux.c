// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <bits/types/__sigval_t.h>         // for __sigval_t
#include <bits/types/sigevent_t.h>         // for sigevent, sigev_notify_fun...
#include <bits/types/sigval_t.h>           // for sigval_t
#include <bits/types/struct_itimerspec.h>  // for itimerspec
#include <bits/types/timer_t.h>            // for timer_t

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

#define DEFAULT_TASK_ARRAY_SIZE         2048
#define MILLISEC_TO_NANOSEC             1000000
#define TP_SEMAPHORE_TIMEOUT_MS         (100*MILLISEC_TO_NANOSEC) // The timespec value is in nanoseconds so need to multiply to get 100 MS

#define TASK_RESULT_VALUES  \
    TASK_NOT_USED,          \
    TASK_INITIALIZING,      \
    TASK_WAITING,           \
    TASK_WORKING

MU_DEFINE_ENUM(TASK_RESULT, TASK_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(TASK_RESULT, TASK_RESULT_VALUES)

MU_DEFINE_ENUM_STRINGS(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES)

#define THREADPOOL_STATE_VALUES \
    THREADPOOL_STATE_NOT_OPEN,  \
    THREADPOOL_STATE_OPENING,   \
    THREADPOOL_STATE_OPEN,      \
    THREADPOOL_STATE_CLOSING

MU_DEFINE_ENUM(THREADPOOL_STATE, THREADPOOL_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(THREADPOOL_STATE, THREADPOOL_STATE_VALUES)

typedef struct TIMER_INSTANCE_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
    timer_t time_id;
} TIMER_INSTANCE;

typedef struct THREADPOOL_TASK_TAG
{
    volatile_atomic int32_t task_state;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
    volatile uint32_t pending_api_calls;
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

static void on_timer_callback(sigval_t timer_data)
{
    TIMER_INSTANCE* timer_instance = timer_data.sival_ptr;
    if (timer_instance == NULL)
    {
        LogError("Invalid Argument timer_instance");
    }
    else
    {
        timer_instance->work_function(timer_instance->work_function_ctx);
    }
}

static void internal_close(THREADPOOL_HANDLE threadpool)
{
    bool should_wait_for_transition = false;
    do
    {
        THREADPOOL_STATE current_state = interlocked_compare_exchange(&threadpool->state, THREADPOOL_STATE_CLOSING, THREADPOOL_STATE_OPEN);
        if (current_state == THREADPOOL_STATE_NOT_OPEN)
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_077: [ If current_state is not THREADPOOL_STATE_NOT_OPEN, internal_close shall return. ]*/
            should_wait_for_transition = false;
        }
        else if (current_state == THREADPOOL_STATE_OPENING || current_state == THREADPOOL_STATE_CLOSING)
        {
            should_wait_for_transition = true;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_078: [ internal_close shall wait and close every thread in threadpool by calling ThreadAPI_Join. ]*/
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
        /* Codes_SRS_THREADPOOL_LINUX_11_055: [ If param is NULL, threadpool_work_func shall return. ]*/
        LogCritical("Invalid args: param: %p", param);
    }
    else
    {
        struct timespec ts;
        THREADPOOL* threadpool = param;
        int64_t current_index;

        do
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_056: [ threadpool_work_func shall get the real time by calling clock_gettime. ]*/
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_080: [ If clock_gettime fails, threadpool_work_func shall return. ]*/
                LogError("Failure getting time from clock_gettime");
            }
            else
            {
                ts.tv_nsec += TP_SEMAPHORE_TIMEOUT_MS;
                /* Codes_SRS_THREADPOOL_LINUX_11_057: [ threadpool_work_func shall decrement the threadpool semaphore with a time limit for 2 seconds. ]*/
                if (sem_timedwait(&threadpool->semaphore, &ts) != 0)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_081: [ If sem_timedwait fails, threadpool_work_func shall timeout and return. ]*/
                }
                else
                {
                    THREADPOOL_WORK_FUNCTION work_function = NULL;
                    void* work_function_ctx = NULL;
                    /* Codes_SRS_THREADPOOL_LINUX_11_058: [ threadpool_work_func shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
                    srw_lock_acquire_shared(threadpool->srw_lock);
                    {
                        /* Codes_SRS_THREADPOOL_LINUX_11_059: [ threadpool_work_func shall get the current task array size and next waiting task consume index. ]*/
                        int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);
                        current_index = interlocked_increment_64(&threadpool->consume_idx) % existing_count;
                        /* Codes_SRS_THREADPOOL_LINUX_11_060: [ If consume index has task state TASK_WAITING, threadpool_work_func shall set the task state to TASK_WORKING. ]*/
                        TASK_RESULT curr_task_result = interlocked_compare_exchange(&threadpool->task_array[current_index].task_state, TASK_WORKING, TASK_WAITING);
                        if (TASK_WAITING == curr_task_result)
                        {
                            /* Codes_SRS_THREADPOOL_LINUX_11_061: [ threadpool_work_func shall initialize task_func and task_param and then set the task state to TASK_NOT_USED. ]*/
                            work_function = threadpool->task_array[current_index].work_function;
                            work_function_ctx = threadpool->task_array[current_index].work_function_ctx;
                            (void)interlocked_exchange(&threadpool->task_array[current_index].task_state, TASK_NOT_USED);
                        }
                        else
                        {
                            // Do nothing here, this should not happen, but log if it does
                            LogWarning("The impossible has happend, someone got the item %" PRId64 " task value %" PRI_MU_ENUM "", current_index, MU_ENUM_VALUE(TASK_RESULT, curr_task_result));
                        }
                    }
                    /* Codes_SRS_THREADPOOL_LINUX_11_062: [ threadpool_work_func shall release the shared SRW lock by calling srw_lock_release_shared. ]*/
                    srw_lock_release_shared(threadpool->srw_lock);

                    if (work_function != NULL)
                    {
                        /* Codes_SRS_THREADPOOL_LINUX_11_063: [ If task_param is not NULL, threadpool_work_func shall execute it with parameter task_param. ]*/
                        work_function(work_function_ctx);
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
        /* Codes_SRS_THREADPOOL_LINUX_11_064: [ reallocate_threadpool_array shall acquire the SRW lock in exclusive mode by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(threadpool->srw_lock);
        /* Codes_SRS_THREADPOOL_LINUX_11_065: [ reallocate_threadpool_array shall get the current size of task array. ]*/
        int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);
        int32_t new_task_array_size = existing_count*2;
        if (new_task_array_size < 0)
        {
            LogError("overflow in computation task_array_size: %" PRId32 "*2 (%" PRId32 ") > UINT32_MAX: %" PRId32 " - 1. ", existing_count, new_task_array_size, INT32_MAX);
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_067: [ Otherwise, reallocate_threadpool_array shall double the current task array size and return zero in success. ]*/
            (void)interlocked_exchange(&threadpool->task_array_size, new_task_array_size);

            /* Codes_SRS_THREADPOOL_LINUX_11_068: [ reallocate_threadpool_array shall realloc the memory used for the doubled array items and on success return a non-NULL handle to it. ]*/
            THREADPOOL_TASK* temp_array = realloc_2(threadpool->task_array, new_task_array_size, sizeof(THREADPOOL_TASK));
            if (temp_array == NULL)
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_069: [ If any error occurs, reallocate_threadpool_array shall fail and return a non-zero value. ]*/
                LogError("Failure realloc_2(threadpool->task_array: %p, threadpool->task_array_size: %" PRId32", sizeof(THREADPOOL_TASK): %zu", threadpool->task_array, new_task_array_size, sizeof(THREADPOOL_TASK));
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_070: [ reallocate_threadpool_array shall initialize every task item in the new task array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
                for (int32_t index = existing_count; index < new_task_array_size; index++)
                {
                    temp_array[index].work_function = NULL;
                    temp_array[index].work_function_ctx = NULL;
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

                int64_t insert_pos = interlocked_add_64(&threadpool->insert_idx, 0) % existing_count;
                int64_t consume_pos = interlocked_add_64(&threadpool->consume_idx, 0) % existing_count;

                uint32_t compress_count = 0;
                if (insert_pos != consume_pos)
                {

                    /* Codes_SRS_THREADPOOL_LINUX_11_071: [ reallocate_threadpool_array shall remove any gap in the task array. ]*/
                    for (int64_t index = insert_pos; index < consume_pos; index++)
                    {
                        if (interlocked_add(&threadpool->task_array[index].task_state, 0) == TASK_NOT_USED)
                        {
                            // Move the place where we're putting the insert_index back because we
                            // compressing the list
                            existing_count--;
                            compress_count++;
                        }
                    }
                    if (compress_count > 0)
                    {
                        (void)memmove(&threadpool->task_array[insert_pos], &threadpool->task_array[consume_pos+1], sizeof(THREADPOOL_TASK)*compress_count);
                    }
                }

                /* Codes_SRS_THREADPOOL_LINUX_11_072: [ reallocate_threadpool_array shall reset the consume_idx and insert_idx to -1 after resize the task array. ]*/
                (void)interlocked_exchange_64(&threadpool->consume_idx, -1);
                (void)interlocked_exchange_64(&threadpool->insert_idx, existing_count-compress_count-1);

                result = 0;
            }
        }
        /* Codes_SRS_THREADPOOL_LINUX_11_073: [ reallocate_threadpool_array shall release the SRW lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(threadpool->srw_lock);
    }
    return result;
}

THREADPOOL_HANDLE threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THREADPOOL* result;

    /* Codes_SRS_THREADPOOL_LINUX_11_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
    if (execution_engine == 0)
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine: %p", execution_engine);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_001: [ threadpool_create shall allocate memory for a threadpool object and on success return a non-NULL handle to it. ]*/
        result = malloc(sizeof(THREADPOOL));
        if (result == NULL)
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_010: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
            LogError("Failure allocating THREADPOOL structure");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_003: [ threadpool_create shall initialize thread count parameters by calling execution_engine_linux_get_parameters with parameter execution_engine. ]*/
            const EXECUTION_ENGINE_PARAMETERS_LINUX* param = execution_engine_linux_get_parameters(execution_engine);

            result->min_thread_count = param->min_thread_count;
            result->max_thread_count = param->max_thread_count;
            result->used_thread_count = result->min_thread_count;

            /* Codes_SRS_THREADPOOL_LINUX_11_004: [ threadpool_create shall allocate memory for an array of thread objects and on success return a non-NULL handle to it. ]*/
            result->thread_handle_array = malloc_2(result->used_thread_count, sizeof(THREAD_HANDLE));
            if (result->thread_handle_array == NULL)
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_010: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                LogError("Failure malloc_2(result->used_thread_count: %" PRIu32 ", sizeof(THREAD_HANDLE)): %zu", result->used_thread_count, sizeof(THREAD_HANDLE));
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_005: [ threadpool_create shall allocate memory for an array of tasks and on success return a non-NULL handle to it. ]*/
                result->task_array = malloc_2(DEFAULT_TASK_ARRAY_SIZE, sizeof(THREADPOOL_TASK));
                if (result->task_array == NULL)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_010: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                    LogError("Failure malloc_2(DEFAULT_TASK_ARRAY_SIZE: %" PRIu32 ", sizeof(THREADPOOL_TASK)): %zu", DEFAULT_TASK_ARRAY_SIZE, sizeof(THREADPOOL_TASK));
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_006: [ threadpool_create shall initialize every task item in the tasks array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
                    result->task_array_size = DEFAULT_TASK_ARRAY_SIZE;

                    for (int32_t index = 0; index < result->task_array_size; index++)
                    {
                        result->task_array[index].work_function = NULL;
                        result->task_array[index].work_function_ctx = NULL;
                        (void)interlocked_exchange(&result->task_array[index].task_state, TASK_NOT_USED);
                    }
                    result->srw_lock = srw_lock_create(false, "threadpool_lock");
                    if (result->srw_lock == NULL)
                    {
                        /* Codes_SRS_THREADPOOL_LINUX_11_010: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                        LogError("Failure srw_lock_create");
                    }
                    else
                    {
                        result->list_index = 0;
                        /* Codes_SRS_THREADPOOL_LINUX_11_007: [ threadpool_create shall create a shared semaphore between threads with initialized value zero. ]*/
                        if (sem_init(&result->semaphore, 0 , 0) != 0)
                        {
                            /* Codes_SRS_THREADPOOL_LINUX_11_010: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                            LogError("Failure creating semi_init");
                        }
                        else
                        {
                            /* Codes_SRS_THREADPOOL_LINUX_11_008: [ threadpool_create shall initilize the state to THREADPOOL_STATE_NOT_OPEN. ]*/
                            (void)interlocked_exchange(&result->state, THREADPOOL_STATE_NOT_OPEN);
                            (void)interlocked_exchange(&result->task_count, 0);
                            (void)interlocked_exchange(&result->pending_call_count, 0);

                            /* Codes_SRS_THREADPOOL_LINUX_11_009: [ insert_idx and consume_idx shall be intialzied to -1 to make the first increment start at zero. ]*/
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
        /* Codes_SRS_THREADPOOL_LINUX_11_011: [ If threadpool is NULL, threadpool_destroy shall return. ]*/
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool: %p", threadpool);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_012: [ threadpool_destroy shall implicit close if threadpool state is set to THREADPOOL_STATE_OPEN. ]*/
        internal_close(threadpool);

        /* Codes_SRS_THREADPOOL_LINUX_11_013: [ threadpool_destroy shall free the resources associated with the threadpool handle. ]*/ 
        free(threadpool->task_array);
        free(threadpool->thread_handle_array);

        /* Codes_SRS_THREADPOOL_LINUX_11_014: [ threadpool_destroy shall destroy the semphore by calling sem_destroy. ]*/
        sem_destroy(&threadpool->semaphore);
        /* Codes_SRS_THREADPOOL_LINUX_11_015: [ threadpool_destroy shall destroy the SRW lock by calling srw_lock_destroy. ]*/
        srw_lock_destroy(threadpool->srw_lock);
        free(threadpool);
    }
}

int threadpool_open_async(THREADPOOL_HANDLE threadpool, ON_THREADPOOL_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_11_016: [ If threadpool is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_11_074: [ If on_open_complete is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
        (on_open_complete == NULL))
    {
        LogError("THREADPOOL_HANDLE threadpool=%p, ON_THREADPOOL_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            threadpool, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_017: [ threadpool_open_async shall set the state to THREADPOOL_STATE_OPENING. ]*/
        int32_t current_state = interlocked_compare_exchange(&threadpool->state, THREADPOOL_STATE_OPENING, THREADPOOL_STATE_NOT_OPEN);
        if (current_state != THREADPOOL_STATE_NOT_OPEN)
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_018: [ If threadpool has already been opened, threadpool_open_async shall fail and return a non-zero value. ]*/
            LogError("Inconsistent state current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            int32_t index;
            
            for (index = 0; index < threadpool->used_thread_count; index++)
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_019: [ threadpool_open_async shall create the threads for threadpool using ThreadAPI_Create. ]*/
                if (ThreadAPI_Create(&threadpool->thread_handle_array[index], threadpool_work_func, threadpool) != THREADAPI_OK)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_076: [ If ThreadAPI_Create fails, threadpool_open_async shall fail and return a non-zero value. ]*/
                    LogError("Failure creating thread %" PRId32 "", index);
                    break;
                }
            }
            if (index < threadpool->used_thread_count)
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_020: [ If one of the thread creation fails, threadpool_open_async shall fail and return a non-zero value, terminate all threads already created, indicate an error to the user by calling the on_open_complete callback with THREADPOOL_OPEN_ERROR and set threadpool state to THREADPOOL_STATE_NOT_OPEN. ]*/
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
                /* Codes_SRS_THREADPOOL_LINUX_11_021: [ Otherwise, threadpool_open_async shall set the state to THREADPOOL_STATE_OPEN, indicate open success to the user by calling the on_open_complete callback with THREADPOOL_OPEN_OK and return zero. ]*/
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
        /* Codes_SRS_THREADPOOL_LINUX_11_022: [ If threadpool is NULL, threadpool_close shall fail and return. ]*/
        LogError("THREADPOOL_HANDLE threadpool=%p", threadpool);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_023: [ Otherwise, threadpool_close shall terminate all threads in the threadpool and set the state to THREADPOOL_STATE_NOT_OPEN. ]*/
        internal_close(threadpool);
    }
}

int threadpool_schedule_work(THREADPOOL_HANDLE threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_ctx)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_11_024: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        threadpool == NULL || 
        /* Codes_SRS_THREADPOOL_LINUX_11_025: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        work_function == NULL)
    {
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool: %p, THREADPOOL_WORK_FUNCTION work_function: %p, void* work_function_ctx: %p", threadpool, work_function, work_function_ctx);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_026: [ threadpool_schedule_work shall get the threadpool state by calling interlocked_add. ]*/
        int32_t current_state = interlocked_add(&threadpool->state, 0);
        if (current_state != THREADPOOL_STATE_OPEN)
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_027: [ threadpool_schedule_work shall validate that it's state is THREADPOOL_STATE_OPEN and if not shall fail and return a non-zero value. ]*/
            LogWarning("Inconsistent state current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(THREADPOOL_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {

            /* Codes_SRS_THREADPOOL_LINUX_11_028: [ threadpool_schedule_work shall increment the count of pending call that are in progress to be executed. ]*/
            (void)interlocked_increment(&threadpool->pending_call_count);
            wake_by_address_single(&threadpool->pending_call_count);
            do
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_029: [ threadpool_schedule_work shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
                srw_lock_acquire_shared(threadpool->srw_lock);
                int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);
                
                /* Codes_SRS_THREADPOOL_LINUX_11_030: [ threadpool_schedule_work shall increment the insert_pos by one. ]*/
                int64_t insert_pos = interlocked_increment_64(&threadpool->insert_idx) % existing_count;
                /* Codes_SRS_THREADPOOL_LINUX_11_031: [ threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
                int32_t task_state = interlocked_compare_exchange(&threadpool->task_array[insert_pos].task_state, TASK_INITIALIZING, TASK_NOT_USED);
                if (task_state != TASK_NOT_USED)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_032: [ If task has been already initialized, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity by calling reallocate_threadpool_array. ]*/
                    srw_lock_release_shared(threadpool->srw_lock);

                    if (reallocate_threadpool_array(threadpool) != 0)
                    {
                        /* Codes_SRS_THREADPOOL_LINUX_11_033: [ If reallcate task array fails, threadpool_schedule_work shall fail and return a non-zero value. ]*/
                        LogError("Failure reallocating threadpool");
                        result = MU_FAILURE;
                        break;
                    }
                    continue;
                }
                else
                {

                    /* Codes_SRS_THREADPOOL_LINUX_11_034: [ threadpool_schedule_work shall obtain task information in next available task array index and return zero on success. ]*/
                    THREADPOOL_TASK* task_item = &threadpool->task_array[insert_pos];
                    task_item->work_function_ctx = work_function_ctx;
                    task_item->work_function = work_function;
                    /* Codes_SRS_THREADPOOL_LINUX_11_035: [ threadpool_schedule_work shall set the task_state to TASK_WAITING and then release the shared SRW lock. ]*/
                    (void)interlocked_exchange(&task_item->task_state, TASK_WAITING);
                    srw_lock_release_shared(threadpool->srw_lock);
                   
                    /* Codes_SRS_THREADPOOL_LINUX_11_036: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
                    sem_post(&threadpool->semaphore);
                    result = 0;
                    break;
                }
            } while (true);
            /* Codes_SRS_THREADPOOL_LINUX_11_037: [ threadpool_schedule_work shall decrement the count of pending call that are in progress to be executed. ]*/
            (void)interlocked_decrement(&threadpool->pending_call_count);
            wake_by_address_all(&threadpool->pending_call_count);
        }
    }
all_ok:
    return result;
}

int threadpool_timer_start(THREADPOOL_HANDLE threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_ctx, TIMER_INSTANCE_HANDLE* timer_handle)
{
    int result;
    
    /* Codes_SRS_THREADPOOL_LINUX_11_041: [ work_function_ctx shall be allowed to be NULL. ]*/
    if (
        /* Codes_SRS_THREADPOOL_LINUX_11_038: [ If threadpool is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
        threadpool == NULL ||
        /* Codes_SRS_THREADPOOL_LINUX_11_039: [ If work_function is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
        work_function == NULL ||
        /* Codes_SRS_THREADPOOL_LINUX_11_040: [ If timer_handle is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
        timer_handle == NULL
        )
    {
        LogError("Invalid args: THREADPOOL_HANDLE threadpool = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 ", THREADPOOL_WORK_FUNCTION work_function = %p, void* work_function_context = %p, TIMER_INSTANCE_HANDLE* timer_handle = %p",
            threadpool, start_delay_ms, timer_period_ms, work_function, work_function_ctx, timer_handle);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_042: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
        TIMER_INSTANCE* timer_instance = malloc(sizeof(TIMER_INSTANCE));
        if (timer_instance == NULL)
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_044: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
            LogError("Failure allocating Timer Instance");
        }
        else
        {
            timer_instance->work_function = work_function;
            timer_instance->work_function_ctx = work_function_ctx;

            // Setup the timer
            struct sigevent sigev = {0};
            timer_t time_id = 0;

            sigev.sigev_notify          = SIGEV_THREAD;
            sigev.sigev_notify_function = on_timer_callback;
            sigev.sigev_value.sival_ptr = timer_instance;
            
            /* Codes_SRS_THREADPOOL_LINUX_11_043: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
            if (timer_create(CLOCK_REALTIME, &sigev, &time_id) != 0)
            {
                /* Codes_SRS_THREADPOOL_LINUX_11_044: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
                char err_msg[128];
                (void)strerror_r(errno, err_msg, 128);
                LogError("Failure calling timer_create. Error: %d: %s", errno, err_msg);
            }
            else
            {
                struct itimerspec its;
                its.it_value.tv_sec = start_delay_ms / 1000;
                its.it_value.tv_nsec = start_delay_ms * MILLISEC_TO_NANOSEC % 1000000000;
                its.it_interval.tv_sec = timer_period_ms / 1000;
                its.it_interval.tv_nsec = timer_period_ms * MILLISEC_TO_NANOSEC % 1000000000;

                if (timer_settime(time_id, 0, &its, NULL) == -1)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_044: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
                    char err_msg[128];
                    (void)strerror_r(errno, err_msg, 128);
                    LogError("Failure calling timer_settime. Error: %s", MU_P_OR_NULL(err_msg));
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_045: [ threadpool_timer_start shall return and allocated handle in timer_handle. ]*/
                    /* Codes_SRS_THREADPOOL_LINUX_11_046: [ threadpool_timer_start shall succeed and return 0. ]*/
                    timer_instance->time_id = time_id;
                    *timer_handle = timer_instance;
                    result = 0;
                    goto all_ok;
                }

                if (timer_delete(time_id) != 0)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_11_075: [ If timer_settime fails, threadpool_timer_start shall delete the timer by calling timer_delete. ]*/
                    char err_msg[128];
                    (void)strerror_r(errno, err_msg, 128);
                    LogError("Failure calling timer_delete. Error: %d: (%s)", errno, err_msg);
                }
            }
            free(timer_instance);
        }
        result = MU_FAILURE;
    }
all_ok:
    return result;
}

int threadpool_timer_restart(TIMER_INSTANCE_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    int result;
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_047: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 "",
            timer, start_delay_ms, timer_period_ms);
        result = MU_FAILURE;
    }
    else
    {
        struct itimerspec its = {0};
        its.it_value.tv_sec = start_delay_ms / 1000;
        its.it_value.tv_nsec = start_delay_ms * MILLISEC_TO_NANOSEC % 1000000000;
        its.it_interval.tv_sec = timer_period_ms / 1000;
        its.it_interval.tv_nsec = timer_period_ms * MILLISEC_TO_NANOSEC % 1000000000;

        /* Codes_SRS_THREADPOOL_LINUX_11_048: [ threadpool_timer_restart` shall call timer_settime to changethe delay and period. ]*/
        if (timer_settime(timer->time_id, 0, &its, NULL) != 0)
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_079: [ If timer_settime fails, threadpool_timer_restart shall fail and return a non-zero value. ]*/
            char err_msg[128];
            (void)strerror_r(errno, err_msg, 128);
            LogError("Failure calling timer_settime. Error: %d: (%s)", errno, err_msg);
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_11_049: [ threadpool_timer_restart shall succeed and return 0. ]*/
            result = 0;
        }
    }
    return result;
}

void threadpool_timer_cancel(TIMER_INSTANCE_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_050: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p", timer);
    }
    else
    {
        struct itimerspec its = {0};
        /* Codes_SRS_THREADPOOL_LINUX_11_051: [ threadpool_timer_cancel shall call timer_settime with 0 for flags and NULL for old_value and {0} for new_value to cancel the ongoing timers. ]*/
        if (timer_settime(timer->time_id, 0, &its, NULL) != 0)
        {
            char err_msg[128];
            (void)strerror_r(errno, err_msg, 128);
            LogError("Failure calling timer_settime. Error: %d: (%s)", errno, err_msg);
        }
        else
        {
            // Do Nothing
        }
    }
}

void threadpool_timer_destroy(TIMER_INSTANCE_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_052: [ If timer is NULL, threadpool_timer_destroy shall fail and return. ]*/
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p", timer);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_11_053: [ threadpool_timer_cancel shall call timer_delete  to destroy the ongoing timers. ]*/
        if (timer_delete(timer->time_id) != 0)
        {
            char err_msg[128];
            (void)strerror_r(errno, err_msg, 128);
            LogError("Failure calling timer_delete. Error: %d: (%s)", errno, err_msg);
        }
        else
        {
            // Do Nothing
        }
        /* Codes_SRS_THREADPOOL_LINUX_11_054: [ threadpool_timer_destroy shall free all resources in timer. ]*/
        free(timer);
    }
}

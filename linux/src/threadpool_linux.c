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
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/srw_lock.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"
#include "c_pal/sm.h"

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
    SRW_LOCK_HANDLE srw_lock;
    uint32_t list_index;
    SM_HANDLE sm;

    THREADPOOL_TASK* task_array;
    volatile_atomic int32_t task_array_size;
    volatile_atomic int64_t insert_idx;
    volatile_atomic int64_t consume_idx;

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
    /* Codes_SRS_THREADPOOL_LINUX_07_026: [ Otherwise, threadpool_close shall call sm_close_begin. ]*/
    if(sm_close_begin(threadpool->sm) != SM_EXEC_GRANTED)
    {
        LogError("sm_close_begin failed.");
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_089: [ threadpool_close shall signal all threads threadpool is closing by calling InterlockedHL_SetAndWakeAll. ]*/
        (void)InterlockedHL_SetAndWakeAll(&threadpool->state, 1);
        for (int32_t index = 0; index < threadpool->used_thread_count; index++)
        {
            int dont_care;
            /* Codes_SRS_THREADPOOL_LINUX_07_027: [ threadpool_close shall join all threads in the threadpool. ]*/
            if (ThreadAPI_Join(threadpool->thread_handle_array[index], &dont_care) != THREADAPI_OK)
            {
                LogError("Failure joining thread number %" PRId32 "", index);
            }
        }
        /* Codes_SRS_THREADPOOL_LINUX_07_028: [ threadpool_close shall call sm_close_end. ]*/
        sm_close_end(threadpool->sm);
    }
}

static int threadpool_work_func(void* param)
{
    if (param == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_073: [ If param is NULL, threadpool_work_func shall fail and return. ]*/
        LogCritical("Invalid args: param: %p", param);
    }
    else
    {
        struct timespec ts;
        THREADPOOL* threadpool = param;
        int64_t current_index;

        do
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_074: [ threadpool_work_func shall get the real time by calling clock_gettime to set the waiting time for semaphore. ]*/
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_088: [ If clock_gettime fails, threadpool_work_func shall run the loop again. ]*/
                LogError("Failure getting time from clock_gettime");
            }
            else
            {
                ts.tv_nsec += TP_SEMAPHORE_TIMEOUT_MS;

                /* Codes_SRS_THREADPOOL_LINUX_07_075: [ threadpool_work_func shall wait on the semaphore with a time limit. ]*/
                if (sem_timedwait(&threadpool->semaphore, &ts) != 0)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_087: [ If sem_timedwait fails, threadpool_work_func shall timeout and run the loop again. ]*/
                }
                else
                {
                    THREADPOOL_WORK_FUNCTION work_function = NULL;
                    void* work_function_ctx = NULL;

                    /* Codes_SRS_THREADPOOL_LINUX_07_076: [ threadpool_work_func shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
                    srw_lock_acquire_shared(threadpool->srw_lock);
                    {
                        /* Codes_SRS_THREADPOOL_LINUX_07_077: [ threadpool_work_func shall get the current task array size by calling interlocked_add. ]*/
                        int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);

                        /* Codes_SRS_THREADPOOL_LINUX_07_078: [ threadpool_work_func shall increment the current consume index by calling interlocked_increment_64. ]*/
                        /* Codes_SRS_THREADPOOL_LINUX_07_079: [ threadpool_work_func shall get the next waiting task consume index from incremented consume index modulo current task array size. ]*/
                        current_index = interlocked_increment_64(&threadpool->consume_idx) - 1 % existing_count;
                        /* Codes_SRS_THREADPOOL_LINUX_07_080: [ If consume index has task state TASK_WAITING, threadpool_work_func shall set the task state to TASK_WORKING. ]*/
                        TASK_RESULT curr_task_result = interlocked_compare_exchange(&threadpool->task_array[current_index].task_state, TASK_WORKING, TASK_WAITING);
                        if (TASK_WAITING == curr_task_result)
                        {
                            /* Codes_SRS_THREADPOOL_LINUX_07_081: [ threadpool_work_func shall copy the function and parameter to local variables. ]*/
                            work_function = threadpool->task_array[current_index].work_function;
                            work_function_ctx = threadpool->task_array[current_index].work_function_ctx;
                            /* Codes_SRS_THREADPOOL_LINUX_07_082: [ threadpool_work_func shall set the task state to TASK_NOT_USED. ]*/
                            (void)interlocked_exchange(&threadpool->task_array[current_index].task_state, TASK_NOT_USED);
                        }
                        else
                        {
                            LogWarning("The impossible has happend, someone got the item %" PRId64 " task value %" PRI_MU_ENUM "", current_index, MU_ENUM_VALUE(TASK_RESULT, curr_task_result));
                        }
                    }
                    /* Codes_SRS_THREADPOOL_LINUX_07_083: [ threadpool_work_func shall release the shared SRW lock by calling srw_lock_release_shared. ]*/
                    srw_lock_release_shared(threadpool->srw_lock);

                    /* Codes_SRS_THREADPOOL_LINUX_07_084: [ If the work item function is not NULL, threadpool_work_func shall execute it with work_function_ctx. ]*/
                    if (work_function != NULL)
                    {
                        work_function(work_function_ctx);
                    }
                }
            }
        /* Codes_SRS_THREADPOOL_LINUX_07_085: [ threadpool_work_func shall loop until threadpool_close or threadpool_destroy is called. ]*/
        } while (interlocked_add(&threadpool->state, 0) != 1);
    }
    return 0;
}

static int reallocate_threadpool_array(THREADPOOL* threadpool)
{
    int result;
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_037: [ threadpool_schedule_work shall acquire the SRW lock in exclusive mode by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(threadpool->srw_lock);

        /* Codes_SRS_THREADPOOL_LINUX_07_038: [ threadpool_schedule_work shall get the current size of task array by calling interlocked_add. ]*/
        int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);

        int32_t new_task_array_size = existing_count*2;
        /* Codes_SRS_THREADPOOL_LINUX_07_039: [ If there is any overflow computing the new size, threadpool_schedule_work shall fail and return a non-zero value . ]*/
        if (new_task_array_size < 0)
        {
            LogError("overflow in computation task_array_size: %" PRId32 "*2 (%" PRId32 ") > UINT32_MAX: %" PRId32 " - 1. ", existing_count, new_task_array_size, INT32_MAX);
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_040: [ Otherwise, threadpool_schedule_work shall double the current task array size. ]*/
            (void)interlocked_exchange(&threadpool->task_array_size, new_task_array_size);

            /* Codes_SRS_THREADPOOL_LINUX_07_041: [ threadpool_schedule_work shall realloc the memory used for the array items. ]*/
            THREADPOOL_TASK* temp_array = realloc_2(threadpool->task_array, new_task_array_size, sizeof(THREADPOOL_TASK));
            if (temp_array == NULL)
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_042: [ If any error occurs, threadpool_schedule_work shall fail and return a non-zero value. ]*/
                LogError("Failure realloc_2(threadpool->task_array: %p, threadpool->task_array_size: %" PRId32", sizeof(THREADPOOL_TASK): %zu", threadpool->task_array, new_task_array_size, sizeof(THREADPOOL_TASK));
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_043: [ threadpool_schedule_work shall initialize every task item in the new task array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
                for (int32_t index = existing_count; index < new_task_array_size; index++)
                {
                    temp_array[index].work_function = NULL;
                    temp_array[index].work_function_ctx = NULL;
                    (void)interlocked_exchange(&temp_array[index].task_state, TASK_NOT_USED);
                }
                threadpool->task_array = temp_array;

                int64_t insert_pos = interlocked_add_64(&threadpool->insert_idx, 0) % existing_count;
                int64_t consume_pos = interlocked_add_64(&threadpool->consume_idx, 0) % existing_count;
                uint32_t compress_count = 0;

                /* Codes_SRS_THREADPOOL_LINUX_07_044: [ threadpool_schedule_work shall remove any gap in the task array. ]*/
                if (insert_pos != consume_pos)
                {
                    for (int64_t index = insert_pos; index < consume_pos; index++)
                    {
                        if (interlocked_add(&threadpool->task_array[index].task_state, 0) == TASK_NOT_USED)
                        {
                            existing_count--;
                            compress_count++;
                        }
                    }
                    if (compress_count > 0)
                    {
                        (void)memmove(&threadpool->task_array[insert_pos], &threadpool->task_array[consume_pos+1], sizeof(THREADPOOL_TASK)*compress_count);
                    }
                }

                /* Codes_SRS_THREADPOOL_LINUX_07_045: [ threadpool_schedule_work shall reset the consume_idx and insert_idx to 0 after resize the task array. ]*/
                (void)interlocked_exchange_64(&threadpool->consume_idx, 0);
                (void)interlocked_exchange_64(&threadpool->insert_idx, existing_count-compress_count);

                result = 0;
            }
        }
        /* Codes_SRS_THREADPOOL_LINUX_07_046: [ threadpool_schedule_work shall release the SRW lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(threadpool->srw_lock);
    }
    return result;
}

THREADPOOL_HANDLE threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THREADPOOL* result;

    /* Codes_SRS_THREADPOOL_LINUX_07_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
    if (execution_engine == 0)
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine: %p", execution_engine);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_001: [ threadpool_create shall allocate memory for a threadpool object and on success return a non-NULL handle to it. ]*/
        result = malloc(sizeof(THREADPOOL));
        if (result == NULL)
        {
            LogError("Failure allocating THREADPOOL structure");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_003: [ threadpool_create shall create a SM_HANDLE by calling sm_create. ]*/
            result->sm = sm_create("threadpool");
            if(result->sm == NULL)
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_011: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                LogError("sm_create failed.");
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_004: [ threadpool_create shall get the min_thread_count and max_thread_count thread parameters from the execution_engine. ]*/
                const EXECUTION_ENGINE_PARAMETERS_LINUX* param = execution_engine_linux_get_parameters(execution_engine);

                result->min_thread_count = param->min_thread_count;
                result->max_thread_count = param->max_thread_count;
                result->used_thread_count = result->min_thread_count;

                /* Codes_SRS_THREADPOOL_LINUX_07_005: [ threadpool_create shall allocate memory for an array of thread handles of size min_thread_count and on success return a non-NULL handle to it. ]*/
                result->thread_handle_array = malloc_2(result->used_thread_count, sizeof(THREAD_HANDLE));
                if (result->thread_handle_array == NULL)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_011: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                    LogError("Failure malloc_2(result->used_thread_count: %" PRIu32 ", sizeof(THREAD_HANDLE)): %zu", result->used_thread_count, sizeof(THREAD_HANDLE));
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_006: [ threadpool_create shall allocate memory with default task array size 2048 for an array of tasks and on success return a non-NULL handle to it. ]*/
                    result->task_array = malloc_2(DEFAULT_TASK_ARRAY_SIZE, sizeof(THREADPOOL_TASK));
                    if (result->task_array == NULL)
                    {
                        /* Codes_SRS_THREADPOOL_LINUX_07_011: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                        LogError("Failure malloc_2(DEFAULT_TASK_ARRAY_SIZE: %" PRIu32 ", sizeof(THREADPOOL_TASK)): %zu", DEFAULT_TASK_ARRAY_SIZE, sizeof(THREADPOOL_TASK));
                    }
                    else
                    {
                        result->task_array_size = DEFAULT_TASK_ARRAY_SIZE;
                        /* Codes_SRS_THREADPOOL_LINUX_07_007: [ threadpool_create shall initialize every task item in the tasks array with task_func and task_param set to NULL and task_state set to TASK_NOT_USED. ]*/
                        for (int32_t index = 0; index < result->task_array_size; index++)
                        {
                            result->task_array[index].work_function = NULL;
                            result->task_array[index].work_function_ctx = NULL;
                            (void)interlocked_exchange(&result->task_array[index].task_state, TASK_NOT_USED);
                        }

                        /* Codes_SRS_THREADPOOL_LINUX_07_008: [ threadpool_create shall create a SRW lock by calling srw_lock_create. ]*/
                        result->srw_lock = srw_lock_create(false, "threadpool_lock");
                        if (result->srw_lock == NULL)
                        {
                            /* Codes_SRS_THREADPOOL_LINUX_07_011: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                            LogError("Failure srw_lock_create");
                        }
                        else
                        {
                            result->list_index = 0;
                            /* Codes_SRS_THREADPOOL_LINUX_07_009: [ threadpool_create shall create a shared semaphore with initialized value zero. ]*/
                            if (sem_init(&result->semaphore, 0 , 0) != 0)
                            {
                                /* Codes_SRS_THREADPOOL_LINUX_07_011: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                                LogError("Failure creating semi_init");
                            }
                            else
                            {
                                (void)interlocked_exchange(&result->state, 0);
                                (void)interlocked_exchange(&result->task_count, 0);

                                /* Codes_SRS_THREADPOOL_LINUX_07_010: [ insert_idx and consume_idx for the task array shall be initialized to 0. ]*/
                                (void)interlocked_exchange_64(&result->insert_idx, 0);
                                (void)interlocked_exchange_64(&result->consume_idx, 0);

                                goto all_ok;
                            }
                            srw_lock_destroy(result->srw_lock);
                        }
                        free(result->task_array);
                    }
                    free(result->thread_handle_array);
                }
                sm_destroy(result->sm);
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
        /* Codes_SRS_THREADPOOL_LINUX_07_012: [ If threadpool is NULL, threadpool_destroy shall return. ]*/
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool: %p", threadpool);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_013: [ threadpool_destroy shall perform an implicit close if threadpool is open. ]*/
        internal_close(threadpool);

        /* Codes_SRS_THREADPOOL_LINUX_07_016: [ threadpool_destroy shall free the memory allocated in threadpool_create. ]*/
        free(threadpool->task_array);
        free(threadpool->thread_handle_array);

        /* Codes_SRS_THREADPOOL_LINUX_07_014: [ threadpool_destroy shall destroy the semphore by calling sem_destroy. ]*/
        sem_destroy(&threadpool->semaphore);
        /* Codes_SRS_THREADPOOL_LINUX_07_015: [ threadpool_destroy shall destroy the SRW lock by calling srw_lock_destroy. ]*/
        srw_lock_destroy(threadpool->srw_lock);
        sm_destroy(threadpool->sm);

        /* Codes_SRS_THREADPOOL_LINUX_07_016: [ threadpool_destroy shall free the memory allocated in threadpool_create. ]*/
        free(threadpool);
    }
}

int threadpool_open_async(THREADPOOL_HANDLE threadpool, ON_THREADPOOL_OPEN_COMPLETE on_open_complete, void* on_open_complete_context)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_07_017: [ If threadpool is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_07_086: [ If on_open_complete is NULL, threadpool_open_async shall fail and return a non-zero value. ]*/
        (on_open_complete == NULL))
    {
        LogError("THREADPOOL_HANDLE threadpool=%p, ON_THREADPOOL_OPEN_COMPLETE on_open_complete=%p, void* on_open_complete_context=%p",
            threadpool, on_open_complete, on_open_complete_context);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_018: [ threadpool_open_async shall call sm_open_begin. ]*/
        SM_RESULT open_result = sm_open_begin(threadpool->sm);
        if(open_result != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_019: [ If sm_open_begin indicates the open cannot be performed, threadpool_open_async shall fail and return a non-zero value. ]*/
            LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
            result = MU_FAILURE;
        }
        else
        {
            int32_t index;
            for (index = 0; index < threadpool->used_thread_count; index++)
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_020: [ threadpool_open_async shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
                if (ThreadAPI_Create(&threadpool->thread_handle_array[index], threadpool_work_func, threadpool) != THREADAPI_OK)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_021: [ If any error occurs, threadpool_open_async shall fail and return a non-zero value. ]*/
                    LogError("Failure creating thread %" PRId32 "", index);
                    break;
                }
            }

            /* Codes_SRS_THREADPOOL_LINUX_07_022: [ If one of the thread creation fails, threadpool_open_async shall fail and return a non-zero value, terminate all threads already created. ]*/
            if (index < threadpool->used_thread_count)
            {
                for (int32_t inner = 0; inner < index; inner++)
                {
                    int dont_care;
                    if (ThreadAPI_Join(threadpool->thread_handle_array[inner], &dont_care) != THREADAPI_OK)
                    {
                        LogError("Failure joining thread number %" PRId32 "", inner);
                    }
                }
                sm_open_end(threadpool->sm, false);
                on_open_complete(on_open_complete_context, THREADPOOL_OPEN_ERROR);
                result = MU_FAILURE;
            }
            /* Codes_SRS_THREADPOOL_LINUX_07_023: [ Otherwise, threadpool_open_async shall shall call sm_open_end with true for success. ]*/
            /* Codes_SRS_THREADPOOL_LINUX_07_024: [ threadpool_open_async shall succeed, indicate open success to the user by calling the on_open_complete callback with THREADPOOL_OPEN_OK and return zero. ]*/
            else
            {
                sm_open_end(threadpool->sm, true);
                on_open_complete(on_open_complete_context, THREADPOOL_OPEN_OK);
                result = 0;
            }
        }
    }
    return result;
}

void threadpool_close(THREADPOOL_HANDLE threadpool)
{
    /* Codes_SRS_THREADPOOL_LINUX_07_025: [ If threadpool is NULL, threadpool_close shall fail and return. ]*/
    if (threadpool == NULL)
    {
        LogError("THREADPOOL_HANDLE threadpool=%p", threadpool);
    }
    else
    {
        internal_close(threadpool);
    }
}

int threadpool_schedule_work(THREADPOOL_HANDLE threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_ctx)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_07_029: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        threadpool == NULL ||
        /* Codes_SRS_THREADPOOL_LINUX_07_030: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        work_function == NULL)
    {
        LogError("Invalid arguments: THREADPOOL_HANDLE threadpool: %p, THREADPOOL_WORK_FUNCTION work_function: %p, void* work_function_ctx: %p", threadpool, work_function, work_function_ctx);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
        SM_RESULT sm_result = sm_exec_begin(threadpool->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_032: [ If sm_exec_begin returns SM_EXEC_REFUSED, threadpool_schedule_work shall fail and return a non-zero value. ]*/
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = MU_FAILURE;
        }
        else
        {
            do
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_033: [ threadpool_schedule_work shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
                srw_lock_acquire_shared(threadpool->srw_lock);
                int32_t existing_count = interlocked_add(&threadpool->task_array_size, 0);

                /* Codes_SRS_THREADPOOL_LINUX_07_034: [ threadpool_schedule_work shall increment the insert_pos. ]*/
                int64_t insert_pos = interlocked_increment_64(&threadpool->insert_idx) - 1 % existing_count;

                /* Codes_SRS_THREADPOOL_LINUX_07_035: [ If task state is TASK_NOT_USED, threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
                int32_t task_state = interlocked_compare_exchange(&threadpool->task_array[insert_pos].task_state, TASK_INITIALIZING, TASK_NOT_USED);

                if (task_state != TASK_NOT_USED)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_036: [ Otherwise, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity: ]*/
                    srw_lock_release_shared(threadpool->srw_lock);

                    if (reallocate_threadpool_array(threadpool) != 0)
                    {
                        /* Codes_SRS_THREADPOOL_LINUX_07_048: [ If reallocating the task array fails, threadpool_schedule_work shall fail and return a non-zero value. ]*/
                        LogError("Failure reallocating threadpool");
                        result = MU_FAILURE;
                        break;
                    }
                    continue;
                }
                /* Codes_SRS_THREADPOOL_LINUX_07_049: [ threadpool_schedule_work shall copy the work function and work function context into insert position in the task array and return zero on success. ]*/
                else
                {
                    THREADPOOL_TASK* task_item = &threadpool->task_array[insert_pos];
                    task_item->work_function_ctx = work_function_ctx;
                    task_item->work_function = work_function;

                    /* Codes_SRS_THREADPOOL_LINUX_07_050: [ threadpool_schedule_work shall set the task_state to TASK_WAITING and then release the shared SRW lock. ]*/
                    (void)interlocked_exchange(&task_item->task_state, TASK_WAITING);
                    srw_lock_release_shared(threadpool->srw_lock);

                    /* Codes_SRS_THREADPOOL_LINUX_07_051: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
                    sem_post(&threadpool->semaphore);

                    /* Codes_SRS_THREADPOOL_LINUX_07_047: [ threadpool_schedule_work shall return zero on success. ]*/
                    result = 0;
                    break;
                }
            } while (true);
            /* Codes_SRS_THREADPOOL_LINUX_07_053: [ threadpool_schedule_work shall call sm_exec_end. ]*/
            sm_exec_end(threadpool->sm);
        }
    }
all_ok:
    return result;
}

int threadpool_timer_start(THREADPOOL_HANDLE threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_ctx, TIMER_INSTANCE_HANDLE* timer_handle)
{
    int result;

    if (
        /* Codes_SRS_THREADPOOL_LINUX_07_054: [ If threadpool is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
        threadpool == NULL ||
        /* Codes_SRS_THREADPOOL_LINUX_07_055: [ If work_function is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
        work_function == NULL ||
        /* Codes_SRS_THREADPOOL_LINUX_07_056: [ If timer_handle is NULL, threadpool_timer_start shall fail and return a non-zero value. ]*/
        timer_handle == NULL
        )
    {
        LogError("Invalid args: THREADPOOL_HANDLE threadpool = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 ", THREADPOOL_WORK_FUNCTION work_function = %p, void* work_function_context = %p, TIMER_INSTANCE_HANDLE* timer_handle = %p",
            threadpool, start_delay_ms, timer_period_ms, work_function, work_function_ctx, timer_handle);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate a context for the timer being started and store work_function and work_function_ctx in it. ]*/
        TIMER_INSTANCE* timer_instance = malloc(sizeof(TIMER_INSTANCE));
        if (timer_instance == NULL)
        {
            LogError("Failure allocating Timer Instance");
        }
        else
        {
            timer_instance->work_function = work_function;
            /* Codes_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
            timer_instance->work_function_ctx = work_function_ctx;

            struct sigevent sigev = {0};
            timer_t time_id = 0;

            sigev.sigev_notify          = SIGEV_THREAD;
            sigev.sigev_notify_function = on_timer_callback;
            sigev.sigev_value.sival_ptr = timer_instance;

            /* Codes_SRS_THREADPOOL_LINUX_07_059: [ threadpool_timer_start shall call timer_create and timer_settime to schedule execution. ]*/
            if (timer_create(CLOCK_REALTIME, &sigev, &time_id) != 0)
            {
                LogErrorNo("Failure calling timer_create.");
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
                    LogErrorNo("Failure calling timer_settime");
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_061: [ threadpool_timer_start shall return and allocated handle in timer_handle. ]*/
                    /* Codes_SRS_THREADPOOL_LINUX_07_062: [ threadpool_timer_start shall succeed and return 0. ]*/
                    timer_instance->time_id = time_id;
                    *timer_handle = timer_instance;
                    result = 0;
                    goto all_ok;
                }
                /* Codes_SRS_THREADPOOL_LINUX_07_063: [ If timer_settime fails, threadpool_timer_start shall delete the timer by calling timer_delete. ]*/
                if (timer_delete(time_id) != 0)
                {
                    LogErrorNo("Failure calling timer_delete.");
                }
            }
            free(timer_instance);
        }
        /* Codes_SRS_THREADPOOL_LINUX_07_060: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
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
        /* Codes_SRS_THREADPOOL_LINUX_07_064: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
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

        /* Codes_SRS_THREADPOOL_LINUX_07_065: [ threadpool_timer_restart shall call timer_settime to change the delay and period. ]*/
        if (timer_settime(timer->time_id, 0, &its, NULL) != 0)
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_066: [ If timer_settime fails, threadpool_timer_restart shall fail and return a non-zero value. ]*/
            LogErrorNo("Failure calling timer_settime.");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_067: [ threadpool_timer_restart shall succeed and return 0. ]*/
            result = 0;
        }
    }
    return result;
}

void threadpool_timer_cancel(TIMER_INSTANCE_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_068: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p", timer);
    }
    else
    {
        struct itimerspec its = {0};
        /* Codes_SRS_THREADPOOL_LINUX_07_069: [ threadpool_timer_cancel shall call timer_settime with 0 for flags and NULL for old_value and {0} for new_value to cancel the ongoing timers. ]*/
        if (timer_settime(timer->time_id, 0, &its, NULL) != 0)
        {
            LogErrorNo("Failure calling timer_settime");
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
        /* Codes_SRS_THREADPOOL_LINUX_07_070: [ If timer is NULL, threadpool_timer_destroy shall fail and return. ]*/
        LogError("Invalid args: TIMER_INSTANCE_HANDLE timer = %p", timer);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_071: [ threadpool_timer_cancel shall call timer_delete to destroy the ongoing timers. ]*/
        if (timer_delete(timer->time_id) != 0)
        {
            LogErrorNo("Failure calling timer_delete.");
        }
        else
        {
            // Do Nothing
        }
        /* Codes_SRS_THREADPOOL_LINUX_07_072: [ threadpool_timer_destroy shall free all resources in timer. ]*/
        free(timer);
    }
}

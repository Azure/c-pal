// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
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

#include "c_logging/log_context.h"
#include "c_logging/log_context_property_type_ascii_char_ptr.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/lazy_init.h"
#include "c_pal/srw_lock.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/sync.h"
#include "c_pal/thandle.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"
#include "c_pal/srw_lock_ll.h"

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

#define TIMER_STATE_VALUES  \
    TIMER_NOT_USED,         \
    TIMER_ARMED,            \
    TIMER_CALLING_CALLBACK  \

MU_DEFINE_ENUM(TIMER_STATE, TIMER_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS(TIMER_STATE, TIMER_STATE_VALUES)

typedef struct CUSTOM_TIMER_DATA_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
    volatile_atomic int32_t timer_state; // TIMER_STATE
    int64_t epoch_number;
} CUSTOM_TIMER_DATA;

typedef struct THREADPOOL_TIMER_TAG
{
    timer_t time_id;
    CUSTOM_TIMER_DATA* custom_timer_data;
} THREADPOOL_TIMER;

THANDLE_TYPE_DEFINE(THREADPOOL_TIMER);

typedef struct THREADPOOL_TASK_TAG
{
    volatile_atomic int32_t task_state;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
} THREADPOOL_TASK;

typedef struct THREADPOOL_WORK_ITEM_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
} THREADPOOL_WORK_ITEM, *THREADPOOL_WORK_ITEM_HANDLE;

THANDLE_TYPE_DEFINE(THREADPOOL_WORK_ITEM);

typedef struct THREADPOOL_TAG
{
    volatile_atomic int32_t stop_thread;
    uint32_t max_thread_count;
    uint32_t min_thread_count;
    int32_t used_thread_count;
    volatile_atomic int32_t task_count;

    sem_t semaphore;
    SRW_LOCK_HANDLE srw_lock;
    uint32_t list_index;

    THREADPOOL_TASK* task_array;
    volatile_atomic int32_t task_array_size;
    volatile_atomic int64_t insert_idx;
    volatile_atomic int64_t consume_idx;

    THREAD_HANDLE* thread_handle_array;
} THREADPOOL;

THANDLE_TYPE_DEFINE(THREADPOOL);

// This gives us 1024 timers, plenty for us
// We'll use the remaining bits as epoch to avoid ABA problems (even though ABA is highly unlikely, we'll make it even less likely :-))
/* Codes_SRS_THREADPOOL_LINUX_01_003: [ 1024 timers shall be supported. ]*/
#define TIMER_TABLE_INDEX_BITS 10
#define TIMER_EPOCH_BITS ((sizeof(uintptr_t) * 8) - TIMER_TABLE_INDEX_BITS)

#define TIMER_TABLE_SIZE (1 << TIMER_TABLE_INDEX_BITS)
#define TIMER_TABLE_SIZE_MASK (TIMER_TABLE_SIZE - 1)
#define TIMER_EPOCH_MAX ((1UL << TIMER_EPOCH_BITS) - 1)

#define TIMER_SIGVAL_TO_EPOCH(x) (x >> TIMER_TABLE_INDEX_BITS)
#define TIMER_SIGVAL_TO_TIMER_INDEX(x) ((x) & TIMER_TABLE_SIZE_MASK)

#define MAKE_SIGVAL_FROM_INDEX_AND_EPOCH(index, epoch) \
    (void*)(uintptr_t)((index & TIMER_TABLE_SIZE_MASK) | (epoch << TIMER_TABLE_INDEX_BITS))

static volatile_atomic int64_t timer_epoch_number;

static call_once_t g_lazy = LAZY_INIT_NOT_DONE;

static CUSTOM_TIMER_DATA timer_table[TIMER_TABLE_SIZE];

static int do_init(void* params)
{
    for (uint32_t i = 0; i < TIMER_TABLE_SIZE; i++)
    {
        /* Codes_SRS_THREADPOOL_LINUX_01_004: [ do_init shall initialize the state for each timer to NOT_USED. ]*/
        (void)interlocked_exchange(&timer_table[i].timer_state, TIMER_NOT_USED);
    }

    /* Codes_SRS_THREADPOOL_LINUX_07_099: [ do_init shall succeed and return 0. ]*/
    return 0;
}

static void on_timer_callback(sigval_t timer_data)
{
    /* Codes_SRS_THREADPOOL_LINUX_45_002: [ on_timer_callback shall extract from the lower 10 bits of timer_data.sival_ptr the information indicating which timer table entry is being triggered. ]*/
    uint32_t timer_index = TIMER_SIGVAL_TO_TIMER_INDEX((uintptr_t)timer_data.sival_ptr);

    /* Codes_SRS_THREADPOOL_LINUX_01_012: [ on_timer_callback shall use the rest of the higher bits of timer_data.sival_ptr as timer epoch. ]*/
    uint64_t timer_epoch = TIMER_SIGVAL_TO_EPOCH((uintptr_t)timer_data.sival_ptr);

    CUSTOM_TIMER_DATA* timer_instance = &timer_table[timer_index];

    /* Codes_SRS_THREADPOOL_LINUX_01_008: [ If the timer is in the state ARMED: ]*/
    /* Codes_SRS_THREADPOOL_LINUX_01_007: [ on_timer_callback shall transition it to CALLING_CALLBACK. ]*/
    if (interlocked_compare_exchange(&timer_instance->timer_state, TIMER_CALLING_CALLBACK, TIMER_ARMED) == TIMER_ARMED)
    {
        if (timer_epoch == timer_instance->epoch_number)
        {
            /* Codes_SRS_THREADPOOL_LINUX_45_004: [ If the timer epoch of the timer table entry is the same like the timer epoch in timer_data.sival_ptr, on_timer_callback shall call the timer's work_function with work_function_ctx. ]*/
            timer_instance->work_function(timer_instance->work_function_ctx);
        }

        /* Codes_SRS_THREADPOOL_LINUX_01_009: [ on_timer_callback shall transition it to ARMED. ]*/
        (void)InterlockedHL_SetAndWake(&timer_instance->timer_state, TIMER_ARMED);
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
                        current_index = (interlocked_increment_64(&threadpool->consume_idx) - 1) % existing_count;
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
                            //do nothing
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
        /* Codes_SRS_THREADPOOL_LINUX_07_085: [ threadpool_work_func shall loop until the flag to stop the threads is not set to 1. ]*/
        } while (interlocked_add(&threadpool->stop_thread, 0) != 1);
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
                for (uint32_t index = existing_count; index < new_task_array_size; index++)
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
                uint32_t move_count = 0;

                /* Codes_SRS_THREADPOOL_LINUX_07_044: [ threadpool_schedule_work shall shall memmove everything between the consume index and the size of the array before resize to the end of the new resized array. ]*/
                if (insert_pos < consume_pos)
                {
                    move_count = existing_count - consume_pos;
                    (void)memmove(&threadpool->task_array[new_task_array_size - move_count], &threadpool->task_array[consume_pos], sizeof(THREADPOOL_TASK)*move_count);
                    (void)interlocked_exchange_64(&threadpool->consume_idx, new_task_array_size - move_count);
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_045: [ threadpool_schedule_work shall reset the consume_idx and insert_idx to 0 after resize the task array. ]*/
                    (void)interlocked_exchange_64(&threadpool->consume_idx, 0);
                    (void)interlocked_exchange_64(&threadpool->insert_idx, existing_count - move_count);
                }

                result = 0;
            }
        }
        /* Codes_SRS_THREADPOOL_LINUX_07_046: [ threadpool_schedule_work shall release the SRW lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(threadpool->srw_lock);
    }
    return result;
}

static void threadpool_dispose(THREADPOOL* threadpool)
{
    /* Codes_SRS_THREADPOOL_LINUX_07_089: [ threadpool_dispose shall signal all threads to return. ]*/
    (void)InterlockedHL_SetAndWakeAll(&threadpool->stop_thread, 1);
    for (uint32_t index = 0; index < threadpool->used_thread_count; index++)
    {
        int dont_care;
        /* Codes_SRS_THREADPOOL_LINUX_07_027: [ threadpool_dispose shall join all threads in the threadpool. ]*/
        if (ThreadAPI_Join(threadpool->thread_handle_array[index], &dont_care) != THREADAPI_OK)
        {
            LogError("Failure joining thread number %" PRIu32 "", index);
        }
        else
        {
            // Everything Okay.
        }
    }

    /* Codes_SRS_THREADPOOL_LINUX_07_016: [ threadpool_dispose shall free the memory allocated in threadpool_create. ]*/
    free(threadpool->task_array);
    free(threadpool->thread_handle_array);

    /* Codes_SRS_THREADPOOL_LINUX_07_014: [ threadpool_dispose shall destroy the semphore by calling sem_destroy. ]*/
    sem_destroy(&threadpool->semaphore);
    /* Codes_SRS_THREADPOOL_LINUX_07_015: [ threadpool_dispose shall destroy the SRW lock by calling srw_lock_destroy. ]*/
    srw_lock_destroy(threadpool->srw_lock);
}

THANDLE(THREADPOOL) threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THREADPOOL* result = NULL;

    /* Codes_SRS_THREADPOOL_LINUX_07_002: [ If execution_engine is NULL, threadpool_create shall fail and return NULL. ]*/
    if (execution_engine == NULL)
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine: %p", execution_engine);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_001: [ threadpool_create shall allocate memory for a threadpool object and on success return a non-NULL handle to it. ]*/
        result = THANDLE_MALLOC(THREADPOOL)(threadpool_dispose);
        if (result == NULL)
        {
            LogError("Failure allocating THREADPOOL structure");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_004: [ threadpool_create shall get the min_thread_count and max_thread_count thread parameters from the execution_engine. ]*/
            const EXECUTION_ENGINE_PARAMETERS* param = execution_engine_linux_get_parameters(execution_engine);

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
                    for (uint32_t index = 0; index < result->task_array_size; index++)
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
                            LogError("Failure creating sem_init");
                        }
                        else
                        {
                            (void)interlocked_exchange(&result->stop_thread, 0);
                            (void)interlocked_exchange(&result->task_count, 0);

                            /* Codes_SRS_THREADPOOL_LINUX_07_010: [ insert_idx and consume_idx for the task array shall be initialized to 0. ]*/
                            (void)interlocked_exchange_64(&result->insert_idx, 0);
                            (void)interlocked_exchange_64(&result->consume_idx, 0);

                            uint32_t index;
                            for (index = 0; index < result->used_thread_count; index++)
                            {
                                /* Codes_SRS_THREADPOOL_LINUX_07_020: [ threadpool_create shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
                                if (ThreadAPI_Create(&result->thread_handle_array[index], threadpool_work_func, result) != THREADAPI_OK)
                                {
                                    /* Codes_SRS_THREADPOOL_LINUX_07_011: [ If any error occurs, threadpool_create shall fail and return NULL. ]*/
                                    LogError("Failure creating thread %" PRIu32 "", index);
                                    break;
                                }
                            }

                            /* Codes_SRS_THREADPOOL_LINUX_07_022: [ If one of the thread creation fails, threadpool_create shall fail and return a non-zero value, terminate all threads already created. ]*/
                            if (index < result->used_thread_count)
                            {
                                (void)interlocked_exchange(&result->stop_thread, 1);
                                for (uint32_t inner = 0; inner < index; inner++)
                                {
                                    int dont_care;
                                    if (ThreadAPI_Join(result->thread_handle_array[inner], &dont_care) != THREADAPI_OK)
                                    {
                                        LogError("Failure joining thread number %" PRIu32 "", inner);
                                    }
                                    else
                                    {
                                        // Everything Okay.
                                    }
                                }
                            }
                            else
                            {
                                goto all_ok;
                            }
                        }
                        srw_lock_destroy(result->srw_lock);
                    }
                    free(result->task_array);
                }
                free(result->thread_handle_array);
            }
            THANDLE_FREE(THREADPOOL)(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

int threadpool_schedule_work(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_ctx)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_07_029: [ If threadpool is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        threadpool == NULL ||
        /* Codes_SRS_THREADPOOL_LINUX_07_030: [ If work_function is NULL, threadpool_schedule_work shall fail and return a non-zero value. ]*/
        work_function == NULL)
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool: %p, THREADPOOL_WORK_FUNCTION work_function: %p, void* work_function_ctx: %p", threadpool, work_function, work_function_ctx);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        do
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_033: [ threadpool_schedule_work shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
            srw_lock_acquire_shared(threadpool_ptr->srw_lock);
            int32_t existing_count = interlocked_add(&threadpool_ptr->task_array_size, 0);

            /* Codes_SRS_THREADPOOL_LINUX_07_034: [ threadpool_schedule_work shall increment the insert_pos. ]*/
            int64_t insert_pos = (interlocked_increment_64(&threadpool_ptr->insert_idx) - 1) % existing_count;

            /* Codes_SRS_THREADPOOL_LINUX_07_035: [ If task state is TASK_NOT_USED, threadpool_schedule_work shall set the current task state to TASK_INITIALIZING. ]*/
            int32_t task_state = interlocked_compare_exchange(&threadpool_ptr->task_array[insert_pos].task_state, TASK_INITIALIZING, TASK_NOT_USED);

            if (task_state != TASK_NOT_USED)
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_036: [ Otherwise, threadpool_schedule_work shall release the shared SRW lock by calling srw_lock_release_shared and increase task_array capacity: ]*/
                srw_lock_release_shared(threadpool_ptr->srw_lock);

                if (reallocate_threadpool_array(threadpool_ptr) != 0)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_048: [ If reallocating the task array fails, threadpool_schedule_work shall fail and return a non-zero value. ]*/
                    LogError("Failure reallocating threadpool_ptr");
                    result = MU_FAILURE;
                    break;
                }
                continue;
            }
            /* Codes_SRS_THREADPOOL_LINUX_07_049: [ threadpool_schedule_work shall copy the work function and work function context into insert position in the task array and assign 0 to the return variable to indicate success. ] */
            else
            {
                THREADPOOL_TASK* task_item = &threadpool_ptr->task_array[insert_pos];
                task_item->work_function_ctx = work_function_ctx;
                task_item->work_function = work_function;

                /* Codes_SRS_THREADPOOL_LINUX_07_050: [ threadpool_schedule_work shall set the task_state to TASK_WAITING and then release the shared SRW lock. ] */
                (void)interlocked_exchange(&task_item->task_state, TASK_WAITING);
                srw_lock_release_shared(threadpool_ptr->srw_lock);

                /* Codes_SRS_THREADPOOL_LINUX_07_051: [ threadpool_schedule_work shall unblock the threadpool semaphore by calling sem_post. ]*/
                sem_post(&threadpool_ptr->semaphore);

                /* Codes_SRS_THREADPOOL_LINUX_07_047: [ threadpool_schedule_work shall return zero on success. ]*/
                result = 0;
                break;
            }
        } while (true);
    }
    return result;
}

static void threadpool_timer_dispose(THREADPOOL_TIMER* timer)
{
    /* Codes_SRS_THREADPOOL_LINUX_07_071: [ threadpool_timer_dispose shall call timer_delete to destroy the ongoing timers. ]*/
    if (timer_delete(timer->time_id) != 0)
    {
        LogErrorNo("Failure calling timer_delete.");
    }
    else
    {
        // Do Nothing
    }

    CUSTOM_TIMER_DATA* custom_timer_data = timer->custom_timer_data;

    do
    {
        /* Codes_SRS_THREADPOOL_LINUX_01_006: [ If the timer state is ARMED, threadpool_timer_dispose shall set the state of the timer to NOT_USED. ]*/
        int32_t current_timer_state = interlocked_compare_exchange(&custom_timer_data->timer_state, TIMER_NOT_USED, TIMER_ARMED);
        if (current_timer_state == TIMER_ARMED)
        {
            wake_by_address_all(&custom_timer_data->timer_state);
            break;
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_01_010: [ Otherwise, threadpool_timer_dispose shall block until the state is ARMED and reattempt to set the state to NOT_USED. ]*/
            (void)InterlockedHL_WaitForNotValue(&custom_timer_data->timer_state, current_timer_state, UINT32_MAX);
        }
    } while (1);
}

THANDLE(THREADPOOL_TIMER) threadpool_timer_start(THANDLE(THREADPOOL) threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_ctx)
{
    THREADPOOL_TIMER* result = NULL;

    if (
        /* Codes_SRS_THREADPOOL_LINUX_07_054: [ If threadpool is NULL, threadpool_timer_start shall fail and return NULL. ]*/
        threadpool == NULL ||
        /* Codes_SRS_THREADPOOL_LINUX_07_055: [ If work_function is NULL, threadpool_timer_start shall fail and return NULL. ]*/
        work_function == NULL
        )
    {
        LogError("Invalid args: THANDLE(THREADPOOL) threadpool = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 ", THREADPOOL_WORK_FUNCTION work_function = %p, void* work_function_context = %p",
            threadpool, start_delay_ms, timer_period_ms, work_function, work_function_ctx);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate memory for THANDLE(THREADPOOL_TIMER), passing threadpool_timer_dispose as dispose function, and store work_function and work_function_ctx in it. ]*/
        result = THANDLE_MALLOC(THREADPOOL_TIMER)(threadpool_timer_dispose);
        if (result == NULL)
        {
            LogError("failure in THANDLE_MALLOC(THREADPOOL_TIMER)(threadpool_timer_dispose=%p)",
                        threadpool_timer_dispose);
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_096: [ threadpool_timer_start shall call lazy_init with do_init as initialization function.  ]*/
            if(lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_060: [ If any error occurs, threadpool_timer_start shall fail and return NULL. ]*/
                LogError("failure in lazy_init(&g_lazy=%p, do_init=%p, NULL)", &g_lazy, do_init);
            }
            else
            {
                uint32_t i;

                /* Codes_SRS_THREADPOOL_LINUX_01_002: [ threadpool_timer_start shall find an unused entry in the timers table maintained by the module. ]*/
                for (i = 0; i < TIMER_TABLE_SIZE; i++)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_01_005: [ If an unused entry is found, it's state shall be marked as ARMED. ]*/
                    if (interlocked_compare_exchange(&timer_table[i].timer_state, TIMER_ARMED, TIMER_NOT_USED) == TIMER_NOT_USED)
                    {
                        // found an unused timer entry
                        break;
                    }
                }

                if (i == TIMER_TABLE_SIZE)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_01_001: [ If all timer entries are used, threadpool_timer_start shall fail and return NULL. ]*/
                    LogError("No timers available");
                }
                else
                {
                    // point custom data to the proper place in the table
                    CUSTOM_TIMER_DATA* custom_timer_data = &timer_table[i];
                    result->custom_timer_data = custom_timer_data;

                    /* Codes_SRS_THREADPOOL_LINUX_01_011: [ threadpool_timer_start shall increment the timer epoch number and store it in the selected entry in the timer table. ]*/
                    do
                    {
                        int64_t current_epoch_number = interlocked_add_64(&timer_epoch_number, 0);
                        int64_t new_epoch_number;
                        if (timer_epoch_number == TIMER_EPOCH_MAX)
                        {
                            new_epoch_number = 0;
                        }
                        else
                        {
                            new_epoch_number = current_epoch_number + 1;
                        }
                        if (interlocked_compare_exchange_64(&timer_epoch_number, new_epoch_number, current_epoch_number) == current_epoch_number)
                        {
                            custom_timer_data->epoch_number = new_epoch_number;
                            break;
                        }
                        else
                        {
                            // somethiung changed, try again
                        }
                    } while (1);
                    
                    custom_timer_data->work_function = work_function;
                    /* Codes_SRS_THREADPOOL_LINUX_07_057: [ work_function_ctx shall be allowed to be NULL. ]*/
                    custom_timer_data->work_function_ctx = work_function_ctx;
    
                    struct sigevent sigev = {0};
                    timer_t time_id = 0;
    
                    sigev.sigev_notify          = SIGEV_THREAD;
                    sigev.sigev_notify_function = on_timer_callback;
                    sigev.sigev_value.sival_ptr = MAKE_SIGVAL_FROM_INDEX_AND_EPOCH(i, custom_timer_data->epoch_number);
    
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
                            /* Codes_SRS_THREADPOOL_LINUX_07_062: [ threadpool_timer_start shall succeed and return a non-NULL handle. ]*/
                            result->time_id = time_id;
                            goto all_ok;
                        }
                        /* Codes_SRS_THREADPOOL_LINUX_07_063: [ If timer_settime fails, threadpool_timer_start shall delete the timer by calling timer_delete. ]*/
                        if (timer_delete(time_id) != 0)
                        {
                            LogErrorNo("Failure calling timer_delete.");
                        }
                    }

                    (void)interlocked_exchange(&custom_timer_data[i].timer_state, TIMER_NOT_USED);
                }
            }

            THANDLE_FREE(THREADPOOL_TIMER)(result);
        }
        /* Codes_SRS_THREADPOOL_LINUX_07_060: [ If any error occurs, threadpool_timer_start shall fail and return NULL. ]*/
        result = NULL;
    }
all_ok:
    return result;
}

int threadpool_timer_restart(THANDLE(THREADPOOL_TIMER) timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    int result;
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_064: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
        LogError("Invalid args: THANDLE(THREADPOOL_TIMER) timer = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 "",
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

        THREADPOOL_TIMER * timer_content = THANDLE_GET_T(THREADPOOL_TIMER)(timer);

        /* Codes_SRS_THREADPOOL_LINUX_07_065: [ threadpool_timer_restart shall call timer_settime to change the delay and period. ]*/
        if (timer_settime(timer_content->time_id, 0, &its, NULL) != 0)
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

void threadpool_timer_cancel(THANDLE(THREADPOOL_TIMER) timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_068: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
        LogError("Invalid args: THANDLE(THREADPOOL_TIMER) timer = %p", timer);
    }
    else
    {
        THREADPOOL_TIMER * timer_content = THANDLE_GET_T(THREADPOOL_TIMER)(timer);

        struct itimerspec its = {0};
        /* Codes_SRS_THREADPOOL_LINUX_07_069: [ threadpool_timer_cancel shall call timer_settime with 0 for flags and NULL for old_value and {0} for new_value to cancel the ongoing timers. ]*/
        if (timer_settime(timer_content->time_id, 0, &its, NULL) != 0)
        {
            LogErrorNo("Failure calling timer_settime");
        }
        else
        {
            // Do Nothing
        }
    }
}

THANDLE(THREADPOOL_WORK_ITEM) threadpool_create_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item_ptr = NULL;

    if (
        /* Codes_SRS_THREADPOOL_LINUX_05_001: [ If threadpool is NULL, threadpool_create_work_item shall fail and return a NULL value. ] */
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_05_002: [ If work_function is NULL, threadpool_create_work_item shall fail and return a NULL value. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool: %p, THREADPOOL_WORK_FUNCTION work_function: %p, void* work_function_ctx: %p", threadpool, work_function, work_function_context);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_05_005: [ threadpool_create_work_item shall allocate memory for threadpool_work_item of type THREADPOOL_WORK_ITEM_HANDLE. ] */
        threadpool_work_item_ptr = THANDLE_MALLOC(THREADPOOL_WORK_ITEM)(NULL);

        if (threadpool_work_item_ptr == NULL)
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_006: [ If during the initialization of threadpool_work_item, malloc fails then threadpool_create_work_item shall fail and return a NULL value. ]*/
            LogError("Could not allocate memory for Work Item Context");
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_007: [ threadpool_create_work_item shall copy the work_function and work_function_context into the threadpool work item. ] */
            threadpool_work_item_ptr->work_function_ctx = work_function_context;
            threadpool_work_item_ptr->work_function = work_function;
        }
    }
    return threadpool_work_item_ptr;
}

int threadpool_schedule_work_item(THANDLE(THREADPOOL) threadpool, THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_05_010: [ If threadpool is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_05_011: [ If threadpool_work_item is NULL, threadpool_schedule_work_item shall fail and return a non-zero value. ] */
        (threadpool_work_item == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool: %p, THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item: %p", threadpool, threadpool_work_item);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        do
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_014: [ threadpool_schedule_work_item shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
            srw_lock_acquire_shared(threadpool_ptr->srw_lock);
            int32_t existing_count = interlocked_add(&threadpool_ptr->task_array_size, 0);

            /* Codes_SRS_THREADPOOL_LINUX_05_015: [ threadpool_schedule_work_item shall increment the insert_pos. ]*/
            int64_t insert_pos = (interlocked_increment_64(&threadpool_ptr->insert_idx) - 1) % existing_count;

            /* Codes_SRS_THREADPOOL_LINUX_05_016: [ If task state is TASK_NOT_USED, threadpool_schedule_work_item shall set the current task state to TASK_INITIALIZING. ]*/
            int32_t task_state = interlocked_compare_exchange(&threadpool_ptr->task_array[insert_pos].task_state, TASK_INITIALIZING, TASK_NOT_USED);

            /* Codes_SRS_THREADPOOL_LINUX_05_017: [ threadpool_schedule_work_item shall release the shared SRW lock by calling srw_lock_release_shared. ]*/
            srw_lock_release_shared(threadpool_ptr->srw_lock);
            /* Codes_SRS_THREADPOOL_LINUX_05_018: [ If the previous task state is not TASK_NOT_USED then threadpool_schedule_work_item shall increase task_array capacity. ]*/
            if (task_state != TASK_NOT_USED)
            {
                if (reallocate_threadpool_array(threadpool_ptr) != 0)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_05_019: [ If reallocating the task array fails, threadpool_schedule_work_item shall fail and return a non-zero value. ]*/
                    LogError("Failure reallocating threadpool_ptr");
                    result = MU_FAILURE;
                    break;
                }
                continue;
            }
            else
            {
                THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item_ptr = THANDLE_GET_T(THREADPOOL_WORK_ITEM)(threadpool_work_item);
                /* Codes_SRS_THREADPOOL_LINUX_05_020: [ threadpool_schedule_work_item shall acquire the SRW lock in shared mode by calling srw_lock_acquire_exclusive. ]*/
                srw_lock_acquire_shared(threadpool_ptr->srw_lock);
                THREADPOOL_TASK* task_item = &threadpool_ptr->task_array[insert_pos];

                /* Codes_SRS_THREADPOOL_LINUX_05_022: [ threadpool_schedule_work_item shall copy the work_function and work_function_context from threadpool_work_item into insert position in the task array. ]*/
                task_item->work_function_ctx = threadpool_work_item_ptr->work_function_ctx;
                task_item->work_function = threadpool_work_item_ptr->work_function;

                /* Codes_SRS_THREADPOOL_LINUX_05_023: [ threadpool_schedule_work_item shall set the task_state to TASK_WAITING and then release the shared SRW lock by calling srw_lock_release_exclusive. ]*/
                (void)interlocked_exchange(&task_item->task_state, TASK_WAITING);

                srw_lock_release_shared(threadpool_ptr->srw_lock);

                /* Codes_SRS_THREADPOOL_LINUX_05_025: [ threadpool_schedule_work_item shall unblock the threadpool semaphore by calling sem_post. ]*/
                sem_post(&threadpool_ptr->semaphore);

                /* Codes_SRS_THREADPOOL_LINUX_05_026: [ threadpool_schedule_work_item shall succeed and return 0. ]*/
                result = 0;
                break;
            }
        } while (true);
    }
    return result;
}
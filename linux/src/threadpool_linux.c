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
#include "c_pal/srw_lock.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"
#include "c_pal/sm.h"
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

typedef struct TIMER_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
    timer_t time_id;
    SRW_LOCK_LL timer_lock;
} TIMER;

THANDLE_TYPE_DEFINE(TIMER);

typedef struct THREADPOOL_TASK_TAG
{
    volatile_atomic int32_t task_state;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
    volatile uint32_t pending_api_calls;
    volatile_atomic int32_t *pending_work_item_count_ptr;
} THREADPOOL_TASK;

typedef struct THREADPOOL_WORK_ITEM_TAG
{
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_ctx;
    volatile_atomic int32_t pending_work_item_count;
} THREADPOOL_WORK_ITEM, * THREADPOOL_WORK_ITEM_HANDLE;

typedef struct THREADPOOL_TAG
{
    volatile_atomic int32_t run_thread;
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

THANDLE_TYPE_DEFINE(THREADPOOL);

static void on_timer_callback(sigval_t timer_data)
{
    /* Codes_SRS_THREADPOOL_LINUX_45_002: [ on_timer_callback shall set the timer instance to timer_data.sival_ptr. ]*/
    /* Codes_SRS_THREADPOOL_LINUX_45_001: [ If timer instance is NULL, then on_timer_callback shall return. ]*/
    TIMER* timer_instance = timer_data.sival_ptr;
    if (timer_instance == NULL)
    {
        LogError("invalid timer_data.sival_ptr=%p", timer_instance);
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_45_004: [ on_timer_callback shall call the timer's work_function with work_function_ctx. ]*/
        timer_instance->work_function(timer_instance->work_function_ctx);
    }
}

static void internal_close(THREADPOOL* threadpool)
{
    /* Codes_SRS_THREADPOOL_LINUX_07_026: [ Otherwise, threadpool_close shall call sm_close_begin. ]*/
    if(sm_close_begin(threadpool->sm) == SM_EXEC_GRANTED)
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_089: [ threadpool_close shall signal all threads threadpool is closing by calling InterlockedHL_SetAndWakeAll. ]*/
        (void)InterlockedHL_SetAndWakeAll(&threadpool->run_thread, 1);
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
                        /* Codes_SRS_THREADPOOL_LINUX_05_039: [ threadpool_work_func shall acquire the shared SRW lock by calling srw_lock_acquire_shared. ]*/
                        srw_lock_acquire_shared(threadpool->srw_lock);
                        /* Codes_SRS_THREADPOOL_LINUX_05_040: [ threadpool_work_func shall save pending_work_item_count_ptr is not NULL in is_pending_work_item_count_ptr_not_null. ]*/
                        bool is_pending_work_item_count_ptr_not_null = (NULL != threadpool->task_array[current_index].pending_work_item_count_ptr);
                        /* Codes_SRS_THREADPOOL_LINUX_05_041: [ threadpool_work_func shall release the shared SRW lock by calling srw_lock_release_shared. ]*/
                        srw_lock_release_shared(threadpool->srw_lock);
                        /* Codes_SRS_THREADPOOL_LINUX_05_042: [ If the is_pending_work_item_count_ptr_not_null is TRUE then: ]*/
                        if (is_pending_work_item_count_ptr_not_null)
                        {
                            /* Codes_SRS_THREADPOOL_LINUX_05_043: [ threadpool_work_func shall acquire the exclusive SRW lock by calling srw_lock_acquire_exclusive. ]*/
                            srw_lock_acquire_exclusive(threadpool->srw_lock);
                            /* Codes_SRS_THREADPOOL_LINUX_05_044: [ threadpool_work_func shall decrement the pending_work_item_count_ptr by calling interlocked_decrement. ]*/
                            interlocked_decrement(threadpool->task_array[current_index].pending_work_item_count_ptr);
                            /* Codes_SRS_THREADPOOL_LINUX_05_045: [ threadpool_work_func shall send wake up signal to single listener for the address in pending_work_item_count_ptr. ]*/
                            wake_by_address_single(threadpool->task_array[current_index].pending_work_item_count_ptr);
                            /* Codes_SRS_THREADPOOL_LINUX_05_046: [ threadpool_work_func shall release the shared SRW lock by calling srw_lock_release_exclusive. ]*/
                            srw_lock_release_exclusive(threadpool->srw_lock);
                        }
                    }
                }
            }
        /* Codes_SRS_THREADPOOL_LINUX_07_085: [ threadpool_work_func shall loop until threadpool_close or threadpool_destroy is called. ]*/
        } while (interlocked_add(&threadpool->run_thread, 0) != 1);
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
                    temp_array[index].pending_work_item_count_ptr = NULL;
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
                        for (int32_t index = 0; index < result->task_array_size; index++)
                        {
                            result->task_array[index].work_function = NULL;
                            result->task_array[index].work_function_ctx = NULL;
                            result->task_array[index].pending_work_item_count_ptr = NULL;
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
                                (void)interlocked_exchange(&result->run_thread, 0);
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
            THANDLE_FREE(THREADPOOL)(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

int threadpool_open(THANDLE(THREADPOOL) threadpool)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_07_017: [ If threadpool is NULL, threadpool_open shall fail and return a non-zero value. ]*/
        threadpool == NULL
    )
    {
        LogError("THANDLE(THREADPOOL) threadpool=%p", threadpool);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        /* Codes_SRS_THREADPOOL_LINUX_07_018: [ threadpool_open shall call sm_open_begin. ]*/
        SM_RESULT open_result = sm_open_begin(threadpool_ptr->sm);
        if(open_result != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_THREADPOOL_LINUX_07_019: [ If sm_open_begin indicates the open cannot be performed, threadpool_open shall fail and return a non-zero value. ]*/
            LogError("sm_open_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
            result = MU_FAILURE;
        }
        else
        {
            int32_t array_size = interlocked_add(&threadpool_ptr->task_array_size, 0);
            // Codes_SRS_THREADPOOL_LINUX_11_001: [ threadpool_open shall initialize internal threapool data items ]
            for (int32_t index = 0; index < array_size; index++)
            {
                threadpool_ptr->task_array[index].work_function = NULL;
                threadpool_ptr->task_array[index].work_function_ctx = NULL;
                (void)interlocked_exchange(&threadpool_ptr->task_array[index].task_state, TASK_NOT_USED);
            }
            (void)interlocked_exchange(&threadpool_ptr->run_thread, 0);
            (void)interlocked_exchange_64(&threadpool_ptr->insert_idx, 0);
            (void)interlocked_exchange_64(&threadpool_ptr->consume_idx, 0);

            int32_t index;
            for (index = 0; index < threadpool_ptr->used_thread_count; index++)
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_020: [ threadpool_open shall create number of min_thread_count threads for threadpool using ThreadAPI_Create. ]*/
                if (ThreadAPI_Create(&threadpool_ptr->thread_handle_array[index], threadpool_work_func, threadpool_ptr) != THREADAPI_OK)
                {
                    /* Codes_SRS_THREADPOOL_LINUX_07_021: [ If any error occurs, threadpool_open shall fail and return a non-zero value. ]*/
                    LogError("Failure creating thread %" PRId32 "", index);
                    break;
                }
            }

            /* Codes_SRS_THREADPOOL_LINUX_07_022: [ If one of the thread creation fails, threadpool_open shall fail and return a non-zero value, terminate all threads already created. ]*/
            if (index < threadpool_ptr->used_thread_count)
            {
                for (int32_t inner = 0; inner < index; inner++)
                {
                    int dont_care;
                    if (ThreadAPI_Join(threadpool_ptr->thread_handle_array[inner], &dont_care) != THREADAPI_OK)
                    {
                        LogError("Failure joining thread number %" PRId32 "", inner);
                    }
                }
                sm_open_end(threadpool_ptr->sm, false);
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_07_023: [ Otherwise, threadpool_open shall shall call sm_open_end with true for success. ]*/
                sm_open_end(threadpool_ptr->sm, true);
                /* Codes_SRS_THREADPOOL_LINUX_07_024: [ threadpool_open shall succeed and return zero. ]*/
                result = 0;
            }
        }
    }
    return result;
}

void threadpool_close(THANDLE(THREADPOOL) threadpool)
{
    /* Codes_SRS_THREADPOOL_LINUX_07_025: [ If threadpool is NULL, threadpool_close shall fail and return. ]*/
    if (threadpool == NULL)
    {
        LogError("THANDLE(THREADPOOL) threadpool=%p", threadpool);
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);
        internal_close(threadpool_ptr);
    }
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

        /* Codes_SRS_THREADPOOL_LINUX_07_031: [ threadpool_schedule_work shall call sm_exec_begin. ]*/
        SM_RESULT sm_result = sm_exec_begin(threadpool_ptr->sm);
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
                /* Codes_SRS_THREADPOOL_LINUX_07_049: [ threadpool_schedule_work shall initialize pending_work_item_count_ptr with NULL then copy the work function and work function context into insert position in the task array and assign 0 to the return variable to indicate success. ] */
                else
                {
                    THREADPOOL_TASK* task_item = &threadpool_ptr->task_array[insert_pos];
                    task_item->work_function_ctx = work_function_ctx;
                    task_item->work_function = work_function;
                    task_item->pending_work_item_count_ptr = NULL;

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
            /* Codes_SRS_THREADPOOL_LINUX_07_053: [ threadpool_schedule_work shall call sm_exec_end. ]*/
            sm_exec_end(threadpool_ptr->sm);
        }
    }
    return result;
}

static void threadpool_timer_dispose(TIMER * timer)
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
    srw_lock_ll_deinit(&timer->timer_lock);
    // Even though timer_delete does cancel any events, there is a small window where an event was triggered just before and a thread is being created.
    // So the callback can still execute after the timer_delete call. A small wait here keeps the timer instance alive long enough for
    // the callback to use it if we hit this window.

    // This doesn't fix the problem, this won't 100% guarantee the thread will be executed before we free the timer data. There are a couple of
    // options to fix the problem, but the way that fits this threadpool model best would be to keep a pool of timers in the threadpool,
    // and manage each state as in-use or not. That way the timer data allocation is not dependent on threadpool_timer_start/destroy.
    // To reproduce this problem, create a large number of threads that delete the timer at the same time it is due to expire and remove the pause
    // above.
}

int threadpool_timer_start(THANDLE(THREADPOOL) threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_ctx, THANDLE(TIMER)* timer_handle)
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
        LogError("Invalid args: THANDLE(THREADPOOL) threadpool = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 ", THREADPOOL_WORK_FUNCTION work_function = %p, void* work_function_context = %p, TIMER ** timer_handle = %p",
            threadpool, start_delay_ms, timer_period_ms, work_function, work_function_ctx, timer_handle);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_058: [ threadpool_timer_start shall allocate memory for THANDLE(TIMER), passing threadpool_timer_dispose as dispose function, and store work_function and work_function_ctx in it. ]*/
        THANDLE(TIMER) timer_temp = THANDLE_MALLOC(TIMER)(threadpool_timer_dispose);
        if (timer_temp == NULL)
        {
            LogError("failure in THANDLE_MALLOC(TIMER)(threadpool_timer_dispose=%p)",
                        threadpool_timer_dispose);
        }
        else
        {
            TIMER * timer_instance = THANDLE_GET_T(TIMER)(timer_temp);

            if (srw_lock_ll_init(&timer_instance->timer_lock) != 0)
            {
                LogError("srw_lock_ll_init failed");
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
                        THANDLE_INITIALIZE_MOVE(TIMER)(timer_handle, &timer_temp);
                        result = 0;
                        goto all_ok;
                    }
                    /* Codes_SRS_THREADPOOL_LINUX_07_063: [ If timer_settime fails, threadpool_timer_start shall delete the timer by calling timer_delete. ]*/
                    if (timer_delete(time_id) != 0)
                    {
                        LogErrorNo("Failure calling timer_delete.");
                    }
                }
            }

            THANDLE_ASSIGN(TIMER)(&timer_temp, NULL);
        }
        /* Codes_SRS_THREADPOOL_LINUX_07_060: [ If any error occurs, threadpool_timer_start shall fail and return a non-zero value. ]*/
        result = MU_FAILURE;
    }
all_ok:
    return result;
}

int threadpool_timer_restart(THANDLE(TIMER) timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    int result;
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_064: [ If timer is NULL, threadpool_timer_restart shall fail and return a non-zero value. ]*/
        LogError("Invalid args: TIMER * timer = %p, uint32_t start_delay_ms = %" PRIu32 ", uint32_t timer_period_ms = %" PRIu32 "",
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

        TIMER * timer_content = THANDLE_GET_T(TIMER)(timer);
        srw_lock_ll_acquire_exclusive(&timer_content->timer_lock);

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
        srw_lock_ll_release_exclusive(&timer_content->timer_lock);
    }
    return result;
}

void threadpool_timer_cancel(THANDLE(TIMER) timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_THREADPOOL_LINUX_07_068: [ If timer is NULL, threadpool_timer_cancel shall fail and return. ]*/
        LogError("Invalid args: TIMER * timer = %p", timer);
    }
    else
    {
        TIMER * timer_content = THANDLE_GET_T(TIMER)(timer);

        srw_lock_ll_acquire_exclusive(&timer_content->timer_lock);

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
        srw_lock_ll_release_exclusive(&timer_content->timer_lock);
    }
}

THREADPOOL_WORK_ITEM_HANDLE threadpool_create_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item = NULL;

    if (
        /* Codes_SRS_THREADPOOL_LINUX_05_001: [ If threadpool is NULL, threadpool_create_work_item shall fail and set the return variable threadpool_work_item a NULL value. ] */
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_05_002: [ If work_function is NULL, threadpool_create_work_item shall fail and set the return variable threadpool_work_item a NULL value. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool: %p, THREADPOOL_WORK_FUNCTION work_function: %p, void* work_function_ctx: %p", threadpool, work_function, work_function_context);
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        /* Codes_SRS_THREADPOOL_LINUX_05_003: [ threadpool_create_work_item` shall call sm_exec_begin. ] */
        SM_RESULT open_result = sm_exec_begin(threadpool_ptr->sm);
        if (open_result != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_004: [ If sm_exec_begin returns SM_EXEC_REFUSED, threadpool_create_work_item shall fail and set the return variable threadpool_work_item a NULL value. ]*/
            LogError("sm_exec_begin failed with %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, open_result));
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_005: [ threadpool_create_work_item shall allocate memory for threadpool_work_item of type THREADPOOL_WORK_ITEM_HANDLE. ] */
            threadpool_work_item = (THREADPOOL_WORK_ITEM_HANDLE)malloc(sizeof(THREADPOOL_WORK_ITEM));

            if (threadpool_work_item == NULL)
            {
                /* Codes_SRS_THREADPOOL_LINUX_05_006: [ If during the initialization of threadpool_work_item, malloc fails then threadpool_create_work_item shall fail. ]*/
                LogError("Could not allocate memory for Work Item Context");
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_05_007: [ threadpool_create_work_item shall initialize pending_work_item_count to 0 then copy the work_function and work_function_context into the threadpool_work_item and an initialized threadpool_work_item when returned indicates success. ] */
                interlocked_exchange(&threadpool_work_item->pending_work_item_count, 0);
                threadpool_work_item->work_function_ctx = work_function_context;
                threadpool_work_item->work_function = work_function;
            }
            /* Codes_SRS_THREADPOOL_LINUX_05_008: [ threadpool_create_work_item shall call sm_exec_end. ]*/
            sm_exec_end(threadpool_ptr->sm);
        }
    }
    /* Codes_SRS_THREADPOOL_LINUX_05_009: [ Return the value inside threadpool_work_item ]*/
    return threadpool_work_item;
}

int threadpool_schedule_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item)
{
    int result;
    if (
        /* Codes_SRS_THREADPOOL_LINUX_05_010: [ If threadpool is NULL, threadpool_schedule_work_item shall fail and set the return variable with a non-zero value. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_05_011: [ If threadpool_work_item is NULL, threadpool_schedule_work_item shall fail and set the return variable with a non-zero value. ] */
        (threadpool_work_item == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool: %p, THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item: %p", threadpool, threadpool_work_item);
        result = MU_FAILURE;
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        /* Codes_SRS_THREADPOOL_LINUX_05_012: [ threadpool_schedule_work_item shall call sm_exec_begin. ]*/
        SM_RESULT sm_result = sm_exec_begin(threadpool_ptr->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_013: [ If sm_exec_begin returns SM_EXEC_REFUSED, threadpool_schedule_work_item shall fail and set the return variable a non-zero value. ]*/
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = MU_FAILURE;
        }
        else
        {
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
                        /* Codes_SRS_THREADPOOL_LINUX_05_019: [ If reallocating the task array fails, threadpool_schedule_work_item shall fail by setting the return variable a non-zero value and break. ]*/
                        LogError("Failure reallocating threadpool_ptr");
                        result = MU_FAILURE;
                        break;
                    }
                    continue;
                }
                else
                {
                    /* Codes_SRS_THREADPOOL_LINUX_05_020: [ threadpool_schedule_work_item shall acquire the SRW lock in exclusive mode by calling srw_lock_acquire_exclusive. ]*/
                    srw_lock_acquire_exclusive(threadpool_ptr->srw_lock);
                    THREADPOOL_TASK* task_item = &threadpool_ptr->task_array[insert_pos];
                    /* Codes_SRS_THREADPOOL_LINUX_05_021: [ threadpool_schedule_work_item shall increment the pending_work_item_count and copy its address to pending_work_item_count_ptr into insert position in the task array. ]*/
                    interlocked_increment(&threadpool_work_item->pending_work_item_count);
                    task_item->pending_work_item_count_ptr = &threadpool_work_item->pending_work_item_count;

                    /* Codes_SRS_THREADPOOL_LINUX_05_022: [ threadpool_schedule_work_item shall copy the work_function and work_function_context from threadpool_work_item into insert position in the task array. ]*/
                    task_item->work_function_ctx = threadpool_work_item->work_function_ctx;
                    task_item->work_function = threadpool_work_item->work_function;

                    /* Codes_SRS_THREADPOOL_LINUX_05_023: [ threadpool_schedule_work_item shall set the task_state to TASK_WAITING and then release the exclusive SRW lock by calling srw_lock_release_exclusive. ]*/
                    interlocked_exchange(&task_item->task_state, TASK_WAITING);

                    /* Codes_SRS_THREADPOOL_LINUX_05_024: [ threadpool_schedule_work_item shall notify a single thread that is waiting for update of this value by a wake signal. ]*/
                    wake_by_address_single(task_item->pending_work_item_count_ptr);

                    srw_lock_release_exclusive(threadpool_ptr->srw_lock);

                    /* Codes_SRS_THREADPOOL_LINUX_05_025: [ threadpool_schedule_work_item shall unblock the threadpool semaphore by calling sem_post. ]*/
                    sem_post(&threadpool_ptr->semaphore);

                    /* Codes_SRS_THREADPOOL_LINUX_05_026: [ threadpool_schedule_work_item shall set the return variable to 0 to indicate success. ]*/
                    result = 0;
                    break;
                }
            } while (true);
            /* Codes_SRS_THREADPOOL_LINUX_05_027: [ threadpool_schedule_work_item shall call sm_exec_end. ]*/
            sm_exec_end(threadpool_ptr->sm);
        }
    }
    /* Codes_SRS_THREADPOOL_LINUX_05_028: [ threadpool_schedule_work_item shall return with the contents of the value of the return variable. ]*/
    return result;
}

void threadpool_destroy_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item)
{
    if (
        /* Codes_SRS_THREADPOOL_LINUX_05_029: [ If threadpool is NULL, threadpool_destroy_work_item shall fail. ]*/
        (threadpool == NULL) ||
        /* Codes_SRS_THREADPOOL_LINUX_05_030: [ If threadpool_work_item is NULL, threadpool_destroy_work_item shall fail. ]*/
        (threadpool_work_item == NULL)
        )
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool: %p, THREADPOOL_WORK_ITEM_HANDLE threadpool_work_item: %p", threadpool, threadpool_work_item);
    }
    else
    {
        THREADPOOL* threadpool_ptr = THANDLE_GET_T(THREADPOOL)(threadpool);

        /* Codes_SRS_THREADPOOL_LINUX_05_031: [ threadpool_destroy_work_item shall call sm_exec_begin ]*/
        SM_RESULT sm_result = sm_exec_begin(threadpool_ptr->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_032: [ If sm_exec_begin returns SM_EXEC_REFUSED, threadpool_destroy_work_item shall fail. ]*/
            LogError("sm_exec_begin failed : %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
        }
        else
        {
            /* Codes_SRS_THREADPOOL_LINUX_05_033: [ Otherwise, threadpool_destroy_work_item shall wait for pending_work_item_count to become 0. ]*/
            if (InterlockedHL_WaitForValue(&threadpool_work_item->pending_work_item_count, 0, UINT32_MAX) == INTERLOCKED_HL_OK)
            {
                /* Codes_SRS_THREADPOOL_LINUX_05_034: [ When pending_work_item_count becomes 0, threadpool_destroy_work_item shall acquire the SRW lock in shared mode by calling srw_lock_acquire_shared. ]*/
                srw_lock_acquire_shared(threadpool_ptr->srw_lock);
                /* Codes_SRS_THREADPOOL_LINUX_05_035: [ threadpool_destroy_work_item shall free the memory allocated to the work item of type THREADPOOL_WORK_ITEM_HANDLE created in threadpool_create_work_item. ]*/
                free(threadpool_work_item);
                /* Codes_SRS_THREADPOOL_LINUX_05_036: [ threadpool_destroy_work_item shall release the shared SRW lock. ]*/
                srw_lock_release_shared(threadpool_ptr->srw_lock);
            }
            else
            {
                /* Codes_SRS_THREADPOOL_LINUX_05_037: [ If InterlockedHL_WaitForValue returns error then log the error and terminate. ]*/
                LogCriticalAndTerminate("Failure in InterlockedHL_WaitForValue(&threadpool_work_item->pending_work_item_count=%p, 0, UINT32_MAX), count was %d", &threadpool_work_item->pending_work_item_count, threadpool_work_item->pending_work_item_count);
            }
        }
        /* Codes_SRS_THREADPOOL_LINUX_05_038: [ threadpool_destroy_work_item shall call sm_exec_end. ]*/
        sm_exec_end(threadpool_ptr->sm);
    }
}
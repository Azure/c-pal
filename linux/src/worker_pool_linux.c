// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/containing_record.h"
#include "c_pal/doublylinkedlist.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_pal/worker_pool_linux.h"

MU_DEFINE_ENUM_STRINGS(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);

#define WORKER_STATE_VALUES \
    WORKER_STATE_RUNNING, \
    WORKER_STATE_IDLE, \
    WORKER_STATE_WOKEN, \
    WORKER_STATE_RETIRED

MU_DEFINE_ENUM(WORKER_STATE, WORKER_STATE_VALUES);

typedef struct WORKER_POOL_TAG WORKER_POOL;

typedef struct WORKER_TAG
{
    WORKER_POOL* worker_pool;
    THREAD_HANDLE thread;
    pthread_cond_t wake_cond;
    WORKER_STATE state;
    uint64_t idle_since_ms;
    DLIST_ENTRY worker_list_entry;
    DLIST_ENTRY idle_list_entry;
} WORKER;

typedef struct WORKER_POOL_TAG
{
    pthread_mutex_t* lock;
    WORKER_POOL_RUN_FUNCTION run_function;
    void* run_function_context;

    DLIST_ENTRY workers;
    DLIST_ENTRY idle_workers;
    DLIST_ENTRY retired_workers;

    uint32_t thread_count;
    uint32_t starting_thread_count;
    uint32_t idle_thread_count;
    uint32_t signaled_thread_count;
    bool is_stopped;
} WORKER_POOL;

static void internal_worker_pool_wait_while_idle(WORKER_POOL* worker_pool, WORKER* worker)
{
    /* Codes_SRS_WORKER_POOL_LINUX_42_049: [ internal_worker_pool_worker_thread_func shall set the state of the worker to idle. ]*/
    worker->state = WORKER_STATE_IDLE;

    /* Codes_SRS_WORKER_POOL_LINUX_42_050: [ internal_worker_pool_worker_thread_func shall record the current time obtained by calling timer_global_get_elapsed_ms as the time the worker became idle. ]*/
    worker->idle_since_ms = (uint64_t)timer_global_get_elapsed_ms();

    /* Codes_SRS_WORKER_POOL_LINUX_42_051: [ internal_worker_pool_worker_thread_func shall insert the worker at the head of the list of idle workers by calling DList_InsertHeadList. ]*/
    DList_InsertHeadList(&worker_pool->idle_workers, &worker->idle_list_entry);

    /* Codes_SRS_WORKER_POOL_LINUX_42_088: [ internal_worker_pool_worker_thread_func shall increment the idle thread count. ]*/
    worker_pool->idle_thread_count++;

    /* Codes_SRS_WORKER_POOL_LINUX_42_052: [ internal_worker_pool_worker_thread_func shall wait by calling pthread_cond_wait on the condition variable of the worker while the worker is idle and the pool is not stopped. ]*/
    while ((worker->state == WORKER_STATE_IDLE) && !worker_pool->is_stopped)
    {
        (void)pthread_cond_wait(&worker->wake_cond, worker_pool->lock);
    }

    if (worker->state != WORKER_STATE_WOKEN)
    {
        /* do nothing */
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_053: [ If the worker was woken: ]*/

        /* Codes_SRS_WORKER_POOL_LINUX_42_089: [ internal_worker_pool_worker_thread_func shall decrement the signaled thread count. ]*/
        worker_pool->signaled_thread_count--;

        /* Codes_SRS_WORKER_POOL_LINUX_42_090: [ internal_worker_pool_worker_thread_func shall set the state of the worker to running. ]*/
        worker->state = WORKER_STATE_RUNNING;
    }

    if (worker->state != WORKER_STATE_IDLE)
    {
        // removed from the list of idle workers by whoever woke or retired it
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_054: [ If the worker is still idle: ]*/
        // the pool is stopped

        /* Codes_SRS_WORKER_POOL_LINUX_42_091: [ internal_worker_pool_worker_thread_func shall remove the worker from the list of idle workers by calling DList_RemoveEntryList. ]*/
        (void)DList_RemoveEntryList(&worker->idle_list_entry);

        /* Codes_SRS_WORKER_POOL_LINUX_42_092: [ internal_worker_pool_worker_thread_func shall decrement the idle thread count. ]*/
        worker_pool->idle_thread_count--;

        /* Codes_SRS_WORKER_POOL_LINUX_42_093: [ internal_worker_pool_worker_thread_func shall set the state of the worker to running. ]*/
        worker->state = WORKER_STATE_RUNNING;
    }
}

static int internal_worker_pool_worker_thread_func(void* arg)
{
    WORKER* worker = arg;
    WORKER_POOL* worker_pool = worker->worker_pool;
    bool is_stopping = false;

    /* Codes_SRS_WORKER_POOL_LINUX_42_040: [ internal_worker_pool_worker_thread_func shall acquire lock by calling pthread_mutex_lock. ]*/
    (void)pthread_mutex_lock(worker_pool->lock);

    /* Codes_SRS_WORKER_POOL_LINUX_42_041: [ internal_worker_pool_worker_thread_func shall decrement the starting thread count. ]*/
    worker_pool->starting_thread_count--;

    /* Codes_SRS_WORKER_POOL_LINUX_42_042: [ Until the worker is retired or the worker stops: ]*/
    while ((worker->state != WORKER_STATE_RETIRED) && !is_stopping)
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_043: [ internal_worker_pool_worker_thread_func shall call run_function with run_function_context. ]*/
        if (worker_pool->run_function(worker_pool->run_function_context))
        {
            // ran work, look for more
        }
        else
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_044: [ If run_function returns false: ]*/
            if (worker_pool->is_stopped)
            {
                /* Codes_SRS_WORKER_POOL_LINUX_42_045: [ If the pool is stopped, internal_worker_pool_worker_thread_func shall stop. ]*/
                is_stopping = true;
            }
            else
            {
                /* Codes_SRS_WORKER_POOL_LINUX_42_046: [ If the pool is not stopped: ]*/
                internal_worker_pool_wait_while_idle(worker_pool, worker);
            }
        }
    }

    /* Codes_SRS_WORKER_POOL_LINUX_42_047: [ internal_worker_pool_worker_thread_func shall release lock by calling pthread_mutex_unlock. ]*/
    (void)pthread_mutex_unlock(worker_pool->lock);

    /* Codes_SRS_WORKER_POOL_LINUX_42_048: [ internal_worker_pool_worker_thread_func shall return 0. ]*/
    return 0;
}

static WORKER* internal_worker_pool_create_worker(WORKER_POOL* worker_pool)
{
    /* Codes_SRS_WORKER_POOL_LINUX_42_031: [ worker_pool_add_worker shall allocate a worker. ]*/
    WORKER* result = malloc(sizeof(WORKER));
    if (result == NULL)
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, worker_pool_add_worker shall free the resources it allocated for the worker. ]*/
        LogError("malloc(sizeof(WORKER)=%zu) failed", sizeof(WORKER));
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_086: [ worker_pool_add_worker shall store worker_pool in the worker. ]*/
        result->worker_pool = worker_pool;

        /* Codes_SRS_WORKER_POOL_LINUX_42_032: [ worker_pool_add_worker shall set the state of the worker to running. ]*/
        result->state = WORKER_STATE_RUNNING;

        /* Codes_SRS_WORKER_POOL_LINUX_42_033: [ worker_pool_add_worker shall initialize the condition variable of the worker by calling pthread_cond_init. ]*/
        if (pthread_cond_init(&result->wake_cond, NULL) != 0)
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, worker_pool_add_worker shall free the resources it allocated for the worker. ]*/
            LogError("pthread_cond_init failed");
        }
        else
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_034: [ worker_pool_add_worker shall start the worker thread by calling ThreadAPI_Create with internal_worker_pool_worker_thread_func and the worker. ]*/
            if (ThreadAPI_Create(&result->thread, internal_worker_pool_worker_thread_func, result) != THREADAPI_OK)
            {
                /* Codes_SRS_WORKER_POOL_LINUX_42_036: [ If any error occurs, worker_pool_add_worker shall free the resources it allocated for the worker. ]*/
                LogError("ThreadAPI_Create failed");
            }
            else
            {
                goto all_ok;
            }
            (void)pthread_cond_destroy(&result->wake_cond);
        }
        free(result);
        result = NULL;
    }
all_ok:
    return result;
}

static void internal_worker_pool_destroy_worker(WORKER* worker)
{
    int thread_result;

    /* Codes_SRS_WORKER_POOL_LINUX_42_096: [ worker_pool_destroy shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
    /* Codes_SRS_WORKER_POOL_LINUX_42_100: [ worker_pool_destroy shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
    /* Codes_SRS_WORKER_POOL_LINUX_42_104: [ worker_pool_join_retired_workers shall wait for the worker thread to exit by calling ThreadAPI_Join. ]*/
    if (ThreadAPI_Join(worker->thread, &thread_result) != THREADAPI_OK)
    {
        LogError("ThreadAPI_Join failed for worker %p, the worker is not freed", worker);
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_097: [ If ThreadAPI_Join succeeds: ]*/
        /* Codes_SRS_WORKER_POOL_LINUX_42_101: [ If ThreadAPI_Join succeeds: ]*/
        /* Codes_SRS_WORKER_POOL_LINUX_42_105: [ If ThreadAPI_Join succeeds: ]*/
        /* Codes_SRS_WORKER_POOL_LINUX_42_098: [ worker_pool_destroy shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
        /* Codes_SRS_WORKER_POOL_LINUX_42_102: [ worker_pool_destroy shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
        /* Codes_SRS_WORKER_POOL_LINUX_42_106: [ worker_pool_join_retired_workers shall destroy the condition variable of the worker by calling pthread_cond_destroy. ]*/
        (void)pthread_cond_destroy(&worker->wake_cond);

        /* Codes_SRS_WORKER_POOL_LINUX_42_099: [ worker_pool_destroy shall free the worker. ]*/
        /* Codes_SRS_WORKER_POOL_LINUX_42_103: [ worker_pool_destroy shall free the worker. ]*/
        /* Codes_SRS_WORKER_POOL_LINUX_42_107: [ worker_pool_join_retired_workers shall free the worker. ]*/
        free(worker);
    }
}

WORKER_POOL_HANDLE worker_pool_create(pthread_mutex_t* lock, WORKER_POOL_RUN_FUNCTION run_function, void* run_function_context)
{
    WORKER_POOL* result;

    if (
        /* Codes_SRS_WORKER_POOL_LINUX_42_001: [ If lock is NULL, worker_pool_create shall fail and return NULL. ]*/
        (lock == NULL) ||
        /* Codes_SRS_WORKER_POOL_LINUX_42_002: [ If run_function is NULL, worker_pool_create shall fail and return NULL. ]*/
        (run_function == NULL)
        )
    {
        LogError("Invalid arguments: pthread_mutex_t* lock=%p, WORKER_POOL_RUN_FUNCTION run_function=%p, void* run_function_context=%p",
            lock, run_function, run_function_context);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_003: [ worker_pool_create shall allocate a new pool. ]*/
        result = malloc(sizeof(WORKER_POOL));
        if (result == NULL)
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_009: [ If any error occurs, worker_pool_create shall fail and return NULL. ]*/
            LogError("malloc(sizeof(WORKER_POOL)=%zu) failed", sizeof(WORKER_POOL));
        }
        else
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_004: [ worker_pool_create shall store lock, run_function and run_function_context in the pool. ]*/
            result->lock = lock;
            result->run_function = run_function;
            result->run_function_context = run_function_context;

            /* Codes_SRS_WORKER_POOL_LINUX_42_005: [ worker_pool_create shall initialize the list of workers, the list of idle workers and the list of retired workers by calling DList_InitializeListHead. ]*/
            DList_InitializeListHead(&result->workers);
            DList_InitializeListHead(&result->idle_workers);
            DList_InitializeListHead(&result->retired_workers);

            /* Codes_SRS_WORKER_POOL_LINUX_42_006: [ worker_pool_create shall set the thread count, the starting thread count, the idle thread count and the signaled thread count to 0. ]*/
            result->thread_count = 0;
            result->starting_thread_count = 0;
            result->idle_thread_count = 0;
            result->signaled_thread_count = 0;

            /* Codes_SRS_WORKER_POOL_LINUX_42_007: [ worker_pool_create shall mark the pool as not stopped. ]*/
            result->is_stopped = false;

            /* Codes_SRS_WORKER_POOL_LINUX_42_008: [ worker_pool_create shall succeed and return the pool. ]*/
        }
    }

    return result;
}

void worker_pool_destroy(WORKER_POOL_HANDLE worker_pool)
{
    if (worker_pool == NULL)
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_010: [ If worker_pool is NULL, worker_pool_destroy shall return. ]*/
        LogError("Invalid arguments: WORKER_POOL_HANDLE worker_pool=%p", worker_pool);
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_011: [ worker_pool_destroy shall acquire lock by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(worker_pool->lock);

        /* Codes_SRS_WORKER_POOL_LINUX_42_012: [ worker_pool_destroy shall mark the pool as stopped. ]*/
        worker_pool->is_stopped = true;

        /* Codes_SRS_WORKER_POOL_LINUX_42_013: [ For each idle worker, worker_pool_destroy shall wake it by calling pthread_cond_signal on its condition variable. ]*/
        for (PDLIST_ENTRY entry = worker_pool->idle_workers.Flink; entry != &worker_pool->idle_workers; entry = entry->Flink)
        {
            WORKER* worker = CONTAINING_RECORD(entry, WORKER, idle_list_entry);
            (void)pthread_cond_signal(&worker->wake_cond);
        }

        /* Codes_SRS_WORKER_POOL_LINUX_42_014: [ worker_pool_destroy shall release lock by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(worker_pool->lock);

        /* Codes_SRS_WORKER_POOL_LINUX_42_015: [ For each worker in the list of workers: ]*/
        while (!DList_IsListEmpty(&worker_pool->workers))
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_082: [ worker_pool_destroy shall remove the worker from the list of workers by calling DList_RemoveHeadList. ]*/
            WORKER* worker = CONTAINING_RECORD(DList_RemoveHeadList(&worker_pool->workers), WORKER, worker_list_entry);

            internal_worker_pool_destroy_worker(worker);
        }

        /* Codes_SRS_WORKER_POOL_LINUX_42_016: [ For each worker in the list of retired workers: ]*/
        while (!DList_IsListEmpty(&worker_pool->retired_workers))
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_084: [ worker_pool_destroy shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
            WORKER* worker = CONTAINING_RECORD(DList_RemoveHeadList(&worker_pool->retired_workers), WORKER, worker_list_entry);

            internal_worker_pool_destroy_worker(worker);
        }

        /* Codes_SRS_WORKER_POOL_LINUX_42_017: [ worker_pool_destroy shall free the pool. ]*/
        free(worker_pool);
    }
}

int worker_pool_add_worker(WORKER_POOL_HANDLE worker_pool)
{
    int result;

    if (worker_pool == NULL)
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_018: [ If worker_pool is NULL, worker_pool_add_worker shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: WORKER_POOL_HANDLE worker_pool=%p", worker_pool);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_019: [ worker_pool_add_worker shall increment the thread count. ]*/
        worker_pool->thread_count++;

        /* Codes_SRS_WORKER_POOL_LINUX_42_020: [ worker_pool_add_worker shall increment the starting thread count. ]*/
        worker_pool->starting_thread_count++;

        /* Codes_SRS_WORKER_POOL_LINUX_42_021: [ worker_pool_add_worker shall release lock by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(worker_pool->lock);

        WORKER* worker = internal_worker_pool_create_worker(worker_pool);

        /* Codes_SRS_WORKER_POOL_LINUX_42_023: [ worker_pool_add_worker shall acquire lock by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(worker_pool->lock);

        if (worker == NULL)
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_024: [ If any error occurs: ]*/
            LogError("internal_worker_pool_create_worker failed, %" PRIu32 " worker threads running", worker_pool->thread_count - 1);

            /* Codes_SRS_WORKER_POOL_LINUX_42_025: [ worker_pool_add_worker shall decrement the thread count. ]*/
            worker_pool->thread_count--;

            /* Codes_SRS_WORKER_POOL_LINUX_42_026: [ worker_pool_add_worker shall decrement the starting thread count. ]*/
            worker_pool->starting_thread_count--;

            /* Codes_SRS_WORKER_POOL_LINUX_42_027: [ worker_pool_add_worker shall fail and return a non-zero value. ]*/
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_028: [ If no error occurs: ]*/

            /* Codes_SRS_WORKER_POOL_LINUX_42_029: [ worker_pool_add_worker shall insert the worker in the list of workers by calling DList_InsertTailList. ]*/
            DList_InsertTailList(&worker_pool->workers, &worker->worker_list_entry);

            /* Codes_SRS_WORKER_POOL_LINUX_42_030: [ worker_pool_add_worker shall succeed and return 0. ]*/
            result = 0;
        }
    }

    return result;
}

WORKER_POOL_WAKE_RESULT worker_pool_wake_idle_worker(WORKER_POOL_HANDLE worker_pool)
{
    WORKER_POOL_WAKE_RESULT result;

    if (worker_pool == NULL)
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_055: [ If worker_pool is NULL, worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_INVALID_ARGS. ]*/
        LogError("Invalid arguments: WORKER_POOL_HANDLE worker_pool=%p", worker_pool);
        result = WORKER_POOL_WAKE_RESULT_INVALID_ARGS;
    }
    else
    {
        if (DList_IsListEmpty(&worker_pool->idle_workers))
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_056: [ If no worker is idle, worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER. ]*/
            result = WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER;
        }
        else
        {
            if (worker_pool->signaled_thread_count != 0)
            {
                /* Codes_SRS_WORKER_POOL_LINUX_42_057: [ If the signaled thread count is not 0, worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_ALREADY_WAKING. ]*/
                // the woken worker wakes the next one if it finds more work to run
                result = WORKER_POOL_WAKE_RESULT_ALREADY_WAKING;
            }
            else
            {
                /* Codes_SRS_WORKER_POOL_LINUX_42_058: [ worker_pool_wake_idle_worker shall remove the worker at the head of the list of idle workers by calling DList_RemoveHeadList. ]*/
                WORKER* worker = CONTAINING_RECORD(DList_RemoveHeadList(&worker_pool->idle_workers), WORKER, idle_list_entry);

                /* Codes_SRS_WORKER_POOL_LINUX_42_094: [ worker_pool_wake_idle_worker shall decrement the idle thread count. ]*/
                worker_pool->idle_thread_count--;

                /* Codes_SRS_WORKER_POOL_LINUX_42_059: [ worker_pool_wake_idle_worker shall set the state of the worker to woken. ]*/
                worker->state = WORKER_STATE_WOKEN;

                /* Codes_SRS_WORKER_POOL_LINUX_42_060: [ worker_pool_wake_idle_worker shall increment the signaled thread count. ]*/
                worker_pool->signaled_thread_count++;

                /* Codes_SRS_WORKER_POOL_LINUX_42_061: [ worker_pool_wake_idle_worker shall wake the worker by calling pthread_cond_signal on its condition variable. ]*/
                (void)pthread_cond_signal(&worker->wake_cond);

                /* Codes_SRS_WORKER_POOL_LINUX_42_062: [ worker_pool_wake_idle_worker shall return WORKER_POOL_WAKE_RESULT_WOKEN. ]*/
                result = WORKER_POOL_WAKE_RESULT_WOKEN;
            }
        }
    }

    return result;
}

void worker_pool_get_counts(WORKER_POOL_HANDLE worker_pool, WORKER_POOL_COUNTS* counts)
{
    if (
        /* Codes_SRS_WORKER_POOL_LINUX_42_063: [ If worker_pool is NULL, worker_pool_get_counts shall return. ]*/
        (worker_pool == NULL) ||
        /* Codes_SRS_WORKER_POOL_LINUX_42_064: [ If counts is NULL, worker_pool_get_counts shall return. ]*/
        (counts == NULL)
        )
    {
        LogError("Invalid arguments: WORKER_POOL_HANDLE worker_pool=%p, WORKER_POOL_COUNTS* counts=%p", worker_pool, counts);
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_065: [ worker_pool_get_counts shall store the thread count, the starting thread count and the idle thread count in counts. ]*/
        counts->thread_count = worker_pool->thread_count;
        counts->starting_thread_count = worker_pool->starting_thread_count;
        counts->idle_thread_count = worker_pool->idle_thread_count;

        if (DList_IsListEmpty(&worker_pool->idle_workers))
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_066: [ If no worker is idle, worker_pool_get_counts shall set oldest_idle_since_ms to 0. ]*/
            counts->oldest_idle_since_ms = 0;
        }
        else
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_067: [ If a worker is idle, worker_pool_get_counts shall set oldest_idle_since_ms to the time the worker at the tail of the list of idle workers became idle. ]*/
            counts->oldest_idle_since_ms = CONTAINING_RECORD(worker_pool->idle_workers.Blink, WORKER, idle_list_entry)->idle_since_ms;
        }
    }
}

void worker_pool_retire_oldest_idle_worker(WORKER_POOL_HANDLE worker_pool)
{
    if (worker_pool == NULL)
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_068: [ If worker_pool is NULL, worker_pool_retire_oldest_idle_worker shall return. ]*/
        LogError("Invalid arguments: WORKER_POOL_HANDLE worker_pool=%p", worker_pool);
    }
    else
    {
        if (DList_IsListEmpty(&worker_pool->idle_workers))
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_069: [ If no worker is idle, worker_pool_retire_oldest_idle_worker shall return. ]*/
            LogError("worker pool %p has no idle worker to retire", worker_pool);
        }
        else
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_070: [ worker_pool_retire_oldest_idle_worker shall remove the worker at the tail of the list of idle workers by calling DList_RemoveTailList. ]*/
            WORKER* worker = CONTAINING_RECORD(DList_RemoveTailList(&worker_pool->idle_workers), WORKER, idle_list_entry);

            /* Codes_SRS_WORKER_POOL_LINUX_42_095: [ worker_pool_retire_oldest_idle_worker shall decrement the idle thread count. ]*/
            worker_pool->idle_thread_count--;

            /* Codes_SRS_WORKER_POOL_LINUX_42_071: [ worker_pool_retire_oldest_idle_worker shall set the state of the worker to retired. ]*/
            worker->state = WORKER_STATE_RETIRED;

            /* Codes_SRS_WORKER_POOL_LINUX_42_072: [ worker_pool_retire_oldest_idle_worker shall wake the worker by calling pthread_cond_signal on its condition variable. ]*/
            (void)pthread_cond_signal(&worker->wake_cond);

            /* Codes_SRS_WORKER_POOL_LINUX_42_073: [ worker_pool_retire_oldest_idle_worker shall decrement the thread count. ]*/
            worker_pool->thread_count--;

            /* Codes_SRS_WORKER_POOL_LINUX_42_074: [ worker_pool_retire_oldest_idle_worker shall remove the worker from the list of workers by calling DList_RemoveEntryList. ]*/
            (void)DList_RemoveEntryList(&worker->worker_list_entry);

            /* Codes_SRS_WORKER_POOL_LINUX_42_075: [ worker_pool_retire_oldest_idle_worker shall insert the worker in the list of retired workers by calling DList_InsertTailList. ]*/
            DList_InsertTailList(&worker_pool->retired_workers, &worker->worker_list_entry);
        }
    }
}

void worker_pool_join_retired_workers(WORKER_POOL_HANDLE worker_pool)
{
    if (worker_pool == NULL)
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_076: [ If worker_pool is NULL, worker_pool_join_retired_workers shall return. ]*/
        LogError("Invalid arguments: WORKER_POOL_HANDLE worker_pool=%p", worker_pool);
    }
    else
    {
        /* Codes_SRS_WORKER_POOL_LINUX_42_077: [ For each worker in the list of retired workers: ]*/
        while (!DList_IsListEmpty(&worker_pool->retired_workers))
        {
            /* Codes_SRS_WORKER_POOL_LINUX_42_078: [ worker_pool_join_retired_workers shall remove the worker from the list of retired workers by calling DList_RemoveHeadList. ]*/
            WORKER* worker = CONTAINING_RECORD(DList_RemoveHeadList(&worker_pool->retired_workers), WORKER, worker_list_entry);

            /* Codes_SRS_WORKER_POOL_LINUX_42_079: [ worker_pool_join_retired_workers shall release lock by calling pthread_mutex_unlock. ]*/
            (void)pthread_mutex_unlock(worker_pool->lock);

            internal_worker_pool_destroy_worker(worker);

            /* Codes_SRS_WORKER_POOL_LINUX_42_081: [ worker_pool_join_retired_workers shall acquire lock by calling pthread_mutex_lock. ]*/
            (void)pthread_mutex_lock(worker_pool->lock);
        }
    }
}

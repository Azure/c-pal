// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/containing_record.h"
#include "c_pal/doublylinkedlist.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/refcount.h"
#include "c_pal/thread_count_policy_linux.h"
#include "c_pal/timer.h"
#include "c_pal/timer_queue_linux.h"
#include "c_pal/worker_pool_linux.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

// the manager thread waits without a deadline when no timer is armed and no thread has to be added or retired
MU_STATIC_ASSERT(TIMER_QUEUE_NO_DUE_TIME == THREAD_COUNT_POLICY_NEVER);

typedef struct EXECUTION_ENGINE_TAG EXECUTION_ENGINE;

typedef void (*FREE_WORK_FUNCTION)(struct EXECUTION_ENGINE_LINUX_WORK_TAG* work);

typedef struct EXECUTION_ENGINE_LINUX_WORK_TAG
{
    EXECUTION_ENGINE* execution_engine;
    EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function;
    void* work_function_context;
    FREE_WORK_FUNCTION free_function;
    uint64_t pending_count;
    uint32_t running_count;
    uint32_t waiter_count;
    bool is_closed;
    DLIST_ENTRY ready_queue_entry;
} EXECUTION_ENGINE_LINUX_WORK;

typedef struct EXECUTION_ENGINE_LINUX_TIMER_TAG
{
    EXECUTION_ENGINE_LINUX_WORK work;
    TIMER_QUEUE_ENTRY timer_queue_entry;
} EXECUTION_ENGINE_LINUX_TIMER;

typedef struct EXECUTION_ENGINE_TAG
{
    EXECUTION_ENGINE_PARAMETERS params;

    pthread_mutex_t lock;
    pthread_cond_t manager_cond;
    pthread_cond_t work_done_cond;
    pthread_t manager_thread;
    bool is_stopping;
    bool is_teardown_deferred;
    bool is_manager_watching_ready_queue;

    DLIST_ENTRY ready_queue;
    uint32_t ready_queue_count;

    THREAD_COUNT_POLICY_HANDLE thread_count_policy;
    TIMER_QUEUE_HANDLE timer_queue;
    WORKER_POOL_HANDLE worker_pool;
} EXECUTION_ENGINE;

DEFINE_REFCOUNT_TYPE(EXECUTION_ENGINE);

// the execution engine whose work function runs on the current thread, NULL if none
static __thread EXECUTION_ENGINE* current_execution_engine = NULL;

// the work object whose work function runs on the current thread, NULL if none or if the thread was disassociated from it
static __thread EXECUTION_ENGINE_LINUX_WORK* current_work = NULL;

static uint64_t internal_execution_engine_linux_get_time_ms(void)
{
    return (uint64_t)timer_global_get_elapsed_ms();
}

static uint64_t internal_execution_engine_linux_get_time_ms_rounded_up(void)
{
    double now = timer_global_get_elapsed_ms();
    uint64_t result = (uint64_t)now;

    if ((double)result >= now)
    {
        // already a whole number of milliseconds
    }
    else
    {
        result++;
    }

    return result;
}

static void internal_execution_engine_linux_free_work(EXECUTION_ENGINE_LINUX_WORK* work)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_045: [ internal_execution_engine_linux_free_work shall free work. ]*/
    free(work);
}

static void internal_execution_engine_linux_free_timer(EXECUTION_ENGINE_LINUX_WORK* work)
{
    EXECUTION_ENGINE_LINUX_TIMER* timer = CONTAINING_RECORD(work, EXECUTION_ENGINE_LINUX_TIMER, work);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_117: [ internal_execution_engine_linux_free_timer shall give back the room reserved for the timer in the timer queue by calling timer_queue_unreserve. ]*/
    timer_queue_unreserve(work->execution_engine->timer_queue);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_118: [ internal_execution_engine_linux_free_timer shall free the timer. ]*/
    free(timer);
}

static void internal_execution_engine_linux_init_work(EXECUTION_ENGINE_LINUX_WORK* work, EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context, FREE_WORK_FUNCTION free_function)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_042: [ internal_execution_engine_linux_init_work shall store execution_engine, work_function, work_function_context and free_function in work. ]*/
    work->execution_engine = execution_engine;
    work->work_function = work_function;
    work->work_function_context = work_function_context;
    work->free_function = free_function;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_043: [ internal_execution_engine_linux_init_work shall set the pending count, the running count and the waiter count of work to 0. ]*/
    work->pending_count = 0;
    work->running_count = 0;
    work->waiter_count = 0;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_044: [ internal_execution_engine_linux_init_work shall mark work as not closed. ]*/
    work->is_closed = false;
}

static void internal_execution_engine_linux_enqueue(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    if (!DList_IsListEmpty(&execution_engine->ready_queue))
    {
        // work already waits in the ready queue
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_055: [ If the ready queue is empty, internal_execution_engine_linux_enqueue shall record progress by calling thread_count_policy_on_progress with the current time obtained by calling timer_global_get_elapsed_ms. ]*/
        // work starts waiting now, so starvation is measured from now
        thread_count_policy_on_progress(execution_engine->thread_count_policy, internal_execution_engine_linux_get_time_ms());
    }

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_056: [ internal_execution_engine_linux_enqueue shall insert work at the tail of the ready queue by calling DList_InsertTailList. ]*/
    DList_InsertTailList(&execution_engine->ready_queue, &work->ready_queue_entry);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_180: [ internal_execution_engine_linux_enqueue shall increment the count of work objects in the ready queue. ]*/
    execution_engine->ready_queue_count++;
}

static void internal_execution_engine_linux_dispatch(EXECUTION_ENGINE* execution_engine)
{
    if (
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_057: [ internal_execution_engine_linux_dispatch shall wake an idle worker thread by calling worker_pool_wake_idle_worker. ]*/
        (worker_pool_wake_idle_worker(execution_engine->worker_pool) != WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER) ||
        execution_engine->is_manager_watching_ready_queue
        )
    {
        // a woken worker thread runs the work, or the manager thread checks the ready queue for progress in time
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_058: [ If worker_pool_wake_idle_worker returns WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER and the manager thread is not watching the ready queue: ]*/
        WORKER_POOL_COUNTS counts;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_059: [ internal_execution_engine_linux_dispatch shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
        worker_pool_get_counts(execution_engine->worker_pool, &counts);

        if (!thread_count_policy_can_add_thread(execution_engine->thread_count_policy, counts.thread_count))
        {
            // no worker thread can be added
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_060: [ If thread_count_policy_can_add_thread returns true for the thread count, internal_execution_engine_linux_dispatch shall wake the manager thread by calling pthread_cond_signal. ]*/
            (void)pthread_cond_signal(&execution_engine->manager_cond);
        }
    }
}

static void internal_execution_engine_linux_submit_work_locked(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_051: [ internal_execution_engine_linux_submit_work_locked shall increment the pending count of work. ]*/
    work->pending_count++;

    if (work->pending_count != 1)
    {
        // work is already in the ready queue
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_052: [ If the pending count of work became 1: ]*/

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_053: [ internal_execution_engine_linux_submit_work_locked shall insert work in the ready queue by calling internal_execution_engine_linux_enqueue. ]*/
        internal_execution_engine_linux_enqueue(execution_engine, work);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_054: [ internal_execution_engine_linux_submit_work_locked shall dispatch the ready queue by calling internal_execution_engine_linux_dispatch. ]*/
        internal_execution_engine_linux_dispatch(execution_engine);
    }
}

static void internal_execution_engine_linux_end_invocation(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_080: [ internal_execution_engine_linux_end_invocation shall decrement the running count of work. ]*/
    work->running_count--;

    if ((work->pending_count != 0) || (work->running_count != 0))
    {
        // work still has pending or running invocations
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_081: [ If work has no pending and no running invocations: ]*/
        if (work->is_closed)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_082: [ If work is closed, internal_execution_engine_linux_end_invocation shall call the free function of work. ]*/
            work->free_function(work);
        }
        else
        {
            if (work->waiter_count == 0)
            {
                // nobody waits for work
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_083: [ If work is not closed and the waiter count of work is not 0, internal_execution_engine_linux_end_invocation shall wake the waiting threads by calling pthread_cond_broadcast. ]*/
                (void)pthread_cond_broadcast(&execution_engine->work_done_cond);
            }
        }
    }
}

static bool internal_execution_engine_linux_run_next_work(void* context)
{
    bool result;
    EXECUTION_ENGINE* execution_engine = context;

    if (DList_IsListEmpty(&execution_engine->ready_queue))
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_061: [ If the ready queue is empty, internal_execution_engine_linux_run_next_work shall return false. ]*/
        result = false;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_062: [ If the ready queue is not empty: ]*/

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_063: [ internal_execution_engine_linux_run_next_work shall remove the work object at the head of the ready queue by calling DList_RemoveHeadList. ]*/
        EXECUTION_ENGINE_LINUX_WORK* work = CONTAINING_RECORD(DList_RemoveHeadList(&execution_engine->ready_queue), EXECUTION_ENGINE_LINUX_WORK, ready_queue_entry);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_181: [ internal_execution_engine_linux_run_next_work shall decrement the count of work objects in the ready queue. ]*/
        execution_engine->ready_queue_count--;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_064: [ internal_execution_engine_linux_run_next_work shall decrement the pending count of the work object. ]*/
        work->pending_count--;

        if (work->pending_count == 0)
        {
            // this is the last pending invocation of work
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_065: [ If the pending count of the work object is not 0: ]*/

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_182: [ internal_execution_engine_linux_run_next_work shall insert the work object at the tail of the ready queue by calling DList_InsertTailList. ]*/
            // taking the work object from the ready queue is progress, recorded below
            DList_InsertTailList(&execution_engine->ready_queue, &work->ready_queue_entry);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_183: [ internal_execution_engine_linux_run_next_work shall increment the count of work objects in the ready queue. ]*/
            execution_engine->ready_queue_count++;
        }

        if (DList_IsListEmpty(&execution_engine->ready_queue))
        {
            // nothing else to run
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_066: [ If the ready queue is not empty, internal_execution_engine_linux_run_next_work shall dispatch the ready queue by calling internal_execution_engine_linux_dispatch. ]*/
            internal_execution_engine_linux_dispatch(execution_engine);
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_067: [ internal_execution_engine_linux_run_next_work shall increment the running count of the work object. ]*/
        work->running_count++;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_068: [ internal_execution_engine_linux_run_next_work shall record progress by calling thread_count_policy_on_progress with the current time obtained by calling timer_global_get_elapsed_ms. ]*/
        thread_count_policy_on_progress(execution_engine->thread_count_policy, internal_execution_engine_linux_get_time_ms());

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_069: [ internal_execution_engine_linux_run_next_work shall record the execution engine as the execution engine running a work function on the current thread. ]*/
        current_execution_engine = execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_070: [ internal_execution_engine_linux_run_next_work shall record the work object as the work object running on the current thread. ]*/
        current_work = work;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_071: [ internal_execution_engine_linux_run_next_work shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_072: [ internal_execution_engine_linux_run_next_work shall call the work function of the work object with its work function context. ]*/
        work->work_function(work->work_function_context);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_073: [ internal_execution_engine_linux_run_next_work shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_074: [ internal_execution_engine_linux_run_next_work shall clear the execution engine running a work function on the current thread. ]*/
        current_execution_engine = NULL;

        if (current_work == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_078: [ If the current thread was disassociated from the work object, internal_execution_engine_linux_run_next_work shall not access the work object, which can be freed by then. ]*/
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_075: [ If the work object is still the work object running on the current thread: ]*/

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_076: [ internal_execution_engine_linux_run_next_work shall clear the work object running on the current thread. ]*/
            current_work = NULL;

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_077: [ internal_execution_engine_linux_run_next_work shall end the invocation of the work object by calling internal_execution_engine_linux_end_invocation. ]*/
            internal_execution_engine_linux_end_invocation(execution_engine, work);
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_079: [ internal_execution_engine_linux_run_next_work shall return true. ]*/
        result = true;
    }

    return result;
}

static void internal_execution_engine_linux_wait_for_invocations(EXECUTION_ENGINE_LINUX_WORK* work, bool cancel_pending)
{
    EXECUTION_ENGINE* execution_engine = work->execution_engine;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_086: [ internal_execution_engine_linux_wait_for_invocations shall acquire the mutex by calling pthread_mutex_lock. ]*/
    (void)pthread_mutex_lock(&execution_engine->lock);

    if (!cancel_pending || (work->pending_count == 0))
    {
        // no invocation to drop
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_087: [ If cancel_pending is true and the pending count of work is not 0: ]*/

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_088: [ internal_execution_engine_linux_wait_for_invocations shall remove work from the ready queue by calling DList_RemoveEntryList. ]*/
        (void)DList_RemoveEntryList(&work->ready_queue_entry);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_184: [ internal_execution_engine_linux_wait_for_invocations shall decrement the count of work objects in the ready queue. ]*/
        execution_engine->ready_queue_count--;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_089: [ internal_execution_engine_linux_wait_for_invocations shall set the pending count of work to 0. ]*/
        work->pending_count = 0;

        if ((work->running_count != 0) || (work->waiter_count == 0))
        {
            // an ending invocation wakes the threads waiting for work, or nobody else waits
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_177: [ If work has no running invocations and the waiter count of work is not 0, internal_execution_engine_linux_wait_for_invocations shall wake the threads waiting for work by calling pthread_cond_broadcast. ]*/
            (void)pthread_cond_broadcast(&execution_engine->work_done_cond);
        }
    }

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_090: [ internal_execution_engine_linux_wait_for_invocations shall increment the waiter count of work. ]*/
    work->waiter_count++;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_091: [ internal_execution_engine_linux_wait_for_invocations shall wait by calling pthread_cond_wait until work has no pending and no running invocations. ]*/
    while ((work->pending_count != 0) || (work->running_count != 0))
    {
        (void)pthread_cond_wait(&execution_engine->work_done_cond, &execution_engine->lock);
    }

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_092: [ internal_execution_engine_linux_wait_for_invocations shall decrement the waiter count of work. ]*/
    work->waiter_count--;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_093: [ internal_execution_engine_linux_wait_for_invocations shall release the mutex by calling pthread_mutex_unlock. ]*/
    (void)pthread_mutex_unlock(&execution_engine->lock);
}

static void internal_execution_engine_linux_close_work_locked(EXECUTION_ENGINE_LINUX_WORK* work)
{
    if ((work->pending_count == 0) && (work->running_count == 0))
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_098: [ If work has no pending and no running invocations, internal_execution_engine_linux_close_work_locked shall call the free function of work. ]*/
        work->free_function(work);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_099: [ If work has pending or running invocations, internal_execution_engine_linux_close_work_locked shall mark work as closed. ]*/
        // the last invocation frees work
        work->is_closed = true;
    }
}

static void internal_execution_engine_linux_disassociate_current_thread(EXECUTION_ENGINE_LINUX_WORK* work)
{
    if (current_work != work)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_102: [ If work is not the work object running on the current thread, internal_execution_engine_linux_disassociate_current_thread shall return. ]*/
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = work->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_103: [ internal_execution_engine_linux_disassociate_current_thread shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_104: [ internal_execution_engine_linux_disassociate_current_thread shall clear the work object running on the current thread. ]*/
        current_work = NULL;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_105: [ internal_execution_engine_linux_disassociate_current_thread shall end the invocation of work by calling internal_execution_engine_linux_end_invocation. ]*/
        internal_execution_engine_linux_end_invocation(execution_engine, work);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_106: [ internal_execution_engine_linux_disassociate_current_thread shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

static void internal_execution_engine_linux_submit_due_timers(EXECUTION_ENGINE* execution_engine, uint64_t now)
{
    TIMER_QUEUE_ENTRY* timer_queue_entry;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_155: [ For each timer returned by timer_queue_pop_due with now: ]*/
    while ((timer_queue_entry = timer_queue_pop_due(execution_engine->timer_queue, now)) != NULL)
    {
        EXECUTION_ENGINE_LINUX_TIMER* timer = CONTAINING_RECORD(timer_queue_entry, EXECUTION_ENGINE_LINUX_TIMER, timer_queue_entry);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_161: [ internal_execution_engine_linux_submit_due_timers shall submit the work object of the timer by calling internal_execution_engine_linux_submit_work_locked. ]*/
        internal_execution_engine_linux_submit_work_locked(execution_engine, &timer->work);
    }
}

static void internal_execution_engine_linux_retire_idle_workers(EXECUTION_ENGINE* execution_engine, uint64_t now)
{
    WORKER_POOL_COUNTS counts;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_162: [ internal_execution_engine_linux_retire_idle_workers shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
    worker_pool_get_counts(execution_engine->worker_pool, &counts);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_163: [ While the time to retire a worker thread obtained by calling thread_count_policy_get_retire_thread_time with the counts and now is not later than now: ]*/
    while (thread_count_policy_get_retire_thread_time(execution_engine->thread_count_policy, counts.thread_count, counts.idle_thread_count, counts.oldest_idle_since_ms, now) <= now)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_164: [ internal_execution_engine_linux_retire_idle_workers shall retire the worker thread idle the longest by calling worker_pool_retire_oldest_idle_worker. ]*/
        worker_pool_retire_oldest_idle_worker(execution_engine->worker_pool);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_165: [ internal_execution_engine_linux_retire_idle_workers shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
        worker_pool_get_counts(execution_engine->worker_pool, &counts);
    }
}

static void internal_execution_engine_linux_add_worker(EXECUTION_ENGINE* execution_engine, uint64_t now)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_166: [ internal_execution_engine_linux_add_worker shall add a worker thread by calling worker_pool_add_worker. ]*/
    if (worker_pool_add_worker(execution_engine->worker_pool) != 0)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_167: [ If worker_pool_add_worker fails, internal_execution_engine_linux_add_worker shall call thread_count_policy_on_add_thread_failed with now. ]*/
        LogError("worker_pool_add_worker failed, retrying later");
        thread_count_policy_on_add_thread_failed(execution_engine->thread_count_policy, now);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_168: [ If worker_pool_add_worker succeeds, internal_execution_engine_linux_add_worker shall record progress by calling thread_count_policy_on_progress with now. ]*/
        // the new worker thread gets time to take work before another one is added
        thread_count_policy_on_progress(execution_engine->thread_count_policy, now);
    }
}

static void internal_execution_engine_linux_wait_for_next_event(EXECUTION_ENGINE* execution_engine, const WORKER_POOL_COUNTS* counts, uint64_t add_thread_time, uint64_t now)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_169: [ internal_execution_engine_linux_wait_for_next_event shall record that the manager thread is watching the ready queue if add_thread_time is not THREAD_COUNT_POLICY_NEVER. ]*/
    // while it watches, submitting work does not need to wake the manager thread
    execution_engine->is_manager_watching_ready_queue = (add_thread_time != THREAD_COUNT_POLICY_NEVER);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_170: [ internal_execution_engine_linux_wait_for_next_event shall get the time to retire a worker thread by calling thread_count_policy_get_retire_thread_time with counts and now. ]*/
    uint64_t retire_thread_time = thread_count_policy_get_retire_thread_time(execution_engine->thread_count_policy, counts->thread_count, counts->idle_thread_count, counts->oldest_idle_since_ms, now);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_171: [ internal_execution_engine_linux_wait_for_next_event shall get the due time of the first timer by calling timer_queue_get_next_due_time. ]*/
    uint64_t next_due_time = timer_queue_get_next_due_time(execution_engine->timer_queue);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_172: [ internal_execution_engine_linux_wait_for_next_event shall compute the deadline as the earliest of add_thread_time, the time to retire a worker thread and the due time of the first timer. ]*/
    uint64_t deadline = (add_thread_time < retire_thread_time) ? add_thread_time : retire_thread_time;
    deadline = (next_due_time < deadline) ? next_due_time : deadline;

    if (deadline == THREAD_COUNT_POLICY_NEVER)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_173: [ If the deadline is THREAD_COUNT_POLICY_NEVER, internal_execution_engine_linux_wait_for_next_event shall wait by calling pthread_cond_wait. ]*/
        (void)pthread_cond_wait(&execution_engine->manager_cond, &execution_engine->lock);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_174: [ If the deadline is not THREAD_COUNT_POLICY_NEVER, internal_execution_engine_linux_wait_for_next_event shall wait until the deadline by calling pthread_cond_timedwait. ]*/
        // manager_cond uses CLOCK_MONOTONIC, the clock of timer_global_get_elapsed_ms
        struct timespec deadline_time;
        deadline_time.tv_sec = (time_t)(deadline / 1000);
        deadline_time.tv_nsec = (long)((deadline % 1000) * 1000000);
        (void)pthread_cond_timedwait(&execution_engine->manager_cond, &execution_engine->lock, &deadline_time);
    }
}

static void internal_execution_engine_linux_teardown(EXECUTION_ENGINE* execution_engine)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_029: [ internal_execution_engine_linux_teardown shall stop the worker threads and wait for them to exit by calling worker_pool_destroy. ]*/
    worker_pool_destroy(execution_engine->worker_pool);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_030: [ internal_execution_engine_linux_teardown shall destroy the condition variable signaled when work objects have no more pending and running invocations by calling pthread_cond_destroy. ]*/
    (void)pthread_cond_destroy(&execution_engine->work_done_cond);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_031: [ internal_execution_engine_linux_teardown shall destroy the condition variable that wakes the manager thread by calling pthread_cond_destroy. ]*/
    (void)pthread_cond_destroy(&execution_engine->manager_cond);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_032: [ internal_execution_engine_linux_teardown shall destroy the mutex by calling pthread_mutex_destroy. ]*/
    (void)pthread_mutex_destroy(&execution_engine->lock);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_033: [ internal_execution_engine_linux_teardown shall destroy the timer queue by calling timer_queue_destroy. ]*/
    timer_queue_destroy(execution_engine->timer_queue);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_034: [ internal_execution_engine_linux_teardown shall destroy the thread count policy by calling thread_count_policy_destroy. ]*/
    thread_count_policy_destroy(execution_engine->thread_count_policy);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_035: [ internal_execution_engine_linux_teardown shall free the execution engine. ]*/
    REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, execution_engine);
}

static void* internal_execution_engine_linux_manager_thread_func(void* arg)
{
    EXECUTION_ENGINE* execution_engine = arg;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_140: [ internal_execution_engine_linux_manager_thread_func shall acquire the mutex by calling pthread_mutex_lock. ]*/
    (void)pthread_mutex_lock(&execution_engine->lock);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_141: [ Until the execution engine is stopping: ]*/
    while (!execution_engine->is_stopping)
    {
        WORKER_POOL_COUNTS counts;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_142: [ internal_execution_engine_linux_manager_thread_func shall get the current time by calling timer_global_get_elapsed_ms. ]*/
        uint64_t now = internal_execution_engine_linux_get_time_ms();

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_143: [ internal_execution_engine_linux_manager_thread_func shall submit the due timers by calling internal_execution_engine_linux_submit_due_timers. ]*/
        internal_execution_engine_linux_submit_due_timers(execution_engine, now);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_144: [ internal_execution_engine_linux_manager_thread_func shall retire the worker threads idle for too long by calling internal_execution_engine_linux_retire_idle_workers. ]*/
        internal_execution_engine_linux_retire_idle_workers(execution_engine, now);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_145: [ internal_execution_engine_linux_manager_thread_func shall join the retired worker threads by calling worker_pool_join_retired_workers. ]*/
        worker_pool_join_retired_workers(execution_engine->worker_pool);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_146: [ internal_execution_engine_linux_manager_thread_func shall get the counts of the worker pool by calling worker_pool_get_counts. ]*/
        worker_pool_get_counts(execution_engine->worker_pool, &counts);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_147: [ internal_execution_engine_linux_manager_thread_func shall get the time to add a worker thread by calling thread_count_policy_get_add_thread_time with the number of work objects in the ready queue and the counts. ]*/
        uint64_t add_thread_time = thread_count_policy_get_add_thread_time(execution_engine->thread_count_policy,
            execution_engine->ready_queue_count, counts.starting_thread_count, counts.idle_thread_count, counts.thread_count);

        if (add_thread_time <= now)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_148: [ If the time to add a worker thread is not later than the current time, internal_execution_engine_linux_manager_thread_func shall add a worker thread by calling internal_execution_engine_linux_add_worker. ]*/
            internal_execution_engine_linux_add_worker(execution_engine, now);
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_149: [ If the time to add a worker thread is later than the current time, internal_execution_engine_linux_manager_thread_func shall wait for the next event by calling internal_execution_engine_linux_wait_for_next_event. ]*/
            internal_execution_engine_linux_wait_for_next_event(execution_engine, &counts, add_thread_time, now);
        }
    }

    bool is_teardown_deferred = execution_engine->is_teardown_deferred;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_150: [ internal_execution_engine_linux_manager_thread_func shall release the mutex by calling pthread_mutex_unlock. ]*/
    (void)pthread_mutex_unlock(&execution_engine->lock);

    if (!is_teardown_deferred)
    {
        // execution_engine_dec_ref joins the manager thread and tears down the execution engine
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_151: [ If the teardown is deferred to the manager thread: ]*/

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_152: [ internal_execution_engine_linux_manager_thread_func shall detach the manager thread by calling pthread_detach. ]*/
        // nobody joins the manager thread
        (void)pthread_detach(execution_engine->manager_thread);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_153: [ internal_execution_engine_linux_manager_thread_func shall tear down the execution engine by calling internal_execution_engine_linux_teardown. ]*/
        internal_execution_engine_linux_teardown(execution_engine);
    }

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_154: [ internal_execution_engine_linux_manager_thread_func shall return NULL. ]*/
    return NULL;
}

static int internal_execution_engine_linux_init_manager_cond(EXECUTION_ENGINE* execution_engine)
{
    int result;
    pthread_condattr_t cond_attributes;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_008: [ internal_execution_engine_linux_init_manager_cond shall initialize a condition variable attribute by calling pthread_condattr_init. ]*/
    if (pthread_condattr_init(&cond_attributes) != 0)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_176: [ If any error occurs, internal_execution_engine_linux_init_manager_cond shall fail. ]*/
        LogError("pthread_condattr_init failed");
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_009: [ internal_execution_engine_linux_init_manager_cond shall set the clock of the attribute to CLOCK_MONOTONIC by calling pthread_condattr_setclock. ]*/
        if (pthread_condattr_setclock(&cond_attributes, CLOCK_MONOTONIC) != 0)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_176: [ If any error occurs, internal_execution_engine_linux_init_manager_cond shall fail. ]*/
            LogError("pthread_condattr_setclock(CLOCK_MONOTONIC) failed");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_010: [ internal_execution_engine_linux_init_manager_cond shall initialize the condition variable that wakes the manager thread with the attribute by calling pthread_cond_init. ]*/
            if (pthread_cond_init(&execution_engine->manager_cond, &cond_attributes) != 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_176: [ If any error occurs, internal_execution_engine_linux_init_manager_cond shall fail. ]*/
                LogError("pthread_cond_init failed");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_175: [ internal_execution_engine_linux_init_manager_cond shall destroy the attribute by calling pthread_condattr_destroy. ]*/
        (void)pthread_condattr_destroy(&cond_attributes);
    }

    return result;
}

static int internal_execution_engine_linux_init_synchronization(EXECUTION_ENGINE* execution_engine)
{
    int result;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_006: [ internal_execution_engine_linux_init_synchronization shall initialize the mutex by calling pthread_mutex_init. ]*/
    if (pthread_mutex_init(&execution_engine->lock, NULL) != 0)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, internal_execution_engine_linux_init_synchronization shall destroy what it initialized and fail. ]*/
        LogError("pthread_mutex_init failed");
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_007: [ internal_execution_engine_linux_init_synchronization shall initialize the condition variable that wakes the manager thread by calling internal_execution_engine_linux_init_manager_cond. ]*/
        if (internal_execution_engine_linux_init_manager_cond(execution_engine) != 0)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, internal_execution_engine_linux_init_synchronization shall destroy what it initialized and fail. ]*/
            LogError("internal_execution_engine_linux_init_manager_cond failed");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_011: [ internal_execution_engine_linux_init_synchronization shall initialize the condition variable signaled when work objects have no more pending and running invocations by calling pthread_cond_init. ]*/
            if (pthread_cond_init(&execution_engine->work_done_cond, NULL) != 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If any error occurs, internal_execution_engine_linux_init_synchronization shall destroy what it initialized and fail. ]*/
                LogError("pthread_cond_init failed");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
                goto all_ok;
            }
            (void)pthread_cond_destroy(&execution_engine->manager_cond);
        }
        (void)pthread_mutex_destroy(&execution_engine->lock);
    }

all_ok:
    return result;
}

static int internal_execution_engine_linux_add_initial_workers(EXECUTION_ENGINE* execution_engine)
{
    int result = 0;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_015: [ internal_execution_engine_linux_start_threads shall add min_thread_count worker threads by calling worker_pool_add_worker. ]*/
    for (uint32_t i = 0; (result == 0) && (i < execution_engine->params.min_thread_count); i++)
    {
        if (worker_pool_add_worker(execution_engine->worker_pool) != 0)
        {
            LogError("Failed adding worker thread %" PRIu32 " of %" PRIu32 "", i, execution_engine->params.min_thread_count);
            result = MU_FAILURE;
        }
        else
        {
            // added a worker thread
        }
    }

    return result;
}

static int internal_execution_engine_linux_start_threads(EXECUTION_ENGINE* execution_engine)
{
    int result;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_013: [ internal_execution_engine_linux_start_threads shall create the worker pool by calling worker_pool_create with the mutex, internal_execution_engine_linux_run_next_work and the execution engine. ]*/
    execution_engine->worker_pool = worker_pool_create(&execution_engine->lock, internal_execution_engine_linux_run_next_work, execution_engine);
    if (execution_engine->worker_pool == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If any error occurs, internal_execution_engine_linux_start_threads shall fail. ]*/
        LogError("worker_pool_create failed");
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_014: [ internal_execution_engine_linux_start_threads shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        int add_result = internal_execution_engine_linux_add_initial_workers(execution_engine);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_016: [ internal_execution_engine_linux_start_threads shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);

        if (add_result != 0)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If any error occurs, internal_execution_engine_linux_start_threads shall fail. ]*/
            LogError("internal_execution_engine_linux_add_initial_workers failed");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_017: [ internal_execution_engine_linux_start_threads shall start the manager thread by calling pthread_create with internal_execution_engine_linux_manager_thread_func and the execution engine. ]*/
            if (pthread_create(&execution_engine->manager_thread, NULL, internal_execution_engine_linux_manager_thread_func, execution_engine) != 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If any error occurs, internal_execution_engine_linux_start_threads shall fail. ]*/
                LogError("pthread_create failed for the manager thread");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
                goto all_ok;
            }
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_018: [ If any error occurs after the worker pool was created, internal_execution_engine_linux_start_threads shall stop the worker threads by calling worker_pool_destroy. ]*/
        worker_pool_destroy(execution_engine->worker_pool);
    }

all_ok:
    return result;
}

EXECUTION_ENGINE_HANDLE execution_engine_create(const EXECUTION_ENGINE_PARAMETERS* execution_engine_parameters)
{
    EXECUTION_ENGINE_HANDLE result;
    EXECUTION_ENGINE_PARAMETERS params;

    if (execution_engine_parameters == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_002: [ If execution_engine_parameters is NULL, execution_engine_create shall use the default DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
        params.min_thread_count = DEFAULT_MIN_THREAD_COUNT;
        params.max_thread_count = DEFAULT_MAX_THREAD_COUNT;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_003: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
        params.min_thread_count = execution_engine_parameters->min_thread_count;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_004: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
        params.max_thread_count = execution_engine_parameters->max_thread_count;
    }

    if ((params.max_thread_count != 0) && (params.max_thread_count < params.min_thread_count))
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_005: [ If max_thread_count is non-zero and less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
        LogError("Invalid arguments: execution_engine_parameters->min_thread_count=%" PRIu32 ", execution_engine_parameters->max_thread_count=%" PRIu32 "",
            params.min_thread_count, params.max_thread_count);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
        result = REFCOUNT_TYPE_CREATE(EXECUTION_ENGINE);
        if (result == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
            LogError("REFCOUNT_TYPE_CREATE(EXECUTION_ENGINE) failed");
        }
        else
        {
            result->params = params;

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_178: [ execution_engine_create shall mark the execution engine as not stopping, with its teardown not deferred and the manager thread not watching the ready queue. ]*/
            result->is_stopping = false;
            result->is_teardown_deferred = false;
            result->is_manager_watching_ready_queue = false;

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_179: [ execution_engine_create shall set the count of work objects in the ready queue to 0. ]*/
            result->ready_queue_count = 0;

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_001: [ execution_engine_create shall create the thread count policy by calling thread_count_policy_create with the minimum and the maximum number of threads. ]*/
            result->thread_count_policy = thread_count_policy_create(params.min_thread_count, params.max_thread_count);
            if (result->thread_count_policy == NULL)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                LogError("thread_count_policy_create failed");
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_002: [ execution_engine_create shall create the timer queue by calling timer_queue_create. ]*/
                result->timer_queue = timer_queue_create();
                if (result->timer_queue == NULL)
                {
                    /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                    LogError("timer_queue_create failed");
                }
                else
                {
                    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_003: [ execution_engine_create shall initialize the ready queue by calling DList_InitializeListHead. ]*/
                    DList_InitializeListHead(&result->ready_queue);

                    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the mutex and the condition variables by calling internal_execution_engine_linux_init_synchronization. ]*/
                    if (internal_execution_engine_linux_init_synchronization(result) != 0)
                    {
                        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                        LogError("internal_execution_engine_linux_init_synchronization failed");
                    }
                    else
                    {
                        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_005: [ execution_engine_create shall start the threads by calling internal_execution_engine_linux_start_threads. ]*/
                        if (internal_execution_engine_linux_start_threads(result) != 0)
                        {
                            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                            LogError("internal_execution_engine_linux_start_threads failed");
                        }
                        else
                        {
                            LogInfo("Created execution engine with min thread count=%" PRIu32 ", max thread count=%" PRIu32 "",
                                params.min_thread_count, params.max_thread_count);
                            goto all_ok;
                        }
                        (void)pthread_cond_destroy(&result->work_done_cond);
                        (void)pthread_cond_destroy(&result->manager_cond);
                        (void)pthread_mutex_destroy(&result->lock);
                    }
                    timer_queue_destroy(result->timer_queue);
                }
                thread_count_policy_destroy(result->thread_count_policy);
            }
            REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, result);
            result = NULL;
        }
    }

all_ok:
    return result;
}

void execution_engine_dec_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_007: [ If execution_engine is NULL, execution_engine_dec_ref shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_008: [ Otherwise execution_engine_dec_ref shall decrement the refcount. ]*/
        if (DEC_REF(EXECUTION_ENGINE, execution_engine) != 0)
        {
            // other references remain
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_020: [ If the refcount reaches 0: ]*/
            bool is_on_own_worker_thread = (current_execution_engine == execution_engine);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_021: [ execution_engine_dec_ref shall acquire the mutex by calling pthread_mutex_lock. ]*/
            (void)pthread_mutex_lock(&execution_engine->lock);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_022: [ execution_engine_dec_ref shall mark the execution engine as stopping. ]*/
            execution_engine->is_stopping = true;

            if (!is_on_own_worker_thread)
            {
                // the current thread tears down the execution engine below
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_023: [ If the current thread is running a work function of execution_engine, execution_engine_dec_ref shall mark the teardown as deferred to the manager thread. ]*/
                // a worker thread cannot wait for itself to exit
                execution_engine->is_teardown_deferred = true;
            }

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_024: [ execution_engine_dec_ref shall wake the manager thread by calling pthread_cond_signal. ]*/
            (void)pthread_cond_signal(&execution_engine->manager_cond);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_025: [ execution_engine_dec_ref shall release the mutex by calling pthread_mutex_unlock. ]*/
            (void)pthread_mutex_unlock(&execution_engine->lock);

            if (is_on_own_worker_thread)
            {
                // the manager thread tears down the execution engine once the worker threads exit
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_026: [ If the current thread is not running a work function of execution_engine: ]*/

                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_027: [ execution_engine_dec_ref shall wait for the manager thread to exit by calling pthread_join. ]*/
                if (pthread_join(execution_engine->manager_thread, NULL) != 0)
                {
                    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_185: [ If pthread_join fails, execution_engine_dec_ref shall return without tearing down the execution engine, which the manager thread can still use. ]*/
                    LogError("pthread_join failed for the manager thread of execution engine %p, the execution engine is not torn down", execution_engine);
                }
                else
                {
                    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_028: [ execution_engine_dec_ref shall tear down the execution engine by calling internal_execution_engine_linux_teardown. ]*/
                    internal_execution_engine_linux_teardown(execution_engine);
                }
            }
        }
    }
}

void execution_engine_inc_ref(EXECUTION_ENGINE_HANDLE execution_engine)
{
    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_010: [ If execution_engine is NULL then execution_engine_inc_ref shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_011: [ Otherwise execution_engine_inc_ref shall increment the reference count for execution_engine. ]*/
        INC_REF(EXECUTION_ENGINE, execution_engine);
    }
}

const EXECUTION_ENGINE_PARAMETERS* execution_engine_linux_get_parameters(EXECUTION_ENGINE_HANDLE execution_engine)
{
    EXECUTION_ENGINE_PARAMETERS* result;

    if (execution_engine == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_012: [ If execution_engine is NULL, execution_engine_linux_get_parameters shall fail and return NULL. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_013: [ Otherwise, execution_engine_linux_get_parameters shall return the parameters in EXECUTION_ENGINE. ]*/
        result = &execution_engine->params;
    }

    return result;
}

EXECUTION_ENGINE_LINUX_WORK_HANDLE execution_engine_linux_create_work(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context)
{
    EXECUTION_ENGINE_LINUX_WORK* result;

    if (
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_036: [ If execution_engine is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
        (execution_engine == NULL) ||
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_037: [ If work_function is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            execution_engine, work_function, work_function_context);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_038: [ execution_engine_linux_create_work shall allocate a work object. ]*/
        result = malloc(sizeof(EXECUTION_ENGINE_LINUX_WORK));
        if (result == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_041: [ If any error occurs, execution_engine_linux_create_work shall fail and return NULL. ]*/
            LogError("malloc(sizeof(EXECUTION_ENGINE_LINUX_WORK)=%zu) failed", sizeof(EXECUTION_ENGINE_LINUX_WORK));
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_039: [ execution_engine_linux_create_work shall initialize the work object with internal_execution_engine_linux_free_work as its free function by calling internal_execution_engine_linux_init_work. ]*/
            internal_execution_engine_linux_init_work(result, execution_engine, work_function, work_function_context, internal_execution_engine_linux_free_work);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_040: [ execution_engine_linux_create_work shall succeed and return the work object. ]*/
        }
    }

    return result;
}

void execution_engine_linux_submit_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_046: [ If work is NULL, execution_engine_linux_submit_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p", work);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = work->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_047: [ execution_engine_linux_submit_work shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_048: [ execution_engine_linux_submit_work shall submit work by calling internal_execution_engine_linux_submit_work_locked. ]*/
        internal_execution_engine_linux_submit_work_locked(execution_engine, work);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_049: [ execution_engine_linux_submit_work shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_wait_for_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work, bool cancel_pending)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_084: [ If work is NULL, execution_engine_linux_wait_for_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p, bool cancel_pending=%" PRI_BOOL "", work, MU_BOOL_VALUE(cancel_pending));
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_085: [ execution_engine_linux_wait_for_work shall wait for the invocations of work by calling internal_execution_engine_linux_wait_for_invocations. ]*/
        internal_execution_engine_linux_wait_for_invocations(work, cancel_pending);
    }
}

void execution_engine_linux_close_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_094: [ If work is NULL, execution_engine_linux_close_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p", work);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = work->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_095: [ execution_engine_linux_close_work shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_096: [ execution_engine_linux_close_work shall close work by calling internal_execution_engine_linux_close_work_locked. ]*/
        internal_execution_engine_linux_close_work_locked(work);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_097: [ execution_engine_linux_close_work shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_disassociate_current_thread_from_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_100: [ If work is NULL, execution_engine_linux_disassociate_current_thread_from_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p", work);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_101: [ execution_engine_linux_disassociate_current_thread_from_work shall disassociate the current thread from work by calling internal_execution_engine_linux_disassociate_current_thread. ]*/
        internal_execution_engine_linux_disassociate_current_thread(work);
    }
}

EXECUTION_ENGINE_LINUX_TIMER_HANDLE execution_engine_linux_create_timer(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context)
{
    EXECUTION_ENGINE_LINUX_TIMER* result;

    if (
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_107: [ If execution_engine is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
        (execution_engine == NULL) ||
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_108: [ If work_function is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            execution_engine, work_function, work_function_context);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_109: [ execution_engine_linux_create_timer shall allocate a timer. ]*/
        result = malloc(sizeof(EXECUTION_ENGINE_LINUX_TIMER));
        if (result == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_116: [ If any error occurs, execution_engine_linux_create_timer shall free the timer, fail and return NULL. ]*/
            LogError("malloc(sizeof(EXECUTION_ENGINE_LINUX_TIMER)=%zu) failed", sizeof(EXECUTION_ENGINE_LINUX_TIMER));
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_110: [ execution_engine_linux_create_timer shall initialize the work object of the timer with internal_execution_engine_linux_free_timer as its free function by calling internal_execution_engine_linux_init_work. ]*/
            internal_execution_engine_linux_init_work(&result->work, execution_engine, work_function, work_function_context, internal_execution_engine_linux_free_timer);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_111: [ execution_engine_linux_create_timer shall initialize the timer queue entry of the timer by calling timer_queue_entry_init. ]*/
            timer_queue_entry_init(&result->timer_queue_entry);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_112: [ execution_engine_linux_create_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
            (void)pthread_mutex_lock(&execution_engine->lock);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_113: [ execution_engine_linux_create_timer shall reserve room for the timer in the timer queue by calling timer_queue_reserve. ]*/
            // reserving room now makes arming the timer infallible
            int reserve_result = timer_queue_reserve(execution_engine->timer_queue);

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_114: [ execution_engine_linux_create_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
            (void)pthread_mutex_unlock(&execution_engine->lock);

            if (reserve_result != 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_116: [ If any error occurs, execution_engine_linux_create_timer shall free the timer, fail and return NULL. ]*/
                LogError("timer_queue_reserve failed");
                free(result);
                result = NULL;
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_115: [ execution_engine_linux_create_timer shall succeed and return the timer. ]*/
            }
        }
    }

    return result;
}

void execution_engine_linux_set_timer(EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    if (timer == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_119: [ If timer is NULL, execution_engine_linux_set_timer shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer=%p, uint32_t start_delay_ms=%" PRIu32 ", uint32_t timer_period_ms=%" PRIu32 "",
            timer, start_delay_ms, timer_period_ms);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = timer->work.execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_120: [ execution_engine_linux_set_timer shall get the current time rounded up to the next millisecond by calling timer_global_get_elapsed_ms. ]*/
        // rounding up makes sure the timer never fires before its start delay
        uint64_t now = internal_execution_engine_linux_get_time_ms_rounded_up();

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_121: [ execution_engine_linux_set_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_124: [ execution_engine_linux_set_timer shall arm timer with the current time plus start_delay_ms as due time and timer_period_ms as period by calling timer_queue_arm. ]*/
        if (!timer_queue_arm(execution_engine->timer_queue, &timer->timer_queue_entry, now + start_delay_ms, timer_period_ms))
        {
            // the manager thread wakes up for an earlier timer first
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_125: [ If timer_queue_arm returns true, execution_engine_linux_set_timer shall wake the manager thread by calling pthread_cond_signal. ]*/
            (void)pthread_cond_signal(&execution_engine->manager_cond);
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_126: [ execution_engine_linux_set_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_cancel_timer(EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_127: [ If timer is NULL, execution_engine_linux_cancel_timer shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer=%p", timer);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = timer->work.execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_128: [ execution_engine_linux_cancel_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_129: [ execution_engine_linux_cancel_timer shall disarm timer by calling timer_queue_disarm. ]*/
        timer_queue_disarm(execution_engine->timer_queue, &timer->timer_queue_entry);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_130: [ execution_engine_linux_cancel_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_wait_for_timer(EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer, bool cancel_pending)
{
    if (timer == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_131: [ If timer is NULL, execution_engine_linux_wait_for_timer shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer=%p, bool cancel_pending=%" PRI_BOOL "", timer, MU_BOOL_VALUE(cancel_pending));
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_132: [ execution_engine_linux_wait_for_timer shall wait for the invocations of the work object of timer by calling internal_execution_engine_linux_wait_for_invocations. ]*/
        internal_execution_engine_linux_wait_for_invocations(&timer->work, cancel_pending);
    }
}

void execution_engine_linux_close_timer(EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_133: [ If timer is NULL, execution_engine_linux_close_timer shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer=%p", timer);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = timer->work.execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_134: [ execution_engine_linux_close_timer shall acquire the mutex by calling pthread_mutex_lock. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_135: [ execution_engine_linux_close_timer shall disarm timer by calling timer_queue_disarm. ]*/
        timer_queue_disarm(execution_engine->timer_queue, &timer->timer_queue_entry);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_136: [ execution_engine_linux_close_timer shall close the work object of timer by calling internal_execution_engine_linux_close_work_locked. ]*/
        internal_execution_engine_linux_close_work_locked(&timer->work);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_137: [ execution_engine_linux_close_timer shall release the mutex by calling pthread_mutex_unlock. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_disassociate_current_thread_from_timer(EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_138: [ If timer is NULL, execution_engine_linux_disassociate_current_thread_from_timer shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer=%p", timer);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_139: [ execution_engine_linux_disassociate_current_thread_from_timer shall disassociate the current thread from the work object of timer by calling internal_execution_engine_linux_disassociate_current_thread. ]*/
        internal_execution_engine_linux_disassociate_current_thread(&timer->work);
    }
}

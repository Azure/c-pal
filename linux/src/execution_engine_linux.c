// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/refcount.h"
#include "c_pal/threadapi.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#define DEFAULT_MAX_WORKER_THREAD_COUNT 500
#define TIMER_HEAP_INITIAL_CAPACITY 16
#define STARVATION_THRESHOLD_MS 50
#define IDLE_WORKER_TIMEOUT_MS 60000
#define THREAD_CREATE_RETRY_DELAY_MS 100
#define TIMER_NOT_ARMED UINT32_MAX
#define NO_DEADLINE UINT64_MAX

typedef struct EXECUTION_ENGINE_TAG EXECUTION_ENGINE;

typedef struct EXECUTION_ENGINE_LINUX_WORK_TAG
{
    EXECUTION_ENGINE* execution_engine;
    EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function;
    void* work_function_context;

    uint32_t pending_count;
    uint32_t running_count;
    uint32_t waiter_count;
    bool is_closed;
    bool is_timer;

    struct EXECUTION_ENGINE_LINUX_WORK_TAG* queue_previous;
    struct EXECUTION_ENGINE_LINUX_WORK_TAG* queue_next;

    uint32_t heap_index;
    uint64_t due_time_ms;
    uint32_t period_ms;
} EXECUTION_ENGINE_LINUX_WORK;

typedef struct WORKER_TAG
{
    EXECUTION_ENGINE* execution_engine;
    THREAD_HANDLE thread;
    pthread_cond_t wake_cond;
    bool is_signaled;
    bool is_retired;
    bool is_idle;
    uint64_t idle_since_ms;
    struct WORKER_TAG* idle_previous;
    struct WORKER_TAG* idle_next;
    struct WORKER_TAG* list_next;
} WORKER;

typedef struct EXECUTION_ENGINE_TAG
{
    EXECUTION_ENGINE_PARAMETERS params;
    uint32_t max_worker_count;

    pthread_mutex_t lock;
    pthread_cond_t manager_cond;
    pthread_cond_t work_done_cond;
    THREAD_HANDLE manager_thread;
    bool stop;
    bool is_manager_watching_queue;

    EXECUTION_ENGINE_LINUX_WORK* queue_head;
    EXECUTION_ENGINE_LINUX_WORK* queue_tail;
    uint32_t queue_count;

    uint32_t worker_count;
    uint32_t starting_worker_count;
    uint32_t signaled_worker_count;
    WORKER* workers;
    WORKER* retired_workers;
    WORKER* idle_head;
    WORKER* idle_tail;

    uint64_t last_progress_ms;
    uint64_t next_create_attempt_ms;

    EXECUTION_ENGINE_LINUX_WORK** timer_heap;
    uint32_t timer_heap_count;
    uint32_t timer_heap_capacity;
    uint32_t timer_count;
} EXECUTION_ENGINE;

DEFINE_REFCOUNT_TYPE(EXECUTION_ENGINE);

static __thread EXECUTION_ENGINE* current_execution_engine = NULL;

// the work object whose callback runs on the current thread, NULL if none or if the thread was disassociated from it
static __thread EXECUTION_ENGINE_LINUX_WORK* current_work = NULL;

static uint64_t get_time_ms(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        LogErrorNo("clock_gettime(CLOCK_MONOTONIC) failed");
        now.tv_sec = 0;
        now.tv_nsec = 0;
    }
    return ((uint64_t)now.tv_sec * 1000) + ((uint64_t)now.tv_nsec / 1000000);
}

static uint64_t get_time_ms_rounded_up(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        LogErrorNo("clock_gettime(CLOCK_MONOTONIC) failed");
        now.tv_sec = 0;
        now.tv_nsec = 0;
    }
    // rounding up makes sure a timer never fires before its start delay
    return ((uint64_t)now.tv_sec * 1000) + (((uint64_t)now.tv_nsec + 999999) / 1000000);
}

static void queue_link_tail(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    work->queue_next = NULL;
    work->queue_previous = execution_engine->queue_tail;
    if (execution_engine->queue_tail == NULL)
    {
        execution_engine->queue_head = work;
    }
    else
    {
        execution_engine->queue_tail->queue_next = work;
    }
    execution_engine->queue_tail = work;
    execution_engine->queue_count++;
}

static void queue_unlink(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    if (work->queue_previous == NULL)
    {
        execution_engine->queue_head = work->queue_next;
    }
    else
    {
        work->queue_previous->queue_next = work->queue_next;
    }
    if (work->queue_next == NULL)
    {
        execution_engine->queue_tail = work->queue_previous;
    }
    else
    {
        work->queue_next->queue_previous = work->queue_previous;
    }
    work->queue_previous = NULL;
    work->queue_next = NULL;
    execution_engine->queue_count--;
}

static void idle_list_unlink(EXECUTION_ENGINE* execution_engine, WORKER* worker)
{
    if (worker->idle_previous == NULL)
    {
        execution_engine->idle_head = worker->idle_next;
    }
    else
    {
        worker->idle_previous->idle_next = worker->idle_next;
    }
    if (worker->idle_next == NULL)
    {
        execution_engine->idle_tail = worker->idle_previous;
    }
    else
    {
        worker->idle_next->idle_previous = worker->idle_previous;
    }
    worker->idle_previous = NULL;
    worker->idle_next = NULL;
    worker->is_idle = false;
}

static void idle_list_link_front(EXECUTION_ENGINE* execution_engine, WORKER* worker)
{
    worker->idle_previous = NULL;
    worker->idle_next = execution_engine->idle_head;
    if (execution_engine->idle_head == NULL)
    {
        execution_engine->idle_tail = worker;
    }
    else
    {
        execution_engine->idle_head->idle_previous = worker;
    }
    execution_engine->idle_head = worker;
    worker->is_idle = true;
}

static void timer_heap_swap(EXECUTION_ENGINE* execution_engine, uint32_t a, uint32_t b)
{
    EXECUTION_ENGINE_LINUX_WORK* temp = execution_engine->timer_heap[a];
    execution_engine->timer_heap[a] = execution_engine->timer_heap[b];
    execution_engine->timer_heap[b] = temp;
    execution_engine->timer_heap[a]->heap_index = a;
    execution_engine->timer_heap[b]->heap_index = b;
}

static void timer_heap_sift_up(EXECUTION_ENGINE* execution_engine, uint32_t index)
{
    while (index > 0)
    {
        uint32_t parent = (index - 1) / 2;
        if (execution_engine->timer_heap[parent]->due_time_ms <= execution_engine->timer_heap[index]->due_time_ms)
        {
            break;
        }
        timer_heap_swap(execution_engine, parent, index);
        index = parent;
    }
}

static void timer_heap_sift_down(EXECUTION_ENGINE* execution_engine, uint32_t index)
{
    while (true)
    {
        uint32_t smallest = index;
        uint32_t left = (2 * index) + 1;
        uint32_t right = left + 1;
        if ((left < execution_engine->timer_heap_count) &&
            (execution_engine->timer_heap[left]->due_time_ms < execution_engine->timer_heap[smallest]->due_time_ms))
        {
            smallest = left;
        }
        if ((right < execution_engine->timer_heap_count) &&
            (execution_engine->timer_heap[right]->due_time_ms < execution_engine->timer_heap[smallest]->due_time_ms))
        {
            smallest = right;
        }
        if (smallest == index)
        {
            break;
        }
        timer_heap_swap(execution_engine, smallest, index);
        index = smallest;
    }
}

static void timer_heap_insert(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* timer)
{
    // the capacity is reserved when the timer is created, so there is always room
    uint32_t index = execution_engine->timer_heap_count++;
    execution_engine->timer_heap[index] = timer;
    timer->heap_index = index;
    timer_heap_sift_up(execution_engine, index);
}

static void timer_heap_remove(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* timer)
{
    uint32_t index = timer->heap_index;
    uint32_t last = --execution_engine->timer_heap_count;
    if (index != last)
    {
        timer_heap_swap(execution_engine, index, last);
        timer_heap_sift_down(execution_engine, index);
        timer_heap_sift_up(execution_engine, index);
    }
    timer->heap_index = TIMER_NOT_ARMED;
}

static void free_work(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    if (work->is_timer)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_056: [ If work is a timer, free_work shall decrement the timer count. ]*/
        execution_engine->timer_count--;
    }

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_057: [ free_work shall free work. ]*/
    free(work);
}

static void wake_idle_worker(EXECUTION_ENGINE* execution_engine)
{
    WORKER* idle_worker = execution_engine->idle_head;
    idle_list_unlink(execution_engine, idle_worker);
    idle_worker->is_signaled = true;
    execution_engine->signaled_worker_count++;
    (void)pthread_cond_signal(&idle_worker->wake_cond);
}

static void end_invocation(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_065: [ end_invocation shall decrement the running count of work. ]*/
    work->running_count--;

    if ((work->pending_count == 0) && (work->running_count == 0))
    {
        if (work->is_closed)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_066: [ If work has no pending and no running invocations and it is closed, end_invocation shall free it. ]*/
            free_work(execution_engine, work);
        }
        else if (work->waiter_count > 0)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_067: [ If work has no pending and no running invocations, it is not closed and threads are waiting for it, end_invocation shall wake them by calling pthread_cond_broadcast. ]*/
            (void)pthread_cond_broadcast(&execution_engine->work_done_cond);
        }
        else
        {
            // nobody is waiting
        }
    }
}

static void enqueue_and_dispatch(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    if (execution_engine->queue_head == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_032: [ If the ready queue was empty, enqueue_and_dispatch shall record the current time as the last progress time. ]*/
        execution_engine->last_progress_ms = get_time_ms();
    }

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_033: [ enqueue_and_dispatch shall link work at the tail of the ready queue. ]*/
    queue_link_tail(execution_engine, work);

    WORKER* idle_worker = execution_engine->idle_head;
    if (idle_worker != NULL)
    {
        if (execution_engine->signaled_worker_count == 0)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_034: [ If a worker is idle and no other worker was woken and has not started running yet, enqueue_and_dispatch shall remove the most recently idle worker from the idle list, mark it signaled and wake it by calling pthread_cond_signal. ]*/
            wake_idle_worker(execution_engine);
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_082: [ If a worker is idle and another worker was woken and has not started running yet, enqueue_and_dispatch shall not wake another worker. ]*/
            // the woken worker wakes the next one if work is still queued when it starts running
        }
    }
    else if ((execution_engine->worker_count < execution_engine->max_worker_count) && !execution_engine->is_manager_watching_queue)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_035: [ Otherwise, if the thread count is below the maximum and the manager thread is not already waiting to check the ready queue for progress, enqueue_and_dispatch shall wake the manager thread by calling pthread_cond_signal. ]*/
        (void)pthread_cond_signal(&execution_engine->manager_cond);
    }
    else
    {
        // either no more workers can be created, or the manager already wakes up in time to check for starvation
    }
}

static void submit_locked(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work)
{
    if (work->pending_count == UINT32_MAX)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_028: [ If the pending count of work is UINT32_MAX, execution_engine_linux_submit_work shall log an error and not queue another invocation. ]*/
        LogError("Work %p already has UINT32_MAX pending invocations", work);
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_029: [ Otherwise execution_engine_linux_submit_work shall increment the pending count of work. ]*/
        work->pending_count++;
        if (work->pending_count == 1)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_030: [ If the pending count became 1, execution_engine_linux_submit_work shall link work at the tail of the ready queue and dispatch it. ]*/
            enqueue_and_dispatch(execution_engine, work);
        }
    }
}

static int worker_thread_func(void* arg)
{
    WORKER* worker = arg;
    EXECUTION_ENGINE* execution_engine = worker->execution_engine;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_058: [ worker_thread_func shall record the execution engine as the current execution engine of the thread until it returns. ]*/
    current_execution_engine = execution_engine;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_059: [ worker_thread_func shall acquire the execution engine mutex. ]*/
    (void)pthread_mutex_lock(&execution_engine->lock);
    execution_engine->starting_worker_count--;

    while (true)
    {
        EXECUTION_ENGINE_LINUX_WORK* work = execution_engine->queue_head;
        if (worker->is_retired)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_081: [ If the worker was retired, worker_thread_func shall release the mutex and return. ]*/
            break;
        }
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_060: [ Otherwise, if the ready queue is not empty, worker_thread_func shall: ]*/
        else if (work != NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_061: [ unlink the work object at the head of the ready queue and decrement its pending count. ]*/
            queue_unlink(execution_engine, work);
            work->pending_count--;

            if (work->pending_count > 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_062: [ If the work object still has pending invocations, link it back at the tail of the ready queue and dispatch it. ]*/
                enqueue_and_dispatch(execution_engine, work);
            }

            if ((execution_engine->queue_head != NULL) && (execution_engine->idle_head != NULL) && (execution_engine->signaled_worker_count == 0))
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_083: [ If the ready queue is still not empty, a worker is idle and no other worker was woken and has not started running yet, wake the most recently idle worker. ]*/
                wake_idle_worker(execution_engine);
            }

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_063: [ Increment the running count of the work object and record the current time as the last progress time. ]*/
            work->running_count++;
            execution_engine->last_progress_ms = get_time_ms();

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_064: [ Record the work object as the work running on the current thread, release the mutex, call the work function with the work function context and acquire the mutex. ]*/
            current_work = work;
            (void)pthread_mutex_unlock(&execution_engine->lock);
            work->work_function(work->work_function_context);
            (void)pthread_mutex_lock(&execution_engine->lock);

            if (current_work == NULL)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_090: [ If the current thread was disassociated from the work object while the work function ran, go on to the next work object without accessing the work object, which can be freed by then. ]*/
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_091: [ Otherwise, clear the work running on the current thread and end the invocation of the work object. ]*/
                current_work = NULL;
                end_invocation(execution_engine, work);
            }
        }
        else if (execution_engine->stop)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_068: [ Otherwise, if the stop flag is set, worker_thread_func shall release the mutex and return. ]*/
            break;
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_069: [ Otherwise worker_thread_func shall add the worker to the front of the idle list and wait by calling pthread_cond_wait until it is signaled, retired or the stop flag is set. ]*/
            worker->is_signaled = false;
            worker->idle_since_ms = get_time_ms();
            idle_list_link_front(execution_engine, worker);

            while (!worker->is_signaled && !worker->is_retired && !execution_engine->stop)
            {
                (void)pthread_cond_wait(&worker->wake_cond, &execution_engine->lock);
            }

            if (worker->is_signaled)
            {
                // the worker is now running, so the next submit can wake another idle worker
                execution_engine->signaled_worker_count--;
            }

            if (worker->is_idle)
            {
                // woken by stop
                idle_list_unlink(execution_engine, worker);
            }
        }
    }

    (void)pthread_mutex_unlock(&execution_engine->lock);
    current_execution_engine = NULL;
    return 0;
}

static int create_worker(EXECUTION_ENGINE* execution_engine, WORKER** worker_out)
{
    int result;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_008: [ create_worker shall allocate a worker. ]*/
    WORKER* worker = malloc(sizeof(WORKER));
    if (worker == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_011: [ If any error occurs, create_worker shall free the resources it allocated and fail. ]*/
        LogError("malloc(sizeof(WORKER)=%zu) failed", sizeof(WORKER));
        result = MU_FAILURE;
    }
    else
    {
        worker->execution_engine = execution_engine;
        worker->is_signaled = false;
        worker->is_retired = false;
        worker->is_idle = false;
        worker->idle_since_ms = 0;
        worker->idle_previous = NULL;
        worker->idle_next = NULL;
        worker->list_next = NULL;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_009: [ create_worker shall initialize the condition variable used to wake the worker by calling pthread_cond_init. ]*/
        if (pthread_cond_init(&worker->wake_cond, NULL) != 0)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_011: [ If any error occurs, create_worker shall free the resources it allocated and fail. ]*/
            LogError("pthread_cond_init failed");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_010: [ create_worker shall start the worker thread by calling ThreadAPI_Create with worker_thread_func. ]*/
            if (ThreadAPI_Create(&worker->thread, worker_thread_func, worker) != THREADAPI_OK)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_011: [ If any error occurs, create_worker shall free the resources it allocated and fail. ]*/
                LogError("ThreadAPI_Create failed");
                result = MU_FAILURE;
            }
            else
            {
                *worker_out = worker;
                result = 0;
                goto all_ok;
            }
            (void)pthread_cond_destroy(&worker->wake_cond);
        }
        free(worker);
    }
all_ok:
    return result;
}

static int init_monotonic_cond(pthread_cond_t* cond)
{
    int result;
    pthread_condattr_t cond_attributes;
    if (pthread_condattr_init(&cond_attributes) != 0)
    {
        LogError("pthread_condattr_init failed");
        result = MU_FAILURE;
    }
    else
    {
        if (pthread_condattr_setclock(&cond_attributes, CLOCK_MONOTONIC) != 0)
        {
            LogError("pthread_condattr_setclock(CLOCK_MONOTONIC) failed");
            result = MU_FAILURE;
        }
        else if (pthread_cond_init(cond, &cond_attributes) != 0)
        {
            LogError("pthread_cond_init failed");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
        (void)pthread_condattr_destroy(&cond_attributes);
    }
    return result;
}

static void join_and_free_worker(WORKER* worker)
{
    int dont_care;
    if (ThreadAPI_Join(worker->thread, &dont_care) != THREADAPI_OK)
    {
        LogError("ThreadAPI_Join failed for worker %p", worker);
    }
    (void)pthread_cond_destroy(&worker->wake_cond);
    free(worker);
}

static bool should_create_worker(EXECUTION_ENGINE* execution_engine, uint64_t now)
{
    return (execution_engine->queue_count > execution_engine->starting_worker_count) &&
        (execution_engine->idle_head == NULL) &&
        (execution_engine->worker_count < execution_engine->max_worker_count) &&
        (now >= execution_engine->next_create_attempt_ms) &&
        // last_progress_ms can be later than now if it was recorded after the manager read the time
        (now >= execution_engine->last_progress_ms) &&
        (now - execution_engine->last_progress_ms >= STARVATION_THRESHOLD_MS);
}

static uint64_t get_manager_deadline(EXECUTION_ENGINE* execution_engine, uint64_t now)
{
    uint64_t deadline = NO_DEADLINE;

    if (execution_engine->timer_heap_count > 0)
    {
        deadline = execution_engine->timer_heap[0]->due_time_ms;
    }

    execution_engine->is_manager_watching_queue = false;
    if ((execution_engine->queue_count > execution_engine->starting_worker_count) &&
        (execution_engine->idle_head == NULL) &&
        (execution_engine->worker_count < execution_engine->max_worker_count))
    {
        uint64_t create_time = execution_engine->last_progress_ms + STARVATION_THRESHOLD_MS;
        if (create_time < execution_engine->next_create_attempt_ms)
        {
            create_time = execution_engine->next_create_attempt_ms;
        }
        if (create_time < deadline)
        {
            deadline = create_time;
        }

        // submitting more work does not need to wake the manager, it wakes up in time to check the ready queue for progress
        execution_engine->is_manager_watching_queue = true;
    }

    if (execution_engine->worker_count > execution_engine->params.min_thread_count)
    {
        // workers going idle do not wake the manager, so it checks for workers to retire at least every IDLE_WORKER_TIMEOUT_MS
        uint64_t retire_time = (execution_engine->idle_tail != NULL) ?
            execution_engine->idle_tail->idle_since_ms + IDLE_WORKER_TIMEOUT_MS :
            now + IDLE_WORKER_TIMEOUT_MS;
        if (retire_time < deadline)
        {
            deadline = retire_time;
        }
    }

    return deadline;
}

static int manager_thread_func(void* arg)
{
    EXECUTION_ENGINE* execution_engine = arg;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_070: [ manager_thread_func shall acquire the execution engine mutex. ]*/
    (void)pthread_mutex_lock(&execution_engine->lock);

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_071: [ Until the stop flag is set, manager_thread_func shall: ]*/
    while (!execution_engine->stop)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_072: [ Get the current CLOCK_MONOTONIC time by calling clock_gettime. ]*/
        uint64_t now = get_time_ms();

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_073: [ For each timer in the timer heap whose due time has passed, remove it from the heap, submit it and, if its period is not 0, insert it back with its due time advanced by the period (or the current time plus the period if that is not later than the current time). ]*/
        while ((execution_engine->timer_heap_count > 0) && (execution_engine->timer_heap[0]->due_time_ms <= now))
        {
            EXECUTION_ENGINE_LINUX_WORK* timer = execution_engine->timer_heap[0];
            timer_heap_remove(execution_engine, timer);
            if (timer->period_ms != 0)
            {
                timer->due_time_ms += timer->period_ms;
                if (timer->due_time_ms <= now)
                {
                    timer->due_time_ms = now + timer->period_ms;
                }
                timer_heap_insert(execution_engine, timer);
            }
            submit_locked(execution_engine, timer);
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_074: [ While the thread count is above min_thread_count and the least recently idle worker has been idle for at least 60000 ms, remove it from the idle list, mark it retired, wake it by calling pthread_cond_signal and decrement the thread count. ]*/
        while ((execution_engine->worker_count > execution_engine->params.min_thread_count) &&
            (execution_engine->idle_tail != NULL) &&
            (now >= execution_engine->idle_tail->idle_since_ms) &&
            (now - execution_engine->idle_tail->idle_since_ms >= IDLE_WORKER_TIMEOUT_MS))
        {
            WORKER* worker = execution_engine->idle_tail;
            idle_list_unlink(execution_engine, worker);
            worker->is_retired = true;
            (void)pthread_cond_signal(&worker->wake_cond);
            execution_engine->worker_count--;

            // move the worker from the list of workers to the list of retired workers
            WORKER** link = &execution_engine->workers;
            while (*link != worker)
            {
                link = &(*link)->list_next;
            }
            *link = worker->list_next;
            worker->list_next = execution_engine->retired_workers;
            execution_engine->retired_workers = worker;
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_075: [ For each retired worker, release the mutex, join it by calling ThreadAPI_Join, free it and acquire the mutex. ]*/
        while (execution_engine->retired_workers != NULL)
        {
            WORKER* worker = execution_engine->retired_workers;
            execution_engine->retired_workers = worker->list_next;
            (void)pthread_mutex_unlock(&execution_engine->lock);
            join_and_free_worker(worker);
            (void)pthread_mutex_lock(&execution_engine->lock);
        }

        if (should_create_worker(execution_engine, now))
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_076: [ If the ready queue has more work objects than workers being started, no worker is idle, the thread count is below the maximum, the thread creation retry time has passed and no work object was taken from the ready queue for 50 ms, increment the thread count, release the mutex, create a worker and acquire the mutex. ]*/
            WORKER* worker;
            execution_engine->worker_count++;
            execution_engine->starting_worker_count++;
            (void)pthread_mutex_unlock(&execution_engine->lock);
            int create_result = create_worker(execution_engine, &worker);
            (void)pthread_mutex_lock(&execution_engine->lock);

            if (create_result != 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_077: [ If creating the worker fails, decrement the thread count and set the thread creation retry time to 100 ms from now. ]*/
                LogError("Failed creating a worker thread, %" PRIu32 " worker threads running, retrying in %d ms", execution_engine->worker_count - 1, THREAD_CREATE_RETRY_DELAY_MS);
                execution_engine->worker_count--;
                execution_engine->starting_worker_count--;
                execution_engine->next_create_attempt_ms = now + THREAD_CREATE_RETRY_DELAY_MS;
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_078: [ If creating the worker succeeds, record the current time as the last progress time. ]*/
                execution_engine->last_progress_ms = now;
                worker->list_next = execution_engine->workers;
                execution_engine->workers = worker;
            }
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_079: [ Otherwise wait for the earliest of the next timer due time, the next time a worker may need to be created and, while the thread count is above min_thread_count, the next time an idle worker may need to be retired (at most 60000 ms) by calling pthread_cond_timedwait, or by calling pthread_cond_wait if there is no such time. ]*/
            uint64_t deadline = get_manager_deadline(execution_engine, now);
            if (deadline == NO_DEADLINE)
            {
                (void)pthread_cond_wait(&execution_engine->manager_cond, &execution_engine->lock);
            }
            else
            {
                struct timespec abstime;
                abstime.tv_sec = (time_t)(deadline / 1000);
                abstime.tv_nsec = (long)((deadline % 1000) * 1000000);
                // manager_cond uses CLOCK_MONOTONIC, so the absolute time is on that clock
                (void)pthread_cond_timedwait(&execution_engine->manager_cond, &execution_engine->lock, &abstime);
            }
        }
    }

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_080: [ manager_thread_func shall release the mutex and return. ]*/
    (void)pthread_mutex_unlock(&execution_engine->lock);
    return 0;
}

static void signal_stop(EXECUTION_ENGINE* execution_engine)
{
    (void)pthread_mutex_lock(&execution_engine->lock);
    execution_engine->stop = true;
    (void)pthread_cond_signal(&execution_engine->manager_cond);
    for (WORKER* worker = execution_engine->idle_head; worker != NULL; worker = worker->idle_next)
    {
        (void)pthread_cond_signal(&worker->wake_cond);
    }
    (void)pthread_mutex_unlock(&execution_engine->lock);
}

static void join_all_workers(EXECUTION_ENGINE* execution_engine)
{
    while (execution_engine->workers != NULL)
    {
        WORKER* worker = execution_engine->workers;
        execution_engine->workers = worker->list_next;
        join_and_free_worker(worker);
    }
    while (execution_engine->retired_workers != NULL)
    {
        WORKER* worker = execution_engine->retired_workers;
        execution_engine->retired_workers = worker->list_next;
        join_and_free_worker(worker);
    }
}

EXECUTION_ENGINE_HANDLE execution_engine_create(const EXECUTION_ENGINE_PARAMETERS* execution_engine_parameters)
{
    EXECUTION_ENGINE_HANDLE result;

    /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_001: [ execution_engine_create shall allocate a new execution engine and on success shall return a non-NULL handle. ]*/
    result = REFCOUNT_TYPE_CREATE(EXECUTION_ENGINE);
    if (result == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
        LogError("REFCOUNT_TYPE_CREATE failed.");
    }
    else
    {
        if (execution_engine_parameters == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_002: [ If execution_engine_parameters is NULL, execution_engine_create shall use the default DEFAULT_MIN_THREAD_COUNT and DEFAULT_MAX_THREAD_COUNT as parameters. ]*/
            result->params.min_thread_count = DEFAULT_MIN_THREAD_COUNT;
            result->params.max_thread_count = DEFAULT_MAX_THREAD_COUNT;
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_003: [ execution_engine_create shall set the minimum number of threads to the min_thread_count field of execution_engine_parameters. ]*/
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_004: [ execution_engine_create shall set the maximum number of threads to the max_thread_count field of execution_engine_parameters. ]*/
            result->params.min_thread_count = execution_engine_parameters->min_thread_count;
            result->params.max_thread_count = execution_engine_parameters->max_thread_count;
        }

        if ((result->params.max_thread_count != 0) &&
            (result->params.max_thread_count < result->params.min_thread_count))
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_005: [ If max_thread_count is non-zero and less than min_thread_count, execution_engine_create shall fail and return NULL. ]*/
            LogError("Invalid arguments: execution_engine_parameters->min_thread_count=%" PRIu32 ", execution_engine_parameters->max_thread_count=%" PRIu32,
                result->params.min_thread_count, result->params.max_thread_count);
        }
        else
        {
            if (result->params.max_thread_count == 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_001: [ If max_thread_count is 0, execution_engine_create shall use 500 as the maximum number of worker threads, or min_thread_count if it is larger. ]*/
                result->max_worker_count = (result->params.min_thread_count > DEFAULT_MAX_WORKER_THREAD_COUNT) ? result->params.min_thread_count : DEFAULT_MAX_WORKER_THREAD_COUNT;
            }
            else
            {
                result->max_worker_count = result->params.max_thread_count;
            }

            result->stop = false;
            result->is_manager_watching_queue = false;
            result->queue_head = NULL;
            result->queue_tail = NULL;
            result->queue_count = 0;
            result->worker_count = 0;
            result->starting_worker_count = 0;
            result->signaled_worker_count = 0;
            result->workers = NULL;
            result->retired_workers = NULL;
            result->idle_head = NULL;
            result->idle_tail = NULL;
            result->last_progress_ms = 0;
            result->next_create_attempt_ms = 0;
            result->timer_heap = NULL;
            result->timer_heap_count = 0;
            result->timer_heap_capacity = 0;
            result->timer_count = 0;

            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_003: [ execution_engine_create shall initialize the mutex guarding the execution engine state by calling pthread_mutex_init. ]*/
            if (pthread_mutex_init(&result->lock, NULL) != 0)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                LogError("pthread_mutex_init failed");
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_004: [ execution_engine_create shall initialize the condition variable used to wake the manager thread with an attribute that uses CLOCK_MONOTONIC by calling pthread_condattr_init, pthread_condattr_setclock, pthread_cond_init and pthread_condattr_destroy, and the condition variable used to signal completed work by calling pthread_cond_init. ]*/
                if (init_monotonic_cond(&result->manager_cond) != 0)
                {
                    /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                    LogError("init_monotonic_cond failed");
                }
                else
                {
                    if (pthread_cond_init(&result->work_done_cond, NULL) != 0)
                    {
                        /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                        LogError("pthread_cond_init failed");
                    }
                    else
                    {
                        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_005: [ execution_engine_create shall create min_thread_count worker threads. ]*/
                        uint32_t i;
                        for (i = 0; i < result->params.min_thread_count; i++)
                        {
                            WORKER* worker;
                            (void)pthread_mutex_lock(&result->lock);
                            result->worker_count++;
                            result->starting_worker_count++;
                            (void)pthread_mutex_unlock(&result->lock);
                            if (create_worker(result, &worker) != 0)
                            {
                                /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                                LogError("Failed creating worker thread %" PRIu32 " of %" PRIu32, i, result->params.min_thread_count);
                                (void)pthread_mutex_lock(&result->lock);
                                result->worker_count--;
                                result->starting_worker_count--;
                                (void)pthread_mutex_unlock(&result->lock);
                                break;
                            }
                            worker->list_next = result->workers;
                            result->workers = worker;
                        }

                        if (i == result->params.min_thread_count)
                        {
                            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_006: [ execution_engine_create shall create the manager thread by calling ThreadAPI_Create with manager_thread_func. ]*/
                            if (ThreadAPI_Create(&result->manager_thread, manager_thread_func, result) != THREADAPI_OK)
                            {
                                /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_006: [ If any error occurs, execution_engine_create shall fail and return NULL. ]*/
                                LogError("ThreadAPI_Create failed for the manager thread");
                            }
                            else
                            {
                                LogInfo("Created execution engine with min thread count=%" PRIu32 ", max thread count=%" PRIu32 "",
                                    result->params.min_thread_count, result->max_worker_count);
                                goto all_ok;
                            }
                        }

                        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_007: [ If any error occurs after worker threads were created, execution_engine_create shall signal them to stop and join them. ]*/
                        signal_stop(result);
                        join_all_workers(result);

                        (void)pthread_cond_destroy(&result->work_done_cond);
                    }
                    (void)pthread_cond_destroy(&result->manager_cond);
                }
                (void)pthread_mutex_destroy(&result->lock);
            }
        }
        REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, result);
        result = NULL;
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
        if (DEC_REF(EXECUTION_ENGINE, execution_engine) == 0)
        {
            if (current_execution_engine == execution_engine)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_012: [ If the refcount reaches 0 on a worker thread of execution_engine, execution_engine_dec_ref shall terminate the process by calling LogCriticalAndTerminate. ]*/
                LogCriticalAndTerminate("The last reference to execution engine %p was released from one of its own worker threads, it cannot join itself", execution_engine);
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_013: [ Otherwise, if the refcount reaches 0, execution_engine_dec_ref shall set the stop flag and signal the manager thread and every idle worker. ]*/
                signal_stop(execution_engine);

                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_014: [ execution_engine_dec_ref shall join the manager thread and all the worker threads. ]*/
                int dont_care;
                if (ThreadAPI_Join(execution_engine->manager_thread, &dont_care) != THREADAPI_OK)
                {
                    LogError("ThreadAPI_Join failed for the manager thread");
                }
                join_all_workers(execution_engine);

                if (execution_engine->timer_count != 0)
                {
                    LogError("Execution engine %p destroyed with %" PRIu32 " timers not closed", execution_engine, execution_engine->timer_count);
                }
                free(execution_engine->timer_heap);
                (void)pthread_cond_destroy(&execution_engine->work_done_cond);
                (void)pthread_cond_destroy(&execution_engine->manager_cond);
                (void)pthread_mutex_destroy(&execution_engine->lock);

                /* Codes_SRS_EXECUTION_ENGINE_LINUX_07_009: [ If the refcount is zero execution_engine_dec_ref shall free the memory for EXECUTION_ENGINE. ]*/
                REFCOUNT_TYPE_DESTROY(EXECUTION_ENGINE, execution_engine);
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

static EXECUTION_ENGINE_LINUX_WORK* allocate_work(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context, bool is_timer)
{
    EXECUTION_ENGINE_LINUX_WORK* result = malloc(sizeof(EXECUTION_ENGINE_LINUX_WORK));
    if (result == NULL)
    {
        LogError("malloc(sizeof(EXECUTION_ENGINE_LINUX_WORK)=%zu) failed", sizeof(EXECUTION_ENGINE_LINUX_WORK));
    }
    else
    {
        result->execution_engine = execution_engine;
        result->work_function = work_function;
        result->work_function_context = work_function_context;
        result->pending_count = 0;
        result->running_count = 0;
        result->waiter_count = 0;
        result->is_closed = false;
        result->is_timer = is_timer;
        result->queue_previous = NULL;
        result->queue_next = NULL;
        result->heap_index = TIMER_NOT_ARMED;
        result->due_time_ms = 0;
        result->period_ms = 0;
    }
    return result;
}

EXECUTION_ENGINE_LINUX_WORK_HANDLE execution_engine_linux_create_work(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context)
{
    EXECUTION_ENGINE_LINUX_WORK* result;

    if (
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_015: [ If execution_engine is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
        (execution_engine == NULL) ||
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_016: [ If work_function is NULL, execution_engine_linux_create_work shall fail and return NULL. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            execution_engine, work_function, work_function_context);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_017: [ execution_engine_linux_create_work shall allocate a work object, store work_function and work_function_context in it, and return it. ]*/
        result = allocate_work(execution_engine, work_function, work_function_context, false);
        if (result == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_018: [ If any error occurs, execution_engine_linux_create_work shall fail and return NULL. ]*/
            LogError("allocate_work failed");
        }
    }

    return result;
}

EXECUTION_ENGINE_LINUX_WORK_HANDLE execution_engine_linux_create_timer(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context)
{
    EXECUTION_ENGINE_LINUX_WORK* result;

    if (
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_019: [ If execution_engine is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
        (execution_engine == NULL) ||
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_020: [ If work_function is NULL, execution_engine_linux_create_timer shall fail and return NULL. ]*/
        (work_function == NULL)
        )
    {
        LogError("Invalid arguments: EXECUTION_ENGINE_HANDLE execution_engine=%p, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function=%p, void* work_function_context=%p",
            execution_engine, work_function, work_function_context);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_021: [ execution_engine_linux_create_timer shall allocate a work object marked as a timer and store work_function and work_function_context in it. ]*/
        result = allocate_work(execution_engine, work_function, work_function_context, true);
        if (result == NULL)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_025: [ If any error occurs, execution_engine_linux_create_timer shall release the mutex if acquired, free the work object, fail and return NULL. ]*/
            LogError("allocate_work failed");
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_022: [ execution_engine_linux_create_timer shall acquire the execution engine mutex. ]*/
            (void)pthread_mutex_lock(&execution_engine->lock);

            bool has_room = true;
            if (execution_engine->timer_count == execution_engine->timer_heap_capacity)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_023: [ If the timer heap is full, execution_engine_linux_create_timer shall double its capacity (16 if it is empty) by calling realloc_2. ]*/
                uint32_t new_capacity = (execution_engine->timer_heap_capacity == 0) ? TIMER_HEAP_INITIAL_CAPACITY : execution_engine->timer_heap_capacity * 2;
                EXECUTION_ENGINE_LINUX_WORK** new_heap = realloc_2(execution_engine->timer_heap, new_capacity, sizeof(EXECUTION_ENGINE_LINUX_WORK*));
                if (new_heap == NULL)
                {
                    LogError("realloc_2(%p, %" PRIu32 ", %zu) failed", execution_engine->timer_heap, new_capacity, sizeof(EXECUTION_ENGINE_LINUX_WORK*));
                    has_room = false;
                }
                else
                {
                    execution_engine->timer_heap = new_heap;
                    execution_engine->timer_heap_capacity = new_capacity;
                }
            }

            if (has_room)
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_024: [ execution_engine_linux_create_timer shall increment the timer count, release the mutex and return the timer. ]*/
                execution_engine->timer_count++;
                (void)pthread_mutex_unlock(&execution_engine->lock);
            }
            else
            {
                /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_025: [ If any error occurs, execution_engine_linux_create_timer shall release the mutex if acquired, free the work object, fail and return NULL. ]*/
                (void)pthread_mutex_unlock(&execution_engine->lock);
                free(result);
                result = NULL;
            }
        }
    }

    return result;
}

void execution_engine_linux_submit_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_026: [ If work is NULL, execution_engine_linux_submit_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p", work);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = work->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_027: [ execution_engine_linux_submit_work shall acquire the execution engine mutex. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        submit_locked(execution_engine, work);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_031: [ execution_engine_linux_submit_work shall release the mutex. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_set_timer(EXECUTION_ENGINE_LINUX_WORK_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms)
{
    if (timer == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_036: [ If timer is NULL, execution_engine_linux_set_timer shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE timer=%p, uint32_t start_delay_ms=%" PRIu32 ", uint32_t timer_period_ms=%" PRIu32 "",
            timer, start_delay_ms, timer_period_ms);
    }
    else if (!timer->is_timer)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_037: [ If timer is not a timer, execution_engine_linux_set_timer shall log an error and return. ]*/
        LogError("Work %p is not a timer", timer);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = timer->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_038: [ execution_engine_linux_set_timer shall acquire the execution engine mutex. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        if (timer->heap_index != TIMER_NOT_ARMED)
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_039: [ If timer is armed, execution_engine_linux_set_timer shall remove it from the timer heap. ]*/
            timer_heap_remove(execution_engine, timer);
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_040: [ execution_engine_linux_set_timer shall set the due time of timer to the current CLOCK_MONOTONIC time obtained by calling clock_gettime plus start_delay_ms, store timer_period_ms and insert timer in the timer heap. ]*/
        timer->due_time_ms = get_time_ms_rounded_up() + start_delay_ms;
        timer->period_ms = timer_period_ms;
        timer_heap_insert(execution_engine, timer);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_041: [ execution_engine_linux_set_timer shall wake the manager thread by calling pthread_cond_signal and release the mutex. ]*/
        (void)pthread_cond_signal(&execution_engine->manager_cond);
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_cancel_timer(EXECUTION_ENGINE_LINUX_WORK_HANDLE timer)
{
    if (timer == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_042: [ If timer is NULL, execution_engine_linux_cancel_timer shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE timer=%p", timer);
    }
    else if (!timer->is_timer)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_043: [ If timer is not a timer, execution_engine_linux_cancel_timer shall log an error and return. ]*/
        LogError("Work %p is not a timer", timer);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = timer->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_044: [ execution_engine_linux_cancel_timer shall acquire the execution engine mutex, remove timer from the timer heap if it is armed and release the mutex. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);
        if (timer->heap_index != TIMER_NOT_ARMED)
        {
            timer_heap_remove(execution_engine, timer);
        }
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_wait_for_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work, bool cancel_pending)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_045: [ If work is NULL, execution_engine_linux_wait_for_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p, bool cancel_pending=%" PRI_BOOL "", work, MU_BOOL_VALUE(cancel_pending));
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = work->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_046: [ execution_engine_linux_wait_for_work shall acquire the execution engine mutex. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        if (cancel_pending && (work->pending_count > 0))
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_047: [ If cancel_pending is true and work has pending invocations, execution_engine_linux_wait_for_work shall unlink work from the ready queue and set its pending count to 0. ]*/
            queue_unlink(execution_engine, work);
            work->pending_count = 0;
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_048: [ execution_engine_linux_wait_for_work shall wait by calling pthread_cond_wait until work has no pending and no running invocations. ]*/
        work->waiter_count++;
        while ((work->pending_count > 0) || (work->running_count > 0))
        {
            (void)pthread_cond_wait(&execution_engine->work_done_cond, &execution_engine->lock);
        }
        work->waiter_count--;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_049: [ execution_engine_linux_wait_for_work shall release the mutex. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_close_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_050: [ If work is NULL, execution_engine_linux_close_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p", work);
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = work->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_051: [ execution_engine_linux_close_work shall acquire the execution engine mutex. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        if (work->is_timer && (work->heap_index != TIMER_NOT_ARMED))
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_052: [ If work is an armed timer, execution_engine_linux_close_work shall remove it from the timer heap. ]*/
            timer_heap_remove(execution_engine, work);
        }

        if ((work->pending_count == 0) && (work->running_count == 0))
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_053: [ If work has no pending and no running invocations, execution_engine_linux_close_work shall free it. ]*/
            free_work(execution_engine, work);
        }
        else
        {
            /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_054: [ Otherwise execution_engine_linux_close_work shall mark work as closed so that the worker running its last invocation frees it. ]*/
            work->is_closed = true;
        }

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_055: [ execution_engine_linux_close_work shall release the mutex. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

void execution_engine_linux_disassociate_current_thread_from_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    if (work == NULL)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_084: [ If work is NULL, execution_engine_linux_disassociate_current_thread_from_work shall return. ]*/
        LogError("Invalid arguments: EXECUTION_ENGINE_LINUX_WORK_HANDLE work=%p", work);
    }
    else if (current_work != work)
    {
        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_085: [ If the current thread is not running a callback of work, or was already disassociated from it, execution_engine_linux_disassociate_current_thread_from_work shall return. ]*/
    }
    else
    {
        EXECUTION_ENGINE* execution_engine = work->execution_engine;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_086: [ Otherwise execution_engine_linux_disassociate_current_thread_from_work shall acquire the execution engine mutex. ]*/
        (void)pthread_mutex_lock(&execution_engine->lock);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_087: [ execution_engine_linux_disassociate_current_thread_from_work shall record that the current thread no longer runs a callback of work. ]*/
        current_work = NULL;

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_088: [ execution_engine_linux_disassociate_current_thread_from_work shall end the invocation of work. ]*/
        end_invocation(execution_engine, work);

        /* Codes_SRS_EXECUTION_ENGINE_LINUX_42_089: [ execution_engine_linux_disassociate_current_thread_from_work shall release the mutex. ]*/
        (void)pthread_mutex_unlock(&execution_engine->lock);
    }
}

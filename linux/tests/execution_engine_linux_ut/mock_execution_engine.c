// Copyright (c) Microsoft. All rights reserved.

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#include "mock_execution_engine.h"

#define pthread_mutex_init mocked_pthread_mutex_init
#define pthread_mutex_lock mocked_pthread_mutex_lock
#define pthread_mutex_unlock mocked_pthread_mutex_unlock
#define pthread_mutex_destroy mocked_pthread_mutex_destroy
#define pthread_condattr_init mocked_pthread_condattr_init
#define pthread_condattr_setclock mocked_pthread_condattr_setclock
#define pthread_condattr_destroy mocked_pthread_condattr_destroy
#define pthread_cond_init mocked_pthread_cond_init
#define pthread_cond_signal mocked_pthread_cond_signal
#define pthread_cond_broadcast mocked_pthread_cond_broadcast
#define pthread_cond_wait mocked_pthread_cond_wait
#define pthread_cond_timedwait mocked_pthread_cond_timedwait
#define pthread_cond_destroy mocked_pthread_cond_destroy
#define clock_gettime mocked_clock_gettime

int mocked_pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int mocked_pthread_mutex_lock(pthread_mutex_t* mutex);
int mocked_pthread_mutex_unlock(pthread_mutex_t* mutex);
int mocked_pthread_mutex_destroy(pthread_mutex_t* mutex);
int mocked_pthread_condattr_init(pthread_condattr_t* attr);
int mocked_pthread_condattr_setclock(pthread_condattr_t* attr, clockid_t clock_id);
int mocked_pthread_condattr_destroy(pthread_condattr_t* attr);
int mocked_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int mocked_pthread_cond_signal(pthread_cond_t* cond);
int mocked_pthread_cond_broadcast(pthread_cond_t* cond);
int mocked_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int mocked_pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* abstime);
int mocked_pthread_cond_destroy(pthread_cond_t* cond);
int mocked_clock_gettime(clockid_t clock_id, struct timespec* tp);

#include "../../src/execution_engine_linux.c"

THREAD_START_FUNC test_get_worker_thread_func(void)
{
    return worker_thread_func;
}

THREAD_START_FUNC test_get_manager_thread_func(void)
{
    return manager_thread_func;
}

EXECUTION_ENGINE_HANDLE test_get_current_execution_engine(void)
{
    return current_execution_engine;
}

pthread_mutex_t* test_engine_get_lock(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return &execution_engine->lock;
}

pthread_cond_t* test_engine_get_manager_cond(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return &execution_engine->manager_cond;
}

pthread_cond_t* test_engine_get_work_done_cond(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return &execution_engine->work_done_cond;
}

uint32_t test_engine_get_max_worker_count(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->max_worker_count;
}

uint32_t test_engine_get_worker_count(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->worker_count;
}

uint32_t test_engine_get_signaled_worker_count(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->signaled_worker_count;
}

bool test_engine_is_manager_watching_queue(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->is_manager_watching_queue;
}

uint32_t test_engine_get_queue_count(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->queue_count;
}

EXECUTION_ENGINE_LINUX_WORK_HANDLE test_engine_get_queue_head(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->queue_head;
}

EXECUTION_ENGINE_LINUX_WORK_HANDLE test_engine_get_queue_tail(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->queue_tail;
}

uint64_t test_engine_get_last_progress_ms(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->last_progress_ms;
}

uint64_t test_engine_get_next_create_attempt_ms(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->next_create_attempt_ms;
}

uint32_t test_engine_get_timer_heap_count(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->timer_heap_count;
}

uint32_t test_engine_get_timer_heap_capacity(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->timer_heap_capacity;
}

uint32_t test_engine_get_timer_count(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->timer_count;
}

EXECUTION_ENGINE_LINUX_WORK_HANDLE test_engine_get_earliest_timer(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return (execution_engine->timer_heap_count == 0) ? NULL : execution_engine->timer_heap[0];
}

void* test_engine_get_idle_head(EXECUTION_ENGINE_HANDLE execution_engine)
{
    return execution_engine->idle_head;
}

void test_engine_set_stop(EXECUTION_ENGINE_HANDLE execution_engine, bool stop)
{
    execution_engine->stop = stop;
}

EXECUTION_ENGINE_LINUX_WORK_FUNCTION test_work_get_work_function(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->work_function;
}

void* test_work_get_work_function_context(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->work_function_context;
}

uint32_t test_work_get_pending_count(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->pending_count;
}

void test_work_set_pending_count(EXECUTION_ENGINE_LINUX_WORK_HANDLE work, uint32_t pending_count)
{
    work->pending_count = pending_count;
}

uint32_t test_work_get_running_count(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->running_count;
}

bool test_work_is_closed(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->is_closed;
}

bool test_work_is_timer(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->is_timer;
}

bool test_work_is_armed(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->heap_index != TIMER_NOT_ARMED;
}

uint64_t test_work_get_due_time_ms(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->due_time_ms;
}

uint32_t test_work_get_period_ms(EXECUTION_ENGINE_LINUX_WORK_HANDLE work)
{
    return work->period_ms;
}

pthread_cond_t* test_worker_get_wake_cond(void* worker)
{
    return &((WORKER*)worker)->wake_cond;
}

bool test_worker_is_signaled(void* worker)
{
    return ((WORKER*)worker)->is_signaled;
}

bool test_worker_is_idle(void* worker)
{
    return ((WORKER*)worker)->is_idle;
}

void test_worker_park(void* worker, uint64_t idle_since_ms)
{
    WORKER* the_worker = worker;
    the_worker->is_signaled = false;
    the_worker->idle_since_ms = idle_since_ms;
    idle_list_link_front(the_worker->execution_engine, the_worker);
}

void test_worker_retire(void* worker)
{
    WORKER* the_worker = worker;
    if (the_worker->is_idle)
    {
        idle_list_unlink(the_worker->execution_engine, the_worker);
    }
    the_worker->is_retired = true;
}

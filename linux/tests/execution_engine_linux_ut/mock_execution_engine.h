// Copyright (c) Microsoft. All rights reserved.

#ifndef MOCK_EXECUTION_ENGINE_H
#define MOCK_EXECUTION_ENGINE_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "c_pal/threadapi.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

// Test-only accessors to the internals of execution_engine_linux.c, implemented in mock_execution_engine.c.
// Workers are passed around as the void* argument captured from ThreadAPI_Create.

THREAD_START_FUNC test_get_worker_thread_func(void);
THREAD_START_FUNC test_get_manager_thread_func(void);
EXECUTION_ENGINE_HANDLE test_get_current_execution_engine(void);

pthread_mutex_t* test_engine_get_lock(EXECUTION_ENGINE_HANDLE execution_engine);
pthread_cond_t* test_engine_get_manager_cond(EXECUTION_ENGINE_HANDLE execution_engine);
pthread_cond_t* test_engine_get_work_done_cond(EXECUTION_ENGINE_HANDLE execution_engine);
uint32_t test_engine_get_max_worker_count(EXECUTION_ENGINE_HANDLE execution_engine);
uint32_t test_engine_get_worker_count(EXECUTION_ENGINE_HANDLE execution_engine);
uint32_t test_engine_get_signaled_worker_count(EXECUTION_ENGINE_HANDLE execution_engine);
bool test_engine_is_manager_watching_queue(EXECUTION_ENGINE_HANDLE execution_engine);
uint32_t test_engine_get_queue_count(EXECUTION_ENGINE_HANDLE execution_engine);
EXECUTION_ENGINE_LINUX_WORK_HANDLE test_engine_get_queue_head(EXECUTION_ENGINE_HANDLE execution_engine);
EXECUTION_ENGINE_LINUX_WORK_HANDLE test_engine_get_queue_tail(EXECUTION_ENGINE_HANDLE execution_engine);
uint64_t test_engine_get_last_progress_ms(EXECUTION_ENGINE_HANDLE execution_engine);
uint64_t test_engine_get_next_create_attempt_ms(EXECUTION_ENGINE_HANDLE execution_engine);
uint32_t test_engine_get_timer_heap_count(EXECUTION_ENGINE_HANDLE execution_engine);
uint32_t test_engine_get_timer_heap_capacity(EXECUTION_ENGINE_HANDLE execution_engine);
uint32_t test_engine_get_timer_count(EXECUTION_ENGINE_HANDLE execution_engine);
EXECUTION_ENGINE_LINUX_WORK_HANDLE test_engine_get_earliest_timer(EXECUTION_ENGINE_HANDLE execution_engine);
void* test_engine_get_idle_head(EXECUTION_ENGINE_HANDLE execution_engine);
void test_engine_set_stop(EXECUTION_ENGINE_HANDLE execution_engine, bool stop);

EXECUTION_ENGINE_LINUX_WORK_FUNCTION test_work_get_work_function(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
void* test_work_get_work_function_context(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
uint32_t test_work_get_pending_count(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
void test_work_set_pending_count(EXECUTION_ENGINE_LINUX_WORK_HANDLE work, uint32_t pending_count);
uint32_t test_work_get_running_count(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
bool test_work_is_closed(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
bool test_work_is_timer(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
bool test_work_is_armed(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
uint64_t test_work_get_due_time_ms(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
uint32_t test_work_get_period_ms(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);

pthread_cond_t* test_worker_get_wake_cond(void* worker);
bool test_worker_is_signaled(void* worker);
bool test_worker_is_idle(void* worker);

// puts the worker in the idle list the same way worker_thread_func does before waiting
void test_worker_park(void* worker, uint64_t idle_since_ms);

// marks the worker retired (removing it from the idle list) the same way manager_thread_func does
void test_worker_retire(void* worker);

#endif // MOCK_EXECUTION_ENGINE_H

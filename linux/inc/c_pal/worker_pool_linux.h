// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef WORKER_POOL_LINUX_H
#define WORKER_POOL_LINUX_H

#ifdef __cplusplus
#include <cstdbool>
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include <pthread.h>

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

// worker_pool_add_worker, worker_pool_retire_oldest_idle_worker, worker_pool_join_retired_workers and worker_pool_destroy
// must not be called concurrently: worker_pool_add_worker releases the lock while it creates a thread, and the new worker
// is linked into the pool only after the lock is acquired again
typedef struct WORKER_POOL_TAG* WORKER_POOL_HANDLE;

// called on a worker thread with the lock held, returns true if it ran work and false if there was no work to run
// when it leaves work for other worker threads, it must call worker_pool_wake_idle_worker: at most one woken worker
// thread is not running at a time, so woken worker threads wake each other one at a time
typedef bool (*WORKER_POOL_RUN_FUNCTION)(void* context);

#define WORKER_POOL_WAKE_RESULT_VALUES \
    WORKER_POOL_WAKE_RESULT_WOKEN, \
    WORKER_POOL_WAKE_RESULT_ALREADY_WAKING, \
    WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER, \
    WORKER_POOL_WAKE_RESULT_INVALID_ARGS

MU_DEFINE_ENUM(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);

typedef struct WORKER_POOL_COUNTS_TAG
{
    uint32_t thread_count;
    uint32_t starting_thread_count;
    uint32_t idle_thread_count;
    uint64_t oldest_idle_since_ms;
} WORKER_POOL_COUNTS;

MOCKABLE_FUNCTION(, WORKER_POOL_HANDLE, worker_pool_create, pthread_mutex_t*, lock, WORKER_POOL_RUN_FUNCTION, run_function, void*, run_function_context);
MOCKABLE_FUNCTION(, void, worker_pool_destroy, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, int, worker_pool_add_worker, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, WORKER_POOL_WAKE_RESULT, worker_pool_wake_idle_worker, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, void, worker_pool_get_counts, WORKER_POOL_HANDLE, worker_pool, WORKER_POOL_COUNTS*, counts);
MOCKABLE_FUNCTION(, void, worker_pool_retire_oldest_idle_worker, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, void, worker_pool_join_retired_workers, WORKER_POOL_HANDLE, worker_pool);

#ifdef __cplusplus
}
#endif

#endif // WORKER_POOL_LINUX_H

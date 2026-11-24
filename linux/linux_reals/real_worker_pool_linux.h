// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef REAL_WORKER_POOL_LINUX_H
#define REAL_WORKER_POOL_LINUX_H

#include <pthread.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/worker_pool_linux.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_WORKER_POOL_LINUX_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        worker_pool_create, \
        worker_pool_destroy, \
        worker_pool_add_worker, \
        worker_pool_wake_idle_worker, \
        worker_pool_get_counts, \
        worker_pool_retire_oldest_idle_worker, \
        worker_pool_join_retired_workers \
    )

WORKER_POOL_HANDLE real_worker_pool_create(pthread_mutex_t* lock, WORKER_POOL_RUN_FUNCTION run_function, void* run_function_context);
void real_worker_pool_destroy(WORKER_POOL_HANDLE worker_pool);
int real_worker_pool_add_worker(WORKER_POOL_HANDLE worker_pool);
WORKER_POOL_WAKE_RESULT real_worker_pool_wake_idle_worker(WORKER_POOL_HANDLE worker_pool);
void real_worker_pool_get_counts(WORKER_POOL_HANDLE worker_pool, WORKER_POOL_COUNTS* counts);
void real_worker_pool_retire_oldest_idle_worker(WORKER_POOL_HANDLE worker_pool);
void real_worker_pool_join_retired_workers(WORKER_POOL_HANDLE worker_pool);

#endif // REAL_WORKER_POOL_LINUX_H

// Copyright (C) Microsoft Corporation. All rights reserved.

#include <pthread.h>

#define pthread_mutex_lock         mocked_pthread_mutex_lock
#define pthread_mutex_unlock       mocked_pthread_mutex_unlock
#define pthread_cond_init          mocked_pthread_cond_init
#define pthread_cond_destroy       mocked_pthread_cond_destroy
#define pthread_cond_wait          mocked_pthread_cond_wait
#define pthread_cond_signal        mocked_pthread_cond_signal

int mocked_pthread_mutex_lock(pthread_mutex_t* mutex);
int mocked_pthread_mutex_unlock(pthread_mutex_t* mutex);
int mocked_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int mocked_pthread_cond_destroy(pthread_cond_t* cond);
int mocked_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int mocked_pthread_cond_signal(pthread_cond_t* cond);

#include "../../src/worker_pool_linux.c"

// lets the tests check the state of a worker, which is not visible through the API
MU_DEFINE_ENUM_STRINGS(WORKER_STATE, WORKER_STATE_VALUES);

const char* mock_worker_pool_linux_get_worker_state(void* worker);

const char* mock_worker_pool_linux_get_worker_state(void* worker)
{
    WORKER* worker_to_check = worker;
    return MU_ENUM_TO_STRING(WORKER_STATE, worker_to_check->state);
}

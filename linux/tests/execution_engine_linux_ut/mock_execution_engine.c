// Copyright (C) Microsoft Corporation. All rights reserved.

#include <pthread.h>
#include <time.h>

#define pthread_mutex_init         mocked_pthread_mutex_init
#define pthread_mutex_lock         mocked_pthread_mutex_lock
#define pthread_mutex_unlock       mocked_pthread_mutex_unlock
#define pthread_mutex_destroy      mocked_pthread_mutex_destroy
#define pthread_condattr_init      mocked_pthread_condattr_init
#define pthread_condattr_setclock  mocked_pthread_condattr_setclock
#define pthread_condattr_destroy   mocked_pthread_condattr_destroy
#define pthread_cond_init          mocked_pthread_cond_init
#define pthread_cond_signal        mocked_pthread_cond_signal
#define pthread_cond_broadcast     mocked_pthread_cond_broadcast
#define pthread_cond_wait          mocked_pthread_cond_wait
#define pthread_cond_timedwait     mocked_pthread_cond_timedwait
#define pthread_cond_destroy       mocked_pthread_cond_destroy
#define pthread_create             mocked_pthread_create
#define pthread_join               mocked_pthread_join
#define pthread_detach             mocked_pthread_detach

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
int mocked_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int mocked_pthread_join(pthread_t thread, void** retval);
int mocked_pthread_detach(pthread_t thread);

#include "../../src/execution_engine_linux.c"

// Copyright (c) Microsoft. All rights reserved.

#include <pthread.h>


#define pthread_rwlock_init        mocked_pthread_rwlock_init
#define pthread_rwlock_wrlock      mocked_pthread_rwlock_wrlock
#define pthread_rwlock_trywrlock   mocked_pthread_rwlock_trywrlock
#define pthread_rwlock_unlock      mocked_pthread_rwlock_unlock
#define pthread_rwlock_rdlock      mocked_pthread_rwlock_rdlock
#define pthread_rwlock_tryrdlock   mocked_pthread_rwlock_tryrdlock
#define pthread_rwlock_destroy     mocked_pthread_rwlock_destroy

int mocked_pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);
int mocked_pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int mocked_pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int mocked_pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int mocked_pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int mocked_pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int mocked_pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

#include "../../src/srw_lock_ll_linux.c"

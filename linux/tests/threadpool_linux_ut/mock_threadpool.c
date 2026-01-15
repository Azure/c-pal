// Copyright (c) Microsoft. All rights reserved.

#include <time.h>
#include <semaphore.h>
#include <bits/types/timer_t.h>

#define timer_create mocked_timer_create
#define timer_settime mocked_timer_settime
#define timer_delete mocked_timer_delete
#define sem_init mocked_sem_init
#define sem_post mocked_sem_post
#define sem_timedwait mocked_sem_timedwait
#define clock_gettime mocked_clock_gettime

struct itimerspec;
struct sigevent;
struct timespec;

int mocked_timer_create(clockid_t clockid, struct sigevent* restrict sevp, timer_t* restrict timerid);
int mocked_timer_settime(timer_t timerid, int flags, const struct itimerspec* new_value, struct itimerspec* old_value);
int mocked_timer_delete(timer_t timerid);
int mocked_sem_post(sem_t* sem);
int mocked_sem_init(sem_t* sem, int pshared, unsigned int value);
int mocked_sem_timedwait(sem_t* sem, const struct timespec* abs_timeout);
int mocked_clock_gettime(clockid_t clockid, struct timespec* tp);

// These exist here to avoid collisions with the mocks
// This is due to the fact that THREADPOOL_WORK_ITEM is internal to threadpool_linux.c
#define THREADPOOL_WORK_ITEM_ASSIGN another_THREADPOOL_WORK_ITEM_ASSIGN
#define THREADPOOL_WORK_ITEM_INITIALIZE another_THREADPOOL_WORK_ITEM_INITIALIZE
#define THREADPOOL_WORK_ITEM_MOVE another_THREADPOOL_WORK_ITEM_MOVE
#define THREADPOOL_WORK_ITEM_INITIALIZE_MOVE another_THREADPOOL_WORK_ITEM_INITIALIZE_MOVE

#include "../../src/threadpool_linux.c"

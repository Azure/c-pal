// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_THREADPOOL_TIMER_THANDLE_H
#define REAL_THREADPOOL_TIMER_THANDLE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "real_thandle_helper.h"

#include "c_pal/threadpool.h"

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: unlike real_threadpool, this does not use the reals for threadpool, it only has reals for the THANDLE functions
// Use this when you want to mock the threadpool functions but still want the reals for the THANDLE functions

typedef struct THREADPOOL_TIMER_TAG
{
    uint8_t dummy;
} THREADPOOL_TIMER;

REAL_THANDLE_DECLARE(THREADPOOL_TIMER)

THANDLE(THREADPOOL_TIMER) real_threadpool_timer_thandle_create(void);

#ifdef __cplusplus
}
#endif

#endif /* REAL_THREADPOOL_TIMER_THANDLE_H */

// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_THREADPOOL_WORK_ITEM_THANDLE_H
#define REAL_THREADPOOL_WORK_ITEM_THANDLE_H

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

// NOTE: unlike real_threadpool, this does not use the reals for threadpool, it only has reals for the THANDLE functions of THANDLE(THREADPOOL_WORK_ITEM)
// Use this when you want to mock the threadpool functions but still want the reals for the THANDLE functions

typedef struct THREADPOOL_WORK_ITEM_TAG
{
    uint8_t dummy;
} THREADPOOL_WORK_ITEM;

REAL_THANDLE_DECLARE(THREADPOOL_WORK_ITEM)

THANDLE(THREADPOOL_WORK_ITEM) real_threadpool_work_item_thandle_create(void);

#ifdef __cplusplus
}
#endif

#endif /* REAL_THREADPOOL_WORK_ITEM_THANDLE_H */

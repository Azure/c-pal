// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THREADPOOL_WORK_ITEM_QUEUE_H
#define THREADPOOL_WORK_ITEM_QUEUE_H

#include "c_pal/thandle.h"
#include "c_pal/tqueue.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct THREADPOOL_WORK_ITEM_TAG THREADPOOL_WORK_ITEM;

THANDLE_TYPE_DECLARE(THREADPOOL_WORK_ITEM);
TQUEUE_DEFINE_STRUCT_TYPE(THANDLE(THREADPOOL_WORK_ITEM));

THANDLE_TYPE_DECLARE(TQUEUE_TYPEDEF_NAME(THANDLE(THREADPOOL_WORK_ITEM)));
TQUEUE_TYPE_DECLARE(THANDLE(THREADPOOL_WORK_ITEM));

#ifdef __cplusplus
}
#endif

#endif /* THREADPOOL_WORK_ITEM_QUEUE_H */

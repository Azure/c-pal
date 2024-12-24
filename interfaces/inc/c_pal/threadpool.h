// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef THREADPOOL_H
#define THREADPOOL_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/execution_engine.h"
#include "c_pal/thandle.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct THREADPOOL_TAG THREADPOOL;
typedef struct THREADPOOL_WORK_ITEM_TAG THREADPOOL_WORK_ITEM;
typedef void (*THREADPOOL_WORK_FUNCTION)(void* context);

typedef struct THREADPOOL_TIMER_TAG THREADPOOL_TIMER;
THANDLE_TYPE_DECLARE(THREADPOOL_TIMER);

THANDLE_TYPE_DECLARE(THREADPOOL);
THANDLE_TYPE_DECLARE(THREADPOOL_WORK_ITEM);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);

MOCKABLE_FUNCTION(, int, threadpool_open, THANDLE(THREADPOOL), threadpool);
MOCKABLE_FUNCTION(, void, threadpool_close, THANDLE(THREADPOOL), threadpool);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_WORK_ITEM), threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THANDLE(THREADPOOL_WORK_ITEM), threadpool_work_item);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_TIMER), threadpool_timer_start, THANDLE(THREADPOOL), threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_timer_restart, THANDLE(THREADPOOL_TIMER), timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);

MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, THANDLE(THREADPOOL_TIMER), timer);

#ifdef __cplusplus
}
#endif

#endif // THREADPOOL_H

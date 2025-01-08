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
typedef struct TIMER_INSTANCE_TAG* TIMER_INSTANCE_HANDLE;
typedef struct THREADPOOL_WORK_ITEM_TAG THREADPOOL_WORK_ITEM;
typedef void (*THREADPOOL_WORK_FUNCTION)(void* context);

THANDLE_TYPE_DECLARE(THREADPOOL);
THANDLE_TYPE_DECLARE(THREADPOOL_WORK_ITEM);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_WORK_ITEM), threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THANDLE(THREADPOOL_WORK_ITEM), threadpool_work_item);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_timer_start, THANDLE(THREADPOOL), threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context, TIMER_INSTANCE_HANDLE*, timer_handle);

MOCKABLE_FUNCTION(, int, threadpool_timer_restart, TIMER_INSTANCE_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);

MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, TIMER_INSTANCE_HANDLE, timer);

MOCKABLE_FUNCTION(, void, threadpool_timer_destroy, TIMER_INSTANCE_HANDLE, timer);

#ifdef __cplusplus
}
#endif

#endif // THREADPOOL_H

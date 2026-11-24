// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef EXECUTION_ENGINE_LINUX_H
#define EXECUTION_ENGINE_LINUX_H

#ifdef __cplusplus
#include <cstdbool>
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "c_pal/execution_engine.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EXECUTION_ENGINE_LINUX_WORK_TAG* EXECUTION_ENGINE_LINUX_WORK_HANDLE;
typedef struct EXECUTION_ENGINE_LINUX_TIMER_TAG* EXECUTION_ENGINE_LINUX_TIMER_HANDLE;
typedef void (*EXECUTION_ENGINE_LINUX_WORK_FUNCTION)(void* context);

MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_WORK_HANDLE, execution_engine_linux_create_work, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
MOCKABLE_FUNCTION(, void, execution_engine_linux_submit_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work, bool, cancel_pending);
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, execution_engine_linux_create_timer, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
MOCKABLE_FUNCTION(, void, execution_engine_linux_set_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
MOCKABLE_FUNCTION(, void, execution_engine_linux_cancel_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, bool, cancel_pending);
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ENGINE_LINUX_H

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_THREADPOOL_H
#define REAL_THREADPOOL_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/threadpool.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_THREADPOOL_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        threadpool_create, \
        threadpool_destroy, \
        threadpool_open_async, \
        threadpool_close, \
        threadpool_schedule_work, \
        threadpool_timer_start, \
        threadpool_timer_restart, \
        threadpool_timer_cancel, \
        threadpool_timer_destroy \
)

#ifdef __cplusplus
extern "C" {
#endif

THREADPOOL_HANDLE real_threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine);
void real_threadpool_destroy(THREADPOOL_HANDLE threadpool);
int real_threadpool_open_async(THREADPOOL_HANDLE threadpool, ON_THREADPOOL_OPEN_COMPLETE on_open_complete, void* on_open_complete_context);
void real_threadpool_close(THREADPOOL_HANDLE threadpool);
int real_threadpool_schedule_work(THREADPOOL_HANDLE threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context);
int real_threadpool_timer_start(THREADPOOL_HANDLE threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context, TIMER_INSTANCE_HANDLE* timer_handle);
int real_threadpool_timer_restart(TIMER_INSTANCE_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms);
void real_threadpool_timer_cancel(TIMER_INSTANCE_HANDLE timer);
void real_threadpool_timer_destroy(TIMER_INSTANCE_HANDLE timer);

#ifdef __cplusplus
}
#endif

#endif // REAL_THREADPOOL_H

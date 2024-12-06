// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_THREADPOOL_H
#define REAL_THREADPOOL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_THREADPOOL_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        threadpool_create, \
        threadpool_open, \
        threadpool_close, \
        threadpool_create_work_item, \
        threadpool_schedule_work, \
        threadpool_schedule_work_item, \
        threadpool_timer_start, \
        threadpool_timer_restart, \
        threadpool_timer_cancel, \
        threadpool_timer_destroy \
    ) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(THREADPOOL), THANDLE_MOVE(real_THREADPOOL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(THREADPOOL), THANDLE_INITIALIZE(real_THREADPOOL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(THREADPOOL), THANDLE_INITIALIZE_MOVE(real_THREADPOOL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(THREADPOOL), THANDLE_ASSIGN(real_THREADPOOL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(THREADPOOL_WORK_ITEM_CONTEXT), THANDLE_MOVE(real_THREADPOOL_WORK_ITEM_CONTEXT)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(THREADPOOL_WORK_ITEM_CONTEXT), THANDLE_INITIALIZE(real_THREADPOOL_WORK_ITEM_CONTEXT)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(THREADPOOL_WORK_ITEM_CONTEXT), THANDLE_INITIALIZE_MOVE(real_THREADPOOL_WORK_ITEM_CONTEXT)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(THREADPOOL_WORK_ITEM_CONTEXT), THANDLE_ASSIGN(real_THREADPOOL_WORK_ITEM_CONTEXT)) \

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct THREADPOOL_TAG real_THREADPOOL;
    THANDLE_TYPE_DECLARE(real_THREADPOOL);
    typedef struct THREADPOOL_WORK_ITEM_CONTEXT_TAG real_THREADPOOL_WORK_ITEM_CONTEXT;
    THANDLE_TYPE_DECLARE(real_THREADPOOL_WORK_ITEM_CONTEXT);

    THANDLE(THREADPOOL) real_threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine);

    int real_threadpool_open(THANDLE(THREADPOOL) threadpool);
    void real_threadpool_close(THANDLE(THREADPOOL) threadpool);

    THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) real_threadpool_create_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context);

    int real_threadpool_schedule_work_item(THANDLE(THREADPOOL) threadpool, THANDLE(THREADPOOL_WORK_ITEM_CONTEXT) threadpool_work_item_context);

    int real_threadpool_schedule_work(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context);

    int real_threadpool_timer_start(THANDLE(THREADPOOL) threadpool, uint32_t start_delay_ms, uint32_t timer_period_ms, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context, TIMER_INSTANCE_HANDLE* timer_handle);

    int real_threadpool_timer_restart(TIMER_INSTANCE_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms);

    void real_threadpool_timer_cancel(TIMER_INSTANCE_HANDLE timer);

    void real_threadpool_timer_destroy(TIMER_INSTANCE_HANDLE timer);

#ifdef __cplusplus
}
#endif

#endif //REAL_THREADPOOL_H

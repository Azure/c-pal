// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef REAL_EXECUTION_ENGINE_LINUX_H
#define REAL_EXECUTION_ENGINE_LINUX_H

#ifdef __cplusplus
#include <cstdbool>
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_EXECUTION_ENGINE_LINUX_GLOBAL_MOCK_HOOK()          \
    MU_FOR_EACH_1(R2,                                   \
        execution_engine_linux_get_parameters, \
        execution_engine_linux_create_work, \
        execution_engine_linux_create_timer, \
        execution_engine_linux_submit_work, \
        execution_engine_linux_set_timer, \
        execution_engine_linux_cancel_timer, \
        execution_engine_linux_wait_for_work, \
        execution_engine_linux_close_work, \
        execution_engine_linux_disassociate_current_thread_from_work \
    )

#ifdef __cplusplus
extern "C" {
#endif

    const EXECUTION_ENGINE_PARAMETERS* real_execution_engine_linux_get_parameters(EXECUTION_ENGINE_HANDLE execution_engine);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE real_execution_engine_linux_create_work(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE real_execution_engine_linux_create_timer(EXECUTION_ENGINE_HANDLE execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context);
    void real_execution_engine_linux_submit_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
    void real_execution_engine_linux_set_timer(EXECUTION_ENGINE_LINUX_WORK_HANDLE timer, uint32_t start_delay_ms, uint32_t timer_period_ms);
    void real_execution_engine_linux_cancel_timer(EXECUTION_ENGINE_LINUX_WORK_HANDLE timer);
    void real_execution_engine_linux_wait_for_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work, bool cancel_pending);
    void real_execution_engine_linux_close_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);
    void real_execution_engine_linux_disassociate_current_thread_from_work(EXECUTION_ENGINE_LINUX_WORK_HANDLE work);

#ifdef __cplusplus
}
#endif

#endif // REAL_EXECUTION_ENGINE_LINUX_H

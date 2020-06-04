// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef EXECUTION_ENGINE_WIN32_H
#define EXECUTION_ENGINE_WIN32_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "windows.h"
#include "execution_engine.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct EXECUTION_ENGINE_PARAMETERS_WIN32_TAG
    {
        uint32_t min_thread_count;
        uint32_t max_thread_count;
    } EXECUTION_ENGINE_PARAMETERS_WIN32;

#define DEFAULT_MIN_THREAD_COUNT 4
#define DEFAULT_MAX_THREAD_COUNT 0 // no max thread count

MOCKABLE_FUNCTION(, PTP_POOL, execution_engine_win32_get_threadpool, EXECUTION_ENGINE_HANDLE, execution_engine);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ENGINE_WIN32_H

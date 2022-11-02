// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef EXECUTION_ENGINE_LINUX_H
#define EXECUTION_ENGINE_LINUX_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "c_pal/execution_engine.h"

#include "umock_c/umock_c_prod.h"

    typedef struct EXECUTION_ENGINE_PARAMETERS_LINUX_TAG
    {
        uint32_t min_thread_count;
        uint32_t max_thread_count;
    } EXECUTION_ENGINE_PARAMETERS_LINUX;

#define DEFAULT_MIN_THREAD_COUNT 4
#define DEFAULT_MAX_THREAD_COUNT 0 // no max thread count

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS_LINUX*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ENGINE_LINUX_H

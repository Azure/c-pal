// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EXECUTION_ENGINE_PARAMETERS_TAG
{
    uint32_t min_thread_count;
    uint32_t max_thread_count;
} EXECUTION_ENGINE_PARAMETERS;

#define DEFAULT_MIN_THREAD_COUNT 4
#define DEFAULT_MAX_THREAD_COUNT 0 // no max thread count

typedef struct EXECUTION_ENGINE_TAG* EXECUTION_ENGINE_HANDLE;

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_parameters);
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ENGINE_H

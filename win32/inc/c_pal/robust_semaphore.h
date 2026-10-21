// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef ROBUST_SEMAPHORE_H
#define ROBUST_SEMAPHORE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

/* a robust semaphore with N units is made of N mutexes, waited on together with the cancel event (MAXIMUM_WAIT_OBJECTS is 64) */
#define ROBUST_SEMAPHORE_MAX_COUNT 63

/* wait for a unit until one is free / hold a unit with no time limit */
#define ROBUST_SEMAPHORE_NO_TIMEOUT UINT32_MAX

#define ROBUST_SEMAPHORE_UNIT_INDEX_NONE UINT32_MAX

#define ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES \
    ROBUST_SEMAPHORE_ACQUIRE_OK, \
    ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT, \
    ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, \
    ROBUST_SEMAPHORE_ACQUIRE_ERROR

MU_DEFINE_ENUM(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES);

typedef struct ROBUST_SEMAPHORE_TAG* ROBUST_SEMAPHORE_HANDLE;

typedef void (*ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE)(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index);

#ifdef __cplusplus
extern "C"
{
#endif
    MOCKABLE_FUNCTION(, ROBUST_SEMAPHORE_HANDLE, robust_semaphore_create, const char*, name, uint32_t, max_count, const char*, security_descriptor);
    MOCKABLE_FUNCTION(, void, robust_semaphore_destroy, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
    MOCKABLE_FUNCTION(, int, robust_semaphore_acquire_async, ROBUST_SEMAPHORE_HANDLE, robust_semaphore, uint32_t, timeout_ms, uint32_t, max_hold_ms, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE, on_acquire_complete, void*, on_acquire_complete_context);
    MOCKABLE_FUNCTION(, void, robust_semaphore_cancel, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
    MOCKABLE_FUNCTION(, void, robust_semaphore_release, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
#ifdef __cplusplus
}
#endif

#endif // ROBUST_SEMAPHORE_H

// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SINGLE_PERFORMANCE_COUNTER_H
#define SINGLE_PERFORMANCE_COUNTER_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#else
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SINGLE_PERFORMANCE_COUNTER_TAG* SINGLE_PERFORMANCE_COUNTER_HANDLE;

#define SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT_VALUES \
        SINGLE_PERFORMANCE_COUNTER_SAMPLE_SUCCESS, \
        SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR, \
        SINGLE_PERFORMANCE_COUNTER_SAMPLE_COLLECT_FAILED, \
        SINGLE_PERFORMANCE_COUNTER_SAMPLE_FORMAT_FAILED

MU_DEFINE_ENUM(SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT_VALUES);

MOCKABLE_FUNCTION(, SINGLE_PERFORMANCE_COUNTER_HANDLE, single_performance_counter_create, const char*, performance_object, const char*, performance_counter);
MOCKABLE_FUNCTION(, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, single_performance_counter_sample_double, SINGLE_PERFORMANCE_COUNTER_HANDLE, handle, double*, sample);
MOCKABLE_FUNCTION(, void, single_performance_counter_destroy, SINGLE_PERFORMANCE_COUNTER_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /* SINGLE_PERFORMANCE_COUNTER_H */

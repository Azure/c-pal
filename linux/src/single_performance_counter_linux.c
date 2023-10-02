// Copyright (C) Microsoft Corporation. All rights reserved.

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/single_performance_counter.h"

// Stubbed functions for Linux
typedef struct SINGLE_PERFORMANCE_COUNTER_TAG
{
    int reserved;
} SINGLE_PERFORMANCE_COUNTER;

MU_DEFINE_ENUM_STRINGS(SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT_VALUES);

IMPLEMENT_MOCKABLE_FUNCTION(, SINGLE_PERFORMANCE_COUNTER_HANDLE, single_performance_counter_create, const char*, performance_object, const char*, performance_counter)
{
    LogError("single_performance_counter_create not implemented");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, single_performance_counter_destroy, SINGLE_PERFORMANCE_COUNTER_HANDLE, handle)
{
    LogError("single_performance_counter_destroy not implemented");
    (void)handle;
}

IMPLEMENT_MOCKABLE_FUNCTION(, SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT, single_performance_counter_sample_double, SINGLE_PERFORMANCE_COUNTER_HANDLE, handle, double*, sample)
{
    SINGLE_PERFORMANCE_COUNTER_SAMPLE_RESULT result = SINGLE_PERFORMANCE_COUNTER_SAMPLE_ERROR;
    LogError("single_performance_counter_sample_double not implemented");
    return result;
}



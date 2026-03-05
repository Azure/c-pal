// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef JOB_OBJECT_HELPER_H
#define JOB_OBJECT_HELPER_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif


#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

// Passing JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL as percent_cpu disables CPU rate control (removes the throttle)
#define JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL 0

// Passing JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT as percent_physical_memory removes memory limits
#define JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT 0

typedef struct JOB_OBJECT_HELPER_TAG JOB_OBJECT_HELPER;
THANDLE_TYPE_DECLARE(JOB_OBJECT_HELPER);

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /*
    * Note: job_object_helper_set_job_limits_to_current_process and job_object_helper_deinit_for_test are NOT thread-safe.
    * These functions must be called from a single thread. Concurrent calls from multiple threads may result in
    * undefined behavior due to non-atomic singleton initialization and cleanup.
    */
    MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory);
    MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test);
#ifdef __cplusplus
}
#endif

#endif // JOB_OBJECT_HELPER_H

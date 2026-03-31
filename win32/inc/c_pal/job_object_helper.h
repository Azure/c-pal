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

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /*
    * Note: job_object_helper_set_job_limits_to_current_process, job_object_helper_deinit_for_test, and
    * job_object_helper_get_internal_job_object_handle_for_test are NOT thread-safe.
    * These functions must be called from a single thread. Concurrent calls from multiple threads may result in
    * undefined behavior due to non-atomic singleton initialization and cleanup.
    */
    MOCKABLE_FUNCTION(, int, job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory);
    MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test);
    MOCKABLE_FUNCTION(, void*, job_object_helper_get_internal_job_object_handle_for_test);
#ifdef __cplusplus
}
#endif

#endif // JOB_OBJECT_HELPER_H

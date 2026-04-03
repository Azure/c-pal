// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _REAL_JOB_OBJECT_HELPER_H
#define _REAL_JOB_OBJECT_HELPER_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "windows.h"

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_JOB_OBJECT_HELPER_GLOBAL_MOCK_HOOK()          \
MU_FOR_EACH_1(R2,                                   \
    job_object_helper_set_job_limits_to_current_process, \
    job_object_helper_deinit_for_test, \
    job_object_helper_get_internal_job_object_handle_for_test \
)

#ifdef __cplusplus
extern "C" {
#endif

    int real_job_object_helper_set_job_limits_to_current_process(const char* job_name, uint32_t percent_cpu, uint32_t percent_physical_memory);

    void real_job_object_helper_deinit_for_test(void);

    void* real_job_object_helper_get_internal_job_object_handle_for_test(void);

#ifdef __cplusplus
}
#endif

#endif //REAL_JOB_OBJECT_HELPER_H

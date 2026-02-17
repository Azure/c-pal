// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _REAL_JOB_OBJECT_HELPER_H
#define _REAL_JOB_OBJECT_HELPER_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_JOB_OBJECT_HELPER_GLOBAL_MOCK_HOOK()          \
MU_FOR_EACH_1(R2,                                   \
    job_object_helper_set_job_limits_to_current_process, \
    job_object_helper_deinit \
) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(JOB_OBJECT_HELPER), THANDLE_MOVE(real_JOB_OBJECT_HELPER)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(JOB_OBJECT_HELPER), THANDLE_INITIALIZE(real_JOB_OBJECT_HELPER)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(JOB_OBJECT_HELPER), THANDLE_INITIALIZE_MOVE(real_JOB_OBJECT_HELPER)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(JOB_OBJECT_HELPER), THANDLE_ASSIGN(real_JOB_OBJECT_HELPER)) \

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct JOB_OBJECT_HELPER_TAG real_JOB_OBJECT_HELPER;
    THANDLE_TYPE_DECLARE(real_JOB_OBJECT_HELPER);

    THANDLE(JOB_OBJECT_HELPER) real_job_object_helper_set_job_limits_to_current_process(const char* job_name, uint32_t percent_cpu, uint32_t percent_physical_memory);

    void real_job_object_helper_deinit(void);

#ifdef __cplusplus
}
#endif

#endif //REAL_JOB_OBJECT_HELPER_H

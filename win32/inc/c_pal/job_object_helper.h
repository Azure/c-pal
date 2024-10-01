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

#include "c_pal/thandle.h"

typedef struct JOB_OBJECT_HELPER_TAG JOB_OBJECT_HELPER;
THANDLE_TYPE_DECLARE(JOB_OBJECT_HELPER);

#ifdef __cplusplus
extern "C"
{
#endif
    MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create);
    MOCKABLE_FUNCTION(, int, job_object_helper_open, THANDLE(JOB_OBJECT_HELPER), job_object_helper);
    MOCKABLE_FUNCTION(, void, job_object_helper_close, THANDLE(JOB_OBJECT_HELPER), job_object_helper);
    MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory);
    MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu);
#ifdef __cplusplus
}
#endif

#endif // JOB_OBJECT_HELPER_H

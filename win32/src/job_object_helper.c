// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/job_object_helper.h"


static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper)
{
    (void) job_object_helper;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create)
{
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory)
{
    (void) job_object_helper;
    (void) percent_physical_memory;
    return MU_FAILURE;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu)
{
    (void) job_object_helper;
    (void) percent_cpu;
    return MU_FAILURE;
}

// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>
#include <inttypes.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/job_object_helper.h"
#include "c_pal/uuid.h"


#define MAX_CPU_PERCENT 100
#define MAX_MEMORY_PERCENT 100
#define TEST_JOB_NAME_PREFIX "job_test_ebs_"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(init)
{
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    job_object_helper_deinit_for_test();
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_001: [ If percent_cpu is 100 and percent_physical_memory is 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object (100% CPU and 100% memory are effectively no limits). ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_with_100_percent_cpu_and_memory_returns_null)
{
    /* 100% CPU with 100% memory is effectively no limit.
       The function should return NULL without creating a job object,
       avoiding any risk of compounding if called again. */

    ///act
    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process("", MAX_CPU_PERCENT, MAX_MEMORY_PERCENT);

    ///assert
    ASSERT_IS_NULL(result, "100%% CPU with 100%% memory should return NULL (no job object needed)");
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_001: [ If percent_cpu is 100 and percent_physical_memory is 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object (100% CPU and 100% memory are effectively no limits). ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_with_100_percent_cpu_and_memory_does_not_create_job_object)
{
    /* 100% CPU with 100% memory is effectively no limit.
       The function should return NULL and no job object should be created. */

    ///arrange
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with job name: %s...", job_name);

    ///act
    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process(job_name, MAX_CPU_PERCENT, MAX_MEMORY_PERCENT);

    ///assert
    ASSERT_IS_NULL(result, "100%% CPU with 100%% memory should return NULL (no job object needed)");

    /* Verify that no job object was actually created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NULL(job_object, "No job object should have been created for 100%% CPU and 100%% memory");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

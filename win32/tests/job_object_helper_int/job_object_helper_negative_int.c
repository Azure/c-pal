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

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_no_effective_limits_and_null_job_name)
{
    /* Both (0,0) and (100,100) are effectively no limits.
       The function should return NULL without creating a job object,
       avoiding any risk of compounding if called again.
       Uses empty string (unnamed job object). */

    uint32_t cpu_limits[2] = { 0, MAX_CPU_PERCENT };
    uint32_t memory_limits[2] = { 0, MAX_MEMORY_PERCENT };

    for (int i = 0; i < MU_COUNT_ARRAY_ITEMS(cpu_limits); ++i)
    {
        ///act
        THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(NULL, cpu_limits[i], memory_limits[i]);

        ///assert
        ASSERT_IS_NULL(job_object_helper, "(%" PRIu32 ", %" PRIu32 ") should return NULL (no job object needed)", cpu_limits[i], memory_limits[i]);
    }
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_no_effective_limits_and_named_job_object)
{
    /* Both (0,0) and (100,100) are effectively no limits.
       The function should return NULL and no job object should be created.
       Uses a named job object so we can verify via OpenJobObjectA that no OS object was created. */

    uint32_t cpu_limits[2] = { 0, MAX_CPU_PERCENT };
    uint32_t memory_limits[2] = { 0, MAX_MEMORY_PERCENT };

    for (int i = 0; i < MU_COUNT_ARRAY_ITEMS(cpu_limits); ++i)
    {
        ///arrange
        UUID_T job_name_uuid;
        (void)uuid_produce(&job_name_uuid);

        char job_name[64];
        (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
        LogInfo("Running test with job name: %s, cpu=%" PRIu32 ", memory=%" PRIu32 "...", job_name, cpu_limits[i], memory_limits[i]);

        ///act
        THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, cpu_limits[i], memory_limits[i]);

        ///assert
        ASSERT_IS_NULL(job_object_helper, "(%" PRIu32 ", %" PRIu32 ") should return NULL (no job object needed)", cpu_limits[i], memory_limits[i]);

        /* Verify that no job object was actually created */
        HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
        ASSERT_IS_NULL(job_object, "No job object should have been created for (%" PRIu32 ", %" PRIu32 ")", cpu_limits[i], memory_limits[i]);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

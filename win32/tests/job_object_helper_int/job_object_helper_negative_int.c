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
#include "c_pal/timed_test_suite.h"


#define MAX_CPU_PERCENT 100
#define MAX_MEMORY_PERCENT 100
#define TEST_JOB_NAME_PREFIX "job_test_ebs_"

#define INITIAL_CPU_PERCENT 50
#define INITIAL_MEMORY_PERCENT 1

static void generate_test_job_name(char* job_name_out, size_t job_name_size)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);
    (void)snprintf(job_name_out, job_name_size, TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Test job name: %s", job_name_out);
}

static THANDLE(JOB_OBJECT_HELPER) create_job_object_with_limits(char* job_name_out, size_t job_name_size, uint32_t cpu, uint32_t memory)
{
    generate_test_job_name(job_name_out, job_name_size);
    LogInfo("Creating job object (cpu=%" PRIu32 ", memory=%" PRIu32 ") with job name: %s...", cpu, memory, job_name_out);

    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process(job_name_out, cpu, memory);
    ASSERT_IS_NOT_NULL(result, "Job object should be created (cpu=%" PRIu32 ", memory=%" PRIu32 ")", cpu, memory);
    return result;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TIMED_TEST_SUITE_INITIALIZE(suite_init, TIMED_TEST_DEFAULT_TIMEOUT_MS)
{
}

TIMED_TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(init)
{
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    job_object_helper_deinit_for_test();
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_both_zero_and_null_job_name)
{
    /* (0,0) is not allowed — at least one limit must be non-zero. */

    ///act
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(NULL, 0, 0);

    ///assert
    ASSERT_IS_NULL(job_object_helper, "(0, 0) should return NULL");
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_both_100_and_null_job_name)
{
    /* (100,100) is effectively no limits — the function returns NULL without creating a job object. */

    ///act
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(NULL, MAX_CPU_PERCENT, MAX_MEMORY_PERCENT);

    ///assert
    ASSERT_IS_NULL(job_object_helper, "(100, 100) should return NULL (no job object needed)");
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_both_zero_and_named_job_object)
{
    /* (0,0) is not allowed. Uses a named job object to verify no OS object was created. */

    ///arrange
    char job_name[64];
    generate_test_job_name(job_name, sizeof(job_name));

    ///act
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, 0, 0);

    ///assert
    ASSERT_IS_NULL(job_object_helper, "(0, 0) should return NULL");

    /* Verify that no job object was actually created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NULL(job_object, "No job object should have been created for (0, 0)");
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_both_100_and_named_job_object)
{
    /* (100,100) is effectively no limits. Uses a named job object to verify no OS object was created. */

    ///arrange
    char job_name[64];
    generate_test_job_name(job_name, sizeof(job_name));

    ///act
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, MAX_CPU_PERCENT, MAX_MEMORY_PERCENT);

    ///assert
    ASSERT_IS_NULL(job_object_helper, "(100, 100) should return NULL (no job object needed)");

    /* Verify that no job object was actually created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NULL(job_object, "No job object should have been created for (100, 100)");
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_cpu_exceeds_100)
{
    /* percent_cpu > 100 is invalid */

    uint32_t cpu_limits[] = { 101, 200, UINT32_MAX };
    uint32_t memory_limits[] = { 50, 50, 50 };

    for (int i = 0; i < MU_COUNT_ARRAY_ITEMS(cpu_limits); ++i)
    {
        LogInfo("Running test with cpu=%" PRIu32 ", memory=%" PRIu32 "...", cpu_limits[i], memory_limits[i]);

        ///act
        THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(NULL, cpu_limits[i], memory_limits[i]);

        ///assert
        ASSERT_IS_NULL(job_object_helper, "(%" PRIu32 ", %" PRIu32 ") should return NULL", cpu_limits[i], memory_limits[i]);
    }
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_memory_exceeds_100)
{
    /* percent_physical_memory > 100 is invalid */

    uint32_t cpu_limits[] = { 50, 50, 50 };
    uint32_t memory_limits[] = { 101, 200, UINT32_MAX };

    for (int i = 0; i < MU_COUNT_ARRAY_ITEMS(cpu_limits); ++i)
    {
        LogInfo("Running test with cpu=%" PRIu32 ", memory=%" PRIu32 "...", cpu_limits[i], memory_limits[i]);

        ///act
        THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(NULL, cpu_limits[i], memory_limits[i]);

        ///assert
        ASSERT_IS_NULL(job_object_helper, "(%" PRIu32 ", %" PRIu32 ") should return NULL", cpu_limits[i], memory_limits[i]);
    }
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_update_cpu_from_nonzero_to_zero)
{
    /* Updating CPU from a non-zero value to 0 is not allowed.
       Once CPU rate control has been applied, it cannot be removed — pass 100 to effectively disable. */

    ///arrange
    char job_name[64];
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_with_limits(job_name, sizeof(job_name), INITIAL_CPU_PERCENT, INITIAL_MEMORY_PERCENT);

    ///act — try to set cpu to 0 (previously was non-zero)
    THANDLE(JOB_OBJECT_HELPER) updated_job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, 0, INITIAL_MEMORY_PERCENT);

    ///assert
    ASSERT_IS_NULL(updated_job_object_helper, "Update cpu from non-zero to 0 should fail and return NULL");

    ///cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_null_when_update_memory_from_nonzero_to_zero)
{
    /* Updating memory from a non-zero value to 0 is not allowed.
       Once memory limits have been applied, they cannot be removed — pass 100 to effectively disable. */

    ///arrange
    char job_name[64];
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_with_limits(job_name, sizeof(job_name), INITIAL_CPU_PERCENT, INITIAL_MEMORY_PERCENT);

    ///act — try to set memory to 0 (previously was non-zero)
    THANDLE(JOB_OBJECT_HELPER) updated_job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, INITIAL_CPU_PERCENT, 0);

    ///assert
    ASSERT_IS_NULL(updated_job_object_helper, "Update memory from non-zero to 0 should fail and return NULL");

    ///cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

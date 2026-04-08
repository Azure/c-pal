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

#define MEGABYTE ((size_t)1024 * 1024)
#define TEST_JOB_NAME_PREFIX "job_test_update_"
#define INITIAL_CPU_PERCENT 50
#define INITIAL_MEMORY_PERCENT 1
#define UPDATED_CPU_PERCENT 30
#define UPDATED_MEMORY_PERCENT 2

static void create_job_object_with_limits(char* job_name_out, size_t job_name_size, uint32_t cpu, uint32_t memory)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);
    (void)snprintf(job_name_out, job_name_size, TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Creating job object (cpu=%" PRIu32 ", memory=%" PRIu32 ") with job name: %s...", cpu, memory, job_name_out);

    int result = job_object_helper_set_job_limits_to_current_process(job_name_out, cpu, memory);
    ASSERT_ARE_EQUAL(int, 0, result, "Job object should be created (cpu=%" PRIu32 ", memory=%" PRIu32 ")", cpu, memory);
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

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_updates_limits_for_named_job_object)
{
    /* This test verifies that:
    *  1. A job object is created with initial limits
    *  2. The limits can be updated in-place via a second call
    *  3. The updated limits are applied to the existing job object
    */

    // Step 1: Create the singleton with initial limits (50% CPU, 1% memory)
    char job_name[64];
    create_job_object_with_limits(job_name, sizeof(job_name), INITIAL_CPU_PERCENT, INITIAL_MEMORY_PERCENT);

    // Verify initial CPU rate
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object");

    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info");
    ASSERT_ARE_EQUAL(uint32_t, INITIAL_CPU_PERCENT * 100, cpu_info.CpuRate, "Initial CPU rate should be %" PRIu32 "", INITIAL_CPU_PERCENT * 100);
    LogInfo("Initial CPU rate verified: %" PRIu32 "", cpu_info.CpuRate);

    // Verify initial memory limit
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T expected_initial_memory_in_MB = INITIAL_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info");
    ASSERT_ARE_EQUAL(size_t, expected_initial_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Initial job memory limit should match");
    LogInfo("Initial memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    (void)CloseHandle(job_object);

    // Step 2: Update to new limits (30% CPU, 2% memory)
    int update_result = job_object_helper_set_job_limits_to_current_process(job_name, UPDATED_CPU_PERCENT, UPDATED_MEMORY_PERCENT);
    ASSERT_ARE_EQUAL(int, 0, update_result, "Update should succeed");

    // Step 3: Verify updated CPU rate
    job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object after update");

    (void)memset(&cpu_info, 0, sizeof(cpu_info));
    query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info after update");
    ASSERT_ARE_EQUAL(uint32_t, UPDATED_CPU_PERCENT * 100, cpu_info.CpuRate, "Updated CPU rate should be %" PRIu32 "", UPDATED_CPU_PERCENT * 100);
    LogInfo("Updated CPU rate verified: %" PRIu32 "", cpu_info.CpuRate);

    // Verify updated memory limit
    SIZE_T expected_updated_memory_in_MB = UPDATED_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    (void)memset(&ext_info, 0, sizeof(ext_info));
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info after update");
    ASSERT_ARE_EQUAL(size_t, expected_updated_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Updated job memory limit should match");
    LogInfo("Updated memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    (void)CloseHandle(job_object);
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_updates_limits_for_unnamed_job_object)
{
    /* This test verifies update works for unnamed job objects (NULL job_name).
    *  Since OpenJobObjectA cannot be used with unnamed objects, we access the
    *  internal job_object HANDLE from the THANDLE to query limits directly.
    */

    LogInfo("Running update test with NULL job name (unnamed job object)...");

    // Step 1: Create the singleton with initial limits
    int initial_result = job_object_helper_set_job_limits_to_current_process(NULL, INITIAL_CPU_PERCENT, INITIAL_MEMORY_PERCENT);
    ASSERT_ARE_EQUAL(int, 0, initial_result);

    // Verify initial CPU rate via internal handle
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject((HANDLE)job_object_helper_get_internal_job_object_handle_for_test(), JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info");
    ASSERT_ARE_EQUAL(uint32_t, INITIAL_CPU_PERCENT * 100, cpu_info.CpuRate, "Initial CPU rate should be %" PRIu32 "", INITIAL_CPU_PERCENT * 100);
    LogInfo("Initial CPU rate verified: %" PRIu32 "", cpu_info.CpuRate);

    // Verify initial memory limit
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T expected_initial_memory_in_MB = INITIAL_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject((HANDLE)job_object_helper_get_internal_job_object_handle_for_test(), JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info");
    ASSERT_ARE_EQUAL(size_t, expected_initial_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Initial job memory limit should match");
    LogInfo("Initial memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    // Step 2: Update to new limits
    int update_result = job_object_helper_set_job_limits_to_current_process(NULL, UPDATED_CPU_PERCENT, UPDATED_MEMORY_PERCENT);
    ASSERT_ARE_EQUAL(int, 0, update_result, "Update should succeed");

    // Step 3: Verify updated CPU rate
    (void)memset(&cpu_info, 0, sizeof(cpu_info));
    query_result = QueryInformationJobObject((HANDLE)job_object_helper_get_internal_job_object_handle_for_test(), JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info after update");
    ASSERT_ARE_EQUAL(uint32_t, UPDATED_CPU_PERCENT * 100, cpu_info.CpuRate, "Updated CPU rate should be %" PRIu32 "", UPDATED_CPU_PERCENT * 100);
    LogInfo("Updated CPU rate verified: %" PRIu32 "", cpu_info.CpuRate);

    // Verify updated memory limit
    SIZE_T expected_updated_memory_in_MB = UPDATED_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    (void)memset(&ext_info, 0, sizeof(ext_info));
    query_result = QueryInformationJobObject((HANDLE)job_object_helper_get_internal_job_object_handle_for_test(), JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info after update");
    ASSERT_ARE_EQUAL(size_t, expected_updated_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Updated job memory limit should match");
    LogInfo("Updated memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

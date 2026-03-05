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

/* Complete the opaque struct definition so integration tests can access
   the internal job_object HANDLE for QueryInformationJobObject calls
   (needed for unnamed job objects where OpenJobObjectA is not available). */
struct JOB_OBJECT_HELPER_TAG {
    HANDLE job_object;
};


#define MEGABYTE ((size_t)1024 * 1024)
#define TEST_JOB_NAME_PREFIX "job_test_reconfig_"
#define INITIAL_CPU_PERCENT 50
#define INITIAL_MEMORY_PERCENT 1
#define RECONFIGURED_CPU_PERCENT 30
#define RECONFIGURED_MEMORY_PERCENT 2

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

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_reconfigures_limits_for_named_job_object)
{
    /* This test verifies that:
    *  1. A job object is created with initial limits
    *  2. The limits can be reconfigured in-place via a second call
    *  3. The reconfigured limits are applied to the existing job object
    */

    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running reconfigure test with job name: %s...", job_name);

    // Step 1: Create the singleton with initial limits (50% CPU, 1% memory)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, INITIAL_CPU_PERCENT, INITIAL_MEMORY_PERCENT);
    ASSERT_IS_NOT_NULL(initial_job_object_helper);

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

    // Step 2: Reconfigure to new limits (30% CPU, 2% memory)
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, RECONFIGURED_CPU_PERCENT, RECONFIGURED_MEMORY_PERCENT);
    ASSERT_IS_NOT_NULL(reconfigured_job_object_helper, "Reconfigure should succeed");
    ASSERT_ARE_EQUAL(void_ptr, initial_job_object_helper, reconfigured_job_object_helper, "Reconfigured call should return same singleton");

    // Step 3: Verify reconfigured CPU rate
    job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object after reconfigure");

    (void)memset(&cpu_info, 0, sizeof(cpu_info));
    query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info after reconfigure");
    ASSERT_ARE_EQUAL(uint32_t, RECONFIGURED_CPU_PERCENT * 100, cpu_info.CpuRate, "Reconfigured CPU rate should be %" PRIu32 "", RECONFIGURED_CPU_PERCENT * 100);
    LogInfo("Reconfigured CPU rate verified: %" PRIu32 "", cpu_info.CpuRate);

    // Verify reconfigured memory limit
    SIZE_T expected_reconfigured_memory_in_MB = RECONFIGURED_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    (void)memset(&ext_info, 0, sizeof(ext_info));
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info after reconfigure");
    ASSERT_ARE_EQUAL(size_t, expected_reconfigured_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Reconfigured job memory limit should match");
    LogInfo("Reconfigured memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    (void)CloseHandle(job_object);

    // Cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&reconfigured_job_object_helper, NULL);
}

TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_reconfigures_limits_for_unnamed_job_object)
{
    /* This test verifies reconfiguration works for unnamed job objects (NULL job_name).
    *  Since OpenJobObjectA cannot be used with unnamed objects, we access the
    *  internal job_object HANDLE from the THANDLE to query limits directly.
    */

    LogInfo("Running reconfigure test with NULL job name (unnamed job object)...");

    // Step 1: Create the singleton with initial limits
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = job_object_helper_set_job_limits_to_current_process(NULL, INITIAL_CPU_PERCENT, INITIAL_MEMORY_PERCENT);
    ASSERT_IS_NOT_NULL(initial_job_object_helper);

    // Verify initial CPU rate via internal handle
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject(initial_job_object_helper->job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info");
    ASSERT_ARE_EQUAL(uint32_t, INITIAL_CPU_PERCENT * 100, cpu_info.CpuRate, "Initial CPU rate should be %" PRIu32 "", INITIAL_CPU_PERCENT * 100);
    LogInfo("Initial CPU rate verified: %" PRIu32 "", cpu_info.CpuRate);

    // Verify initial memory limit
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T expected_initial_memory_in_MB = INITIAL_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject(initial_job_object_helper->job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info");
    ASSERT_ARE_EQUAL(size_t, expected_initial_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Initial job memory limit should match");
    LogInfo("Initial memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    // Step 2: Reconfigure to new limits
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process(NULL, RECONFIGURED_CPU_PERCENT, RECONFIGURED_MEMORY_PERCENT);
    ASSERT_IS_NOT_NULL(reconfigured_job_object_helper, "Reconfigure should succeed");
    ASSERT_ARE_EQUAL(void_ptr, initial_job_object_helper, reconfigured_job_object_helper, "Reconfigured call should return same singleton");

    // Step 3: Verify reconfigured CPU rate
    (void)memset(&cpu_info, 0, sizeof(cpu_info));
    query_result = QueryInformationJobObject(reconfigured_job_object_helper->job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info after reconfigure");
    ASSERT_ARE_EQUAL(uint32_t, RECONFIGURED_CPU_PERCENT * 100, cpu_info.CpuRate, "Reconfigured CPU rate should be %" PRIu32 "", RECONFIGURED_CPU_PERCENT * 100);
    LogInfo("Reconfigured CPU rate verified: %" PRIu32 "", cpu_info.CpuRate);

    // Verify reconfigured memory limit
    SIZE_T expected_reconfigured_memory_in_MB = RECONFIGURED_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    (void)memset(&ext_info, 0, sizeof(ext_info));
    query_result = QueryInformationJobObject(reconfigured_job_object_helper->job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info after reconfigure");
    ASSERT_ARE_EQUAL(size_t, expected_reconfigured_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Reconfigured job memory limit should match");
    LogInfo("Reconfigured memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    // Cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&reconfigured_job_object_helper, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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


#define MEGABYTE ((size_t)1024 * 1024)
#define TEST_JOB_NAME_PREFIX "job_test_partial_"
#define TEST_CPU_PERCENT 50
#define TEST_MEMORY_PERCENT 1
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

TEST_FUNCTION(job_object_helper_set_job_limits_with_zero_cpu_and_nonzero_memory_succeeds)
{
    /* This test verifies that calling job_object_helper_set_job_limits_to_current_process
       with cpu=0 (skip CPU setup) and memory=1%
       creates a job object with:
       - CPU rate control not applied (ControlFlags == 0)
       - Memory limit set to 1% of total physical memory */

    ///arrange
    char job_name[64];

    ///act
    create_job_object_with_limits(job_name, sizeof(job_name), 0, TEST_MEMORY_PERCENT);

    ///assert
    /* Verify the job object was created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object");

    /* Verify CPU rate control is not applied */
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info");
    ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0, cpu_info.ControlFlags, "CPU rate control should not be applied (ControlFlags == 0)");
    LogInfo("CPU rate control verified as not applied (ControlFlags=%" PRIu32 ")", cpu_info.ControlFlags);

    /* Verify memory limit is set */
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T expected_memory_in_MB = TEST_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info");
    ASSERT_ARE_EQUAL(size_t, expected_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Job memory limit should be %" PRIu32 "%% of total physical memory", (uint32_t)TEST_MEMORY_PERCENT);
    ASSERT_ARE_EQUAL(size_t, expected_memory_in_MB, ext_info.ProcessMemoryLimit / MEGABYTE, "Process memory limit should be %" PRIu32 "%% of total physical memory", (uint32_t)TEST_MEMORY_PERCENT);
    LogInfo("Memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    /* Verify process is assigned */
    BOOL is_in_job = FALSE;
    ASSERT_IS_TRUE(IsProcessInJob(GetCurrentProcess(), job_object, &is_in_job));
    ASSERT_IS_TRUE(is_in_job, "Current process should be assigned to the job object");

    (void)CloseHandle(job_object);
}

TEST_FUNCTION(job_object_helper_set_job_limits_with_nonzero_cpu_and_zero_memory_succeeds)
{
    /* This test verifies that calling job_object_helper_set_job_limits_to_current_process
       with cpu=50% and memory=0 (skip memory setup)
       creates a job object with:
       - CPU rate control enabled with hard cap at 50%
       - Memory limit not applied (LimitFlags == 0) */

    ///arrange
    char job_name[64];

    ///act
    create_job_object_with_limits(job_name, sizeof(job_name), TEST_CPU_PERCENT, 0);

    ///assert
    /* Verify the job object was created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object");

    /* Verify CPU rate control is enabled with correct rate */
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info");
    ASSERT_ARE_EQUAL(uint32_t, (uint32_t)(JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP), cpu_info.ControlFlags, "CPU rate control should be enabled with hard cap");
    ASSERT_ARE_EQUAL(uint32_t, TEST_CPU_PERCENT * 100, cpu_info.CpuRate, "CPU rate should be %" PRIu32 "", TEST_CPU_PERCENT * 100);
    LogInfo("CPU rate control verified: ControlFlags=%" PRIu32 ", CpuRate=%" PRIu32 "", cpu_info.ControlFlags, cpu_info.CpuRate);

    /* Verify memory limit is not applied (LimitFlags should not have memory flags) */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info");
    ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0, ext_info.BasicLimitInformation.LimitFlags & (JOB_OBJECT_LIMIT_JOB_MEMORY | JOB_OBJECT_LIMIT_PROCESS_MEMORY), "Memory limit flags should not be set");
    LogInfo("Memory limits verified as not applied (LimitFlags=0x%08lx)", (unsigned long)ext_info.BasicLimitInformation.LimitFlags);

    /* Verify process is assigned */
    BOOL is_in_job = FALSE;
    ASSERT_IS_TRUE(IsProcessInJob(GetCurrentProcess(), job_object, &is_in_job));
    ASSERT_IS_TRUE(is_in_job, "Current process should be assigned to the job object");

    (void)CloseHandle(job_object);
}

TEST_FUNCTION(job_object_helper_update_both_limits_succeeds)
{
    /* This test verifies that an existing job object created with both CPU and memory limits
       can be updated to new limits:
       (50% CPU, 1% memory) -> (30% CPU, 2% memory) */

    ///arrange
    char job_name[64];
    create_job_object_with_limits(job_name, sizeof(job_name), INITIAL_CPU_PERCENT, INITIAL_MEMORY_PERCENT);

    ///act - update to new values
    int result = job_object_helper_set_job_limits_to_current_process(job_name, UPDATED_CPU_PERCENT, UPDATED_MEMORY_PERCENT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result, "Update should succeed");

    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object after update");

    /* Verify CPU rate control is updated */
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info after update");
    ASSERT_ARE_EQUAL(uint32_t, (uint32_t)(JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP), cpu_info.ControlFlags, "CPU rate control should be enabled with hard cap after update");
    ASSERT_ARE_EQUAL(uint32_t, UPDATED_CPU_PERCENT * 100, cpu_info.CpuRate, "Updated CPU rate should be %" PRIu32 "", UPDATED_CPU_PERCENT * 100);
    LogInfo("Updated CPU rate verified: ControlFlags=%" PRIu32 ", CpuRate=%" PRIu32 "", cpu_info.ControlFlags, cpu_info.CpuRate);

    /* Verify memory limit is updated */
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T expected_memory_in_MB = UPDATED_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info after update");
    ASSERT_ARE_EQUAL(size_t, expected_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Updated job memory limit should be %" PRIu32 "%% of total physical memory", (uint32_t)UPDATED_MEMORY_PERCENT);
    ASSERT_ARE_EQUAL(size_t, expected_memory_in_MB, ext_info.ProcessMemoryLimit / MEGABYTE, "Updated process memory limit should be %" PRIu32 "%% of total physical memory", (uint32_t)UPDATED_MEMORY_PERCENT);
    LogInfo("Updated memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    (void)CloseHandle(job_object);
}

TEST_FUNCTION(job_object_helper_update_memory_only_when_cpu_was_zero_succeeds)
{
    /* This test verifies that when the initial creation had cpu=0 (CPU not applied),
       an update with cpu=0 (still not applied) and updated memory succeeds.
       (0% CPU, 1% memory) -> (0% CPU, 2% memory) */

    ///arrange
    char job_name[64];
    create_job_object_with_limits(job_name, sizeof(job_name), 0, INITIAL_MEMORY_PERCENT);

    ///act - update: cpu still 0, memory updated
    int result = job_object_helper_set_job_limits_to_current_process(job_name, 0, UPDATED_MEMORY_PERCENT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result, "Update should succeed");

    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object after update");

    /* Verify CPU rate control is still not applied */
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info");
    ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0, cpu_info.ControlFlags, "CPU rate control should still not be applied");
    LogInfo("CPU rate control verified as still not applied (ControlFlags=%" PRIu32 ")", cpu_info.ControlFlags);

    /* Verify memory limit is updated */
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T expected_memory_in_MB = UPDATED_MEMORY_PERCENT * mem_status.ullTotalPhys / 100 / MEGABYTE;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info after update");
    ASSERT_ARE_EQUAL(size_t, expected_memory_in_MB, ext_info.JobMemoryLimit / MEGABYTE, "Updated job memory limit should be %" PRIu32 "%% of total physical memory", (uint32_t)UPDATED_MEMORY_PERCENT);
    LogInfo("Updated memory limit verified: %zu MB", ext_info.JobMemoryLimit / MEGABYTE);

    (void)CloseHandle(job_object);
}

TEST_FUNCTION(job_object_helper_update_cpu_only_when_memory_was_zero_succeeds)
{
    /* This test verifies that when the initial creation had memory=0 (memory not applied),
       an update with memory=0 (still not applied) and updated CPU succeeds.
       (50% CPU, 0% memory) -> (30% CPU, 0% memory) */

    ///arrange
    char job_name[64];
    create_job_object_with_limits(job_name, sizeof(job_name), INITIAL_CPU_PERCENT, 0);

    ///act - update: cpu updated, memory still 0
    int result = job_object_helper_set_job_limits_to_current_process(job_name, UPDATED_CPU_PERCENT, 0);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result, "Update should succeed");

    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object after update");

    /* Verify CPU rate control is updated */
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info = { 0 };
    DWORD return_length = 0;
    BOOL query_result = QueryInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query CPU rate info after update");
    ASSERT_ARE_EQUAL(uint32_t, (uint32_t)(JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP), cpu_info.ControlFlags, "CPU rate control should be enabled with hard cap");
    ASSERT_ARE_EQUAL(uint32_t, UPDATED_CPU_PERCENT * 100, cpu_info.CpuRate, "Updated CPU rate should be %" PRIu32 "", UPDATED_CPU_PERCENT * 100);
    LogInfo("Updated CPU rate verified: ControlFlags=%" PRIu32 ", CpuRate=%" PRIu32 "", cpu_info.ControlFlags, cpu_info.CpuRate);

    /* Verify memory limit is still not applied */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_info = { 0 };
    query_result = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &ext_info, sizeof(ext_info), &return_length);
    ASSERT_IS_TRUE(query_result, "Failed to query extended limit info after update");
    ASSERT_ARE_EQUAL(uint32_t, (uint32_t)0, ext_info.BasicLimitInformation.LimitFlags & (JOB_OBJECT_LIMIT_JOB_MEMORY | JOB_OBJECT_LIMIT_PROCESS_MEMORY), "Memory limit flags should still not be set");
    LogInfo("Memory limits verified as still not applied (LimitFlags=0x%08lx)", (unsigned long)ext_info.BasicLimitInformation.LimitFlags);

    (void)CloseHandle(job_object);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

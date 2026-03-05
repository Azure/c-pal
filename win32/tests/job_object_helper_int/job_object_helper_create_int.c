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

TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process)
{
    /* Check that calling job_object_helper_set_job_limits_to_current_process
    *  1. creates the job object
    *  2. assigns the current process to the job object
    *  3. sets the job limits
    */

    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with job name: %s...", job_name);

    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* Check that the job object was created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object");

    /* Query the Job Object to check if it has 1 process associated with it */
    JOBOBJECT_BASIC_PROCESS_ID_LIST process_id_list;
    ZeroMemory(&process_id_list, sizeof(process_id_list));
    DWORD return_length = 0;
    BOOL result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_ARE_EQUAL(ULONG, process_id_list.NumberOfAssignedProcesses, 1, "Job object should have 1 process associated with it");

    /* Check that the current process is assigned to the job object */
    BOOL ret = FALSE;
    ASSERT_IS_TRUE(IsProcessInJob(GetCurrentProcess(), job_object, &ret));
    ASSERT_IS_TRUE(ret, "Current process should be assigned to the job object");

    /* get the 1% of the total physical memory and check if that is equal to job objects memory limit*/
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T total_memory = mem_status.ullTotalPhys;
    SIZE_T one_percent_of_total_memory_in_MB = total_memory / 100 / MEGABYTE;
    LogInfo("Total Memory: %zu", total_memory);
    LogInfo("1%% of Total Memory: %zu", one_percent_of_total_memory_in_MB);

    /* Check that the job limits were set */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
    ZeroMemory(&job_info, sizeof(job_info));
    return_length = 0;
    result_query = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    LogInfo("Job memory limit: %zu", job_info.JobMemoryLimit / MEGABYTE);
    ASSERT_ARE_EQUAL(size_t, job_info.JobMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have job memory limit set to 1%% of total physical memory");
    ASSERT_ARE_EQUAL(size_t, job_info.ProcessMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have process memory limit set to 1%% of total physical memory");

    /* Performing the same action from the same process shall not change anything */
    THANDLE(JOB_OBJECT_HELPER) job_object_helper_duplicate = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(job_object_helper_duplicate);
    ASSERT_ARE_EQUAL(void_ptr, job_object_helper, job_object_helper_duplicate, "Duplicate call with same params should return same singleton");

    job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object);

    /* Even though API was called twice, however since it was called from the same process, it shall show only one process is associated with it.*/
    result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_ARE_EQUAL(int, process_id_list.NumberOfAssignedProcesses, 1, "Job object should have 1 process associated with it");

    ZeroMemory(&job_info, sizeof(job_info));
    return_length = 0;
    result_query = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    LogInfo("Job memory limit: %zu", job_info.JobMemoryLimit / MEGABYTE);
    ASSERT_ARE_EQUAL(size_t, job_info.JobMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have job memory limit set to 1%% of total physical memory");
    ASSERT_ARE_EQUAL(size_t, job_info.ProcessMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have process memory limit set to 1%% of total physical memory");

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper_duplicate, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

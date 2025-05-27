// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/job_object_helper.h"
#include "c_pal/timer.h"
#include "c_pal/uuid.h"


#define MEGABYTE ((size_t)1024 * 1024)
#define GIGABYTE (MEGABYTE * 1024)
#define MAX_MEMORY_POSSIBLE (GIGABYTE * 256)
#define TEST_BUFFER_SIZE (MEGABYTE * 512)
#define MAX_BUFFERS_BEFORE_FAILURE (MAX_MEMORY_POSSIBLE / TEST_BUFFER_SIZE)
#define NUM_TEST_PROCESSES 3
#define NUM_ALLOCATE_MEMORY_BLOCKS 12
#define NUM_RETRY 3

static const size_t max_buffers_before_failure = MAX_BUFFERS_BEFORE_FAILURE;

static size_t get_malloc_limit_before_failure(void)
{
    /* malloc until failure and return the count of (512MB) buffers we were able to malloc. */
    void** buffers = malloc(sizeof(void*) * max_buffers_before_failure);
    ASSERT_IS_NOT_NULL(buffers);

    size_t max_buffers = 0;
    for (size_t i = 0; i < max_buffers_before_failure; i++)
    {
        buffers[i] = malloc(TEST_BUFFER_SIZE);
        if (buffers[i] == NULL)
        {
            max_buffers = i;
            break;
        }
    }
    ASSERT_ARE_NOT_EQUAL(size_t, 0, max_buffers);
    for (size_t i = 0; i < max_buffers; i++)
    {
        free(buffers[i]);
    }
    free(buffers);

    return max_buffers;
}


/* We need something to keep the CPU busy. */
/* If you list the numbers from 0 to 2^28-1 in binary, you'll find that 3,758,096,384 bits are set. */
/* Lets verify this. It takes about 5 seconds on my desktop PC. */
static size_t limit = 1 << 28;
static size_t expected_bits = 3758096384;

static size_t get_elapsed_milliseconds_for_cpu_bound_task(void)
{
    double start = timer_global_get_elapsed_ms();

    size_t total_bits = 0;

    for (size_t i = 0; i < limit; i++)
    {
        size_t this = i;
        while (this != 0)
        {
            total_bits += 1;
            this = this & (this - 1);
        }
    }

    double end = timer_global_get_elapsed_ms();
    ASSERT_ARE_EQUAL(size_t, expected_bits, total_bits);
    return (size_t)(end - start);
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
}


TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process)
{
    /* Check that calling job_object_helper_set_job_limits_to_current_process
    *  1. creates the job object
    *  2. assigns the current process to the job object
    *  3. sets the job limits
    */

    UUID_T job_name_uuid;
    (void)uuid_produce(job_name_uuid);

    char job_name[64];
    snprintf(job_name, sizeof(job_name), "job_test_ebs_%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Runnint test with job name: %s...", job_name);

    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(result);

    /* Check that the job object was created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object");

    /* Query the Job Object to check if it has 1 process associated with it */
    JOBOBJECT_BASIC_PROCESS_ID_LIST process_id_list;
    DWORD return_length = 0;
    BOOL result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_ARE_EQUAL(int, return_length, sizeof(process_id_list), "Failed to query job object");
    ASSERT_ARE_EQUAL(int, process_id_list.NumberOfAssignedProcesses, 1, "Job object should have 1 process associated with it");

    /* Check that the current process is assigned to the job object */
    BOOL ret = TRUE;
    ASSERT_IS_TRUE(IsProcessInJob(GetCurrentProcess(), job_object, &ret));
    ASSERT_IS_TRUE(ret, "Current process should be assigned to the job object");

    /* get the 1% of the total physical memory and check if that is equal to job objects memory limit*/
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    SIZE_T totalMemory = memStatus.ullTotalPhys;
    SIZE_T onePercentOfTotalMemorynMB = totalMemory / 100 / MEGABYTE;
    LogInfo("Total Memory: %zu", totalMemory);
    LogInfo("1%% of Total Memory: %zu", onePercentOfTotalMemorynMB);

    /* Check that the job limits were set */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
    result_query = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_ARE_EQUAL(int, return_length, sizeof(job_info), "Failed to query job object");
    LogInfo("Job memory limit: %zu", job_info.JobMemoryLimit / MEGABYTE);
    ASSERT_ARE_EQUAL(size_t, job_info.JobMemoryLimit / MEGABYTE, onePercentOfTotalMemorynMB, "Job object should have memory limit set to 1%% of total physical memory");
    ASSERT_ARE_EQUAL(size_t, job_info.ProcessMemoryLimit / MEGABYTE, onePercentOfTotalMemorynMB, "Job object should have process memory limit set to 1%% of total physical memory");

    /* Performing the same action from the same process shall not change anything */
    THANDLE(JOB_OBJECT_HELPER) result_1 = job_object_helper_set_job_limits_to_current_process(job_name, 50, 50);
    ASSERT_IS_NOT_NULL(result_1);

    job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object);

    // Even though API was called twice, however since it was called from the same process, it shall show only one process is associated with it.
    result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_ARE_EQUAL(int, process_id_list.NumberOfAssignedProcesses, 1, "Job object should have 1 process associated with it");

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&result, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&result_1, NULL);
}


TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process_from_multiple_processes)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), "job_test_ebs_%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Runnint test with job name: %s...", job_name);

    /* Use GetModuleFileNameA to get the path of the current executable */
    char path[MAX_PATH];
    char directory[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, path, sizeof(path));
    ASSERT_ARE_NOT_EQUAL(int, length, 0, "GetModuleFileNameA failed");
    /* Get the full path to the executable// Copy the full path to directory and remove the executable name */
    strcpy(directory, path);
    for (int i = length - 1; i >= 0; --i)
    {
        if (directory[i] == '\\')
        {
            directory[i] = '\0';
            break;
        }
    }

    // Create the full path to the job_object_helper_tester executable
    char fullPath[MAX_PATH];
    (void)snprintf(fullPath, sizeof(fullPath), "%s\\..\\job_object_helper_tester\\job_object_helper_tester.exe", directory);

    // start 3 new process using CreateNewProcess, those processes shall
    // call job_object_helper_set_job_limits_to_current_process
    STARTUPINFOA si[NUM_TEST_PROCESSES];
    PROCESS_INFORMATION pi[NUM_TEST_PROCESSES];

    for (int i = 0; i < NUM_TEST_PROCESSES; ++i)
    {
        LogInfo("Starting process %d", i);
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        char cmdLine[512];
        (void)snprintf(cmdLine, sizeof(cmdLine), "\"%s\" %s %d %d", fullPath, job_name, 50, 50);

        /* Start the process; this process calls job_object_helper_set_job_limits_to_current_process */
        (void)CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si[i], &pi[i]);

        /* sleep for 2 seconds to allow the child process to run */
        Sleep(2000);
        /* try for 3 times before declaring a failure, as it may take time for a new process to get scheduled and create the job object  */
        HANDLE job_object = NULL;
        for (int j = 0; j < NUM_RETRY; ++j)
        {
            job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
            if (job_object != NULL)
            {
                break;
            }
            LogInfo("OpenJobObjectA failed, will retry %d", j + 1);
            /* sleep for 2 seconds */
            Sleep(2000);
        }

        ASSERT_IS_NOT_NULL(job_object);
        ASSERT_IS_TRUE(job_object != NULL, "Failed to open job object");

        /* Query the Job Object to check if it has all processes associated with it */
        DWORD bufferSize = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * 16;
        JOBOBJECT_BASIC_PROCESS_ID_LIST* pidList = NULL;
        while (1)
        {
            pidList = (JOBOBJECT_BASIC_PROCESS_ID_LIST*)malloc(bufferSize);

            if (!QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, pidList, bufferSize, NULL))
            {
                DWORD err = GetLastError();
                if (err == ERROR_MORE_DATA) {
                    // Increase buffer size and try again
                    bufferSize *= 2;
                    free(pidList);
                    continue;
                }
            }
            break;
        }

        ASSERT_IS_NOT_NULL(pidList);
        ASSERT_IS_TRUE(pidList->NumberOfAssignedProcesses == (unsigned long)(i + 1), "Job object should have 1 process associated with it");
        free(pidList);
    }

    for (int i = 0; i < NUM_TEST_PROCESSES; ++i)
    {
        // Terminate the process; Terminate is forceful
        (void)TerminateProcess(pi[i].hProcess, 0);

        // Just to make sure, process has terminated
        (void)WaitForSingleObject(pi[i].hProcess, INFINITE);

        // Close process and thread handles.
        (void)CloseHandle(pi[i].hProcess);
        (void)CloseHandle(pi[i].hThread);
    }
}

TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process_check_memory_limits)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), "job_test_ebs_%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));

    LogInfo("Running test with Job name: %s...", job_name);

    /* Get the total available Memory */
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    (void)GlobalMemoryStatusEx(&memStatus);

    SIZE_T totalMemory = memStatus.ullTotalPhys;
    /* Calculate 1% of the memory */
    SIZE_T onePercentOfTotalMemory = totalMemory / 100;
    LogInfo("Total Memory: %zu", totalMemory);
    LogInfo("1%% of Total Memory: %zu", onePercentOfTotalMemory);

    /* Set the process's memory limit to 1% */
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* allocations till 1% should pass */
    char* buffer[NUM_ALLOCATE_MEMORY_BLOCKS];
    long long int memory_size = onePercentOfTotalMemory / 10;
    LogInfo("Allocating %lld in every allocation", memory_size);
    int allocationFailed = 0;
    for (int i = 0; i < NUM_ALLOCATE_MEMORY_BLOCKS; ++i)
    {
        LogInfo("Allocating %d", i);
        buffer[i] = (char*)malloc(memory_size);
        if (buffer[i] == NULL) {
            allocationFailed++;
        }
        Sleep(10);
    }
    /* After reaching the limit, allocations are expected to be failed */
    ASSERT_ARE_NOT_EQUAL(int, 0, allocationFailed);
    /* With the set limit, not all allocations should have failed*/
    ASSERT_ARE_NOT_EQUAL(int, NUM_ALLOCATE_MEMORY_BLOCKS, allocationFailed);

    /* Free the allocated memory */
    for (int i = 0; i < NUM_ALLOCATE_MEMORY_BLOCKS; ++i)
    {
        if (buffer[i])
            free(buffer[i]);
    }

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

TEST_FUNCTION(test_job_object_helper_limit_memory)
{
    LogInfo("Each block is %zu MB", TEST_BUFFER_SIZE / MEGABYTE);

    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* Constrain to 50% of physical memory and measure. */
    int result = job_object_helper_limit_memory(job_object_helper, 50);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t blocks_at_50_percent = get_malloc_limit_before_failure();
    ASSERT_ARE_NOT_EQUAL(size_t, 0, blocks_at_50_percent);
    LogInfo("Memory limits:  50%% of physical memory = %zu blocks = %zu MB", blocks_at_50_percent, blocks_at_50_percent * TEST_BUFFER_SIZE / MEGABYTE);

    /* To make math easier to explain, assume that 100% is 2 x 50% */
    size_t blocks_at_100_percent = blocks_at_50_percent * 2;

    /* Constrain to 10% of physical memory and re-measure. */
    result = job_object_helper_limit_memory(job_object_helper, 10);
    ASSERT_ARE_EQUAL(int, 0, result);
    size_t blocks_at_10_percent = get_malloc_limit_before_failure();
    ASSERT_ARE_NOT_EQUAL(size_t, 0, blocks_at_10_percent);
    LogInfo("Memory limits:  10%% of physical memory = %zu blocks = %zu MB", blocks_at_10_percent, blocks_at_10_percent * TEST_BUFFER_SIZE / MEGABYTE);

    size_t actual_10_percent_percentage = blocks_at_10_percent * 100 / blocks_at_100_percent;
    LogInfo("10%% limit measured at %zu%% of physical", actual_10_percent_percentage);
    ASSERT_IS_TRUE(actual_10_percent_percentage >= 7 && actual_10_percent_percentage <= 13, "10% memory limit not between 7-13% of physical memory");

    result = job_object_helper_limit_memory(job_object_helper, 100);
    ASSERT_ARE_EQUAL(int, 0, result);

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

DISABLED_TEST_FUNCTION(test_job_object_helper_limit_cpu)
{
    size_t time_with_no_limit = get_elapsed_milliseconds_for_cpu_bound_task();
    LogInfo("Unconstrained CPU time = %zu milliseconds", time_with_no_limit);

    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* Our CPU limiting works by giving up the CPU earlier than we normally would. */
    /* Since we're not competing for CPU, giving it up early doesn't slow us down much */
    /* because we give it up and then get it again fairly quickly. The only penalty is */
    /* the context switch. Because of this, we don't see a _measurable_ benefit until */
    /* we set to 2%, but this is mostly because of context switch overhead. */

    /* Constrain to 2% of CPU and re-measure. */
    int result = job_object_helper_limit_cpu(job_object_helper, 2);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t time_with_at_2_percent_cpu = get_elapsed_milliseconds_for_cpu_bound_task();
    LogInfo("Using 2%% CPU = %zu milliseconds", time_with_at_2_percent_cpu);

    ASSERT_IS_TRUE(time_with_at_2_percent_cpu > (time_with_no_limit * 11 / 10), "Using 2% CPU value should slow us down by 10% at least");

    /* Constrain to 1% of CPU and re-measure. */
    result = job_object_helper_limit_cpu(job_object_helper, 1);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t time_with_at_1_percent_cpu = get_elapsed_milliseconds_for_cpu_bound_task();
    LogInfo("Using 1%% CPU = %zu milliseconds", time_with_at_1_percent_cpu);

    ASSERT_IS_TRUE(time_with_at_1_percent_cpu > (time_with_no_limit * 12 / 10), "Using 4% CPU value should slow us down by 20% at least");

    /* Go back to 100% */
    result = job_object_helper_limit_cpu(job_object_helper, 100);
    ASSERT_ARE_EQUAL(int, 0, result);

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

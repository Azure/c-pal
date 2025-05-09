// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/timer.h"
#include "c_pal/job_object_helper.h"

#define MEGABYTE ((size_t)1024 * 1024)
#define GIGABYTE (MEGABYTE * 1024)
#define MAX_MEMORY_POSSIBLE (GIGABYTE * 256)
#define TEST_BUFFER_SIZE (MEGABYTE * 512)
#define MAX_BUFFERS_BEFORE_FAILURE (MAX_MEMORY_POSSIBLE / TEST_BUFFER_SIZE)

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

    ASSERT_IS_TRUE(time_with_at_2_percent_cpu  > (time_with_no_limit * 11 / 10), "Using 2% CPU value should slow us down by 10% at least");

    /* Constrain to 1% of CPU and re-measure. */
    result = job_object_helper_limit_cpu(job_object_helper, 1);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t time_with_at_1_percent_cpu = get_elapsed_milliseconds_for_cpu_bound_task();
    LogInfo("Using 1%% CPU = %zu milliseconds", time_with_at_1_percent_cpu);

    ASSERT_IS_TRUE(time_with_at_1_percent_cpu  > (time_with_no_limit * 12 / 10), "Using 4% CPU value should slow us down by 20% at least");

    /* Go back to 100% */
    result = job_object_helper_limit_cpu(job_object_helper, 100);
    ASSERT_ARE_EQUAL(int, 0, result);

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process)
{
    /* Check that calling job_object_helper_set_job_limits_to_current_process
    *  1. creates the job object
	*  2. assigns the current process to the job object
	*  3. sets the job limits
    */

    int result = job_object_helper_set_job_limits_to_current_process("job_1ebs", 50, 50);
    ASSERT_ARE_EQUAL(int, 0, result);

    /* Check that the job object was created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, "job_1ebs");
    ASSERT_IS_NOT_NULL(job_object);
    ASSERT_IS_TRUE(job_object != NULL, "Failed to open job object");
    ASSERT_IS_TRUE(job_object != INVALID_HANDLE_VALUE, "Failed to open job object");

    /* Query the Job Object to check if it has 1 process associated with it */
    JOBOBJECT_BASIC_PROCESS_ID_LIST process_id_list;
    DWORD return_length = 0;
    BOOL result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_IS_TRUE(return_length == sizeof(process_id_list), "Failed to query job object");
    ASSERT_IS_TRUE(process_id_list.NumberOfAssignedProcesses == 1, "Job object should have 1 process associated with it");

    /* Check that the current process is assigned to the job object */
    HANDLE current_process = GetCurrentProcess();
    ASSERT_IS_NOT_NULL(current_process);
    ASSERT_IS_TRUE(process_id_list.ProcessIdList[0] == GetProcessId(current_process), "Current process should be assigned to the job object");

    /* Check that the job limits were set */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
    result_query = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_IS_TRUE(return_length == sizeof(job_info), "Failed to query job object");
    ASSERT_IS_TRUE(job_info.JobMemoryLimit > 0, "Job object should have memory limit set");
    ASSERT_IS_TRUE(job_info.ProcessMemoryLimit > 0, "Job object should have process memory limit set");

    /* Performing the same action from the same process won't change anything*/
    result = job_object_helper_set_job_limits_to_current_process("job_1ebs", 50, 50);
    ASSERT_ARE_EQUAL(int, 0, result);

    job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, "job_1ebs");
    ASSERT_IS_NOT_NULL(job_object);
    ASSERT_IS_TRUE(job_object != NULL, "Failed to open job object");
    ASSERT_IS_TRUE(job_object != INVALID_HANDLE_VALUE, "Failed to open job object");

    result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_IS_TRUE(return_length == sizeof(process_id_list), "Failed to query job object");
    ASSERT_IS_TRUE(process_id_list.NumberOfAssignedProcesses == 1, "Job object should have 1 process associated with it");

    result_query = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_IS_TRUE(return_length == sizeof(job_info), "Failed to query job object");
    ASSERT_IS_TRUE(job_info.JobMemoryLimit > 0, "Job object should have memory limit set");
    ASSERT_IS_TRUE(job_info.ProcessMemoryLimit > 0, "Job object should have process memory limit set");
}


TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process_check_memory_limits)
{
    LogInfo("Each block is %zu MB", TEST_BUFFER_SIZE / MEGABYTE);

    /* Constrain to 50% of physical memory and measure. */
     int result = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);
    ASSERT_ARE_EQUAL(int, 0, result);
    size_t blocks_at_50_percent = get_malloc_limit_before_failure();
    ASSERT_ARE_NOT_EQUAL(size_t, 0, blocks_at_50_percent);
    LogInfo("Memory limits:  50%% of physical memory = %zu blocks = %zu MB", blocks_at_50_percent, blocks_at_50_percent * TEST_BUFFER_SIZE / MEGABYTE);
    /* To make math easier to explain, assume that 100% is 2 x 50% */
    size_t blocks_at_100_percent = blocks_at_50_percent * 2;

    /* Constrain to 10% of physical memory and re-measure. */
    result = job_object_helper_set_job_limits_to_current_process("job_name", 10, 10);
    ASSERT_ARE_EQUAL(int, 0, result);
    size_t blocks_at_10_percent = get_malloc_limit_before_failure();
    ASSERT_ARE_NOT_EQUAL(size_t, 0, blocks_at_10_percent);
    LogInfo("Memory limits:  10%% of physical memory = %zu blocks = %zu MB", blocks_at_10_percent, blocks_at_10_percent * TEST_BUFFER_SIZE / MEGABYTE);
    size_t actual_10_percent_percentage = blocks_at_10_percent * 100 / blocks_at_100_percent;
    LogInfo("10%% limit measured at %zu%% of physical", actual_10_percent_percentage);
    ASSERT_IS_TRUE(actual_10_percent_percentage >= 7 && actual_10_percent_percentage <= 13, "10% memory limit not between 7-13% of physical memory");

    result = job_object_helper_set_job_limits_to_current_process("job_name", 100, 100);
    ASSERT_ARE_EQUAL(int, 0, result);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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
#define NUM_ALLOCATE_MEMORY_BLOCKS 12
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

TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process_check_memory_limits)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with Job name: %s...", job_name);

    /* Get the total available Memory */
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));

    SIZE_T total_memory = mem_status.ullTotalPhys;
    /* Calculate 1% of the memory */
    SIZE_T one_percent_of_total_memory = total_memory / 100;
    LogInfo("Total Memory: %zu", total_memory);
    LogInfo("1%% of Total Memory: %zu", one_percent_of_total_memory);

    /* Set the process's memory limit to 1% */
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* allocations till 1% should pass */
    char* buffer[NUM_ALLOCATE_MEMORY_BLOCKS];
    uint64_t memory_size = one_percent_of_total_memory / 10;
    LogInfo("Allocating %" PRIu64 "in every allocation", memory_size);
    int allocation_failed = 0;
    for (int i = 0; i < NUM_ALLOCATE_MEMORY_BLOCKS; ++i)
    {
        buffer[i] = malloc(memory_size);
        if (buffer[i] == NULL) {
            allocation_failed++;
        }
    }
    /* After reaching the limit, allocations are expected to be failed */
    ASSERT_ARE_NOT_EQUAL(int, 0, allocation_failed);
    /* With the set limit, not all allocations should have failed*/
    ASSERT_ARE_NOT_EQUAL(int, NUM_ALLOCATE_MEMORY_BLOCKS, allocation_failed);

    /* Free the allocated memory */
    for (int i = 0; i < NUM_ALLOCATE_MEMORY_BLOCKS; ++i)
    {
        if (buffer[i])
            free(buffer[i]);
    }

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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

const size_t max_buffers_before_failure = MAX_BUFFERS_BEFORE_FAILURE;

static size_t get_malloc_limit_before_failure()
{
    /* malloc until failure and return the count of (512MB) buffers we were able to malloc. */
    void** buffers = malloc(sizeof(void*) * max_buffers_before_failure);
    ASSERT_IS_NOT_NULL(buffers);

    size_t max_buffers = 0;
    for (size_t i = 0; i < max_buffers_before_failure; i++)
    {
        buffers[i] = malloc(TEST_BUFFER_SIZE);
        if (buffers[i] == 0)
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

static size_t get_elapsed_milliseconds_for_cpu_bound_task()
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

#if 0
    /* First, measure without any limits. This tells us virtual memory size. */
    size_t blocks_with_no_limit = get_malloc_limit_before_failure();
    ASSERT_ARE_NOT_EQUAL(size_t, 0, blocks_with_no_limit);
    LogInfo("Memory limits: No artificial limit     = %zu blocks = %zu MB", blocks_with_no_limit, blocks_with_no_limit * TEST_BUFFER_SIZE / MEGABYTE);
#endif

    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* Constrain to 100% of physical memory and re-measure. */
    int result = job_object_helper_limit_memory(job_object_helper, 100);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t blocks_at_100_percent = get_malloc_limit_before_failure();
    ASSERT_ARE_NOT_EQUAL(size_t, 0, blocks_at_100_percent);

    LogInfo("Memory limits: 100%% of physical memory = %zu blocks = %zu MB", blocks_at_100_percent, blocks_at_100_percent * TEST_BUFFER_SIZE / MEGABYTE);
#if 0
    size_t actual_100_percent_percentage = blocks_at_100_percent * 100 / blocks_with_no_limit;
    LogInfo("100%% limit measured at %zu%% of virtual", actual_100_percent_percentage);
#endif

    /* Constrain to 100% of physical memory and re-measure. */
    result = job_object_helper_limit_memory(job_object_helper, 50);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t blocks_at_50_percent = get_malloc_limit_before_failure();
    ASSERT_ARE_NOT_EQUAL(size_t, 0, blocks_at_50_percent);
    LogInfo("Memory limits:  50%% of physical memory = %zu blocks = %zu MB", blocks_at_50_percent, blocks_at_50_percent * TEST_BUFFER_SIZE / MEGABYTE);

    size_t actual_50_percent_percentage = blocks_at_50_percent * 100 / blocks_at_100_percent;
    LogInfo("50%% limit measured at %zu%% of physical", actual_50_percent_percentage);
    ASSERT_IS_TRUE(actual_50_percent_percentage >= 45 && actual_50_percent_percentage <= 55, "50% memory limit not between 45-55% of physical memory");

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

TEST_FUNCTION(test_job_object_helper_limit_cpu)
{
    size_t time_with_no_limit = get_elapsed_milliseconds_for_cpu_bound_task();
    LogInfo("Unconstrained CPU time = %zu milliseconds", time_with_no_limit);

    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* Our CPU limiting works by giving up the CPU earlier than we normally would. */
    /* Since we're not competing for CPU, giving it up early doesn't slow us down much */
    /* because we give it up and then get it again fairly quickly. The only penalty is */
    /* the context switch. Because of this, we don't see a _measurable_ benefit until */
    /* we set to 2%, at witch point it looks like we spend more time switching contexts */
    /* than actually doing work */

    /* Constrain to 2% of CPU and re-measure. */
    int result = job_object_helper_limit_cpu(job_object_helper, 2);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t time_with_at_2_percent_cpu = get_elapsed_milliseconds_for_cpu_bound_task();
    LogInfo("Using 2%% CPU = %zu milliseconds", time_with_at_2_percent_cpu);

    ASSERT_IS_TRUE(time_with_at_2_percent_cpu  > (time_with_no_limit * 15 / 10), "Code should take almost 2X time when using 2% CPU");

    /* Constrain to 1% of CPU and re-measure. */
    result = job_object_helper_limit_cpu(job_object_helper, 1);
    ASSERT_ARE_EQUAL(int, 0, result);

    size_t time_with_at_1_percent_cpu = get_elapsed_milliseconds_for_cpu_bound_task();
    LogInfo("Using 1%% CPU = %zu milliseconds", time_with_at_1_percent_cpu);

    ASSERT_IS_TRUE(time_with_at_1_percent_cpu  > (time_with_no_limit * 35 / 10), "Code should take almost 4X time when using 1% CPU");

    /* Go back to 100% */
    result = job_object_helper_limit_cpu(job_object_helper, 100);
    ASSERT_ARE_EQUAL(int, 0, result);

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

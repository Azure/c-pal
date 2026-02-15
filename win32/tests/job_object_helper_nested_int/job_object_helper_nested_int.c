// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/job_object_helper.h"
#include "c_pal/threadapi.h"

#define MAX_JOB_OBJECTS 10
#define MEASUREMENT_DURATION_MS 5000
#define MALLOC_SIZE 64
/* Tolerance: count with 10 job objects should be at least 50% of count with 1 job object */
#define TOLERANCE_RATIO 0.5

typedef struct WORKER_CONTEXT_TAG
{
    volatile_atomic int32_t should_stop;
    volatile_atomic int64_t total_ops;
} WORKER_CONTEXT;

static int worker_thread_func(void* arg)
{
    WORKER_CONTEXT* context = (WORKER_CONTEXT*)arg;

    while (interlocked_add(&context->should_stop, 0) == 0)
    {
        void* ptr = malloc(MALLOC_SIZE);
        if (ptr != NULL)
        {
            free(ptr);
            (void)interlocked_increment_64(&context->total_ops);
        }
    }

    return 0;
}

static int64_t measure_malloc_throughput(uint32_t num_threads, uint32_t duration_ms)
{
    WORKER_CONTEXT context;
    (void)interlocked_exchange(&context.should_stop, 0);
    (void)interlocked_exchange_64(&context.total_ops, 0);

    THREAD_HANDLE* threads = (THREAD_HANDLE*)malloc(sizeof(THREAD_HANDLE) * num_threads);
    ASSERT_IS_NOT_NULL(threads);

    /* Start worker threads */
    for (uint32_t i = 0; i < num_threads; i++)
    {
        THREADAPI_RESULT tr = ThreadAPI_Create(&threads[i], worker_thread_func, &context);
        ASSERT_ARE_EQUAL(int, THREADAPI_OK, (int)tr, "Failed to create worker thread %" PRIu32, i);
    }

    /* Let threads run for the measurement duration */
    Sleep(duration_ms);

    /* Signal threads to stop */
    (void)interlocked_exchange(&context.should_stop, 1);

    /* Wait for all threads to finish */
    for (uint32_t i = 0; i < num_threads; i++)
    {
        int thread_result;
        THREADAPI_RESULT tr = ThreadAPI_Join(threads[i], &thread_result);
        ASSERT_ARE_EQUAL(int, THREADAPI_OK, (int)tr, "Failed to join worker thread %" PRIu32, i);
    }

    free(threads);

    return interlocked_add_64(&context.total_ops, 0);
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

/* This test demonstrates that nested job objects with CPU hard caps cause */
/* multiplicative CPU throttling. With 10 nested job objects each at 50%, */
/* the effective CPU should still be 50% if limits don't compound, but */
/* per Windows docs nested job CPU rates are relative to the parent. */
TEST_FUNCTION(test_nested_job_objects_do_not_throttle_cpu)
{
    /* Determine number of worker threads = half the cores, at least 1 */
    DWORD num_cores = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    uint32_t num_threads = (uint32_t)(num_cores / 2);
    if (num_threads == 0)
    {
        num_threads = 1;
    }
    LogInfo("Number of cores: %" PRIu32 ", using %" PRIu32 " worker threads", (uint32_t)num_cores, num_threads);

    /* Phase 1: Create 1 job object with 50%% CPU hard cap */
    LogInfo("Phase 1: Creating 1 job object with 50%% CPU limit...");
    THANDLE(JOB_OBJECT_HELPER) job_object_0 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_0, "Failed to create job object 0");

    /* Measure malloc throughput with 1 job object */
    LogInfo("Phase 1: Measuring malloc throughput for %d ms with 1 job object...", MEASUREMENT_DURATION_MS);
    int64_t count_1 = measure_malloc_throughput(num_threads, MEASUREMENT_DURATION_MS);
    LogInfo("Phase 1: Total malloc/free operations with 1 job object: %" PRId64, count_1);

    /* Phase 2: Create 9 more job objects (total 10), each with 50%% CPU hard cap */
    LogInfo("Phase 2: Creating 9 more job objects (total 10) with 50%% CPU limit each...");
    THANDLE(JOB_OBJECT_HELPER) job_object_1 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_1, "Failed to create job object 1");
    THANDLE(JOB_OBJECT_HELPER) job_object_2 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_2, "Failed to create job object 2");
    THANDLE(JOB_OBJECT_HELPER) job_object_3 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_3, "Failed to create job object 3");
    THANDLE(JOB_OBJECT_HELPER) job_object_4 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_4, "Failed to create job object 4");
    THANDLE(JOB_OBJECT_HELPER) job_object_5 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_5, "Failed to create job object 5");
    THANDLE(JOB_OBJECT_HELPER) job_object_6 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_6, "Failed to create job object 6");
    THANDLE(JOB_OBJECT_HELPER) job_object_7 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_7, "Failed to create job object 7");
    THANDLE(JOB_OBJECT_HELPER) job_object_8 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_8, "Failed to create job object 8");
    THANDLE(JOB_OBJECT_HELPER) job_object_9 = job_object_helper_set_job_limits_to_current_process(NULL, 50, 50);
    ASSERT_IS_NOT_NULL(job_object_9, "Failed to create job object 9");

    /* Release the first 9 job objects to mimic production behavior: */
    /* In prod, replica destroy calls THANDLE_ASSIGN(..., NULL) which CloseHandles */
    /* the job object, but the process remains assigned to the orphaned job objects */
    LogInfo("Phase 2: Releasing first 9 job objects (mimicking prod destroy+recreate)...");
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_0, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_1, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_2, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_3, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_4, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_5, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_6, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_7, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_8, NULL);

    /* Measure malloc throughput — only job_object_9 handle is held, */
    /* but process is still assigned to all 10 orphaned job objects */
    LogInfo("Phase 2: Measuring malloc throughput for %d ms with 10 nested job objects (9 orphaned)...", MEASUREMENT_DURATION_MS);
    int64_t count_10 = measure_malloc_throughput(num_threads, MEASUREMENT_DURATION_MS);
    LogInfo("Phase 2: Total malloc/free operations with 10 job objects (9 orphaned): %" PRId64, count_10);

    /* Log the ratio */
    double ratio = (count_1 > 0) ? ((double)count_10 / (double)count_1) : 0.0;
    LogInfo("Ratio (count_10 / count_1): %.4f (expected >= %.2f if no compounding)", ratio, TOLERANCE_RATIO);

    /* Assert: with 10 job objects, throughput should be in the same range as with 1 */
    /* If nested job objects compound CPU limits, count_10 will be dramatically lower */
    ASSERT_IS_TRUE(count_10 >= (int64_t)(count_1 * TOLERANCE_RATIO),
        "Nested job objects caused CPU throttling! count_1=%" PRId64 ", count_10=%" PRId64 ", ratio=%.4f",
        count_1, count_10, ratio);

    /* Cleanup — only job_object_9 remains */
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_9, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

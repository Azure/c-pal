// Copyright (c) Microsoft. All rights reserved.


#include <stddef.h>
#include <inttypes.h>



#include "c_logging/xlogging.h"
#include "testrunnerswitcher.h"

#include "c_pal/timer.h"

#include "c_pal/gballoc_hl.h"

#define malloc gballoc_hl_malloc
#define malloc_2 gballoc_hl_malloc_2
#define calloc gballoc_hl_calloc
#define realloc gballoc_hl_realloc
#define free gballoc_hl_free

static TEST_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);

    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/* alloc_perf */

#define ALLOC_COUNT                 100000
#define MAX_RANDOM_ALLOCATION_SIZE  (256 * 1024) // 256k max allocations in the random test
#define ALLOC_CYCLES                100

TEST_FUNCTION(alloc_performance_random)
{
    size_t iter;
    for (iter = 0; iter < ALLOC_CYCLES; iter++)
    {
        // arrange
        uint32_t i;

        void** blocks = (void**)malloc(sizeof(void*) * ALLOC_COUNT);
        ASSERT_IS_NOT_NULL(blocks);

        double start_time = timer_global_get_elapsed_ms();

        // act
        for (i = 0; i < ALLOC_COUNT; i++)
        {
            size_t size = (((uint64_t)rand() * (MAX_RANDOM_ALLOCATION_SIZE + 1)) / (RAND_MAX + 1));;
            blocks[i] = malloc(size);
            ASSERT_IS_NOT_NULL(blocks[i]);
        }

        double end_time = timer_global_get_elapsed_ms();
        LogInfo("%" PRIu32 " allocations done in %.02f ms", ALLOC_COUNT, (end_time - start_time));

        GBALLOC_LATENCY_BUCKETS latency_buckets;
        gballoc_hl_get_malloc_latency_buckets(&latency_buckets);

        const GBALLOC_LATENCY_BUCKET_METADATA* latency_buckets_metadata = gballoc_hl_get_latency_bucket_metadata();

        for (i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
        {
            LogInfo("Bucket %" PRIu32 " (%" PRIu32 "-%" PRIu32 ") had %" PRIu32 " allocations, latency avg=%.02f us, min=%" PRIu32 ", max=%" PRIu32,
                i, latency_buckets_metadata[i].size_range_low, latency_buckets_metadata[i].size_range_high, latency_buckets.buckets[i].count,
                latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_avg : 0.0,
                latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_min : 0,
                latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_max : 0);
        }

        // assert
        for (i = 0; i < ALLOC_COUNT; i++)
        {
            free(blocks[i]);
        }

        free(blocks);
    }
}

TEST_FUNCTION(alloc_performance)
{
    // arrange
    uint32_t i;

    void** blocks = (void**)malloc_2(sizeof(void*), ALLOC_COUNT);
    ASSERT_IS_NOT_NULL(blocks);

    double start_time = timer_global_get_elapsed_ms();

    // act
    for (i = 0; i < ALLOC_COUNT; i++)
    {
        blocks[i] = malloc(16384);
        ASSERT_IS_NOT_NULL(blocks[i]);
    }

    double end_time = timer_global_get_elapsed_ms();
    LogInfo("%" PRIu32 " allocations done in %.02f ms", ALLOC_COUNT, (end_time - start_time));

    GBALLOC_LATENCY_BUCKETS latency_buckets;
    gballoc_hl_get_malloc_latency_buckets(&latency_buckets);

    const GBALLOC_LATENCY_BUCKET_METADATA* latency_buckets_metadata = gballoc_hl_get_latency_bucket_metadata();

    for (i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        LogInfo("Bucket %" PRIu32 " (%" PRIu32 "-%" PRIu32 ") had %" PRIu32 " allocations, latency avg=%.02f us, min=%" PRIu32 ", max=%" PRIu32,
            i, latency_buckets_metadata[i].size_range_low, latency_buckets_metadata[i].size_range_high, latency_buckets.buckets[i].count,
            latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_avg : 0.0,
            latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_min : 0,
            latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_max : 0);
    }

    // assert
    for (i = 0; i < ALLOC_COUNT; i++)
    {
        free(blocks[i]);
    }

    free(blocks);
}

TEST_FUNCTION(free_performance)
{
    // arrange
    uint32_t i;

    void** blocks = (void**)malloc(sizeof(void*) * ALLOC_COUNT);
    ASSERT_IS_NOT_NULL(blocks);

    for (i = 0; i < ALLOC_COUNT; i++)
    {
        blocks[i] = malloc(16384);
        ASSERT_IS_NOT_NULL(blocks[i]);
    }

    double start_time = timer_global_get_elapsed_ms();

    // act
    for (i = 0; i < ALLOC_COUNT; i++)
    {
        free(blocks[i]);
    }

    double end_time = timer_global_get_elapsed_ms();
    LogInfo("%" PRIu32 " frees done in %.02f ms", ALLOC_COUNT, (end_time - start_time));

    GBALLOC_LATENCY_BUCKETS latency_buckets;
    gballoc_hl_get_free_latency_buckets(&latency_buckets);

    const GBALLOC_LATENCY_BUCKET_METADATA* latency_buckets_metadata = gballoc_hl_get_latency_bucket_metadata();

    for (i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        LogInfo("Bucket %" PRIu32 " (%" PRIu32 "-%" PRIu32 ") had %" PRIu32 " frees, latency avg=%.02f us, min=%" PRIu32 ", max=%" PRIu32,
            i, latency_buckets_metadata[i].size_range_low, latency_buckets_metadata[i].size_range_high, latency_buckets.buckets[i].count,
            latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_avg : 0.0,
            latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_min : 0,
            latency_buckets.buckets[i].count > 0 ? latency_buckets.buckets[i].latency_max : 0);
    }

    // assert
    free(blocks);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

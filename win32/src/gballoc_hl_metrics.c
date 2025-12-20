// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/timer.h"
#include "c_pal/gballoc_ll.h"
#include "c_pal/lazy_init.h"
#include "c_pal/interlocked.h"

#include "c_pal/gballoc_hl.h"

// This code snippet generates the below structure
//static void generate_metadata_as_text(void)
//{
//    printf("{ \"Bucket [0-511]\", 0, 511 }, \r\n");
//    for (size_t i = 1; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
//    {
//        uint32_t size_low = (uint32_t)1 << (8 + i);
//        uint32_t size_high = ((uint64_t)1 << (9 + i)) - 1;
//        printf("{ \"Bucket [%" PRIu32 "-%"  PRIu32 "]\", %" PRIu32 ", %" PRIu32 " }, \r\n",
//            size_low, size_high, size_low, size_high);
//    }
//}

/* Codes_SRS_GBALLOC_HL_METRICS_01_038: [ The first latency bucket shall be [0-511]. ]*/
/* Codes_SRS_GBALLOC_HL_METRICS_01_039: [ Each consecutive bucket shall be [1 << n, (1 << (n + 1)) - 1], where n starts at 8. ]*/
static const GBALLOC_LATENCY_BUCKET_METADATA latency_buckets_metadata[GBALLOC_LATENCY_BUCKET_COUNT] =
{
    { "Bucket [0-511]", 0, 511 },
    { "Bucket [512-1023]", 512, 1023 },
    { "Bucket [1024-2047]", 1024, 2047 },
    { "Bucket [2048-4095]", 2048, 4095 },
    { "Bucket [4096-8191]", 4096, 8191 },
    { "Bucket [8192-16383]", 8192, 16383 },
    { "Bucket [16384-32767]", 16384, 32767 },
    { "Bucket [32768-65535]", 32768, 65535 },
    { "Bucket [65536-131071]", 65536, 131071 },
    { "Bucket [131072-262143]", 131072, 262143 },
    { "Bucket [262144-524287]", 262144, 524287 },
    { "Bucket [524288-1048575]", 524288, 1048575 },
    { "Bucket [1048576-2097151]", 1048576, 2097151 },
    { "Bucket [2097152-4194303]", 2097152, 4194303 },
    { "Bucket [4194304-8388607]", 4194304, 8388607 },
    { "Bucket [8388608-16777215]", 8388608, 16777215 },
    { "Bucket [16777216-33554431]", 16777216, 33554431 },
    { "Bucket [33554432-67108863]", 33554432, 67108863 },
    { "Bucket [67108864-134217727]", 67108864, 134217727 },
    { "Bucket [134217728-268435455]", 134217728, 268435455 },
    { "Bucket [268435456-536870911]", 268435456, 536870911 },
    { "Bucket [536870912-1073741823]", 536870912, 1073741823 },
    { "Bucket [1073741824-2147483647]", 1073741824, 2147483647 },
    { "Bucket [2147483648-4294967295]", 2147483648, 4294967295 }
};

typedef struct LATENCY_BUCKET_TAG
{
    volatile_atomic int64_t latency_sum;
    volatile_atomic int32_t latency_min;
    volatile_atomic int32_t latency_max;
    volatile_atomic int32_t count;
} LATENCY_BUCKET;

static LATENCY_BUCKET malloc_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT] = { 0 };
static LATENCY_BUCKET calloc_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT] = { 0 };
static LATENCY_BUCKET realloc_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT] = { 0 };
static LATENCY_BUCKET free_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT] = { 0 };

static size_t determine_latency_bucket_for_size(size_t size)
{
    size_t bucket = 0;

#if SIZE_MAX != UINT32_MAX
    if (size > UINT32_MAX)
    {
        bucket = GBALLOC_LATENCY_BUCKET_COUNT - 1;
    }
    else
#endif
    {
        // start at 512
        size >>= 9;
        while (size != 0)
        {
            bucket++;
            size >>= 1;
        }
    }

    return bucket;
}

static void init_latency_bucket(LATENCY_BUCKET* latency_bucket)
{
    (void)interlocked_exchange(&latency_bucket->count, 0);
    (void)interlocked_exchange_64(&latency_bucket->latency_sum, 0);
    (void)interlocked_exchange(&latency_bucket->latency_min, LONG_MAX);
    (void)interlocked_exchange(&latency_bucket->latency_max, 0);
}

static void internal_init_latency_counters(void)
{
    size_t i;

    /* Codes_SRS_GBALLOC_HL_METRICS_01_041: [ For each bucket, for each of the 4 flavors of latencies tracked, do_init shall initialize the count, latency sum used for computing the average and the min and max latency values. ]*/
    for (i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        init_latency_bucket(&malloc_latency_buckets[i]);
        init_latency_bucket(&calloc_latency_buckets[i]);
        init_latency_bucket(&realloc_latency_buckets[i]);
        init_latency_bucket(&free_latency_buckets[i]);
    }
}

static void internal_copy_latency_data(GBALLOC_LATENCY_BUCKETS* latency_buckets_out, LATENCY_BUCKET* source_latency_buckets)
{
    size_t i;

    for (i = 0; i < GBALLOC_LATENCY_BUCKET_COUNT; i++)
    {
        latency_buckets_out->buckets[i].count = interlocked_add(&source_latency_buckets[i].count, 0);
        latency_buckets_out->buckets[i].latency_avg = (latency_buckets_out->buckets[i].count == 0) ? 0 : (double)interlocked_add_64(&source_latency_buckets[i].latency_sum, 0) / latency_buckets_out->buckets[i].count;
        latency_buckets_out->buckets[i].latency_min = interlocked_add(&source_latency_buckets[i].latency_min, 0);
        latency_buckets_out->buckets[i].latency_max = interlocked_add(&source_latency_buckets[i].latency_max, 0);
    }
}

static call_once_t g_lazy = LAZY_INIT_NOT_DONE;

static int do_init(void* ll_params)
{
    int result;
    /* Codes_SRS_GBALLOC_HL_METRICS_02_005: [ do_init shall call gballoc_ll_init(ll_params). ]*/
    if (gballoc_ll_init(ll_params) != 0)
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_007: [ If gballoc_ll_init fails then do_init shall return a non-zero value. ]*/
        LogError("failure in gballoc_ll_init(ll_params=%p)", ll_params);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_006: [ do_init shall succeed and return 0. ]*/
        internal_init_latency_counters();
        result = 0;
    }
    return result;
}

int gballoc_hl_init(void* hl_params, void* ll_params)
{
    int result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_004: [ gballoc_hl_init shall call lazy_init with do_init as initialization function. ]*/
    if (lazy_init(&g_lazy, do_init, ll_params) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_01_004: [ If any error occurs, gballoc_hl_init shall fail and return a non-zero value. ]*/
        LogError("failure in lazy_init(&g_lazy=%p, do_init=%p, void* hl_params=%p, ll_params=%p)", &g_lazy, do_init, hl_params, ll_params);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_01_001: [ If the module is already initialized, gballoc_hl_init shall succeed and return 0. ]*/
        /*Codes_SRS_GBALLOC_HL_METRICS_01_003: [ On success, gballoc_hl_init shall return 0. ]*/
        result = 0;
    }

    return result;
}

void gballoc_hl_deinit(void)
{
    /*Codes_SRS_GBALLOC_HL_METRICS_01_005: [ If gballoc_hl_deinit is called while not initialized, gballoc_hl_deinit shall return. ]*/
    if (interlocked_add(&g_lazy, 0) != LAZY_INIT_NOT_DONE)
    {
        interlocked_exchange(&g_lazy, LAZY_INIT_NOT_DONE);

        /*Codes_SRS_GBALLOC_HL_METRICS_01_006: [ Otherwise it shall call gballoc_ll_deinit to deinitialize the ll layer. ]*/
        gballoc_ll_deinit();
    }
    
}

void gballoc_hl_reset_counters(void)
{
    /* Codes_SRS_GBALLOC_HL_METRICS_01_036: [ gballoc_hl_reset_counters shall reset the latency counters for all buckets for the APIs (malloc, calloc, realloc and free). ]*/
    internal_init_latency_counters();
}

int gballoc_hl_get_malloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    int result;

    if (latency_buckets_out == NULL)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_020: [ If latency_buckets_out is NULL, gballoc_hl_get_malloc_latency_buckets shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: GBALLOC_LATENCY_BUCKETS* latency_buckets_out=%p", latency_buckets_out);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_021: [ Otherwise, gballoc_hl_get_malloc_latency_buckets shall copy the latency stats maintained by the module for the malloc API into latency_buckets_out. ]*/
        internal_copy_latency_data(latency_buckets_out, malloc_latency_buckets);
        result = 0;
    }

    return result;
}

int gballoc_hl_get_calloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    int result;

    if (latency_buckets_out == NULL)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_022: [ If latency_buckets_out is NULL, gballoc_hl_get_calloc_latency_buckets shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: GBALLOC_LATENCY_BUCKETS* latency_buckets_out=%p", latency_buckets_out);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_023: [ Otherwise, gballoc_hl_get_calloc_latency_buckets shall copy the latency stats maintained by the module for the calloc API into latency_buckets_out. ]*/
        internal_copy_latency_data(latency_buckets_out, calloc_latency_buckets);
        result = 0;
    }

    return result;
}

int gballoc_hl_get_realloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    int result;

    if (latency_buckets_out == NULL)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_024: [ If latency_buckets_out is NULL, gballoc_hl_get_realloc_latency_buckets shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: GBALLOC_LATENCY_BUCKETS* latency_buckets_out=%p", latency_buckets_out);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_025: [ Otherwise, gballoc_hl_get_realloc_latency_buckets shall copy the latency stats maintained by the module for the realloc API into latency_buckets_out. ]*/
        internal_copy_latency_data(latency_buckets_out, realloc_latency_buckets);
        result = 0;
    }

    return result;
}

int gballoc_hl_get_free_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    int result;

    if (latency_buckets_out == NULL)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_026: [ If latency_buckets_out is NULL, gballoc_hl_get_free_latency_buckets shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: GBALLOC_LATENCY_BUCKETS* latency_buckets_out=%p", latency_buckets_out);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_027: [ Otherwise, gballoc_hl_get_free_latency_buckets shall copy the latency stats maintained by the module for the free API into latency_buckets_out. ]*/
        internal_copy_latency_data(latency_buckets_out, free_latency_buckets);
        result = 0;
    }

    return result;
}

const GBALLOC_LATENCY_BUCKET_METADATA* gballoc_hl_get_latency_bucket_metadata(void)
{
    /* Codes_SRS_GBALLOC_HL_METRICS_01_037: [ gballoc_hl_get_latency_bucket_metadata shall return an array of size LATENCY_BUCKET_COUNT that contains the metadata for each latency bucket. ]*/
    return latency_buckets_metadata;
}

static void internal_add_call_latency(LATENCY_BUCKET* latency_buckets, size_t size, int32_t latency)
{
    size_t bucket = determine_latency_bucket_for_size(size);

    /* Codes_SRS_GBALLOC_HL_METRICS_01_043: [ gballoc_hl_malloc shall add the computed latency to the running malloc latency sum used to compute the average. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_046: [ gballoc_hl_malloc_2 shall add the computed latency to the running malloc latency sum used to compute the average. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_050: [ gballoc_hl_malloc_flex shall add the computed latency to the running malloc latency sum used to compute the average. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_054: [ gballoc_hl_calloc shall add the computed latency to the running calloc latency sum used to compute the average. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_058: [ gballoc_hl_realloc shall add the computed latency to the running realloc latency sum used to compute the average. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_062: [ gballoc_hl_realloc_2 shall add the computed latency to the running realloc latency sum used to compute the average. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_066: [ gballoc_hl_realloc_flex shall add the computed latency to the running realloc latency sum used to compute the average. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_070: [ gballoc_hl_free shall add the computed latency to the running free latency sum used to compute the average. ]*/
    (void)interlocked_add_64(&latency_buckets[bucket].latency_sum, latency);
    do
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_044: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_malloc shall store it as the new minimum malloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_047: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_malloc_2 shall store it as the new minimum malloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_051: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_malloc_flex shall store it as the new minimum malloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_055: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_calloc shall store it as the new minimum calloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_059: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_realloc shall store it as the new minimum realloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_063: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_realloc_2 shall store it as the new minimum realloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_067: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_realloc_flex shall store it as the new minimum realloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_071: [ If the computed latency is less than the minimum tracked latency, gballoc_hl_free shall store it as the new minimum free latency. ]*/
        int32_t current_min = interlocked_add(&latency_buckets[bucket].latency_min, 0);
        if (current_min > latency)
        {
            if (interlocked_compare_exchange(&latency_buckets[bucket].latency_min, latency, current_min) == current_min)
            {
                break;
            }
        }
        else
        {
            break;
        }
    } while (1);
    do
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_045: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_malloc shall store it as the new maximum malloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_048: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_malloc_2 shall store it as the new maximum malloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_052: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_malloc_flex shall store it as the new maximum malloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_056: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_calloc shall store it as the new maximum calloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_060: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_realloc shall store it as the new maximum realloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_064: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_realloc_2 shall store it as the new maximum realloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_068: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_realloc_flex shall store it as the new maximum realloc latency. ]*/
        /* Codes_SRS_GBALLOC_HL_METRICS_01_072: [ If the computed latency is more than the maximum tracked latency, gballoc_hl_free shall store it as the new maximum free latency. ]*/
        int32_t current_max = interlocked_add(&latency_buckets[bucket].latency_max, 0);
        if (current_max < latency)
        {
            if (interlocked_compare_exchange(&latency_buckets[bucket].latency_max, latency, current_max) == current_max)
            {
                break;
            }
        }
        else
        {
            break;
        }
    } while (1);
    
    /* Codes_SRS_GBALLOC_HL_METRICS_01_042: [ gballoc_hl_malloc shall increment the count of malloc latency samples. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_049: [ gballoc_hl_malloc_2 shall increment the count of malloc latency samples. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_053: [ gballoc_hl_malloc_flex shall increment the count of malloc latency samples. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_057: [ gballoc_hl_calloc shall increment the count of calloc latency samples. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_061: [ gballoc_hl_realloc shall increment the count of realloc latency samples. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_065: [ gballoc_hl_realloc_2 shall increment the count of realloc latency samples. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_069: [ gballoc_hl_realloc_flex shall increment the count of realloc latency samples. ]*/
    /* Codes_SRS_GBALLOC_HL_METRICS_01_073: [ gballoc_hl_free shall increment the count of free latency samples. ]*/
    (void)interlocked_increment(&latency_buckets[bucket].count);
}

void* gballoc_hl_malloc(size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_001: [ gballoc_hl_malloc shall call lazy_init to initialize. ]*/
    if (lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_008: [ If the module was not initialized, gballoc_hl_malloc shall return NULL. ]*/
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_028: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
        double start_time = timer_global_get_elapsed_us();

        /* Codes_SRS_GBALLOC_HL_METRICS_01_007: [ gballoc_hl_malloc shall call gballoc_ll_malloc(size) and return the result of gballoc_ll_malloc. ]*/
        result = gballoc_ll_malloc(size);

        if (result == NULL)
        {
            LogError("failure in gballoc_ll_malloc(size=%zu)", size);
        }

        /* Codes_SRS_GBALLOC_HL_METRICS_01_029: [ gballoc_hl_malloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
        double end_time = timer_global_get_elapsed_us();

        int32_t latency = (int32_t)(end_time - start_time);
        internal_add_call_latency(malloc_latency_buckets, size, latency);
    }

    return result;
}

void* gballoc_hl_malloc_2(size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_026: [ gballoc_hl_malloc_2 shall call lazy_init to initialize. ]*/
    if (lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_027: [ If the module was not initialized, gballoc_hl_malloc_2 shall return NULL. ]*/
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_022: [ gballoc_hl_malloc_2 shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
        double start_time = timer_global_get_elapsed_us();

        /*Codes_SRS_GBALLOC_HL_METRICS_02_023: [ gballoc_hl_malloc_2 shall call gballoc_ll_malloc_2(nmemb, size) and return the result of gballoc_ll_malloc_2. ]*/
        result = gballoc_ll_malloc_2(nmemb, size);

        if (result == NULL)
        {
            LogError("failure in gballoc_ll_malloc_2(nmemb=%zu, size=%zu)", nmemb, size);
        }

        /*Codes_SRS_GBALLOC_HL_METRICS_02_024: [ gballoc_hl_malloc_2 shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
        double end_time = timer_global_get_elapsed_us();

        int32_t latency = (int32_t)(end_time - start_time);
        internal_add_call_latency(malloc_latency_buckets, nmemb * size, latency);
    }

    return result;
}

void* gballoc_hl_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_025: [ gballoc_hl_malloc_flex shall call lazy_init to initialize. ]*/
    if (lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_008: [ If the module was not initialized, gballoc_hl_malloc_flex shall return NULL. ]*/
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_009: [ gballoc_hl_malloc_flex shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
        double start_time = timer_global_get_elapsed_us();

        /*Codes_SRS_GBALLOC_HL_METRICS_02_010: [ gballoc_hl_malloc_flex shall call gballoc_ll_malloc_flex(base, nmemb, size) and return the result of gballoc_ll_malloc_flex. ]*/
        result = gballoc_ll_malloc_flex(base, nmemb, size);

        if (result == NULL)
        {
            LogError("failure in gballoc_ll_malloc_flex(base=%zu, nmemb=%zu, size=%zu)", base, nmemb, size);
        }

        /*Codes_SRS_GBALLOC_HL_METRICS_02_011: [ gballoc_hl_malloc_flex shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
        double end_time = timer_global_get_elapsed_us();

        int32_t latency = (int32_t)(end_time - start_time);
        internal_add_call_latency(malloc_latency_buckets, base + nmemb * size, latency);
    }

    return result;
}

void* gballoc_hl_calloc(size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_002: [ gballoc_hl_calloc shall call lazy_init to initialize. ]*/
    if (lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_011: [ If the module was not initialized, gballoc_hl_calloc shall return NULL. ]*/
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_030: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
        double start_time = timer_global_get_elapsed_us();

        /* Codes_SRS_GBALLOC_HL_METRICS_01_009: [ gballoc_hl_calloc shall call gballoc_ll_calloc(nmemb, size) and return the result of gballoc_ll_calloc. ]*/
        result = gballoc_ll_calloc(nmemb, size);

        if (result == NULL)
        {
            LogError("failure in gballoc_ll_calloc(nmemb=%zu, size=%zu)", nmemb, size);
        }

        /* Codes_SRS_GBALLOC_HL_METRICS_01_031: [ gballoc_hl_calloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
        double end_time = timer_global_get_elapsed_us();

        int32_t latency = (int32_t)(end_time - start_time);
        internal_add_call_latency(calloc_latency_buckets, nmemb * size, latency);
    }

    return result;
}

void* gballoc_hl_realloc(void* ptr, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_003: [ gballoc_hl_realloc shall call lazy_init to initialize. ]*/
    if (lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_015: [ If the module was not initialized, gballoc_hl_realloc shall return NULL. ]*/
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_032: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
        double start_time = timer_global_get_elapsed_us();

        /* Codes_SRS_GBALLOC_HL_METRICS_01_013: [ gballoc_hl_realloc shall call gballoc_ll_realloc(ptr, size) and return the result of gballoc_ll_realloc ]*/
        result = gballoc_ll_realloc(ptr, size);

        if (result == NULL)
        {
            LogError("failure in gballoc_ll_realloc(ptr=%p, size=%zu)", ptr, size);
        }

        /* Codes_SRS_GBALLOC_HL_METRICS_01_033: [ gballoc_hl_realloc shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
        double end_time = timer_global_get_elapsed_us();

        int32_t latency = (int32_t)(end_time - start_time);
        internal_add_call_latency(realloc_latency_buckets, size, latency);
    }

    return result;
}

void* gballoc_hl_realloc_2(void* ptr, size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_028: [ gballoc_hl_realloc_2 shall call lazy_init to initialize. ]*/
    if (lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_012: [ If the module was not initialized, gballoc_hl_realloc_2 shall return NULL. ]*/
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_029: [ gballoc_hl_realloc_2 shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
        double start_time = timer_global_get_elapsed_us();

        /*Codes_SRS_GBALLOC_HL_METRICS_02_014: [ gballoc_hl_realloc_2 shall call gballoc_ll_realloc_2(ptr, nmemb, size) and return the result of gballoc_ll_realloc_2. ]*/
        result = gballoc_ll_realloc_2(ptr, nmemb, size);

        if (result == NULL)
        {
            LogError("failure in gballoc_ll_realloc(ptr=%p, nmemb=%zu, size=%zu)", ptr, nmemb, size);
        }

        /*Codes_SRS_GBALLOC_HL_METRICS_02_015: [ gballoc_hl_realloc_2 shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
        double end_time = timer_global_get_elapsed_us();

        int32_t latency = (int32_t)(end_time - start_time);
        internal_add_call_latency(realloc_latency_buckets, nmemb * size, latency);
    }

    return result;
}

void* gballoc_hl_realloc_flex(void* ptr, size_t base, size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_HL_METRICS_02_016: [ gballoc_hl_realloc_flex shall call lazy_init to initialize. ]*/
    if (lazy_init(&g_lazy, do_init, NULL) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_017: [ If the module was not initialized, gballoc_hl_realloc_flex shall return NULL. ]*/
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_HL_METRICS_02_018: [ gballoc_hl_realloc_flex shall call timer_global_get_elapsed_us to obtain the start time of the allocate. ]*/
        double start_time = timer_global_get_elapsed_us();

        /*Codes_SRS_GBALLOC_HL_METRICS_02_019: [ gballoc_hl_realloc_flex shall call gballoc_hl_realloc_flex(ptr, base, nmemb, size) and return the result of gballoc_hl_realloc_flex. ]*/
        result = gballoc_ll_realloc_flex(ptr, base, nmemb, size);

        if (result == NULL)
        {
            LogError("failure in gballoc_ll_realloc_flex(ptr=%p, nmemb=%zu, size=%zu)", ptr, nmemb, size);
        }

        /*Codes_SRS_GBALLOC_HL_METRICS_02_020: [ gballoc_hl_realloc_flex shall call timer_global_get_elapsed_us to obtain the end time of the allocate. ]*/
        double end_time = timer_global_get_elapsed_us();

        int32_t latency = (int32_t)(end_time - start_time);
        internal_add_call_latency(realloc_latency_buckets, base + nmemb * size, latency);
    }

    return result;
}

void gballoc_hl_free(void* ptr)
{
    if (interlocked_add(&g_lazy,0) == LAZY_INIT_NOT_DONE)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_016: [ If the module was not initialized, gballoc_hl_free shall return. ]*/
        LogError("Not initialized");
    }
    else
    {
        if (ptr != NULL)
        {
            /* Codes_SRS_GBALLOC_HL_METRICS_01_034: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the start time of the free. ]*/
            double start_time = timer_global_get_elapsed_us();
            size_t size;

            /* Codes_SRS_GBALLOC_HL_METRICS_01_019: [ gballoc_hl_free shall call gballoc_ll_size to obtain the size of the allocation (used for latency counters). ]*/
            size = gballoc_ll_size(ptr);

            /* Codes_SRS_GBALLOC_HL_METRICS_01_017: [ gballoc_hl_free shall call gballoc_ll_free(ptr). ]*/
            gballoc_ll_free(ptr);

            /* Codes_SRS_GBALLOC_HL_METRICS_01_035: [ gballoc_hl_free shall call timer_global_get_elapsed_us to obtain the end time of the free. ]*/
            double end_time = timer_global_get_elapsed_us();

            int32_t latency = (int32_t)(end_time - start_time);
            internal_add_call_latency(free_latency_buckets, size, latency);
        }
    }
}

void gballoc_hl_print_stats()
{
    /* Codes_SRS_GBALLOC_HL_METRICS_01_040: [ gballoc_hl_print_stats shall call into gballoc_ll_print_stats to print the memory allocator statistics. ]*/
    gballoc_ll_print_stats();
}

size_t gballoc_hl_size(void* ptr)
{
    size_t result;

    /* Codes_SRS_GBALLOC_HL_METRICS_01_074: [ If the module was not initialized, gballoc_hl_size shall return 0. ]*/
    if (interlocked_add(&g_lazy, 0) == LAZY_INIT_NOT_DONE)
    {
        LogError("Not initialized");
        result = 0;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_075: [ Otherwise, gballoc_hl_size shall call gballoc_ll_size with ptr as argument and return the result of gballoc_ll_size. ]*/
        result = gballoc_ll_size(ptr);
    }

    return result;
}

int gballoc_hl_set_option(const char* option_name, void* option_value)
{
    /* Codes_SRS_GBALLOC_HL_METRICS_28_001: [ gballoc_hl_set_option shall call gballoc_ll_set_option with option_name and option_value as arguments. ]*/
    return gballoc_ll_set_option(option_name, option_value);
}

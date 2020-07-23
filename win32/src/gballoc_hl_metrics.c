// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "windows.h"
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"

#include "azure_c_pal/timer.h"
#include "azure_c_pal/gballoc_ll.h"

#include "azure_c_pal/gballoc_hl.h"

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
    volatile LONG64 latency_sum;
    volatile LONG latency_min;
    volatile LONG latency_max;
    volatile LONG count;
} LATENCY_BUCKET;

static LATENCY_BUCKET malloc_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT];
static LATENCY_BUCKET calloc_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT];
static LATENCY_BUCKET realloc_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT];
static LATENCY_BUCKET free_latency_buckets[GBALLOC_LATENCY_BUCKET_COUNT];

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
    (void)InterlockedExchange(&latency_bucket->count, 0);
    (void)InterlockedExchange64(&latency_bucket->latency_sum, 0);
    (void)InterlockedExchange(&latency_bucket->latency_min, LONG_MAX);
    (void)InterlockedExchange(&latency_bucket->latency_max, 0);
}

static void internal_init_latency_counters(void)
{
    size_t i;

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
        latency_buckets_out->buckets[i].count = InterlockedAdd(&source_latency_buckets[i].count, 0);
        latency_buckets_out->buckets[i].latency_avg = (latency_buckets_out->buckets[i].count == 0) ? 0 : (double)InterlockedAdd64(&source_latency_buckets[i].latency_sum, 0) / latency_buckets_out->buckets[i].count;
        latency_buckets_out->buckets[i].latency_min = InterlockedAdd(&source_latency_buckets[i].latency_min, 0);
        latency_buckets_out->buckets[i].latency_max = InterlockedAdd(&source_latency_buckets[i].latency_max, 0);
    }
}

static int wasInitialized = 0;

int gballoc_hl_init(void* hl_params, void* ll_params)
{
    (void)hl_params;
    int result;

    /* Codes_SRS_GBALLOC_HL_METRICS_01_001: [ If the module is already initialized, gballoc_hl_init shall fail and return a non-zero value. ]*/
    if (wasInitialized != 0)
    {
        LogError("Already initialized");
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_002: [ Otherwise, gballoc_hl_init shall call gballoc_ll_init(ll_params) . ]*/
        if (gballoc_ll_init(ll_params) != 0)
        {
            /*Codes_SRS_GBALLOC_HL_METRICS_01_004: [ If any error occurs, gballoc_hl_init shall fail and return a non-zero value. ]*/
            LogError("failure in gballoc_ll_init(ll_params=%p)", ll_params);
            result = MU_FAILURE;
        }
        else
        {
            internal_init_latency_counters();
            wasInitialized = 1;
            /* Codes_SRS_GBALLOC_HL_METRICS_01_003: [ On success, gballoc_hl_init shall return 0. ]*/
            result = 0;
        }
    }

    return result;
}

void gballoc_hl_deinit(void)
{
    if (wasInitialized == 0)
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_005: [ If gballoc_hl_deinit is called while not initialized, gballoc_hl_deinit shall return. ]*/
        LogError("Not initialized");
    }
    else
    {
        /* Codes_SRS_GBALLOC_HL_METRICS_01_006: [ Otherwise it shall call gballoc_ll_deinit to deinitialize the ll layer. ]*/
        // error ignored as we can't do much with it
        gballoc_ll_deinit();
        wasInitialized = 0;
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
    /* Codes_SRS_GBALLOC_HL_METRICS_01_037: [ gballoc_hl_get_latency_bucket_metadata shall return an array of size GBALLOC_LATENCY_BUCKET_COUNT that contains the metadata for each latency bucket. ]*/
    return latency_buckets_metadata;
}

static void internal_add_call_latency(LATENCY_BUCKET* latency_buckets, size_t size, LONG latency)
{
    size_t bucket = determine_latency_bucket_for_size(size);
    (void)InterlockedAdd64(&latency_buckets[bucket].latency_sum, latency);
    do
    {
        LONG current_min = InterlockedAdd(&latency_buckets[bucket].latency_min, 0);
        if (current_min > latency)
        {
            if (InterlockedCompareExchange(&latency_buckets[bucket].latency_min, latency, current_min) == current_min)
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
        LONG current_max = InterlockedAdd(&latency_buckets[bucket].latency_max, 0);
        if (current_max < latency)
        {
            if (InterlockedCompareExchange(&latency_buckets[bucket].latency_max, latency, current_max) == current_max)
            {
                break;
            }
        }
        else
        {
            break;
        }
    } while (1);
    (void)InterlockedIncrement(&latency_buckets[bucket].count);

}

void* gballoc_hl_malloc(size_t size)
{
    void* result;

    if (wasInitialized == 0)
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

        LONG latency = (LONG)(end_time - start_time);
        internal_add_call_latency(malloc_latency_buckets, size, latency);
    }

    return result;
}

void* gballoc_hl_calloc(size_t nmemb, size_t size)
{
    void* result;

    if (wasInitialized == 0)
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

        LONG latency = (LONG)(end_time - start_time);
        internal_add_call_latency(calloc_latency_buckets, nmemb * size, latency);
    }

    return result;
}

void* gballoc_hl_realloc(void* ptr, size_t size)
{
    void* result;

    if (wasInitialized == 0)
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

        LONG latency = (LONG)(end_time - start_time);
        internal_add_call_latency(realloc_latency_buckets, size, latency);
    }

    return result;
}

void gballoc_hl_free(void* ptr)
{
    if (wasInitialized == 0)
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

            LONG latency = (LONG)(end_time - start_time);
            internal_add_call_latency(free_latency_buckets, size, latency);
        }
    }
}

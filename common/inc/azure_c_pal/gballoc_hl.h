// Copyright (c) Microsoft. All rights reserved.

#ifndef GBALLOC_WIN32_HEAP_H
#define GBALLOC_WIN32_HEAP_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#include "windows.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    #define GBALLOC_WIN32_LATENCY_BUCKET_COUNT 24

    typedef struct GBALLOC_WIN32_LATENCY_BUCKET_METADATA_TAG
    {
        const char* bucket_name;
        uint32_t size_range_low;
        uint32_t size_range_high;
    } GBALLOC_WIN32_LATENCY_BUCKET_METADATA;

    typedef struct GBALLOC_WIN32_LATENCY_BUCKET_TAG
    {
        double latency_avg;
        uint32_t latency_min;
        uint32_t latency_max;
        uint32_t count;
    } GBALLOC_WIN32_LATENCY_BUCKET;

    typedef struct GBALLOC_WIN32_LATENCY_BUCKETS_TAG
    {
        GBALLOC_WIN32_LATENCY_BUCKET buckets[GBALLOC_WIN32_LATENCY_BUCKET_COUNT];
    } GBALLOC_WIN32_LATENCY_BUCKETS;

    MOCKABLE_FUNCTION(, int, gballoc_win32_heap_init);
    MOCKABLE_FUNCTION(, void, gballoc_win32_heap_deinit);

    MOCKABLE_FUNCTION(, void, gballoc_win32_heap_reset_counters);

    MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_malloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
    MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_realloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
    MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_calloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
    MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_free_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);

    MOCKABLE_FUNCTION(, const GBALLOC_WIN32_LATENCY_BUCKET_METADATA*, gballoc_win32_heap_get_latency_bucket_metadata);

    MOCKABLE_FUNCTION(, void*, gballoc_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_realloc, void*, ptr, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_free, void*, ptr);

#ifdef __cplusplus
}
#endif

#endif // GBALLOC_WIN32_HEAP_H

# `gballoc_hl_metrics` requirements

## Overview

`gballoc_hl_metrics` is a module that computes metrics for calls with destination `gballoc_ll`.

## Exposed API

```c
    #define GBALLOC_LATENCY_BUCKET_COUNT 24

    typedef struct GBALLOC_LATENCY_BUCKET_METADATA_TAG
    {
        const char* bucket_name;
        uint32_t size_range_low;
        uint32_t size_range_high;
    } GBALLOC_LATENCY_BUCKET_METADATA;

    typedef struct GBALLOC_LATENCY_BUCKET_TAG
    {
        double latency_avg;
        uint32_t latency_min;
        uint32_t latency_max;
        uint32_t count;
    } GBALLOC_LATENCY_BUCKET;

    typedef struct GBALLOC_LATENCY_BUCKETS_TAG
    {
        GBALLOC_LATENCY_BUCKET buckets[GBALLOC_LATENCY_BUCKET_COUNT];
    } GBALLOC_LATENCY_BUCKETS;

    MOCKABLE_FUNCTION(, int, gballoc_hl_init, void*, hl_params, void*, ll_params);
    MOCKABLE_FUNCTION(, void, gballoc_hl_deinit);

    MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc_2, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc, void*, ptr, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_hl_free, void*, ptr);

    MOCKABLE_FUNCTION(, void, gballoc_hl_reset_counters);

    MOCKABLE_FUNCTION(, int, gballoc_hl_get_malloc_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
    MOCKABLE_FUNCTION(, int, gballoc_hl_get_realloc_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
    MOCKABLE_FUNCTION(, int, gballoc_hl_get_calloc_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
    MOCKABLE_FUNCTION(, int, gballoc_hl_get_free_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);

    MOCKABLE_FUNCTION(, const GBALLOC_LATENCY_BUCKET_METADATA*, gballoc_hl_get_latency_bucket_metadata);
```

### gballoc_hl_init

```c
    MOCKABLE_FUNCTION(, int, gballoc_hl_init, void*, hl_params, void*, ll_params);
```

`gballoc_hl_init` initializes the module. `hl_params` is ignored. `ll_params` is passed to `gballoc_ll_init`. This function is not thread-safe.

**SRS_GBALLOC_HL_METRICS_01_001: [** If the module is already initialized, `gballoc_hl_init` shall succeed and return 0. **]**

**SRS_GBALLOC_HL_METRICS_02_004: [** `gballoc_hl_init` shall call `lazy_init` with `do_init` as initialization function. **]**

**SRS_GBALLOC_HL_METRICS_02_005: [** `do_init` shall call `gballoc_ll_init(ll_params)`. **]**

**SRS_GBALLOC_HL_METRICS_02_006: [** `do_init` shall succeed and return 0. **]**

**SRS_GBALLOC_HL_METRICS_02_007: [** If `gballoc_ll_init` fails then `do_init` shall return a non-zero value. **]**

**SRS_GBALLOC_HL_METRICS_01_003: [** On success, `gballoc_hl_init` shall return 0. **]**

**SRS_GBALLOC_HL_METRICS_01_004: [** If any error occurs, `gballoc_hl_init` shall fail and return a non-zero value. **]**

### gballoc_hl_deinit

```c
MOCKABLE_FUNCTION(, void, gballoc_hl_deinit);
```

`gballoc_hl_deinit` deinitializes the module. This function is not thread-safe.

**SRS_GBALLOC_HL_METRICS_01_005: [** If `gballoc_hl_deinit` is called while not initialized, `gballoc_hl_deinit` shall return. **]**

**SRS_GBALLOC_HL_METRICS_01_006: [** Otherwise it shall call `gballoc_ll_deinit` to deinitialize the ll layer. **]**

### gballoc_hl_malloc

```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc, size_t, size);
```

`gballoc_hl_malloc` allocates `size` bytes of memory.

**SRS_GBALLOC_HL_METRICS_02_001: [** `gballoc_hl_malloc` shall call `lazy_init` to initialize. **]**

**SRS_GBALLOC_HL_METRICS_01_008: [** If the module was not initialized, `gballoc_hl_malloc` shall return NULL. **]**

**SRS_GBALLOC_HL_METRICS_01_028: [** `gballoc_hl_malloc` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_HL_METRICS_01_007: [** `gballoc_hl_malloc` shall call `gballoc_ll_malloc(size)` and return the result of `gballoc_ll_malloc`. **]**

**SRS_GBALLOC_HL_METRICS_01_029: [** `gballoc_hl_malloc` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

### gballoc_hl_malloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc_2, size_t, nmemb, size_t, size);
```

`gballoc_hl_malloc_2` allocates `nmemb` * `size` bytes of memory.

**SRS_GBALLOC_HL_METRICS_02_026: [** `gballoc_hl_malloc_2` shall call `lazy_init` to initialize. **]**

**SRS_GBALLOC_HL_METRICS_02_027: [** If the module was not initialized, `gballoc_hl_malloc_2` shall return NULL. **]**

**SRS_GBALLOC_HL_METRICS_02_022: [** `gballoc_hl_malloc_2` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_HL_METRICS_02_023: [** `gballoc_hl_malloc_2` shall call `gballoc_ll_malloc_2(nmemb, size)` and return the result of `gballoc_ll_malloc_2`. **]**

**SRS_GBALLOC_HL_METRICS_02_024: [** `gballoc_hl_malloc_2` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

### gballoc_hl_malloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_hl_malloc_flex` allocates `base + nmemb` * `size` bytes of memory.

**SRS_GBALLOC_HL_METRICS_02_025: [** `gballoc_hl_malloc_flex` shall call `lazy_init` to initialize. **]**

**SRS_GBALLOC_HL_METRICS_02_008: [** If the module was not initialized, `gballoc_hl_malloc_flex` shall return NULL. **]**

**SRS_GBALLOC_HL_METRICS_02_009: [** `gballoc_hl_malloc_flex` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_HL_METRICS_02_010: [** `gballoc_hl_malloc_flex` shall call `gballoc_ll_malloc_flex(base, nmemb, size)` and return the result of `gballoc_ll_malloc_flex`. **]**

**SRS_GBALLOC_HL_METRICS_02_011: [** `gballoc_hl_malloc_flex` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

### gballoc_hl_calloc

```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_calloc, size_t, nmemb, size_t, size);
```

`gballoc_hl_calloc` allocates `size` * `nmemb` bytes of memory and initializes the memory with 0.

**SRS_GBALLOC_HL_METRICS_02_002: [** `gballoc_hl_calloc` shall call `lazy_init` to initialize. **]**

**SRS_GBALLOC_HL_METRICS_01_011: [** If the module was not initialized, `gballoc_hl_calloc` shall return NULL. **]**

**SRS_GBALLOC_HL_METRICS_01_030: [** `gballoc_hl_calloc` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_HL_METRICS_01_009: [** `gballoc_hl_calloc` shall call `gballoc_ll_calloc(nmemb, size)` and return the result of `gballoc_ll_calloc`. **]**

**SRS_GBALLOC_HL_METRICS_01_031: [** `gballoc_hl_calloc` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

### gballoc_hl_realloc

```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc, void*, ptr, size_t, size);
```

`gballoc_hl_realloc` allocates `size` bytes of memory making sure that the original memory at `ptr` is copied to the new reallocated memory block.

**SRS_GBALLOC_HL_METRICS_02_003: [** `gballoc_hl_realloc` shall call lazy_init to initialize. **]**

**SRS_GBALLOC_HL_METRICS_01_015: [** If the module was not initialized, `gballoc_hl_realloc` shall return NULL. **]**

**SRS_GBALLOC_HL_METRICS_01_032: [** `gballoc_hl_realloc` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_HL_METRICS_01_013: [** `gballoc_hl_realloc` shall call `gballoc_ll_realloc(ptr, size)` and return the result of `gballoc_ll_realloc` **]**

**SRS_GBALLOC_HL_METRICS_01_033: [** `gballoc_hl_realloc` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**


### gballoc_hl_realloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
```

`gballoc_hl_realloc_2` reallocates `ptr` to have size `nmemb * size`.

**SRS_GBALLOC_HL_METRICS_02_028: [** `gballoc_hl_realloc_2` shall call `lazy_init` to initialize. **]**

**SRS_GBALLOC_HL_METRICS_02_012: [** If the module was not initialized, `gballoc_hl_realloc_2` shall return NULL. **]**

**SRS_GBALLOC_HL_METRICS_02_029: [** `gballoc_hl_realloc_2` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_HL_METRICS_02_014: [** `gballoc_hl_realloc_2` shall call `gballoc_ll_realloc_2(ptr, nmemb, size)` and return the result of `gballoc_ll_realloc_2`. **]**

**SRS_GBALLOC_HL_METRICS_02_015: [** `gballoc_hl_realloc_2` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

### gballoc_hl_realloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_hl_realloc_flex` reallocates `ptr` to have size `base + nmemb * size`.

**SRS_GBALLOC_HL_METRICS_02_016: [** `gballoc_hl_realloc_flex` shall call `lazy_init` to initialize. **]**

**SRS_GBALLOC_HL_METRICS_02_017: [** If the module was not initialized, `gballoc_hl_realloc_flex` shall return NULL. **]**

**SRS_GBALLOC_HL_METRICS_02_018: [** `gballoc_hl_realloc_flex` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_HL_METRICS_02_019: [** `gballoc_hl_realloc_flex` shall call `gballoc_hl_realloc_flex(ptr, base, nmemb, size)` and return the result of `gballoc_hl_realloc_flex`. **]**

**SRS_GBALLOC_HL_METRICS_02_020: [** `gballoc_hl_realloc_flex` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**


### gballoc_hl_free

```c
MOCKABLE_FUNCTION(, void, gballoc_hl_free, void*, ptr);
```

`gballoc_hl_free` frees the memory allocated with `gballoc_hl_malloc`, `gballoc_hl_calloc` or `gballoc_hl_realloc`.

**SRS_GBALLOC_HL_METRICS_01_016: [** If the module was not initialized, `gballoc_hl_free` shall return. **]**

**SRS_GBALLOC_HL_METRICS_01_034: [** `gballoc_hl_free` shall call `timer_global_get_elapsed_us` to obtain the start time of the free. **]**

**SRS_GBALLOC_HL_METRICS_01_019: [** `gballoc_hl_free` shall call `gballoc_ll_size` to obtain the size of the allocation (used for latency counters). **]**

**SRS_GBALLOC_HL_METRICS_01_017: [** `gballoc_hl_free` shall call `gballoc_ll_free(ptr)`. **]**

**SRS_GBALLOC_HL_METRICS_01_035: [** `gballoc_hl_free` shall call `timer_global_get_elapsed_us` to obtain the end time of the free. **]**

### gballoc_hl_reset_counters

```c
MOCKABLE_FUNCTION(, void, gballoc_hl_reset_counters);
```

`gballoc_hl_reset_counters` resets the latency counters tracked for the heap.

**SRS_GBALLOC_HL_METRICS_01_036: [** `gballoc_hl_reset_counters` shall reset the latency counters for all buckets for the APIs (malloc, calloc, realloc and free). **]**

### gballoc_hl_get_malloc_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_malloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_hl_get_malloc_latency_buckets` gets the malloc latency stats.

**SRS_GBALLOC_HL_METRICS_01_020: [** If `latency_buckets_out` is `NULL`, `gballoc_hl_get_malloc_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_HL_METRICS_01_021: [** Otherwise, `gballoc_hl_get_malloc_latency_buckets` shall copy the latency stats maintained by the module for the malloc API into `latency_buckets_out`. **]**

### gballoc_hl_get_calloc_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_calloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_hl_get_calloc_latency_buckets` gets the calloc latency stats.

**SRS_GBALLOC_HL_METRICS_01_022: [** If `latency_buckets_out` is `NULL`, `gballoc_hl_get_calloc_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_HL_METRICS_01_023: [** Otherwise, `gballoc_hl_get_calloc_latency_buckets` shall copy the latency stats maintained by the module for the calloc API into `latency_buckets_out`. **]**

### gballoc_hl_get_realloc_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_realloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_hl_get_realloc_latency_buckets` gets the calloc latency stats.

**SRS_GBALLOC_HL_METRICS_01_024: [** If `latency_buckets_out` is `NULL`, `gballoc_hl_get_realloc_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_HL_METRICS_01_025: [** Otherwise, `gballoc_hl_get_realloc_latency_buckets` shall copy the latency stats maintained by the module for the realloc API into `latency_buckets_out`. **]**

### gballoc_hl_get_free_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_free_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_hl_get_free_latency_buckets` gets the calloc latency stats.

**SRS_GBALLOC_HL_METRICS_01_026: [** If `latency_buckets_out` is `NULL`, `gballoc_hl_get_free_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_HL_METRICS_01_027: [** Otherwise, `gballoc_hl_get_free_latency_buckets` shall copy the latency stats maintained by the module for the free API into `latency_buckets_out`. **]**

### gballoc_hl_get_latency_bucket_metadata

```c
MOCKABLE_FUNCTION(, const GBALLOC_WIN32_LATENCY_BUCKET_METADATA*, gballoc_hl_get_latency_bucket_metadata);
```

`gballoc_hl_get_latency_bucket_metadata` returns an array with the metadata for the latency buckets tracked for malloc/calls/realloc/free calls.

One latency bucket contains a friendly name for the bucket and its low and high size inclusive boundary.

**SRS_GBALLOC_HL_METRICS_01_037: [** `gballoc_hl_get_latency_bucket_metadata` shall return an array of size `LATENCY_BUCKET_COUNT` that contains the metadata for each latency bucket. **]**

**SRS_GBALLOC_HL_METRICS_01_038: [** The first latency bucket shall be `[0-511]`. **]**

**SRS_GBALLOC_HL_METRICS_01_039: [** Each consecutive bucket shall be `[1 << n, (1 << (n + 1)) - 1]`, where n starts at 8. **]**

Note: buckets are in this case: `[512-1023]`, `1024-2047`, etc.
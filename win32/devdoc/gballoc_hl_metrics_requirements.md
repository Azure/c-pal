`gballoc_hl_metrics` requirements
================

## Overview

`gballoc_hl_metrics` is a module that computes metrics for calls with destination `gballoc_ll`.

## Exposed API

```c
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
```

### gballoc_win32_heap_init

```c
MOCKABLE_FUNCTION(, int, gballoc_win32_heap_init);
```

`gballoc_win32_heap_init` initializes the heap to be used by the gballoc_malloc/gballoc_free functions.

**SRS_GBALLOC_WIN32_HEAP_01_001: [** If the module is already initialized, `gballoc_win32_heap_init` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_WIN32_HEAP_01_002: [** Otherwise, `gballoc_win32_heap_init` shall call `HeapCreate` to create a new heap with the initial size and maximum size set to 0. **]**

**SRS_GBALLOC_WIN32_HEAP_01_003: [** On success, `gballoc_win32_heap_init` shall return 0. **]**

**SRS_GBALLOC_WIN32_HEAP_01_004: [** If any error occurs, `gballoc_win32_heap_init` shall fail and return a non-zero value. **]**

### gballoc_win32_heap_deinit

```c
MOCKABLE_FUNCTION(, void, gballoc_win32_heap_deinit);
```

`gballoc_win32_heap_deinit` deinitializes the heap created in `gballoc_win32_heap_init`.

**SRS_GBALLOC_WIN32_HEAP_01_005: [** If `gballoc_win32_heap_deinit` is called while not initialized, `gballoc_win32_heap_deinit` shall return. **]**

**SRS_GBALLOC_WIN32_HEAP_01_006: [** Otherwise it shall call `HeapDestroy` to destroy the heap created in `gballoc_win32_heap_init`. **]**

### gballoc_win32_heap_reset_counters

```c
MOCKABLE_FUNCTION(, void, gballoc_win32_heap_reset_counters);
```

`gballoc_win32_heap_reset_counters` resets the latency counters tracked for the heap.

**SRS_GBALLOC_WIN32_HEAP_01_036: [** `gballoc_win32_heap_reset_counters` shall reset the latency counters for all buckets for the APIs (malloc, calloc, realloc and free). **]**

### gballoc_win32_heap_get_malloc_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_malloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_win32_heap_get_malloc_latency_buckets` gets the malloc latency stats.

**SRS_GBALLOC_WIN32_HEAP_01_020: [** If `latency_buckets_out` is `NULL`, `gballoc_win32_heap_get_malloc_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_WIN32_HEAP_01_021: [** Otherwise, `gballoc_win32_heap_get_malloc_latency_buckets` shall copy the latency stats maintained by the module for the malloc API into `latency_buckets_out`. **]**

### gballoc_win32_heap_get_calloc_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_calloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_win32_heap_get_calloc_latency_buckets` gets the calloc latency stats.

**SRS_GBALLOC_WIN32_HEAP_01_022: [** If `latency_buckets_out` is `NULL`, `gballoc_win32_heap_get_calloc_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_WIN32_HEAP_01_023: [** Otherwise, `gballoc_win32_heap_get_calloc_latency_buckets` shall copy the latency stats maintained by the module for the calloc API into `latency_buckets_out`. **]**

### gballoc_win32_heap_get_realloc_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_realloc_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_win32_heap_get_realloc_latency_buckets` gets the calloc latency stats.

**SRS_GBALLOC_WIN32_HEAP_01_024: [** If `latency_buckets_out` is `NULL`, `gballoc_win32_heap_get_realloc_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_WIN32_HEAP_01_025: [** Otherwise, `gballoc_win32_heap_get_realloc_latency_buckets` shall copy the latency stats maintained by the module for the realloc API into `latency_buckets_out`. **]**

### gballoc_win32_heap_get_free_latency_buckets

```c
MOCKABLE_FUNCTION(, int, gballoc_win32_heap_get_free_latency_buckets, GBALLOC_WIN32_LATENCY_BUCKETS*, latency_buckets_out);
```

`gballoc_win32_heap_get_free_latency_buckets` gets the calloc latency stats.

**SRS_GBALLOC_WIN32_HEAP_01_026: [** If `latency_buckets_out` is `NULL`, `gballoc_win32_heap_get_free_latency_buckets` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_WIN32_HEAP_01_027: [** Otherwise, `gballoc_win32_heap_get_free_latency_buckets` shall copy the latency stats maintained by the module for the free API into `latency_buckets_out`. **]**

### gballoc_win32_heap_get_latency_bucket_metadata

```c
MOCKABLE_FUNCTION(, const GBALLOC_WIN32_LATENCY_BUCKET_METADATA*, gballoc_win32_heap_get_latency_bucket_metadata);
```

`gballoc_win32_heap_get_latency_bucket_metadata` returns an array with the metadata for the latency buckets tracked for malloc/calls/realloc/free calls.

One latency bucket contains a friendly name for the bucket and its low and high size inclusive boundary.

**SRS_GBALLOC_WIN32_HEAP_01_037: [** `gballoc_win32_heap_get_latency_bucket_metadata` shall return an array of size `LATENCY_BUCKET_COUNT` that contains the metadata for each latency bucket. **]**

**SRS_GBALLOC_WIN32_HEAP_01_038: [** The first latency bucket shall be `[0-511]`. **]**

**SRS_GBALLOC_WIN32_HEAP_01_039: [** Each consecutive bucket shall be `[1 << n, (1 << (n + 1)) - 1]`, where n starts at 8. **]**

Note: buckets are in this case: `[512-1023]`, `1024-2047`, etc.

### gballoc_malloc

```c
MOCKABLE_FUNCTION(, void*, gballoc_malloc, size_t, size);
```

`gballoc_malloc` allocates `size` bytes of memory.

**SRS_GBALLOC_WIN32_HEAP_01_008: [** If the module was not initialized, `gballoc_malloc` shall return NULL. **]**

**SRS_GBALLOC_WIN32_HEAP_01_028: [** `gballoc_malloc` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_WIN32_HEAP_01_007: [** `gballoc_malloc` shall call `HeapAlloc` for the heap created in `gballoc_win32_heap_init`, allocating `size` bytes and return the result of `HeapAlloc`. **]**

**SRS_GBALLOC_WIN32_HEAP_01_029: [** `gballoc_malloc` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

### gballoc_calloc

```c
MOCKABLE_FUNCTION(, void*, gballoc_calloc, size_t, nmemb, size_t, size);
```

`gballoc_calloc` allocates `size` * `nmemb` bytes of memory and initializes the memory with 0.

**SRS_GBALLOC_WIN32_HEAP_01_011: [** If the module was not initialized, `gballoc_calloc` shall return NULL. **]**

**SRS_GBALLOC_WIN32_HEAP_01_030: [** `gballoc_calloc` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_WIN32_HEAP_01_009: [** `gballoc_calloc` shall call `HeapAlloc` for the heap created in `gballoc_win32_heap_init`, allocating `size` * `nmemb` bytes. **]**

**SRS_GBALLOC_WIN32_HEAP_01_031: [** `gballoc_calloc` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

**SRS_GBALLOC_WIN32_HEAP_01_010: [** If `HeapAlloc` succeeds, `gballoc_calloc` shall zero the allocated memory and return the pointer to it. **]**

**SRS_GBALLOC_WIN32_HEAP_01_012: [** If `HeapAlloc` fails, `gballoc_calloc` shall return NULL. **]**

### gballoc_realloc

```c
MOCKABLE_FUNCTION(, void*, gballoc_realloc, void*, ptr, size_t, size);
```

`gballoc_realloc` allocates `size` bytes of memory making sure that the original memory at `ptr` is copied to the new reallocated memory block.

**SRS_GBALLOC_WIN32_HEAP_01_015: [** If the module was not initialized, `gballoc_realloc` shall return NULL. **]**

**SRS_GBALLOC_WIN32_HEAP_01_032: [** `gballoc_realloc` shall call `timer_global_get_elapsed_us` to obtain the start time of the allocate. **]**

**SRS_GBALLOC_WIN32_HEAP_01_013: [** If `ptr` is NULL, `gballoc_realloc` shall call `HeapAlloc` for the heap created in `gballoc_win32_heap_init`, allocating `size` bytes and return the result of `HeapAlloc`. **]**

**SRS_GBALLOC_WIN32_HEAP_01_014: [** If `ptr` is not NULL, `gballoc_realloc` shall call `HeapReAlloc` for the heap created in `gballoc_win32_heap_init`, passing `ptr` and `size` as arguments and return the result of `HeapReAlloc`. **]**

**SRS_GBALLOC_WIN32_HEAP_01_033: [** `gballoc_realloc` shall call `timer_global_get_elapsed_us` to obtain the end time of the allocate. **]**

**SRS_GBALLOC_WIN32_HEAP_01_018: [** If any error occurs, `gballoc_realloc` shall fail and return NULL. **]**

### gballoc_free

```c
MOCKABLE_FUNCTION(, void, gballoc_free, void*, ptr);
```

`gballoc_free` frees the memory allocated with `gballoc_malloc`, `gballoc_calloc` or `gballoc_realloc`.

**SRS_GBALLOC_WIN32_HEAP_01_016: [** If the module was not initialized, `gballoc_free` shall return. **]**

**SRS_GBALLOC_WIN32_HEAP_01_034: [** `gballoc_free` shall call `timer_global_get_elapsed_us` to obtain the start time of the free. **]**

**SRS_GBALLOC_WIN32_HEAP_01_019: [** `gballoc_free` shall call `HeapSize` to obtain the size of the allocation (used for latency counters). **]**

**SRS_GBALLOC_WIN32_HEAP_01_017: [** `gballoc_free` shall call `HeapFree` for the heap created in `gballoc_win32_heap_init`, freeing the memory at `ptr`. **]**

**SRS_GBALLOC_WIN32_HEAP_01_035: [** `gballoc_free` shall call `timer_global_get_elapsed_us` to obtain the end time of the free. **]**

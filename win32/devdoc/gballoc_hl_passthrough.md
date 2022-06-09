# gballoc_hl_passthrough requirements

## Overview

gballoc_hl_passthrough is a module that delegates all call of its APIs to the ones from gballoc_ll. 

## References


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
MOCKABLE_FUNCTION(, int, gballoc_hl_init, void*, gballoc_hl_init_params, void*, gballoc_ll_init_params);
```

`gballoc_hl_init` calls `gballoc_ll_init(gballoc_ll_init_params)`. Since `gballoc_hl` is passthrough it has no other functionality and `gballoc_hl_init_params` is ignored. This function is not thread-safe.

**SRS_GBALLOC_HL_PASSTHROUGH_02_017: [** `gballoc_hl_init` shall call `lazy_init` with `do_init` as function to execute and `gballoc_ll_init_params` as parameter. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_002: [** `gballoc_hl_init` shall succeed and return 0. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_003: [** If  there are any failures then `gballoc_hl_init` shall fail and return a non-zero value. **]**


### do_init(void* params)
```c
static int do_init(void* params)
```

`do_init` is a one-time-call function that initializes the module. 

**SRS_GBALLOC_HL_PASSTHROUGH_02_018: [** `do_init` shall call `gballoc_ll_init(params)`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_019: [** `do_init` shall return 0. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_020: [** If there are any failures then `do_init` shall fail and return a non-zero value. **]**


### gballoc_hl_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_hl_deinit);
```

`gballoc_hl_deinit` calls `gballoc_ll_deinit`. Since `gballoc_hl` is passthrough it has no other functionality.


**SRS_GBALLOC_HL_PASSTHROUGH_02_004: [** `gballoc_hl_deinit` shall call `gballoc_ll_deinit`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_021: [** `gballoc_hl_deinit` shall switch module's state to `LAZY_INIT_NOT_DONE` **]**


### gballoc_hl_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc, size_t, size);
```

`gballoc_hl_malloc` calls `gballoc_ll_malloc` and returns what `gballoc_ll_malloc` returned.

**SRS_GBALLOC_HL_PASSTHROUGH_02_022: [** `gballoc_hl_malloc` shall call `lazy_init` passing as execution function `do_init` and `NULL` for argument. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_023: [** If `lazy_init` fail then `gballoc_hl_malloc` shall fail and return `NULL`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_005: [** `gballoc_hl_malloc` shall call `gballoc_ll_malloc(size)` and return what `gballoc_ll_malloc` returned. **]**


### gballoc_hl_malloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc_2, size_t, nmemb, size_t, size);
```

`gballoc_hl_malloc_2` calls `gballoc_ll_malloc_2` and returns what `gballoc_ll_malloc_2` returned.

**SRS_GBALLOC_HL_PASSTHROUGH_02_028: [** `gballoc_hl_malloc_2` shall call `lazy_init` passing as execution function `do_init` and `NULL` for argument. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_029: [** If `lazy_init` fail then `gballoc_hl_malloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_030: [** `gballoc_hl_malloc_2` shall call `gballoc_ll_malloc_2(size)` and return what `gballoc_ll_malloc_2` returned. **]**


### gballoc_hl_malloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_hl_malloc_flex` calls `gballoc_ll_malloc_flex` and returns what `gballoc_ll_malloc_flex` returned.

**SRS_GBALLOC_HL_PASSTHROUGH_02_031: [** `gballoc_hl_malloc_flex` shall call `lazy_init` passing as execution function `do_init` and `NULL` for argument. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_032: [** If `lazy_init` fail then `gballoc_hl_malloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_033: [** `gballoc_hl_malloc_flex` shall call `gballoc_ll_malloc_flex(size)` and return what `gballoc_hl_malloc_flex` returned. **]**

### gballoc_hl_free
```c
MOCKABLE_FUNCTION(, void, gballoc_hl_free, void*, ptr);
```

`gballoc_hl_free` calls `gballoc_ll_free(ptr)`.

**SRS_GBALLOC_HL_PASSTHROUGH_02_006: [** `gballoc_hl_free` shall call `gballoc_ll_free(ptr)`. **]**

### gballoc_hl_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_calloc, size_t, nmemb, size_t, size);
```

`gballoc_hl_calloc` calls `gballoc_ll_calloc`.

**SRS_GBALLOC_HL_PASSTHROUGH_02_024: [** `gballoc_hl_calloc` shall call `lazy_init` passing as execution function `do_init` and `NULL` for argument. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_025: [** If `lazy_init` fail then `gballoc_hl_calloc` shall fail and return `NULL`.  **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_007: [** `gballoc_hl_calloc` shall call `gballoc_ll_calloc(nmemb, size)` and return what `gballoc_ll_calloc` returned. **]**


### gballoc_hl_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc, void*, ptr, size_t, size);
```

`gballoc_hl_realloc` calls `gballoc_ll_realloc`.

**SRS_GBALLOC_HL_PASSTHROUGH_02_026: [** `gballoc_hl_realloc` shall call `lazy_init` passing as execution function `do_init` and `NULL` for argument. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_027: [** If `lazy_init` fail then `gballoc_hl_realloc` shall fail and return `NULL`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_008: [** `gballoc_hl_realloc` shall call `gballoc_ll_realloc(ptr, size)` and return what `gballoc_ll_realloc` returned. **]**


### gballoc_hl_realloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
```

`gballoc_hl_realloc_2` calls `gballoc_ll_realloc_2` and returns what `gballoc_ll_realloc_2` returned.

**SRS_GBALLOC_HL_PASSTHROUGH_02_034: [** `gballoc_hl_realloc_2` shall call `lazy_init` passing as execution function `do_init` and `NULL` for argument. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_035: [** If `lazy_init` fail then `gballoc_hl_realloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_036: [** `gballoc_hl_realloc_2` shall call `gballoc_ll_realloc_2(ptr, nmemb, size)` and return what `gballoc_ll_realloc_2` returned. **]**


### gballoc_hl_realloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_hl_realloc_flex` calls `gballoc_ll_realloc_flex` and returns what `gballoc_ll_realloc_flex` returned.

**SRS_GBALLOC_HL_PASSTHROUGH_02_037: [** `gballoc_hl_realloc_flex` shall call `lazy_init` passing as execution function `do_init` and `NULL` for argument. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_038: [** If `lazy_init` fail then `gballoc_hl_realloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_039: [** `gballoc_hl_realloc_flex` shall call `gballoc_ll_realloc_flex(ptr, base, nmemb, size)` and return what `gballoc_ll_realloc_flex` returned. **]**


### gballoc_hl_reset_counters)
```c
MOCKABLE_FUNCTION(, void, gballoc_hl_reset_counters);
```

Since this is a passthough layer without any counter implementation `gballoc_hl_reset_counters` returns.

**SRS_GBALLOC_HL_PASSTHROUGH_02_009: [** `gballoc_hl_reset_counters` shall return. **]**

### gballoc_hl_get_malloc_latency_buckets
```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_malloc_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
```

Since this is a passthough layer without any counter implementation `gballoc_hl_get_malloc_latency_buckets` returns.

**SRS_GBALLOC_HL_PASSTHROUGH_02_010: [** `gballoc_hl_get_malloc_latency_buckets` shall set `latency_buckets_out`'s bytes all to 0 and return 0. **]**


### gballoc_hl_get_realloc_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_realloc_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
```
   
Since this is a passthough layer without any counter implementation `gballoc_hl_get_realloc_latency_buckets` returns.

**SRS_GBALLOC_HL_PASSTHROUGH_02_011: [** `gballoc_hl_get_realloc_latency_buckets` shall set `latency_buckets_out`'s bytes all to 0 and return 0. **]**


### gballoc_hl_get_calloc_latency_buckets
```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_calloc_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
```

Since this is a passthough layer without any counter implementation `gballoc_hl_get_calloc_latency_buckets` returns.

**SRS_GBALLOC_HL_PASSTHROUGH_02_012: [** `gballoc_hl_get_calloc_latency_buckets` shall set `latency_buckets_out`'s bytes all to 0 and return 0. **]**


### gballoc_hl_get_free_latency_buckets
```c
MOCKABLE_FUNCTION(, int, gballoc_hl_get_free_latency_buckets, GBALLOC_LATENCY_BUCKETS*, latency_buckets_out);
```

Since this is a passthough layer without any counter implementation `gballoc_hl_get_free_latency_buckets` returns.

**SRS_GBALLOC_HL_PASSTHROUGH_02_013: [** `gballoc_hl_get_free_latency_buckets` shall set `latency_buckets_out`'s bytes all to 0 and return 0. **]**


### gballoc_hl_get_latency_bucket_metadata
```c
MOCKABLE_FUNCTION(, const GBALLOC_LATENCY_BUCKET_METADATA*, gballoc_hl_get_latency_bucket_metadata);
```

`gballoc_hl_get_latency_bucket_metadata` returns an array with the metadata for the latency buckets tracked for malloc/calls/realloc/free calls.

One latency bucket contains a friendly name for the bucket and its low and high size inclusive boundary.

**SRS_GBALLOC_HL_PASSTHROUGH_02_014: [** `gballoc_hl_get_latency_bucket_metadata` shall return an array of size `LATENCY_BUCKET_COUNT` that contains the metadata for each latency bucket. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_015: [** The first latency bucket shall be `[0-511]`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_016: [** Each consecutive bucket shall be `[1 << n, (1 << (n + 1)) - 1]`, where n starts at 9. **]**

Note: buckets are in this case: `[512-1023]`, `[1024-2047]`, etc.
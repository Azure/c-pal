# gballoc_ll_passthrough requirements

## Overview

gballoc_ll_passthrough is a module that delegates all call of its APIs to the ones from C standard lib.

## References


## Exposed API

```c
    MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
    MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);

    MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_2, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);

    MOCKABLE_FUNCTION(, size_t, gballoc_ll_size, void*, ptr);
```

### gballoc_ll_init

```c
MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
```

gballoc_ll_init return 0. `params` is ignored. Function exists merely as a placeholder.

**SRS_GBALLOC_LL_PASSTHROUGH_02_001: [** `gballoc_ll_init` shall return 0. **]**

### gballoc_ll_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);
```

`gballoc_ll_deinit` returns. Function exists merely as a placeholder.

**SRS_GBALLOC_LL_PASSTHROUGH_02_002: [** `gballoc_ll_deinit` shall return. **]**

### gballoc_ll_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
```

`gballoc_ll_malloc` returns what `malloc` from stdlib returns.

**SRS_GBALLOC_LL_PASSTHROUGH_02_003: [** `gballoc_ll_malloc` shall call `malloc(size)` and return what `malloc` returned. **]**

### gballoc_ll_malloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_2, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_2` returns what `malloc` from stdlib returns when called with `nmemb`*`size`. This is useful for example when allocating a pointer to an array of `nmemb` elements each having `size` size. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_PASSTHROUGH_02_008: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_PASSTHROUGH_02_009: [** `gballoc_ll_malloc_2` shall call `malloc(nmemb*size)` and returns what `malloc` returned. **]**

### gballoc_ll_malloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_flex` returns what `malloc` from stdlib returns when called with `base + nmemb * size`. This is useful for example when allocating a structure with a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_PASSTHROUGH_02_011: [** If `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_PASSTHROUGH_02_012: [** `gballoc_ll_malloc_flex` shall return what `malloc(base +  nmemb * size)` returns.  **]**


### gballoc_ll_free
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
```

`gballoc_ll_free` calls `free` from stdlib.

**SRS_GBALLOC_LL_PASSTHROUGH_02_004: [** `gballoc_ll_free` shall call `free(ptr)`. **]**

### gballoc_ll_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
```

`gballoc_ll_calloc` calls `calloc` from stdlib.

**SRS_GBALLOC_LL_PASSTHROUGH_02_005: [** `gballoc_ll_calloc` shall call `calloc(nmemb, size)` and return what `calloc` returned. **]**

### gballoc_ll_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
```

`gballoc_ll_realloc` calls `realloc` from stdlib.

**SRS_GBALLOC_LL_PASSTHROUGH_02_006: [** `gballoc_ll_realloc` shall call `realloc(ptr, size)` and return what `realloc` returned. **]**


### gballoc_ll_realloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_2` calls `realloc(ptr, nmemb * size)` from stdlib. This is useful for example when resizing a previously allocated array of elements. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_PASSTHROUGH_02_013: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_PASSTHROUGH_02_014: [** `gballoc_ll_realloc_2` shall return what `realloc(ptr, nmemb * size)` returns. **]**


### gballoc_ll_realloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_flex` calls `realloc(ptr, base + nmemb * size)` from stdlib. This is useful when reallocating a structure that has a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_PASSTHROUGH_02_015: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_PASSTHROUGH_02_016: [** If `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_PASSTHROUGH_02_017: [** `gballoc_ll_realloc_flex` shall return what `realloc(ptr, base + nmemb * size)` returns. **]**


### gballoc_ll_size
```c
MOCKABLE_FUNCTION(, size_t, gballoc_ll_size, void*, ptr);
```

`gballoc_ll_size` returns what `_msize` returns.

**SRS_GBALLOC_LL_PASSTHROUGH_02_007: [** `gballoc_ll_size` shall return what `_msize` returns. **]**



# gballoc_ll_mimalloc requirements
================

## Overview

gballoc_ll_mimalloc is a module that delegates all call of its APIs to the ones from mimalloc.

## References
[mimalloc](https://github.com/microsoft/mimalloc)


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

`gballoc_ll_init` returns. `params` exists as a placeholder and is ignored.

**SRS_GBALLOC_LL_MIMALLOC_02_001: [** `gballoc_ll_init` shall return 0. **]**


### gballoc_ll_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);
```

`gballoc_ll_deinit` returns.

**SRS_GBALLOC_LL_MIMALLOC_02_002: [** `gballoc_ll_deinit` shall return. **]**

### gballoc_ll_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
```

`gballoc_ll_malloc` calls `mi_malloc` and return a memory area of `size` bytes.

**SRS_GBALLOC_LL_MIMALLOC_02_003: [** `gballoc_ll_malloc` shall call `mi_malloc` and returns what `mi_malloc` returned. **]**


### gballoc_ll_malloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_2, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_2` calls `mi_malloc(nmemb * size)`. This is useful for example when allocating a pointer to an array of `nmemb` elements each having `size` size. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_MIMALLOC_02_008: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_MIMALLOC_02_009: [** `gballoc_ll_malloc_2` shall call `mi_malloc(nmemb * size)` and returns what `mi_malloc` returned. **]**


### gballoc_ll_malloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_flex` calls `mi_malloc(base + nmemb * size)`. This is useful for example when allocating a structure with a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_MIMALLOC_02_011: [** If `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_MIMALLOC_02_012: [** `gballoc_ll_malloc_flex` shall call `mi_malloc(base + nmemb * size)` and returns what `mi_malloc` returned. **]**


### gballoc_ll_free
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
```

`gballoc_ll_free` frees `ptr`.

**SRS_GBALLOC_LL_MIMALLOC_02_004: [** `gballoc_ll_free` shall call `mi_free(ptr)`. **]**


### gballoc_ll_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
```

`gballoc_ll_calloc` returns a memory area of `nmemb*size` bytes initialized to 0.

**SRS_GBALLOC_LL_MIMALLOC_02_005: [** `gballoc_ll_calloc` shall call `mi_calloc(nmemb, size)` and return what `mi_calloc` returned. **]**


### gballoc_ll_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
```

`gballoc_ll_realloc` reallocates `ptr` to have size `size`.

**SRS_GBALLOC_LL_MIMALLOC_02_006: [** `gballoc_ll_realloc` calls `mi_realloc(ptr, size)` and returns what `mi_realloc` returned. **]**


### gballoc_ll_realloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_2` reallocates `ptr` to have size `nmemb` * `size`. This is useful for example when resizing a previously allocated array of elements. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_MIMALLOC_02_013: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_MIMALLOC_02_014: [** `gballoc_ll_realloc_2` calls `mi_realloc(ptr, nmemb * size)` and returns what `mi_realloc` returned. **]**


### gballoc_ll_realloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_flex` reallocates `ptr` to have size `base + nmemb` * `size`. This is useful when reallocating a structure that has a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_MIMALLOC_02_016: [** If `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_MIMALLOC_02_017: [** `gballoc_ll_realloc_flex` calls `mi_realloc(ptr, base + nmemb * size)` and returns what `mi_realloc` returned. **]**

### gballoc_ll_size
```c
MOCKABLE_FUNCTION(, size_t, gballoc_ll_size, void*, ptr);
```

`gballoc_ll_size` returns the size of the memory block at `ptr`.

**SRS_GBALLOC_LL_MIMALLOC_02_007: [** `gballoc_ll_size` shall call `mi_usable_size` and return what `mi_usable_size` returned. **]**








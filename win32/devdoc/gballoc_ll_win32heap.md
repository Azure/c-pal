# gballoc_ll_win32heap requirements

## Overview

gballoc_ll_win32heap is a module that delegates all call of its APIs to the ones from windows heap functions.

## References
[Heap Functions](https://docs.microsoft.com/en-us/windows/win32/memory/heap-functions)


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

    MOCKABLE_FUNCTION(, void, gballoc_ll_print_stats);

    MOCKABLE_FUNCTION(, int, gballoc_ll_set_decay, int64_t, decay_milliseconds);
```

### gballoc_ll_init
```c
MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
```

`gballoc_ll_init` initializes the module by storing a HANDLE to a heap in a global variable. `params` exists as a placeholder and is ignored.

**SRS_GBALLOC_LL_WIN32HEAP_02_018: [** `gballoc_ll_init` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_002: [** `gballoc_ll_init` shall succeed and return 0. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_003: [** If there are any failures then `gballoc_ll_init` shall fail and return a non-0 value. **]**

### heap_init
```c
static int heap_init(void* init_params)
```

**SRS_GBALLOC_LL_WIN32HEAP_02_019: [** `heap_init` shall call `HeapCreate(0,0,0)` to create a heap. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_020: [** `heap_init` shall succeed and return 0. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_021: [** If there are any failures then `heap_init` shall fail and return a non-zero value. **]**


### gballoc_ll_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);
```

`gballoc_ll_deinit` deinitializes the global state and frees all the used resources. This function is not thread-safe.

**SRS_GBALLOC_LL_WIN32HEAP_02_016: [** If the global state is not initialized then `gballoc_ll_deinit` shall return. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_004: [** `gballoc_ll_deinit` shall call `HeapDestroy` on the handle stored by `gballoc_ll_init` in the global variable. **]**


### gballoc_ll_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
```

`gballoc_ll_malloc` calls `HeapAlloc` and return a memory area of `size` bytes.

**SRS_GBALLOC_LL_WIN32HEAP_02_022: [** `gballoc_ll_malloc` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_023: [** If `lazy_init` fails then `gballoc_ll_malloc` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_006: [** `gballoc_ll_malloc` shall call `HeapAlloc`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_007: [** `gballoc_ll_malloc` shall return what `HeapAlloc` returned. **]**


### gballoc_ll_malloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_2, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_2` returns what `HeapAlloc` returns when called with `nmemb`*`size`. This is useful for example when allocating a pointer to an array of `nmemb` elements each having `size` size. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_WIN32HEAP_02_028: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_029: [** `gballoc_ll_malloc_2` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_030: [** If `lazy_init` fails then `gballoc_ll_malloc_2` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_031: [** `gballoc_ll_malloc_2` shall call `HeapAlloc(nmemb*size)` and return what `HeapAlloc` returned. **]**

### gballoc_ll_malloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_flex` returns what `HeapAlloc` from stdlib returns when called with `base + nmemb * size`. This is useful for example when allocating a structure with a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_WIN32HEAP_02_033: [** If `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_034: [** `gballoc_ll_malloc_flex` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_035: [** If `lazy_init` fails then `gballoc_ll_malloc_flex` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_036: [** `gballoc_ll_malloc_flex` shall return what `HeapAlloc(base +  nmemb * size)` returns. **]**


### gballoc_ll_free
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
```

`gballoc_ll_free` frees `ptr`. It is assumed ptr is one of the pointers returned from a previous call to one of the allocation APIs and the module has not been deinit()'d since.

**SRS_GBALLOC_LL_WIN32HEAP_02_009: [** `gballoc_ll_free` shall call `HeapFree`. **]**


### gballoc_ll_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
```

`gballoc_ll_calloc` returns a memory area of `nmemb*size` bytes initialized to 0.

**SRS_GBALLOC_LL_WIN32HEAP_02_024: [** `gballoc_ll_calloc` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_025: [** If `lazy_init` fails then `gballoc_ll_calloc` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_011: [** `gballoc_ll_calloc` shall call `HeapAlloc` with `flags` set to `HEAP_ZERO_MEMORY`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_012: [** `gballoc_ll_calloc` shall return what `HeapAlloc` returns. **]**

### gballoc_ll_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
```

`gballoc_ll_realloc` reallocates `ptr` to have size `size`.

**SRS_GBALLOC_LL_WIN32HEAP_02_026: [** `gballoc_ll_realloc` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_027: [** If `lazy_init` fails then `gballoc_ll_realloc` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_014: [** If `ptr` is `NULL` then `gballoc_ll_realloc` shall call `HeapAlloc` and return what `HeapAlloc` returns. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_015: [** If `ptr` is not `NULL` then `gballoc_ll_realloc` shall call `HeapReAlloc` and return what `HeapReAlloc` returns. **]**


### gballoc_ll_realloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_2` calls `HeapReAlloc(ptr, nmemb * size)`. This is useful for example when resizing a previously allocated array of elements. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_WIN32HEAP_02_037: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_038: [** `gballoc_ll_realloc_2` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_039: [** If `lazy_init` fails then `gballoc_ll_realloc_2` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_046: [** If `ptr` is `NULL` then `gballoc_ll_realloc_2` shall call `HeapAlloc(nmemb * size)` and return what `HeapAlloc` returned. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_047: [** If `ptr` is not `NULL` then `gballoc_ll_realloc_2` shall call `HeapReAlloc(ptr, nmemb * size)` and return what `HeapReAlloc` returned. **]**


### gballoc_ll_realloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_flex` calls `HeapReAlloc(ptr, base + nmemb * size)`. This is useful when reallocating a structure that has a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_WIN32HEAP_02_042: [** `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_043: [** `gballoc_ll_realloc_flex` shall call `lazy_init` with parameter `do_init` set to `heap_init`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_044: [** If `lazy_init` fails then `gballoc_ll_malloc_flex` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_048: [** If `ptr` is `NULL` then `gballoc_ll_realloc_flex` shall return what `HeapAlloc(ptr, base + nmemb * size)` returns. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_049: [** If `ptr` is not `NULL` then `gballoc_ll_realloc_flex` shall return what `HeapReAlloc(ptr, base + nmemb * size)` returns. **]**


### gballoc_ll_size
```c
MOCKABLE_FUNCTION(, size_t, gballoc_ll_size, void*, ptr);
```

`gballoc_ll_size` returns `ptr`'s size. It is assumed `'ptr` is one of the pointers produced by this module and the module hasn't been deinit()'d since.

**SRS_GBALLOC_LL_WIN32HEAP_02_017: [** `gballoc_ll_size` shall call `HeapSize` and returns what `HeapSize` returns.  **]**

### gballoc_ll_print_stats

```c
MOCKABLE_FUNCTION(, void, gballoc_ll_print_stats);
```

**SRS_GBALLOC_LL_WIN32HEAP_01_001: [** `gballoc_ll_print_stats` shall return without printing any statistics. **]**

Note: printing of statistics is not implemented for `win32heap`.

### gballoc_ll_set_decay

```c
MOCKABLE_FUNCTION(, int, gballoc_ll_set_decay, int64_t, decay_milliseconds);
```

`gballoc_ll_set_decay` does nothing and returns a non-zero value.

**SRS_GBALLOC_LL_WIN32HEAP_28_001: [** `gballoc_ll_set_decay` shall do nothing and return a non-zero value. **]**

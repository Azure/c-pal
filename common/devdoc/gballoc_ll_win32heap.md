# gballoc_ll_win32heap requirements
================

## Overview

gballoc_ll_win32heap is a module that delegates all call of its APIs to the ones from windows heap functions.

## References
[Heap Functions](https://docs.microsoft.com/en-us/windows/win32/memory/heap-functions)


## Exposed API

```c
    MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
    MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);

    MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
```

### gballoc_ll_init
```c
MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
```

`gballoc_ll_init` globally initializes the module. `params` exists as a placeholder and is ignored.

**SRS_GBALLOC_LL_WIN32HEAP_02_001: [** `gballoc_ll_init` shall call `HeapCreate(0,0,0)`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_002: [** `gballoc_ll_init` shall succeed and return 0. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_003: [** If `HeapCreate` fails then `gballoc_ll_init` shall fail and return a non-0 value. **]**

### gballoc_ll_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);
```

`gballoc_ll_deinit` deinitializes the global state and frees all the used resources.

**SRS_GBALLOC_LL_WIN32HEAP_02_016: [** If the global state is not initialized then `gballoc_ll_deinit` shall return. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_004: [** `gballoc_ll_deinit` shall call `HeapDestroy`. **]**



### gballoc_ll_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
```

`gballoc_ll_malloc` calls `HeapAlloc` and return a memory area of `size` bytes.

**SRS_GBALLOC_LL_WIN32HEAP_02_005: [** If the global state is not initialized then `gballoc_ll_malloc` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_006: [** `gballoc_ll_malloc` shall call `HeapAlloc`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_007: [** `gballoc_ll_malloc` shall return what `HeapAlloc` returned. **]**

### gballoc_ll_free
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
```

`gballoc_ll_free` frees `ptr`.

**SRS_GBALLOC_LL_WIN32HEAP_02_008: [** If the global state is not initialized then `gballoc_ll_free` shall return. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_009: [** `gballoc_ll_free` shall call `HeapFree`. **]**


### gballoc_ll_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
```

`gballoc_ll_calloc` returns a memory area of `nmemb*size` bytes initialized to 0.

**SRS_GBALLOC_LL_WIN32HEAP_02_010: [** If the global state is not initialized then `gballoc_ll_calloc` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_011: [** `gballoc_ll_calloc` shall call `HeapAlloc` with `flags` set to `HEAP_ZERO_MEMORY`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_012: [** `gballoc_ll_calloc` shall return what `HeapAlloc` returns. **]**

### gballoc_ll_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
```

`gballoc_ll_realloc` reallocates `ptr` to have size `size`.

**SRS_GBALLOC_LL_WIN32HEAP_02_013: [** If the global state is not initialized then `gballoc_ll_realloc` shall return `NULL`. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_014: [** If `ptr` is `NULL` then `gballoc_ll_realloc` shall call `HeapAlloc` and return what `HeapAlloc` returns. **]**

**SRS_GBALLOC_LL_WIN32HEAP_02_015: [** If `ptr` is not `NULL` then `gballoc_ll_realloc` shall call `HeapReAlloc` and return what `HeapReAlloc` returns. **]**









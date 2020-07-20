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

`gballoc_ll_init` globally initializes the module.

`gballoc_ll_init` shall call `CreateHeap(0,0,0)`.

`gballoc_ll_init` shall succeed and return 0.

If `CreateHeap` fails then `gballoc_ll_init` shall fail and return a non-0 value.

### gballoc_ll_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);
```

`gballoc_ll_deinit` deinitializes the global state and frees all the used resources.

`gballoc_ll_deinit` shall call `HeapDestroy`.



### gballoc_ll_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
```

`gballoc_ll_malloc` calls `HeapAlloc` and return a memory area of `size` bytes.

If the global state is not initialized then `gballoc_ll_malloc` shall return `NULL`.

`gballoc_ll_malloc` shall call `HeapAlloc`.

`gballoc_ll_malloc` shall return what `HeapAlloc` returned.

### gballoc_ll_free
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
```

`gballoc_ll_free` frees `ptr`.

If the global state is not initialized then `gballoc_ll_free` shall return.

`gballoc_ll_free` shall call `HeapFree`.


### gballoc_ll_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
```

`gballoc_ll_calloc` returns a memory area of `nmemb*size` bytes initialized to 0.

If the global state is not initialized then `gballoc_ll_calloc` shall return `NULL`.

`gballoc_ll_calloc` shall call `HeapAlloc` with `flags` set to `HEAP_ZERO_MEMORY`.

`gballoc_ll_calloc` shall return what `HeapAlloc` returns.

### gballoc_ll_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
```

`gballoc_ll_realloc` reallocates `ptr` to have size `size`.

If the global state is not initialized then `gballoc_ll_realloc` shall return `NULL`.

If `ptr` is `NULL` then `gballoc_ll_realloc` shall call `HeapAlloc` and return what `HeapAlloc` returns.

If `ptr` is not `NULL` then `gballoc_ll_realloc` shall call `HeapReAlloc` and return what `HeapAlloc` returns.









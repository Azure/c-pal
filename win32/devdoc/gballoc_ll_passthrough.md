# gballoc_ll_passthrough requirements
================

## Overview

gballoc_ll_passthrough is a module that delegates all call of its APIs to the ones from C standard lib.

## References


## Exposed API

```c
    MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
    MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);

    MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);

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

**SRS_GBALLOC_LL_PASSTHROUGH_02_006: [** `gballoc_ll_realloc` shall call `realloc(nmemb, size)` and return what `realloc` returned. **]**


### gballoc_ll_size
```c
MOCKABLE_FUNCTION(, size_t, gballoc_ll_size, void*, ptr);
```

`gballoc_ll_size` returns what `_msize` returns.

**SRS_GBALLOC_LL_PASSTHROUGH_02_007: [** `gballoc_ll_size` shall return what `_msize` returns. **]**



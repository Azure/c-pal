# gballoc_hl_passthrough requirements
================

## Overview

gballoc_hl_passthrough is a module that delegates all call of its APIs to the ones from gballoc_ll. 

## References


## Exposed API

```c
    MOCKABLE_FUNCTION(, int, gballoc_hl_init, void*, gballoc_hl_init_params, void*, gballoc_ll_init_params);
    MOCKABLE_FUNCTION(, void, gballoc_hl_deinit);

    MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_hl_free, void*, ptr);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc, void*, ptr, size_t, size);
```


### gballoc_hl_init
```c
MOCKABLE_FUNCTION(, int, gballoc_hl_init, void*, gballoc_hl_init_params, void*, gballoc_ll_init_params);
```

`gballoc_hl_init` calls `gballoc_ll_init(gballoc_ll_init_params)`. Since `gballoc_hl` is passthrough it has no other functionality and `gballoc_hl_init_params` is ignored 

**SRS_GBALLOC_HL_PASSTHROUGH_02_001: [** `gballoc_hl_init` shall call `gballoc_ll_init(gballoc_ll_init_params)`. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_002: [** `gballoc_hl_init` shall succeed and return 0. **]**

**SRS_GBALLOC_HL_PASSTHROUGH_02_003: [** If  there are any failures then `gballoc_hl_init` shall fail and return a non-zero value. **]**

### gballoc_hl_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_hl_deinit);
```

`gballoc_hl_deinit` calls `gballoc_ll_deinit`. Since `gballoc_hl` is passthrough it has no other functionality.

**SRS_GBALLOC_HL_PASSTHROUGH_02_004: [** `gballoc_hl_deinit` shall call `gballoc_ll_deinit`. **]**


### gballoc_hl_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc, size_t, size);
```

`gballoc_hl_malloc` calls `gballoc_ll_malloc` and returns what `gballoc_ll_malloc` returned.

**SRS_GBALLOC_HL_PASSTHROUGH_02_005: [** `gballoc_hl_malloc` shall call `gballoc_ll_malloc(size)` and return what `gballoc_ll_malloc` returned. **]**

### gballoc_hl_free
```c
MOCKABLE_FUNCTION(, void, gballoc_hl_free, void*, ptr);
```

`gballoc_hl_free` calls `gballoc_hl_free(ptr)`.

**SRS_GBALLOC_HL_PASSTHROUGH_02_006: [** `gballoc_hl_free` shall call `gballoc_hl_free(ptr)`. **]**

### gballoc_hl_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_calloc, size_t, nmemb, size_t, size);
```

`gballoc_hl_calloc` calls `gballoc_ll_calloc`.

**SRS_GBALLOC_HL_PASSTHROUGH_02_007: [** `gballoc_hl_calloc` shall call `gballoc_ll_calloc(nmemb, size)` and return what `gballoc_ll_calloc` returned. **]**


### gballoc_hl_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc, void*, ptr, size_t, size);
```

`gballoc_hl_realloc` calls `gballoc_ll_realloc`.

**SRS_GBALLOC_HL_PASSTHROUGH_02_008: [** `gballoc_hl_realloc` shall call `gballoc_ll_realloc(ptr, size)` and return what `gballoc_ll_realloc` returned. **]**
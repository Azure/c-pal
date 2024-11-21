# gballoc_ll_jemalloc requirements

## Overview

gballoc_ll_jemalloc is a module that delegates all call of its APIs to the ones from jemalloc.

## References
[jemalloc](https://github.com/jemalloc/jemalloc)


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

    MOCKABLE_FUNCTION(, int, gballoc_ll_set_option, char*, option_name, void*, option_value);
```

### gballoc_ll_init
```c
MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
```

`gballoc_ll_init` returns. `params` exists as a placeholder and is ignored. This function is not thread-safe.

**SRS_GBALLOC_LL_JEMALLOC_01_001: [** `gballoc_ll_init` shall return 0. **]**


### gballoc_ll_deinit
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);
```

`gballoc_ll_deinit` returns.

**SRS_GBALLOC_LL_JEMALLOC_01_002: [** `gballoc_ll_deinit` shall return. **]**

### gballoc_ll_malloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
```

`gballoc_ll_malloc` calls `je_malloc` and return a memory area of `size` bytes.

**SRS_GBALLOC_LL_JEMALLOC_01_003: [** `gballoc_ll_malloc` shall call `je_malloc` and returns what `je_malloc` returned. **]**


### gballoc_ll_malloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_2, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_2` returns what `je_malloc` returns when called with `nmemb`*`size`. This is useful for example when allocating a pointer to an array of `nmemb` elements each having `size` size. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_JEMALLOC_02_001: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_JEMALLOC_02_002: [** `gballoc_ll_malloc_2` shall call `je_malloc(nmemb*size)` and returns what `je_malloc` returned. **]**


### gballoc_ll_malloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc_flex, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_malloc_flex` returns what `je_malloc` returns when called with `base + nmemb * size`. This is useful for example when allocating a structure with a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_JEMALLOC_02_004: [** If `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_malloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_JEMALLOC_02_005: [** `gballoc_ll_malloc_flex` shall return what `je_malloc(base +  nmemb * size)` returns. **]**


### gballoc_ll_free
```c
MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
```

`gballoc_ll_free` frees `ptr`.

**SRS_GBALLOC_LL_JEMALLOC_01_004: [** `gballoc_ll_free` shall call `je_free(ptr)`. **]**


### gballoc_ll_calloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
```

`gballoc_ll_calloc` returns a memory area of `nmemb*size` bytes initialized to 0.

**SRS_GBALLOC_LL_JEMALLOC_01_005: [** `gballoc_ll_calloc` shall call `je_calloc(nmemb, size)` and return what `je_calloc` returned. **]**


### gballoc_ll_realloc
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);
```

`gballoc_ll_realloc` reallocates `ptr` to have size `size`.

**SRS_GBALLOC_LL_JEMALLOC_01_006: [** `gballoc_ll_realloc` calls `je_realloc(ptr, size)` and returns what `je_realloc` returned. **]**


### gballoc_ll_realloc_2
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_2, void*, ptr, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_2` calls `je_realloc(ptr, nmemb * size)`. This is useful for example when resizing a previously allocated array of elements. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_JEMALLOC_02_006: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_2` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_JEMALLOC_02_007: [** `gballoc_ll_realloc_2` shall return what `je_realloc(ptr, nmemb * size)` returns. **]**


### gballoc_ll_realloc_flex
```c
MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc_flex, void*, ptr, size_t, base, size_t, nmemb, size_t, size);
```

`gballoc_ll_realloc_flex` calls `je_realloc(ptr, base + nmemb * size)`. This is useful when reallocating a structure that has a flexible array member. The function checks for arithmetic overflows.

**SRS_GBALLOC_LL_JEMALLOC_02_008: [** If `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_JEMALLOC_02_009: [** If `base` + `nmemb` * `size` exceeds `SIZE_MAX` then `gballoc_ll_realloc_flex` shall fail and return `NULL`. **]**

**SRS_GBALLOC_LL_JEMALLOC_02_010: [** `gballoc_ll_realloc_flex` shall return what `je_realloc(ptr, base + nmemb * size)` returns. **]**

### gballoc_ll_size
```c
MOCKABLE_FUNCTION(, size_t, gballoc_ll_size, void*, ptr);
```

`gballoc_ll_size` returns the size of the memory block at `ptr`.

**SRS_GBALLOC_LL_JEMALLOC_01_007: [** `gballoc_ll_size` shall call `je_malloc_usable_size` and return what `je_malloc_usable_size` returned. **]**

### gballoc_ll_print_stats

```c
MOCKABLE_FUNCTION(, void, gballoc_ll_print_stats);
```

`gballoc_ll_print_stats` prints the memory allocation statistics.

**SRS_GBALLOC_LL_JEMALLOC_01_008: [** `gballoc_ll_print_stats` shall call `je_malloc_stats_print` and pass to it `jemalloc_print_stats_callback` as print callback. **]**

### jemalloc_print_stats_callback

```c
static void jemalloc_print_stats_callback(void* context, const char* text)
```

`jemalloc_print_stats_callback` is the callback that logs the stats information.

**SRS_GBALLOC_LL_JEMALLOC_01_009: [** If `text` is NULL, `jemalloc_print_stats_callback` shall return. **]**

**SRS_GBALLOC_LL_JEMALLOC_01_010: [** Otherwise, `jemalloc_print_stats_callback` shall print (log) `text`, breaking it does in chunks of `LOG_MAX_MESSAGE_LENGTH / 2`. **]**

### gballoc_ll_set_option

```c
MOCKABLE_FUNCTION(, int, gballoc_ll_set_option, char*, option_name, void*, option_value);
```

`gballoc_ll_set_option` sets the option `option_name` to `option_value`.

**SRS_GBALLOC_LL_JEMALLOC_28_001: [** If `option_name` is `NULL`, `gballoc_ll_set_option` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_LL_JEMALLOC_28_002: [** If `option_value` is `NULL`, `gballoc_ll_set_option` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_LL_JEMALLOC_28_003: [** If `option_name` has value as `dirty_decay`: **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_004: [** `gballoc_ll_set_option` shall fetch the `decay_milliseconds` value by casting `option_value` to `int64_t`. **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_005: [** `gballoc_ll_set_option` shall retrieve the old dirty decay value and set the new dirty decay value to `decay_milliseconds` for new arenas by calling `je_mallctl` with `arenas.dirty_decay_ms` as the command. **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_007: [** `gballoc_ll_set_option` shall fetch the number of existing jemalloc arenas by calling `je_mallctl` with `arenas.narenas` as the command. **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_008: [** For each existing arena except last (since it is reserved for huge arena) **]**

    - **SRS_GBALLOC_LL_JEMALLOC_28_009: [** `gballoc_ll_set_option` shall set the dirty decay time for the arena to `decay_milliseconds` milliseconds by calling `je_mallctl` with `arena.<i>.dirty_decay_ms` as the command. **]**

    - **SRS_GBALLOC_LL_JEMALLOC_28_020: [** If `je_mallctl` returns `EFAULT`, `gballoc_ll_set_option` shall continue without failing as this error is expected when the arena is deleted or is a huge arena. **]**

**SRS_GBALLOC_LL_JEMALLOC_28_010: [** Else if `option_name` has value as `muzzy_decay`: **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_011: [** `gballoc_ll_set_option` shall fetch the `decay_milliseconds` value by casting `option_value` to `int64_t`. **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_012: [** `gballoc_ll_set_option` shall retrieve the old muzzy decay value and set the new muzzy decay value to `decay_milliseconds` for new arenas by calling `je_mallctl` with `arenas.muzzy_decay_ms` as the command. **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_014: [** `gballoc_ll_set_option` shall fetch the number of existing jemalloc arenas by calling `je_mallctl` with `arenas.narenas` as the command. **]**

- **SRS_GBALLOC_LL_JEMALLOC_28_015: [** For each existing arena except last (since it is reserved for huge arena) **]**

    - **SRS_GBALLOC_LL_JEMALLOC_28_016: [** `gballoc_ll_set_option` shall set the muzzy decay time for the arena to `decay_milliseconds` milliseconds by calling `je_mallctl` with `arena.<i>.muzzy_decay_ms` as the command. **]**

    - **SRS_GBALLOC_LL_JEMALLOC_28_020: [** If `je_mallctl` returns `EFAULT`, `gballoc_ll_set_option` shall continue without failing as this error is expected when the arena is deleted or is a huge arena. **]**

**SRS_GBALLOC_LL_JEMALLOC_28_017: [** Otherwise `gballoc_ll_set_option` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_LL_JEMALLOC_28_019: [** If `decay_milliseconds` is less than -1, `gballoc_ll_set_option` shall fail and return a non-zero value. **]**

**SRS_GBALLOC_LL_JEMALLOC_28_018: [** If there are any errors, `gballoc_ll_set_option` shall fail and return a non-zero value. **]**

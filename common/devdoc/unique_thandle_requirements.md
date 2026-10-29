# unique_thandle_requirements

## Overview

`unique_thandle` is a module that provides macros for handles that have a single owner.

`UNIQUE_THANDLE(T)` is a `THANDLE(T)` whose reference count is always 1. `UNIQUE_THANDLE_TYPE_DECLARE` and `UNIQUE_THANDLE_TYPE_DEFINE` declare and define only the functions that move and dispose the handle, and the memory functions of `T`. They do not declare or define `THANDLE_ASSIGN`, `THANDLE_INITIALIZE`, `THANDLE_MOVE` or `THANDLE_INITIALIZE_MOVE` for `T`, so no code can copy a `UNIQUE_THANDLE(T)` through an API, not even the .c file that defines `T`.

Ownership moves only through `UNIQUE_THANDLE(T)*` parameters and through return values. A `UNIQUE_THANDLE(T)` passed by value is borrowed. C cannot prevent a raw copy of a handle (for example `UNIQUE_THANDLE(T) copy = handle;`), and this module does not detect one in general. `UNIQUE_THANDLE_MOVE` only refuses a move when both of its arguments hold the same handle.

A type is either a `THANDLE` or a `UNIQUE_THANDLE`, never both. A `UNIQUE_THANDLE(T)` variable must not be moved or disposed from multiple threads at the same time.

## Exposed API

```c
/*the type of the handle that wraps T*/
#define UNIQUE_THANDLE(T)

/*to be used in a header file*/
#define UNIQUE_THANDLE_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define UNIQUE_THANDLE_TYPE_DEFINE(T)
#define UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)

/*declared by UNIQUE_THANDLE_TYPE_DECLARE*/
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_MOVE(T), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(T), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_DISPOSE(T), UNIQUE_THANDLE(T)*, t);

/*defined by UNIQUE_THANDLE_TYPE_DEFINE for use in the .c file, the same as THANDLE_MALLOC, THANDLE_MALLOC_FLEX, THANDLE_FREE and THANDLE_GET_T*/
static T* UNIQUE_THANDLE_MALLOC(T)(void(*dispose)(T*));
static T* UNIQUE_THANDLE_MALLOC_FLEX(T)(void(*dispose)(T*), size_t nmemb, size_t size);
static void UNIQUE_THANDLE_FREE(T)(T* t);
static T* UNIQUE_THANDLE_GET_T(T)(UNIQUE_THANDLE(T) t);
```

`unique_thandle_ll.h` also provides `UNIQUE_THANDLE_LL_TYPE_DECLARE(C, T)`, `UNIQUE_THANDLE_LL_TYPE_DEFINE(C, T)` and `UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(C, T, malloc_function, malloc_flex_function, free_function)`. Like `THANDLE_LL_TYPE_DECLARE` and `THANDLE_LL_TYPE_DEFINE`, they name the functions after `C` and wrap the type `T`.

## UNIQUE_THANDLE_TYPE_DEFINE(T)

```c
#define UNIQUE_THANDLE_TYPE_DEFINE(T)
```

`UNIQUE_THANDLE_TYPE_DEFINE` defines the functions declared by `UNIQUE_THANDLE_TYPE_DECLARE` and the memory functions `UNIQUE_THANDLE_MALLOC(T)`, `UNIQUE_THANDLE_MALLOC_FLEX(T)`, `UNIQUE_THANDLE_FREE(T)` and `UNIQUE_THANDLE_GET_T(T)`. It does not define `THANDLE_ASSIGN`, `THANDLE_INITIALIZE`, `THANDLE_MOVE` or `THANDLE_INITIALIZE_MOVE` for `T`.

**SRS_UNIQUE_THANDLE_43_001: [** `UNIQUE_THANDLE_TYPE_DEFINE` shall use `malloc`, `malloc_flex` and `free` to allocate and free memory. **]**

## UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)

```c
#define UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)
```

`UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS` behaves like `UNIQUE_THANDLE_TYPE_DEFINE`, with memory functions specific to `T`.

**SRS_UNIQUE_THANDLE_43_002: [** `UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS` shall use `malloc_function`, `malloc_flex_function` and `free_function` to allocate and free memory. **]**

## UNIQUE_THANDLE_MOVE(T)

```c
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_MOVE(T), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);
```

`UNIQUE_THANDLE_MOVE` moves `*t2` into `*t1` and disposes the handle that `*t1` held.

**SRS_UNIQUE_THANDLE_43_003: [** If `t1` is `NULL` then `UNIQUE_THANDLE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_004: [** If `t2` is `NULL` then `UNIQUE_THANDLE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_005: [** If `*t1` is not `NULL` and `*t1` is equal to `*t2` then `UNIQUE_THANDLE_MOVE` shall return without changing `*t1` and `*t2`. **]**

**SRS_UNIQUE_THANDLE_43_006: [** `UNIQUE_THANDLE_MOVE` shall store `*t2` in `*t1` and set `*t2` to `NULL`. **]**

**SRS_UNIQUE_THANDLE_43_007: [** If `*t1` was not `NULL` then, after updating `*t1` and `*t2`, `UNIQUE_THANDLE_MOVE` shall call the `dispose` function of the previous `*t1`, if it is not `NULL`, and free its memory. **]**

## UNIQUE_THANDLE_INITIALIZE_MOVE(T)

```c
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(T), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);
```

`UNIQUE_THANDLE_INITIALIZE_MOVE` moves `*t2` into `*t1`, where `*t1` is not initialized. For example, `*t1` is a field of a structure that has just been allocated.

**SRS_UNIQUE_THANDLE_43_008: [** If `t1` is `NULL` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_009: [** If `t2` is `NULL` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_010: [** If `t1` is equal to `t2` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return without changing `*t1`. **]**

**SRS_UNIQUE_THANDLE_43_011: [** `UNIQUE_THANDLE_INITIALIZE_MOVE` shall store `*t2` in `*t1` and set `*t2` to `NULL`, without disposing the previous content of `*t1`. **]**

## UNIQUE_THANDLE_DISPOSE(T)

```c
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_DISPOSE(T), UNIQUE_THANDLE(T)*, t);
```

`UNIQUE_THANDLE_DISPOSE` disposes the handle in `*t` and sets `*t` to `NULL`.

**SRS_UNIQUE_THANDLE_43_012: [** If `t` is `NULL` then `UNIQUE_THANDLE_DISPOSE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_013: [** If `*t` is `NULL` then `UNIQUE_THANDLE_DISPOSE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_014: [** `UNIQUE_THANDLE_DISPOSE` shall set `*t` to `NULL`. **]**

**SRS_UNIQUE_THANDLE_43_015: [** `UNIQUE_THANDLE_DISPOSE` shall then call the `dispose` function passed when the handle was created, if it is not `NULL`. **]**

**SRS_UNIQUE_THANDLE_43_016: [** `UNIQUE_THANDLE_DISPOSE` shall free the memory of the handle. **]**

# unique_thandle_requirements

## Overview

`unique_thandle` is a module that provides macros for handles that have a single owner.

`UNIQUE_THANDLE(T)` is a handle to an object of type `T` that has exactly one owner at a time. The .c file that defines `T` creates the object with `UNIQUE_THANDLE_MALLOC` or `UNIQUE_THANDLE_MALLOC_FLEX`. Each owner then either moves the handle to the next owner with `UNIQUE_THANDLE_INITIALIZE_MOVE`, or disposes the object with `UNIQUE_THANDLE_DISPOSE`. These two are the only functions that change a handle, even in the .c file that defines `T`, so an object is disposed only by `UNIQUE_THANDLE_DISPOSE` and keeps a single owner.

Ownership moves only through `UNIQUE_THANDLE(T)*` parameters and through return values. A `UNIQUE_THANDLE(T)` passed by value is borrowed. C allows a raw copy of a handle, for example `UNIQUE_THANDLE(T) copy = handle;`. A raw copy gives the object a second owner, so it is a bug.

Only one thread at a time may move or dispose a given `UNIQUE_THANDLE(T)` variable.

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
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(T), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_DISPOSE(T), UNIQUE_THANDLE(T)*, t);

/*defined by UNIQUE_THANDLE_TYPE_DEFINE for use in the .c file that defines T*/
/*allocates a T. dispose, if it is not NULL, is called when the object is disposed*/
static T* UNIQUE_THANDLE_MALLOC(T)(void(*dispose)(T*));
/*allocates a T with an additional nmemb * size bytes for its flexible array member*/
static T* UNIQUE_THANDLE_MALLOC_FLEX(T)(void(*dispose)(T*), size_t nmemb, size_t size);
/*frees the memory of a T allocated by UNIQUE_THANDLE_MALLOC or UNIQUE_THANDLE_MALLOC_FLEX, for example when creating the object fails*/
static void UNIQUE_THANDLE_FREE(T)(T* t);
/*returns a pointer to the T of a handle, or NULL if t is NULL*/
static T* UNIQUE_THANDLE_GET_T(T)(UNIQUE_THANDLE(T) t);
```

`unique_thandle_ll.h` also provides `UNIQUE_THANDLE_LL_TYPE_DECLARE(C, T)`, `UNIQUE_THANDLE_LL_TYPE_DEFINE(C, T)` and `UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(C, T, malloc_function, malloc_flex_function, free_function)`. They name the functions after `C` and wrap the type `T`.

## UNIQUE_THANDLE_TYPE_DEFINE(T)

```c
#define UNIQUE_THANDLE_TYPE_DEFINE(T)
```

`UNIQUE_THANDLE_TYPE_DEFINE` defines the functions declared by `UNIQUE_THANDLE_TYPE_DECLARE` and the memory functions `UNIQUE_THANDLE_MALLOC(T)`, `UNIQUE_THANDLE_MALLOC_FLEX(T)`, `UNIQUE_THANDLE_FREE(T)` and `UNIQUE_THANDLE_GET_T(T)`.

**SRS_UNIQUE_THANDLE_43_001: [** `UNIQUE_THANDLE_TYPE_DEFINE` shall use `malloc`, `malloc_flex` and `free` to allocate and free memory. **]**

## UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)

```c
#define UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)
```

`UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS` behaves like `UNIQUE_THANDLE_TYPE_DEFINE`, with memory functions specific to `T`.

**SRS_UNIQUE_THANDLE_43_002: [** `UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS` shall use `malloc_function`, `malloc_flex_function` and `free_function` to allocate and free memory. **]**

## UNIQUE_THANDLE_INITIALIZE_MOVE(T)

```c
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(T), UNIQUE_THANDLE(T)*, t1, UNIQUE_THANDLE(T)*, t2);
```

`UNIQUE_THANDLE_INITIALIZE_MOVE` moves `*t2` into `*t1`, where `*t1` is either `NULL` or not initialized, for example a field of a structure that has just been allocated.

To move into a handle that holds an object, first dispose that object with `UNIQUE_THANDLE_DISPOSE` or move it to another handle. If the object owns `*t2`, for example `t2` points to one of its fields, first move `*t2` into a local handle, then dispose the object, then move the local handle into `*t1`.

To set a handle that is not initialized to `NULL`, move a `NULL` handle into it.

**SRS_UNIQUE_THANDLE_43_008: [** If `t1` is `NULL` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_009: [** If `t2` is `NULL` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_010: [** If `t1` is equal to `t2` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_011: [** `UNIQUE_THANDLE_INITIALIZE_MOVE` shall store `*t2` in `*t1` and set `*t2` to `NULL`. **]**

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

# unique_thandle_requirements

## Overview

`unique_thandle` is a module that provides macros for handles that have a single owner.

`UNIQUE_THANDLE(T)` is a handle to an object of type `T` that has exactly one owner at a time. A handle either holds an object or is `NULL`. The .c file that defines `T` creates the object with `UNIQUE_THANDLE_MALLOC` or `UNIQUE_THANDLE_MALLOC_FLEX` and puts it in a handle with `UNIQUE_THANDLE_ADOPT`. Each owner then either moves the object to the next owner with `UNIQUE_THANDLE_INITIALIZE_MOVE`, or disposes the object with `UNIQUE_THANDLE_DISPOSE`.

A `UNIQUE_THANDLE(T)` is an array of one element, like `jmp_buf`. A function receives a handle as a pointer to the caller's handle, so a call passes the handle itself, as in `UNIQUE_THANDLE_DISPOSE(T)(handle)`, and in the functions below `t1`, `t2` and `t` point to handles. A handle variable is set to `NULL` with `UNIQUE_THANDLE(T) handle = { { NULL } };`.

Functions have two kinds of handle parameters:

- A function that takes the object, or that gives an object to its caller, has a `UNIQUE_THANDLE(T)` parameter.
- A function that uses the object only during the call has a `const UNIQUE_THANDLE(T)` parameter.

A function uses a handle parameter during the call. To keep the object, it moves the object into a handle that it owns.

For example, for a type `ITEM` and a list that owns items:

```c
int item_create(int value, UNIQUE_THANDLE(ITEM) item);     /*puts a new item in item*/
int item_get_value(const UNIQUE_THANDLE(ITEM) item);       /*uses the item during the call*/
int list_add(LIST_HANDLE list, UNIQUE_THANDLE(ITEM) item); /*takes the item when it succeeds*/

/*a caller*/
UNIQUE_THANDLE(ITEM) item = { { NULL } };
if (item_create(42, item) == 0)
{
    int value = item_get_value(item);   /*the caller still owns the item*/
    int result = list_add(list, item);  /*on success, the list owns the item and item is NULL*/
    UNIQUE_THANDLE_DISPOSE(ITEM)(item); /*disposes the item when list_add failed*/
}

/*item_create, when it succeeds*/
UNIQUE_THANDLE_ADOPT(ITEM)(item, new_item);

/*list_add, when it succeeds*/
UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(list->items[list->count], item);
```

The compiler rejects `UNIQUE_THANDLE(T) copy = handle;`, assignments of handles and functions that return a handle. With warnings treated as errors, as in c-pal builds, it also rejects moving or disposing a handle through a `const UNIQUE_THANDLE(T)`.

Only one thread at a time may move or dispose a given `UNIQUE_THANDLE(T)` variable.

## Exposed API

```c
/*the type of a handle to a T*/
#define UNIQUE_THANDLE(T)

/*to be used in a header file*/
#define UNIQUE_THANDLE_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define UNIQUE_THANDLE_TYPE_DEFINE(T)
#define UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)

/*declared by UNIQUE_THANDLE_TYPE_DECLARE*/
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(T), UNIQUE_THANDLE(T), t1, UNIQUE_THANDLE(T), t2);
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_DISPOSE(T), UNIQUE_THANDLE(T), t);

/*defined by UNIQUE_THANDLE_TYPE_DEFINE for use in the .c file that defines T*/
/*allocates a T. dispose, if it is not NULL, is called when the object is disposed*/
static T* UNIQUE_THANDLE_MALLOC(T)(void(*dispose)(T*));
/*allocates a T with an additional nmemb * size bytes for its flexible array member*/
static T* UNIQUE_THANDLE_MALLOC_FLEX(T)(void(*dispose)(T*), size_t nmemb, size_t size);
/*frees the memory of a T allocated by UNIQUE_THANDLE_MALLOC or UNIQUE_THANDLE_MALLOC_FLEX, for example when creating the object fails*/
static void UNIQUE_THANDLE_FREE(T)(T* t);
/*puts a new object, which UNIQUE_THANDLE_MALLOC or UNIQUE_THANDLE_MALLOC_FLEX returned, in *t, which is NULL or not initialized*/
static void UNIQUE_THANDLE_ADOPT(T)(UNIQUE_THANDLE(T) t, T* object);
/*returns a pointer to the T that *t holds, or NULL if t is NULL*/
static T* UNIQUE_THANDLE_GET_T(T)(const UNIQUE_THANDLE(T) t);
```

`unique_thandle_ll.h` also provides `UNIQUE_THANDLE_MACRO(T)`, `UNIQUE_THANDLE_LL_TYPE_DECLARE(C, T)`, `UNIQUE_THANDLE_LL_TYPE_DEFINE(C, T)` and `UNIQUE_THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(C, T, malloc_function, malloc_flex_function, free_function)`. `UNIQUE_THANDLE_MACRO(T)` introduces `UNIQUE_THANDLE(T)`, once per translation unit. The other three name the functions after `C` and wrap the type `T`. `UNIQUE_THANDLE_TYPE_DECLARE(T)` is `UNIQUE_THANDLE_MACRO(T)` followed by `UNIQUE_THANDLE_LL_TYPE_DECLARE(T, T)`.

## UNIQUE_THANDLE_TYPE_DEFINE(T)

```c
#define UNIQUE_THANDLE_TYPE_DEFINE(T)
```

`UNIQUE_THANDLE_TYPE_DEFINE` defines the functions declared by `UNIQUE_THANDLE_TYPE_DECLARE`, the functions `UNIQUE_THANDLE_MALLOC(T)`, `UNIQUE_THANDLE_MALLOC_FLEX(T)`, `UNIQUE_THANDLE_FREE(T)`, `UNIQUE_THANDLE_ADOPT(T)` and `UNIQUE_THANDLE_GET_T(T)`, and `UNIQUE_THANDLE_INSPECT(T)`, which shows the object of a handle in a debugger.

**SRS_UNIQUE_THANDLE_43_001: [** `UNIQUE_THANDLE_TYPE_DEFINE` shall use `malloc`, `malloc_flex` and `free` to allocate and free memory. **]**

## UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)

```c
#define UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function)
```

`UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS` behaves like `UNIQUE_THANDLE_TYPE_DEFINE`, with memory functions specific to `T`.

**SRS_UNIQUE_THANDLE_43_002: [** `UNIQUE_THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS` shall use `malloc_function`, `malloc_flex_function` and `free_function` to allocate and free memory. **]**

## UNIQUE_THANDLE_INITIALIZE_MOVE(T)

```c
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_INITIALIZE_MOVE(T), UNIQUE_THANDLE(T), t1, UNIQUE_THANDLE(T), t2);
```

`UNIQUE_THANDLE_INITIALIZE_MOVE` moves `*t2` into `*t1`, where `*t1` is either `NULL` or not initialized, for example a field of a structure that has just been allocated.

To set a handle that is not initialized to `NULL`, move a `NULL` handle into it.

**SRS_UNIQUE_THANDLE_43_008: [** If `t1` is `NULL` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_009: [** If `t2` is `NULL` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_010: [** If `t1` is equal to `t2` then `UNIQUE_THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_011: [** `UNIQUE_THANDLE_INITIALIZE_MOVE` shall store `*t2` in `*t1` and set `*t2` to `NULL`. **]**

## UNIQUE_THANDLE_DISPOSE(T)

```c
MOCKABLE_FUNCTION(, void, UNIQUE_THANDLE_DISPOSE(T), UNIQUE_THANDLE(T), t);
```

`UNIQUE_THANDLE_DISPOSE` disposes the handle in `*t` and sets `*t` to `NULL`.

**SRS_UNIQUE_THANDLE_43_012: [** If `t` is `NULL` then `UNIQUE_THANDLE_DISPOSE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_013: [** If `*t` is `NULL` then `UNIQUE_THANDLE_DISPOSE` shall return. **]**

**SRS_UNIQUE_THANDLE_43_014: [** `UNIQUE_THANDLE_DISPOSE` shall set `*t` to `NULL`. **]**

**SRS_UNIQUE_THANDLE_43_015: [** `UNIQUE_THANDLE_DISPOSE` shall then call the `dispose` function passed when the handle was created, if it is not `NULL`. **]**

**SRS_UNIQUE_THANDLE_43_016: [** `UNIQUE_THANDLE_DISPOSE` shall free the memory of the handle. **]**

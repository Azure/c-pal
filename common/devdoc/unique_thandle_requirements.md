# unique_thandle_requirements

## Overview

`unique_thandle` is a module that provides macros for handles that have a single owner.

`UNIQUE_THANDLE(T)` is a handle to an object of type `T` that has exactly one owner at a time. A handle either holds an object or is `NULL`.

The .c file that defines `T` creates the object with `UNIQUE_THANDLE_MALLOC` or `UNIQUE_THANDLE_MALLOC_FLEX` and puts it in a handle with `UNIQUE_THANDLE_ADOPT`:

```c
/*item.h*/
typedef struct ITEM_TAG ITEM;

UNIQUE_THANDLE_TYPE_DECLARE(ITEM);

int item_create(int value, UNIQUE_THANDLE(ITEM) item);

/*item.c*/
struct ITEM_TAG
{
    int value;
};

UNIQUE_THANDLE_TYPE_DEFINE(ITEM);

int item_create(int value, UNIQUE_THANDLE(ITEM) item)
{
    int result;
    ITEM* new_item = UNIQUE_THANDLE_MALLOC(ITEM)(NULL);
    if (new_item == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        new_item->value = value;
        UNIQUE_THANDLE_ADOPT(ITEM)(item, new_item);
        result = 0;
    }
    return result;
}
```

The object can be moved with `UNIQUE_THANDLE_INITIALIZE_MOVE` or disposed with `UNIQUE_THANDLE_DISPOSE`.

```c
/*list.h*/
typedef struct NODE_TAG
{
    struct NODE_TAG* next;
    UNIQUE_THANDLE(ITEM) item;
} NODE;

typedef struct LIST_TAG
{
    NODE head; /*holds no item*/
    NODE* end; /*the last node, &head when the list is empty*/
} LIST;

typedef LIST* LIST_HANDLE;

int list_insert_num(LIST_HANDLE list, int n);
int list_insert_item(LIST_HANDLE list, UNIQUE_THANDLE(ITEM) item);

/*list.c*/
int list_insert_num(LIST_HANDLE list, int n)
{
    int result;

    UNIQUE_THANDLE(ITEM) item = UNIQUE_THANDLE_NULL;
    if (item_create(n, item) != 0)
    {
        result = MU_FAILURE;
    }
    else
    {
        if (list_insert_item(list, item) != 0)
        {
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
            goto all_ok;
        }
        UNIQUE_THANDLE_DISPOSE(ITEM)(item);
    }
all_ok:
    return result;
}

int list_insert_item(LIST_HANDLE list, UNIQUE_THANDLE(ITEM) item)
{
    int result;
    if (list == NULL || item == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        NODE* node = malloc(sizeof(NODE));
        if (node == NULL)
        {
            result = MU_FAILURE;
        }
        else
        {
            node->next = NULL;
            UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(node->item, item);
            list->end->next = node;
            list->end = node;
            result = 0;
        }
    }
    return result;
}
```

The compiler rejects the following misuses:

```c
/*other.h*/
typedef struct OTHER_TAG OTHER;

UNIQUE_THANDLE_TYPE_DECLARE(OTHER);

/*item.c*/
void misuses(UNIQUE_THANDLE(ITEM) item, UNIQUE_THANDLE(OTHER) other)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;
    ITEM* pointer = UNIQUE_THANDLE_GET_T(ITEM)(item);

    /*copies a handle: copy and item would both own the item*/
    UNIQUE_THANDLE(ITEM) copy = item;

    /*assigns a handle: temp and item would both own the item*/
    temp = item;

    /*writes the pointer in a handle: temp and item would both own the item*/
    temp[0].object = item[0].object;

    /*warnings as errors: makes a handle from a pointer to an item, which a handle may already own*/
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, &pointer);

    /*warnings as errors: moves a handle to another type, OTHER, into a handle to an ITEM*/
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, other);
}

/*list.h*/
/*returns a handle: the caller and the list would both own the item*/
UNIQUE_THANDLE(ITEM) list_get(LIST_HANDLE list, uint32_t index);

/*list.c*/
/*node_get_value reads a node*/
int node_get_value(const NODE* node)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;

    /*warnings as errors: moves the item, which the node owns, into temp*/
    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, node->item);
    ...
}
```

Read-only copies should be passed as `const`:

```c
/*item_get_value uses the item during the call*/
int item_get_value(const UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;

    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, item); /*warnings as errors: does not compile*/

    UNIQUE_THANDLE_DISPOSE(ITEM)(item); /*warnings as errors: does not compile*/

    ITEM* item_ptr = UNIQUE_THANDLE_GET_T(ITEM)(item);
    return item_ptr->value;
}
```

If a read-only copy is passed as non-const and the copy is modified, the original is also changed:

```c
int item_get_value(UNIQUE_THANDLE(ITEM) item)
{
    UNIQUE_THANDLE(ITEM) temp = UNIQUE_THANDLE_NULL;

    UNIQUE_THANDLE_INITIALIZE_MOVE(ITEM)(temp, item);

    ITEM* item_ptr = UNIQUE_THANDLE_GET_T(ITEM)(temp);

    return item_ptr->value;
}

int caller(void)
{
    UNIQUE_THANDLE(ITEM) item = UNIQUE_THANDLE_NULL;
    item_create(42, item);

    int val = item_get_value(item);

    /*item is now NULL: item_get_value moved the item into temp, which leaked it. Another call to item_get_value dereferences NULL*/

    int val2 = item_get_value(item);

    return val + val2;
}
```

Only one thread at a time may move or dispose a given `UNIQUE_THANDLE(T)` variable.

## Exposed API

```c
/*the type of a handle to a T*/
#define UNIQUE_THANDLE(T)

/*initializes a handle to NULL, as in UNIQUE_THANDLE(T) handle = UNIQUE_THANDLE_NULL;*/
#define UNIQUE_THANDLE_NULL

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

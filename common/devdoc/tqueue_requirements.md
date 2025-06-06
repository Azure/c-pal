# `tqueue` requirements

## Overview

`TQUEUE` is a module that implement a typed queue that holds elements of a certain type. `TQUEUE` is homogenous, all elements are of the same type.

`TQUEUE` is thread safe (push/pop calls are thread safe).

`TQUEUE` aims at allowing producers and consumers to make independent progress as much as possible (producers can push in the queue while the queue is not full without getting blocked).

Same for consumers: consumers can pop from the queue without getting blocked on producers while there are items in the queue.

The module provides the following functionality:

- Create a queue with a given size.
- Push an element at the head of the queue.
- Pop an element from the tail of the queue. Pop returns the oldest element in the queue.

The module allows the user to specify 3 different callback functions:

- A copy item callback (which is invoked by the queue as a result of a push call or a pop call, allowing the user to fill in the information in/from T rather than performing a memory copy).
A typical use for a queue of a `THANDLE` would be to have a `THANDLE_INITIALIZE`, initializing the `dst` with the `src`.

- A dispose item function (which is invoked by the queue when the queue is disposed and there are still items in the queue).
A typical use for a queue of a `THANDLE` would be to have a `THANDLE_ASSIGN` with `NULL` performed in the dispose function callback.

- A condition function used for allowing the pop of an item. If the called condition function returns `true`, the pop is performed. If it returns `false`, the pop is not performed.

If no push/pop/dispose callbacks functions are specified by the user, the queue copies the memory of T passed to `TQUEUE_PUSH`, respectively `TQUEUE_POP`.

The callbacks used for copy item/dispose item/pop condition are not re-entrant, they should not call `TQUEUE` APIs on the same queue.

Because `TQUEUE` is a kind of `THANDLE`, all of `THANDLE`'s APIs apply to `TQUEUE`. For convenience the following macros are provided out of the box with the same semantics as those of `THANDLE`'s:

`TQUEUE_INITIALIZE(T)`
`TQUEUE_ASSIGN(T)`
`TQUEUE_MOVE(T)`
`TQUEUE_INITIALIZE_MOVE(T)`

`TQUEUE` queues grow by doubling their capacity when there are no more entries available, up to the `max_queue_size` specified when the queue is created.

## Design

`TQUEUE` uses an array of type `T` to store the elements in the queue.

The head and tail of the queue are `int64_t` values accessed through `interlocked_xx_64` functions.

When pushing, the head value modulo queue size is used as array index where the element is written.

When popping, the tail value modulo queue size is used as array index where the element is read from.

A state is associated with each array entry, stored as an `int32_t` accessed through `interlocked_xx` functions.

The possible states for an array entry are:

- `NOT_USED` - No data exists at the entry.
- `PUSHING` - Data is currently being written at the entry due to a push. Data is not safe to be popped yet.
- `USED` - Data exists in the entry can can be popped.
- `POPPING` - Data is being popped and should not be popped by any other thread.

## Exposed API

```c

#define TQUEUE_PUSH_RESULT_VALUES \
    TQUEUE_PUSH_OK, \
    TQUEUE_PUSH_INVALID_ARG, \
    TQUEUE_PUSH_QUEUE_FULL, \
    TQUEUE_PUSH_ERROR \

MU_DEFINE_ENUM(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_RESULT_VALUES);

#define TQUEUE_POP_RESULT_VALUES \
    TQUEUE_POP_OK, \
    TQUEUE_POP_INVALID_ARG, \
    TQUEUE_POP_QUEUE_EMPTY, \
    TQUEUE_POP_REJECTED

MU_DEFINE_ENUM(TQUEUE_POP_RESULT, TQUEUE_POP_RESULT_VALUES);

/*to be used as the type of handle*/
#define TQUEUE(T)

/*to be used in a header file*/
#define TQUEUE_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define TQUEUE_TYPE_DEFINE(T)
```

The macros expand to these useful somewhat more useful APIs:

```c
TQUEUE(T) TQUEUE_CREATE(T)(uint32_t initial_queue_size, uint32_t max_queue_size, TQUEUE_COPY_ITEM_FUNC(T) copy_item_function, TQUEUE_DISPOSE_ITEM_FUNC(T) dispose_item_function, void* dispose_item_function_context);
int TQUEUE_PUSH(T)(TQUEUE(T) tqueue, T* item, void* copy_function_context)
TQUEUE_POP_RESULT TQUEUE_POP(T)(TQUEUE(T) tqueue, T* item, void* copy_function_context, TQUEUE_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T), condition_function, void*, condition_function_context);
int64_t TQUEUE_GET_VOLATILE_COUNT(T)(TQUEUE(T) tqueue)
```

The signature of the push callback function `TQUEUE_COPY_ITEM_FUNC(T)` is:

```c
void TQUEUE_DEFINE_COPY_ITEM_FUNCTION_TYPE_NAME(T)(void* context, T* push_dst, T* push_src);
```

The signature of the dispose function `TQUEUE_DISPOSE_ITEM_FUNC(T)` is:

```c
void TQUEUE_DEFINE_DISPOSE_FUNCTION_TYPE_NAME(T)(void* context, T* item);
```

The signature of the condition function `TQUEUE_CONDITION_FUNC(T)` is:

```c
bool TQUEUE_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T)(void* context, T* item);
```

It returns `true` if the condition is satisfied and the pop should be performed, `false` otherwise.

### TQUEUE(T)

```c
#define TQUEUE(T) 
```
`TQUEUE(T)` is a `THANDLE`(`TQUEUE_STRUCT_T`), where `TQUEUE_STRUCT_T` is a structure introduced like below:
```c
typedef struct TQUEUE_STRUCT_T_TAG
{
    ...
} TQUEUE_STRUCT_T;
```

### TQUEUE_TYPE_DECLARE(T)
```c
#define TQUEUE_TYPE_DECLARE(T)
```

`TQUEUE_TYPE_DECLARE(T)` is a macro to be used in a header declaration.

It introduces the APIs (as MOCKABLE_FUNCTIONS) that can be called for a `TQUEUE`.

Example usage:

```c
TQUEUE_TYPE_DECLARE(int32_t);
```

### TQUEUE_TYPE_DEFINE(T)
```c
#define TQUEUE_TYPE_DEFINE(T)
```

`TQUEUE_TYPE_DEFINE(T)` is a macro to be used in a .c file to define all the needed functions for `TQUEUE(T)`.

Example usage:

```c
TQUEUE_TYPE_DEFINE(int32_t);
```

### TQUEUE_CREATE(T)
```c
TQUEUE(T) TQUEUE_CREATE(T)(uint32_t initial_queue_size, uint32_t max_queue_size, TQUEUE_COPY_ITEM_FUNC(T) copy_item_function, TQUEUE_DISPOSE_ITEM_FUNC(T) dispose_item_function, void* dispose_item_function_context);
```

`TQUEUE_CREATE(T)` creates a new `TQUEUE(T)` which doubles in size when it reaches capacity.

**SRS_TQUEUE_01_046: [** If `initial_queue_size` is 0, `TQUEUE_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_TQUEUE_01_047: [** If `initial_queue_size` is greater than `max_queue_size`, `TQUEUE_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_TQUEUE_01_048: [** If any of `copy_item_function` and `dispose_item_function` are `NULL` and at least one of them is not `NULL`, `TQUEUE_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_TQUEUE_01_049: [** `TQUEUE_CREATE(T)` shall call `THANDLE_MALLOC` with `TQUEUE_DISPOSE_FUNC(T)` as dispose function. **]**

**SRS_TQUEUE_01_050: [** `TQUEUE_CREATE(T)` shall allocate memory for an array of size `size` containing elements of type `T`. **]**

**SRS_TQUEUE_01_051: [** `TQUEUE_CREATE(T)` shall initialize the head and tail of the list with 0 by using `interlocked_exchange_64`. **]**

**SRS_TQUEUE_01_052: [** `TQUEUE_CREATE(T)` shall initialize the state for each entry in the array used for the queue with `NOT_USED` by using `interlocked_exchange`. **]**

**SRS_TQUEUE_01_053: [** `TQUEUE_CREATE(T)` shall initialize a `SRW_LOCK_LL` to be used for locking the queue when it needs to grow in size. **]**

**SRS_TQUEUE_01_054: [** `TQUEUE_CREATE(T)` shall succeed and return a non-`NULL` value. **]**

**SRS_TQUEUE_01_071: [** If there are any failures then `TQUEUE_CREATE(T)` shall fail and return `NULL`. **]**

### TQUEUE_DISPOSE_FUNC(T)
```c
void TQUEUE_DISPOSE_FUNC(T)(TQUEUE(T) tqueue);
```

`TQUEUE_DISPOSE_FUNC(T)` is called when there are no more references to the queue and the contents of it should be disposed of.

**SRS_TQUEUE_01_008: [** If `dispose_item_function` passed to `TQUEUE_CREATE(T)` is `NULL`, `TQUEUE_DISPOSE_FUNC(T)` shall return. **]**

**SRS_TQUEUE_01_009: [** Otherwise, `TQUEUE_DISPOSE_FUNC(T)` shall obtain the current queue head by calling `interlocked_add_64`. **]**

**SRS_TQUEUE_01_010: [** `TQUEUE_DISPOSE_FUNC(T)` shall obtain the current queue tail by calling `interlocked_add_64`. **]**

**SRS_TQUEUE_01_011: [** For each item in the queue, `dispose_item_function` shall be called with `dispose_item_function_context` and a pointer to the array entry value (T*). **]**

- **SRS_TQUEUE_01_056: [** The lock initialized in `TQUEUE_CREATE(T)` shall be de-initialized. **]**

- **SRS_TQUEUE_01_057: [** The array backing the queue shall be freed. **]**

### TQUEUE_PUSH(T)
```c
TQUEUE_PUSH_RESULT TQUEUE_PUSH(T)(TQUEUE(T) tqueue, T* item, void* copy_item_function_context)
```

`TQUEUE_PUSH(T)` pushes an item in the queue at the head.

**SRS_TQUEUE_01_012: [** If `tqueue` is `NULL` then `TQUEUE_PUSH(T)` shall fail and return `TQUEUE_PUSH_INVALID_ARG`. **]**

**SRS_TQUEUE_01_013: [** If `item` is `NULL` then `TQUEUE_PUSH(T)` shall fail and return `TQUEUE_PUSH_INVALID_ARG`. **]**

**SRS_TQUEUE_01_058: [** `TQUEUE_PUSH(T)` shall acquire in shared mode the lock used to guard the growing of the queue. **]**

**SRS_TQUEUE_01_014: [** `TQUEUE_PUSH(T)` shall execute the following actions until it is either able to push the item in the queue or the queue is full: **]**

- **SRS_TQUEUE_01_015: [** `TQUEUE_PUSH(T)` shall obtain the current head queue by calling `interlocked_add_64`. **]**

- **SRS_TQUEUE_01_016: [** `TQUEUE_PUSH(T)` shall obtain the current tail queue by calling `interlocked_add_64`. **]**

- **SRS_TQUEUE_01_060: [** If the queue is full (current head >= current tail + queue size): **]**

  - **SRS_TQUEUE_01_061: [** If the current queue size is equal to the max queue size, `TQUEUE_PUSH(T)` shall return `TQUEUE_PUSH_QUEUE_FULL`. **]**

  - **SRS_TQUEUE_01_062: [** If the current queue size is less than the max queue size: **]**

    - **SRS_TQUEUE_01_063: [** `TQUEUE_PUSH(T)` shall release in shared mode the lock used to guard the growing of the queue. **]**

    - **SRS_TQUEUE_01_064: [** `TQUEUE_PUSH(T)` shall acquire in exclusive mode the lock used to guard the growing of the queue. **]**

    - **SRS_TQUEUE_01_074: [** If the size of the queue did not change after acquiring the lock in shared mode: **]**

      - **SRS_TQUEUE_01_075: [** `TQUEUE_PUSH(T)` shall obtain again the current head or the queue. **]**

      - **SRS_TQUEUE_01_076: [** `TQUEUE_PUSH(T)` shall obtain again the current tail or the queue. **]**
    
      - **SRS_TQUEUE_01_067: [** `TQUEUE_PUSH(T)` shall double the size of the queue. **]**

      - **SRS_TQUEUE_01_070: [** If the newly computed queue size is higher than the `max_queue_size` value passed to `TQUEUE_CREATE(T)`, `TQUEUE_PUSH(T)` shall use `max_queue_size` as the new queue size. **]**

      - **SRS_TQUEUE_01_068: [** `TQUEUE_PUSH(T)` shall reallocate the array used to store the queue items based on the newly computed size. **]**

      - **SRS_TQUEUE_01_077: [** `TQUEUE_PUSH(T)` shall move the entries between the tail index and the array end like below: **]**

        - **SRS_TQUEUE_01_078: [** Entries at the tail shall be moved to the end of the resized array **]**

      Before resize:

      T = 2
      H = 5

      [X HO TX X]

      After resize (doubling from 4 to 8):

      T = 6
      H = 9

      [X HO O O O O TX X]

      Legend:
      O - unused
      X - used
      H - head
      T - tail

      - **SRS_TQUEUE_01_065: [** `TQUEUE_PUSH(T)` shall release in exclusive mode the lock used to guard the growing of the queue. **]**

      - **SRS_TQUEUE_01_069: [** If reallocation fails, `TQUEUE_PUSH(T)` shall return `TQUEUE_PUSH_ERROR`. **]**

    - **SRS_TQUEUE_01_066: [** `TQUEUE_PUSH(T)` shall acquire in shared mode the lock used to guard the growing of the queue and retry the `TQUEUE_PUSH(T)`. **]**

- **SRS_TQUEUE_01_017: [** Using `interlocked_compare_exchange`, `TQUEUE_PUSH(T)` shall change the head array entry state to `PUSHING` (from `NOT_USED`). **]**

  - **SRS_TQUEUE_01_023: [** If the state of the array entry corresponding to the head is not `NOT_USED`, `TQUEUE_PUSH(T)` shall retry the whole push. **]**

- **SRS_TQUEUE_01_018: [** Using `interlocked_compare_exchange_64`, `TQUEUE_PUSH(T)` shall replace the head value with the head value obtained earlier + 1. **]**

- **SRS_TQUEUE_01_043: [** If the queue head has changed, `TQUEUE_PUSH(T)` shall set the state back to `NOT_USED` and retry the push. **]**

- **SRS_TQUEUE_01_019: [** If no `copy_item_function` was specified in `TQUEUE_CREATE(T)`, `TQUEUE_PUSH(T)` shall copy the value of `item` into the array entry value whose state was changed to `PUSHING`. **]**

- **SRS_TQUEUE_01_024: [** If a `copy_item_function` was specified in `TQUEUE_CREATE(T)`, `TQUEUE_PUSH(T)` shall call the `copy_item_function` with `copy_item_function_context` as `context`, a pointer to the array entry value whose state was changed to `PUSHING` as `push_dst` and `item` as `push_src`. **]**

- **SRS_TQUEUE_01_020: [** `TQUEUE_PUSH(T)` shall set the state to `USED` by using `interlocked_exchange`. **]**

- **SRS_TQUEUE_01_021: [** `TQUEUE_PUSH(T)` shall succeed and return `TQUEUE_PUSH_OK`. **]**

**SRS_TQUEUE_01_059: [** `TQUEUE_PUSH(T)` shall release in shared mode the lock used to guard the growing of the queue. **]**

### TQUEUE_POP(T)
```c
TQUEUE_POP_RESULT TQUEUE_POP(T)(TQUEUE(T) tqueue, T* item, void* pop_function_context, TQUEUE_CONDITION_FUNC(T), condition_function, void*, condition_function_context);
```

`TQUEUE_POP(T)` pops an item from the queue (from the tail) if available.

**SRS_TQUEUE_01_025: [** If `tqueue` is `NULL` then `TQUEUE_POP(T)` shall fail and return `TQUEUE_POP_INVALID_ARG`. **]**

**SRS_TQUEUE_01_027: [** If `item` is `NULL` then `TQUEUE_POP(T)` shall fail and return `TQUEUE_POP_INVALID_ARG`. **]**

**SRS_TQUEUE_01_072: [** `TQUEUE_POP(T)` shall acquire in shared mode the lock used to guard the growing of the queue. **]**

**SRS_TQUEUE_01_026: [** `TQUEUE_POP(T)` shall execute the following actions until it is either able to pop the item from the queue or the queue is empty: **]**

- **SRS_TQUEUE_01_028: [** `TQUEUE_POP(T)` shall obtain the current head queue by calling `interlocked_add_64`. **]**

- **SRS_TQUEUE_01_029: [** `TQUEUE_POP(T)` shall obtain the current tail queue by calling `interlocked_add_64`. **]**

- **SRS_TQUEUE_01_035: [** If the queue is empty (current tail >= current head), `TQUEUE_POP(T)` shall return `TQUEUE_POP_QUEUE_EMPTY`. **]**

- **SRS_TQUEUE_01_030: [** Using `interlocked_compare_exchange`, `TQUEUE_PUSH(T)` shall set the tail array entry state to `POPPING` (from `USED`). **]**

  - **SRS_TQUEUE_01_036: [** If the state of the array entry corresponding to the tail is not `USED`, `TQUEUE_POP(T)` shall try again. **]**

  - **SRS_TQUEUE_01_039: [** If `condition_function` is not `NULL`: **]**

    - **SRS_TQUEUE_01_040: [** `TQUEUE_POP(T)` shall call `condition_function` with `condition_function_context` and a pointer to the array entry value whose state was changed to `POPPING`. **]**

    - **SRS_TQUEUE_01_041: [** If `condition_function` returns `false`, `TQUEUE_POP(T)` shall set the state to `USED` by using `interlocked_exchange` and return `TQUEUE_POP_REJECTED`. **]**

    - **SRS_TQUEUE_01_042: [** Otherwise, shall proceed with the pop. **]**

  - **SRS_TQUEUE_01_031: [** `TQUEUE_POP(T)` shall replace the tail value with the tail value obtained earlier + 1 by using `interlocked_compare_exchange_64`. **]**

    - **SRS_TQUEUE_01_044: [** If incrementing the tail by using `interlocked_compare_exchange_64` does not succeed, `TQUEUE_POP(T)` shall revert the state of the array entry to `USED` and retry. **]**

  - **SRS_TQUEUE_01_032: [** If a `copy_item_function` was not specified in `TQUEUE_CREATE(T)`: **]**
  
    - **SRS_TQUEUE_01_033: [** `TQUEUE_POP(T)` shall copy array entry value whose state was changed to `POPPING` to `item`. **]**

  - **SRS_TQUEUE_01_037: [** If `copy_item_function` and `sispose_item_function` were specified in `TQUEUE_CREATE(T)`: **]**
  
    - **SRS_TQUEUE_01_038: [** `TQUEUE_POP(T)` shall call `copy_item_function` with `copy_item_function_context` as `context`, the array entry value whose state was changed to `POPPING` to `item` as `pop_src` and `item` as `pop_dst`. **]**

    - **SRS_TQUEUE_01_045: [** `TQUEUE_POP(T)` shall call `dispose_item_function` with `dispose_item_function_context` as `context` and the array entry value whose state was changed to `POPPING` as `item`. **]**

  - **SRS_TQUEUE_01_034: [** `TQUEUE_POP(T)` shall set the state to `NOT_USED` by using `interlocked_exchange`, succeed and return `TQUEUE_POP_OK`. **]**

**SRS_TQUEUE_01_073: [** `TQUEUE_POP(T)` shall release in shared mode the lock used to guard the growing of the queue. **]**

### TQUEUE_GET_VOLATILE_COUNT(T)
```c
int64_t TQUEUE_GET_VOLATILE_COUNT(T)(TQUEUE(T) tqueue);
```

`TQUEUE_GET_VOLATILE_COUNT(T)` returns the item count of the queue. Note that the returned value is a point in time value. If the caller needs any synchronization related to the count obtained, lock/use other means of synchronization is required.

Note that the resize lock is acquired in shared mode since it is possible that `TQUEUE_PUSH` alters head and tail in such a way that they can go to smaller numbers.

**SRS_TQUEUE_22_001: [** If `tqueue` is `NULL` then `TQUEUE_GET_VOLATILE_COUNT(T)` shall return zero. **]**

**SRS_TQUEUE_01_080: [** `TQUEUE_GET_VOLATILE_COUNT(T)` shall acquire in shared mode the lock used to guard the growing of the queue. **]**

**SRS_TQUEUE_22_002: [** `TQUEUE_GET_VOLATILE_COUNT(T)` shall obtain the current head queue by calling `interlocked_add_64`. **]**

**SRS_TQUEUE_22_003: [** `TQUEUE_GET_VOLATILE_COUNT(T)` shall obtain the current tail queue by calling `interlocked_add_64`. **]**

**SRS_TQUEUE_22_006: [** `TQUEUE_GET_VOLATILE_COUNT(T)` shall obtain the current tail queue again by calling `interlocked_add_64` and compare with the previosuly obtained tail value.  The tail value is valid only if it has not changed. **]**

**SRS_TQUEUE_22_004: [** If the queue is empty (current tail >= current head), `TQUEUE_GET_VOLATILE_COUNT(T)` shall return zero. **]**

**SRS_TQUEUE_01_081: [** `TQUEUE_GET_VOLATILE_COUNT(T)` shall release in shared mode the lock used to guard the growing of the queue. **]**

**SRS_TQUEUE_22_005: [** `TQUEUE_GET_VOLATILE_COUNT(T)` shall return the item count of the queue. **]**
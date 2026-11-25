# `min_heap` requirements

## Overview

`min_heap` is a binary min-heap of caller-owned entries ordered by a 64-bit key. A `MIN_HEAP_ENTRY` is placed inside the caller's structure (`CONTAINING_RECORD` gets the structure back) and stores its position in the heap, so removing any entry takes O(log n).

Room for entries is reserved separately from inserting them: `min_heap_reserve` can fail, but `min_heap_insert` cannot fail once room is reserved. A caller can reserve room when it creates an object (for example a timer), so that inserting the object later cannot fail.

`min_heap` is not thread-safe: the caller serializes the calls.

## Exposed API

```c
#define MIN_HEAP_ENTRY_NOT_IN_HEAP UINT32_MAX

typedef struct MIN_HEAP_ENTRY_TAG
{
    uint64_t key;
    uint32_t index;
} MIN_HEAP_ENTRY;

typedef struct MIN_HEAP_TAG* MIN_HEAP_HANDLE;

MOCKABLE_FUNCTION(, MIN_HEAP_HANDLE, min_heap_create);
MOCKABLE_FUNCTION(, void, min_heap_destroy, MIN_HEAP_HANDLE, min_heap);
MOCKABLE_FUNCTION(, int, min_heap_reserve, MIN_HEAP_HANDLE, min_heap);
MOCKABLE_FUNCTION(, void, min_heap_unreserve, MIN_HEAP_HANDLE, min_heap);
MOCKABLE_FUNCTION(, void, min_heap_entry_init, MIN_HEAP_ENTRY*, entry);
MOCKABLE_FUNCTION(, void, min_heap_insert, MIN_HEAP_HANDLE, min_heap, MIN_HEAP_ENTRY*, entry, uint64_t, key);
MOCKABLE_FUNCTION(, void, min_heap_remove, MIN_HEAP_HANDLE, min_heap, MIN_HEAP_ENTRY*, entry);
MOCKABLE_FUNCTION(, MIN_HEAP_ENTRY*, min_heap_peek, MIN_HEAP_HANDLE, min_heap);
```

`key` is the key of the entry. `index` is the position of the entry in the heap, or `MIN_HEAP_ENTRY_NOT_IN_HEAP` if the entry is not in a heap. Callers read `key` and do not modify either field.

### min_heap_create

```c
MOCKABLE_FUNCTION(, MIN_HEAP_HANDLE, min_heap_create);
```

`min_heap_create` creates an empty heap with no reserved room.

**SRS_MIN_HEAP_42_001: [** `min_heap_create` shall allocate a new heap by calling `malloc`. **]**

**SRS_MIN_HEAP_42_038: [** `min_heap_create` shall set the heap to have no entries, no reserved room and no array of entries. **]**

**SRS_MIN_HEAP_42_039: [** `min_heap_create` shall succeed and return the heap. **]**

**SRS_MIN_HEAP_42_002: [** If any error occurs, `min_heap_create` shall fail and return `NULL`. **]**

### min_heap_destroy

```c
MOCKABLE_FUNCTION(, void, min_heap_destroy, MIN_HEAP_HANDLE, min_heap);
```

`min_heap_destroy` frees the heap without accessing the entries still in it.

**SRS_MIN_HEAP_42_003: [** If `min_heap` is `NULL`, `min_heap_destroy` shall return. **]**

**SRS_MIN_HEAP_42_004: [** `min_heap_destroy` shall free the array of entries. **]**

**SRS_MIN_HEAP_42_005: [** `min_heap_destroy` shall free `min_heap`. **]**

### min_heap_reserve

```c
MOCKABLE_FUNCTION(, int, min_heap_reserve, MIN_HEAP_HANDLE, min_heap);
```

`min_heap_reserve` reserves room for one more entry.

**SRS_MIN_HEAP_42_006: [** If `min_heap` is `NULL`, `min_heap_reserve` shall fail and return a non-zero value. **]**

**SRS_MIN_HEAP_42_007: [** If the reserved count is `UINT32_MAX`, `min_heap_reserve` shall fail and return a non-zero value. **]**

**SRS_MIN_HEAP_42_041: [** If the reserved count is equal to the capacity of the array of entries: **]**

- **SRS_MIN_HEAP_42_009: [** `min_heap_reserve` shall compute the new capacity as 16 if the capacity is 0, or as twice the capacity capped at `UINT32_MAX` otherwise. **]**

- **SRS_MIN_HEAP_42_010: [** `min_heap_reserve` shall grow the array of entries to the new capacity by calling `realloc_2`. **]**

- **SRS_MIN_HEAP_42_042: [** If `realloc_2` fails, `min_heap_reserve` shall fail and return a non-zero value. **]**

**SRS_MIN_HEAP_42_012: [** `min_heap_reserve` shall increment the reserved count. **]**

**SRS_MIN_HEAP_42_040: [** `min_heap_reserve` shall succeed and return 0. **]**

### min_heap_unreserve

```c
MOCKABLE_FUNCTION(, void, min_heap_unreserve, MIN_HEAP_HANDLE, min_heap);
```

`min_heap_unreserve` gives back the room reserved for one entry, without shrinking the array of entries.

**SRS_MIN_HEAP_42_013: [** If `min_heap` is `NULL`, `min_heap_unreserve` shall return. **]**

**SRS_MIN_HEAP_42_014: [** If the heap has as many entries as the reserved count, `min_heap_unreserve` shall return. **]**

**SRS_MIN_HEAP_42_015: [** `min_heap_unreserve` shall decrement the reserved count. **]**

### min_heap_entry_init

```c
MOCKABLE_FUNCTION(, void, min_heap_entry_init, MIN_HEAP_ENTRY*, entry);
```

`min_heap_entry_init` initializes an entry before it is first inserted.

**SRS_MIN_HEAP_42_016: [** If `entry` is `NULL`, `min_heap_entry_init` shall return. **]**

**SRS_MIN_HEAP_42_017: [** `min_heap_entry_init` shall set the key of `entry` to 0. **]**

**SRS_MIN_HEAP_42_018: [** `min_heap_entry_init` shall set the index of `entry` to `MIN_HEAP_ENTRY_NOT_IN_HEAP`. **]**

### min_heap_insert

```c
MOCKABLE_FUNCTION(, void, min_heap_insert, MIN_HEAP_HANDLE, min_heap, MIN_HEAP_ENTRY*, entry, uint64_t, key);
```

`min_heap_insert` inserts `entry` with the key `key` in room reserved by calling `min_heap_reserve`.

**SRS_MIN_HEAP_42_019: [** If `min_heap` is `NULL`, `min_heap_insert` shall return. **]**

**SRS_MIN_HEAP_42_020: [** If `entry` is `NULL`, `min_heap_insert` shall return. **]**

**SRS_MIN_HEAP_42_021: [** If the index of `entry` is not `MIN_HEAP_ENTRY_NOT_IN_HEAP`, `min_heap_insert` shall return. **]**

**SRS_MIN_HEAP_42_022: [** If the heap has as many entries as the reserved count, `min_heap_insert` shall return. **]**

**SRS_MIN_HEAP_42_023: [** `min_heap_insert` shall set the key of `entry` to `key`. **]**

**SRS_MIN_HEAP_42_024: [** `min_heap_insert` shall add `entry` after the last entry of the heap. **]**

**SRS_MIN_HEAP_42_025: [** `min_heap_insert` shall swap `entry` with its parent while its key is lower than the key of its parent. **]**

### min_heap_remove

```c
MOCKABLE_FUNCTION(, void, min_heap_remove, MIN_HEAP_HANDLE, min_heap, MIN_HEAP_ENTRY*, entry);
```

`min_heap_remove` removes `entry` from the heap without changing its key.

**SRS_MIN_HEAP_42_026: [** If `min_heap` is `NULL`, `min_heap_remove` shall return. **]**

**SRS_MIN_HEAP_42_027: [** If `entry` is `NULL`, `min_heap_remove` shall return. **]**

**SRS_MIN_HEAP_42_028: [** If `entry` is not in `min_heap`, `min_heap_remove` shall return. **]**

**SRS_MIN_HEAP_42_029: [** `min_heap_remove` shall remove the last position of the heap. **]**

**SRS_MIN_HEAP_42_030: [** If `entry` was not the last entry of the heap: **]**

- **SRS_MIN_HEAP_42_031: [** `min_heap_remove` shall move the last entry to the position of `entry`. **]**

- **SRS_MIN_HEAP_42_032: [** `min_heap_remove` shall swap the moved entry with its child with the lowest key while its key is greater than the key of that child. **]**

- **SRS_MIN_HEAP_42_033: [** `min_heap_remove` shall swap the moved entry with its parent while its key is lower than the key of its parent. **]**

**SRS_MIN_HEAP_42_034: [** `min_heap_remove` shall set the index of `entry` to `MIN_HEAP_ENTRY_NOT_IN_HEAP`. **]**

### min_heap_peek

```c
MOCKABLE_FUNCTION(, MIN_HEAP_ENTRY*, min_heap_peek, MIN_HEAP_HANDLE, min_heap);
```

`min_heap_peek` returns the entry with the lowest key without removing it.

**SRS_MIN_HEAP_42_035: [** If `min_heap` is `NULL`, `min_heap_peek` shall return `NULL`. **]**

**SRS_MIN_HEAP_42_036: [** If the heap has no entries, `min_heap_peek` shall return `NULL`. **]**

**SRS_MIN_HEAP_42_037: [** Otherwise, `min_heap_peek` shall return the entry with the lowest key. **]**

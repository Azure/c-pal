// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_HEAP_ENTRY_NOT_IN_HEAP UINT32_MAX

// placed inside the caller's structure, CONTAINING_RECORD gets the structure back
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

#ifdef __cplusplus
}
#endif

#endif // MIN_HEAP_H

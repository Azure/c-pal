// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef REAL_MIN_HEAP_H
#define REAL_MIN_HEAP_H

#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/min_heap.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_MIN_HEAP_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        min_heap_create, \
        min_heap_destroy, \
        min_heap_reserve, \
        min_heap_unreserve, \
        min_heap_entry_init, \
        min_heap_insert, \
        min_heap_remove, \
        min_heap_peek \
    )

MIN_HEAP_HANDLE real_min_heap_create(void);
void real_min_heap_destroy(MIN_HEAP_HANDLE min_heap);
int real_min_heap_reserve(MIN_HEAP_HANDLE min_heap);
void real_min_heap_unreserve(MIN_HEAP_HANDLE min_heap);
void real_min_heap_entry_init(MIN_HEAP_ENTRY* entry);
void real_min_heap_insert(MIN_HEAP_HANDLE min_heap, MIN_HEAP_ENTRY* entry, uint64_t key);
void real_min_heap_remove(MIN_HEAP_HANDLE min_heap, MIN_HEAP_ENTRY* entry);
MIN_HEAP_ENTRY* real_min_heap_peek(MIN_HEAP_HANDLE min_heap);

#endif // REAL_MIN_HEAP_H

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "jemalloc/jemalloc.h"

#define je_malloc mock_je_malloc
#define je_free mock_je_free
#define je_calloc mock_je_calloc
#define je_realloc mock_je_realloc
#define je_malloc_usable_size mock_je_malloc_usable_size

void* mock_je_malloc(size_t size);
void* mock_je_calloc(size_t nmemb, size_t size);
void* mock_je_realloc(void* ptr, size_t size);
void mock_je_free(void* ptr);
size_t mock_je_malloc_usable_size(void* ptr);

#include "../../src/gballoc_ll_jemalloc.c"

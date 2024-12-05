// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "jemalloc/jemalloc.h"

#define je_malloc mock_je_malloc
#define je_free mock_je_free
#define je_calloc mock_je_calloc
#define je_realloc mock_je_realloc
#define je_malloc_usable_size mock_je_malloc_usable_size
#define je_malloc_stats_print mock_je_malloc_stats_print
#define je_mallctl mock_je_mallctl

void* mock_je_malloc(size_t size);
void* mock_je_calloc(size_t nmemb, size_t size);
void* mock_je_realloc(void* ptr, size_t size);
void mock_je_free(void* ptr);
size_t mock_je_malloc_usable_size(void* ptr);
void mock_je_malloc_stats_print(void (*write_cb)(void*, const char*), void* cbopaque, const char* opts);
int mock_je_mallctl(const char* name, void* oldp, size_t* oldlenp, void* newp, size_t newlen);

#include "../../src/gballoc_ll_jemalloc.c"

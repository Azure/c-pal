// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>

#include "azure_c_pal/gballoc_ll.h"

int gballoc_ll_init(void* params)
{
    (void)params;
    return 0;
}

void gballoc_ll_deinit(void)
{
}

void* gballoc_ll_malloc(size_t size)
{
    return malloc(size);
}

void gballoc_ll_free(void* ptr)
{
    free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>

#include "azure_c_logging/xlogging.h"
#include "azure_c_pal/gballoc_ll.h"

#include "jemalloc/jemalloc.h"

int gballoc_ll_init(void* params)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_001: [ gballoc_ll_init shall return 0. ]*/
    (void)params;
    return 0;
}

void gballoc_ll_deinit(void)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_002: [ gballoc_ll_deinit shall return. ] */
}

void* gballoc_ll_malloc(size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_003: [ gballoc_ll_malloc shall call je_malloc and returns what je_malloc returned. ]*/
    if ((result = je_malloc(size)) == NULL)
    {
        LogError("failure in je_malloc(size=%zu)", size);
    }

    return result;
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_004: [ gballoc_ll_free shall call je_free(ptr). ]*/
    je_free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_005: [ gballoc_ll_calloc shall call je_calloc(nmemb, size) and return what je_calloc returned. ]*/
    if ((result = je_calloc(nmemb, size)) == NULL)
    {
        LogError("failure in je_calloc(nmemb=%zu, size=%zu)", nmemb, size);
    }

    return result;
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_006: [ gballoc_ll_realloc calls je_realloc(ptr, size) and returns what je_realloc returned. ]*/
    if ((result = je_realloc(ptr, size)) == NULL)
    {
        LogError("failure in je_realloc(ptr=%p, size=%zu)", ptr, size);
    }

    return result;
}

size_t gballoc_ll_size(void* ptr)
{
    size_t result;

    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_007: [ gballoc_ll_size shall call je_usable_size and return what je_usable_size returned. ]*/
    result = je_malloc_usable_size(ptr);

    return result;
}


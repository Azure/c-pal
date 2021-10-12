// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include "mimalloc.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_ll.h"

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

static void* gballoc_ll_malloc_internal(size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_003: [ gballoc_ll_malloc shall call mi_malloc and returns what mi_malloc returned. ]*/
    result = mi_malloc(size);

    if (result == NULL)
    {
        LogError("failure in mi_malloc(size=%zu)", size);
    }

    return result;
}

void* gballoc_ll_malloc(size_t size)
{
    return gballoc_ll_malloc_internal(size);
}

void* gballoc_ll_malloc_2(size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
    if (SIZE_MAX / nmemb < size)
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_009: [ gballoc_ll_malloc_2 shall call mi_malloc(nmemb * size) and returns what mi_malloc returned. ]*/
        result = gballoc_ll_malloc_internal(nmemb * size);
    }
    return result;
}

void* gballoc_ll_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_010: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
    if (SIZE_MAX / nmemb < size)
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_011: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/

        /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_009: [ gballoc_ll_malloc_2 shall call mi_malloc(nmemb * size) and returns what mi_malloc returned. ]*/
        result = gballoc_ll_malloc_internal(nmemb * size);
    }
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_004: [ gballoc_ll_free shall call mi_free(ptr). ]*/
    mi_free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_005: [ gballoc_ll_calloc shall call mi_calloc(nmemb, size) and return what mi_calloc returned. ]*/
    if ((result = mi_calloc(nmemb, size)) == NULL)
    {
        LogError("failure in mi_calloc(nmemb=%zu, size=%zu)", nmemb, size);
    }

    return result;
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_006: [ gballoc_ll_realloc calls mi_realloc(ptr, size) and returns what mi_realloc returned. ]*/
    if ((result = mi_realloc(ptr, size)) == NULL)
    {
        LogError("failure in mi_realloc(ptr=%p, size=%zu)", ptr, size);
    }

    return result;
}

size_t gballoc_ll_size(void* ptr)
{
    size_t result;

    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_007: [ gballoc_ll_size shall call mi_usable_size and return what mi_usable_size returned. ]*/
    result = mi_usable_size(ptr);

    return result;
}


// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>

#include "c_logging/xlogging.h"
#include "c_pal/gballoc_ll.h"

#include "jemalloc/jemalloc.h"

int gballoc_ll_init(void* params)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_001: [ gballoc_ll_init shall return 0. ]*/
    (void)params;
    return 0;
}

void gballoc_ll_deinit(void)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_002: [ gballoc_ll_deinit shall return. ]*/
}

static void* gballoc_ll_malloc_internal(size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_003: [ gballoc_ll_malloc shall call je_malloc and returns what je_malloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_002: [ gballoc_ll_malloc_2 shall call je_malloc(nmemb*size) and returns what je_malloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_005: [ gballoc_ll_malloc_flex shall return what je_malloc(base + nmemb * size) returns. ]*/
    result = je_malloc(size);

    if (result == NULL)
    {
        LogError("failure in je_malloc(size=%zu)", size);
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
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_001: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
    if (
        (nmemb != 0) &&
        (SIZE_MAX / nmemb < size)
        )
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_002: [ gballoc_ll_malloc_2 shall call je_malloc(nmemb*size) and returns what je_malloc returned. ]*/
        result = gballoc_ll_malloc_internal(nmemb * size);
    }
    return result;
}

void* gballoc_ll_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_003: [ If nmemb*size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
    if (
        (nmemb != 0) &&
        (SIZE_MAX / nmemb < size)
        )
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_004: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
        if (SIZE_MAX - base < nmemb * size)
        {
            LogError("overflow in computation of base=%zu + nmemb=%zu * size=%zu",
                base, nmemb, size);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_005: [ gballoc_ll_malloc_flex shall return what je_malloc(base + nmemb * size) returns. ]*/
            result = gballoc_ll_malloc_internal(base + nmemb * size);
        }
    }
    return result;
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_004: [ gballoc_ll_free shall call je_free(ptr). ]*/
    je_free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_005: [ gballoc_ll_calloc shall call je_calloc(nmemb, size) and return what je_calloc returned. ]*/
    if ((result = je_calloc(nmemb, size)) == NULL)
    {
        LogError("failure in je_calloc(nmemb=%zu, size=%zu)", nmemb, size);
    }

    return result;
}

static void* gballoc_ll_realloc_internal(void* ptr, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_006: [ gballoc_ll_realloc calls je_realloc(ptr, size) and returns what je_realloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_007: [ gballoc_ll_realloc_2 shall return what je_realloc(ptr, nmemb * size) returns. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_010: [ gballoc_ll_realloc_flex shall return what je_realloc(ptr, base + nmemb * size) returns. ]*/
    result = je_realloc(ptr, size);

    if (result == NULL)
    {
        LogError("failure in je_realloc(ptr=%p, size=%zu)", ptr, size);
    }

    return result;
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_006: [ gballoc_ll_realloc calls je_realloc(ptr, size) and returns what je_realloc returned. ]*/
    return gballoc_ll_realloc_internal(ptr, size);
}

void* gballoc_ll_realloc_2(void* ptr, size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_006: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_2 shall fail and return NULL. ]*/
    if (
        (nmemb != 0) &&
        (SIZE_MAX / nmemb < size)
        )
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_007: [ gballoc_ll_realloc_2 shall return what je_realloc(ptr, nmemb * size) returns. ]*/
        result = gballoc_ll_realloc_internal(ptr, nmemb * size);
    }
    return result;
}

void* gballoc_ll_realloc_flex(void* ptr, size_t base, size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
    if ((nmemb != 0) &&
        (SIZE_MAX / nmemb < size)
        )
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_009: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
        if (SIZE_MAX - base < nmemb * size)
        {
            LogError("overflow in computation of base=%zu + nmemb=%zu * size=%zu",
                base, nmemb, size);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_010: [ gballoc_ll_realloc_flex shall return what je_realloc(ptr, base + nmemb * size) returns. ]*/
            result = gballoc_ll_realloc_internal(ptr, base + nmemb * size);
        }
    }
    return result;
}

size_t gballoc_ll_size(void* ptr)
{
    size_t result;

    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_007: [ gballoc_ll_size shall call je_malloc_usable_size and return what je_malloc_usable_size returned. ]*/
    result = je_malloc_usable_size(ptr);

    return result;
}


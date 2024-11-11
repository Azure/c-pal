// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_ll.h"

int gballoc_ll_init(void* params)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_001: [ gballoc_ll_init shall return 0. ]*/
    (void)params;
    return 0;
}

void gballoc_ll_deinit(void)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_002: [ gballoc_ll_deinit shall return. ]*/
}

static void* gballoc_ll_malloc_internal(size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_003: [ gballoc_ll_malloc shall call malloc(size) and return what malloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_009: [ gballoc_ll_malloc_2 shall call malloc(nmemb*size) and return what malloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_012: [ gballoc_ll_malloc_flex shall return what malloc(base + nmemb * size) returns. ]*/
    result = malloc(size);

    if (result == NULL)
    {
        LogError("failure in malloc(size=%zu)", size);
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
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
    if (
        (size != 0) &&
        (SIZE_MAX / size < nmemb)
        )
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_009: [ gballoc_ll_malloc_2 shall call malloc(nmemb*size) and return what malloc returned. ]*/
        result = gballoc_ll_malloc_internal(nmemb * size);
    }
    return result;
}

void* gballoc_ll_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_011: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
    if (
        (size != 0) &&
        ((SIZE_MAX - base) / size < nmemb)
        )
    {
        LogError("overflow in computation of base=%zu + nmemb=%zu * size=%zu",
            base, nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_012: [ gballoc_ll_malloc_flex shall return what malloc(base + nmemb * size) returns. ]*/
        result = gballoc_ll_malloc_internal(base + nmemb * size);
    }
    
    return result;
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_004: [ gballoc_ll_free shall call free(ptr). ]*/
    free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_005: [ gballoc_ll_calloc shall call calloc(nmemb, size) and return what calloc returned. ]*/
    result = calloc(nmemb, size);

    if (result == NULL)
    {
        LogError("failure in calloc(nmemb=%zu, size=%zu)", nmemb, size);
    }

    return result;
}

static void* gballoc_ll_realloc_internal(void* ptr, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_006: [ gballoc_ll_realloc shall call realloc(ptr, size) and return what realloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_014: [ gballoc_ll_realloc_2 shall return what realloc(ptr, nmemb * size) returns. ]*/
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_017: [ gballoc_ll_realloc_flex shall return what realloc(ptr, base + nmemb * size) returns. ]*/
    result = realloc(ptr, size);

    if (result == NULL)
    {
        LogError("failure in realloc(ptr=%p, size=%zu)", ptr, size);
    }

    return result;
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_006: [ gballoc_ll_realloc shall call realloc(ptr, size) and return what realloc returned. ]*/
    return gballoc_ll_realloc_internal(ptr, size);
}

void* gballoc_ll_realloc_2(void* ptr, size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_013: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_2 shall fail and return NULL. ]*/
    if (
        (size != 0) &&
        (SIZE_MAX / size < nmemb)
        )
    {
        LogError("overflow in computation of nmemb=%zu * size=%zu",
            nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_014: [ gballoc_ll_realloc_2 shall return what realloc(ptr, nmemb * size) returns. ]*/
        result = gballoc_ll_realloc_internal(ptr, nmemb * size);
    }
    return result;
}

void* gballoc_ll_realloc_flex(void* ptr, size_t base, size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_016: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
    if (
        (size != 0) &&
        ((SIZE_MAX - base) / size < nmemb)
        )
    {
        LogError("overflow in computation of base=%zu + nmemb=%zu * size=%zu",
            base, nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_017: [ gballoc_ll_realloc_flex shall return what realloc(ptr, base + nmemb * size) returns. ]*/
        result = gballoc_ll_realloc_internal(ptr, base + nmemb * size);
    }
    
    return result;
}

size_t gballoc_ll_size(void* ptr)
{
    size_t result;

    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_007: [ gballoc_ll_size shall return what _msize returns. ]*/
    result = malloc_usable_size(ptr);

    return result;
}

int gballoc_ll_set_decay(int64_t decay_milliseconds)
{
    (void)decay_milliseconds;

    /* Codes_SRS_GBALLOC_LL_PASSTHROUGH_28_001: [ gballoc_ll_set_decay shall do nothing and return a non-zero value. ]*/
    return MU_FAILURE;
}

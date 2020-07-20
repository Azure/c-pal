// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>

#include "azure_c_pal/gballoc_ll.h"

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

void* gballoc_ll_malloc(size_t size)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_003: [ gballoc_ll_malloc shall call malloc(size) and return what malloc returned. ]*/
    return malloc(size);
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_004: [ gballoc_ll_free shall call free(ptr). ]*/
    free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_005: [ gballoc_ll_calloc shall call calloc(nmemb, size) and return what calloc returned. ]*/
    return calloc(nmemb, size);
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_PASSTHROUGH_02_006: [ gballoc_ll_realloc shall call realloc(nmemb, size) and return what realloc returned. ]*/
    return realloc(ptr, size);
}

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"
#include "azure_c_pal/gballoc_ll.h"

#include "mimalloc.h"

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
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_003: [ gballoc_ll_malloc shall call mi_malloc and returns what mi_malloc returned. ]*/
    return mi_malloc(size);
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_004: [ gballoc_ll_free shall call mi_free(ptr). ]*/
    mi_free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_005: [ gballoc_ll_calloc shall call mi_calloc(nmemb, size) and return what mi_calloc returned. ]*/
    return mi_calloc(nmemb, size);
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_MIMALLOC_02_006: [ gballoc_ll_realloc calls mi_realloc(ptr, size) and returns what mi_realloc returned. ]*/
    return mi_realloc(ptr, size);
}

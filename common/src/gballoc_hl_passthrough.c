// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_pal/gballoc_ll.h"

#include "azure_c_pal/gballoc_hl.h"

int gballoc_hl_init(void* gballoc_hl_init_params, void* gballoc_ll_init_params)
{
    (void)gballoc_hl_init_params; /*are ignored, this is "passthrough*/

    return gballoc_ll_init(gballoc_ll_init_params);
}

void gballoc_hl_deinit(void)
{
    /*no work for this layer, it is passthough*/
    gballoc_ll_deinit();
}

void* gballoc_hl_malloc(size_t size)
{
    return gballoc_ll_malloc(size);
}

void gballoc_hl_free(void* ptr)
{
    gballoc_ll_free(ptr);
}


void* gballoc_hl_calloc(size_t nmemb, size_t size)
{
    return gballoc_ll_calloc(nmemb, size);
}


void* gballoc_hl_realloc(void* ptr, size_t size)
{
    return gballoc_ll_realloc(ptr, size);
}

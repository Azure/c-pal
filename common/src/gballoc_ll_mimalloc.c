// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>

#include "windows.h"

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"
#include "azure_c_pal/gballoc_ll.h"

#include "mimalloc.h"

/*not thread safe, only supports 1 call to init*/
int gballoc_ll_init(void* params)
{
    (void)params;
    return 0;
}

void gballoc_ll_deinit(void)
{
    /*do nothing*/
}

void* gballoc_ll_malloc(size_t size)
{
    return mi_malloc(size);
}

void gballoc_ll_free(void* ptr)
{
    mi_free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    return mi_calloc(nmemb, size);
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    return mi_realloc(ptr, size);
}

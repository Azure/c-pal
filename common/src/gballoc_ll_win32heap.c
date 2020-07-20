// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "windows.h"

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"
#include "azure_c_pal/gballoc_ll.h"

static HANDLE the_heap = NULL;

/*not thread safe, only supports 1 call to init*/
int gballoc_ll_init(void* params)
{
    (void)params;
    int result;
    the_heap = HeapCreate(0, 0, 0);
    if (the_heap == NULL)
    {
        LogLastError("HeapCreate failed.");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }
    return result;
}

void gballoc_ll_deinit(void)
{
    if (the_heap == NULL)
    {
        LogError("the_heap was %p.", the_heap);
    }
    else
    {
        if (!HeapDestroy(the_heap))
        {
            LogLastError("failure in HeapDestroy(the_heap=%p).", the_heap);
        }
        the_heap = NULL;
    }
}

void* gballoc_ll_malloc(size_t size)
{
    void* result;
    if (the_heap == NULL)
    {
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        result = HeapAlloc(the_heap, 0, size);
        if (result == NULL)
        {
            LogError("failure in HeapAlloc(the_heap=%p, 0, size=%zu);", the_heap, size);
            /*return as is*/
        }
    }
    return result;
}

void gballoc_ll_free(void* ptr)
{
    if (!HeapFree(the_heap, 0, ptr))
    {
        LogLastError("failure in HeapFree(custom_heap=%p, 0, ptr=%p)", the_heap, ptr);
    }
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;
    if (the_heap == NULL)
    {
        LogError("Not initialized.");
        result = NULL;
    }
    else
    {
        result = HeapAlloc(the_heap, HEAP_ZERO_MEMORY, nmemb * size);
        /*return as is*/
    }
    return result;
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    void* result;
    if (the_heap == NULL)
    {
        LogError("Not initialized");
        result = NULL;
    }
    else
    {
        if (ptr == NULL)
        {
            result = HeapAlloc(the_heap, 0, size);
            /*return as is*/
        }
        else
        {
            result = HeapReAlloc(the_heap, 0, ptr, size);
            /*return as is*/
        }
    }
    return result;
}

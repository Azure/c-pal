// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "windows.h"

#include "azure_c_logging/xlogging.h"

#include "azure_c_pal/lazy_init.h"

#include "azure_c_pal/gballoc_ll.h"

static call_once_t g_lazy = LAZY_INIT_NOT_DONE;
static HANDLE the_heap = NULL;

static int heap_init(void* init_params)
{
    int result;
    HANDLE* pthe_heap = (HANDLE*)init_params;
    
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_019: [ heap_init shall call HeapCreate(0,0,0) to create a heap. ]*/
    *pthe_heap = HeapCreate(0, 0, 0);
    if (*pthe_heap == NULL)
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_021: [ If there are any failures then heap_init shall fail and return a non-zero value. ]*/
        LogLastError("HeapCreate(0,0,0) failed.");
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_020: [ heap_init shall succeed and return 0. ]*/
        result = 0;
    }
    return result;
}

int gballoc_ll_init(void* params)
{
    (void)params;
    int result;

    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_018: [ gballoc_ll_init shall call lazy_init with parameter do_init set to heap_init. ]*/
    if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_003: [ If there are any failures then gballoc_ll_init shall fail and return a non-0 value. ]*/
        LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
            &g_lazy, heap_init, &the_heap);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_002: [ gballoc_ll_init shall succeed and return 0. ]*/

        result = 0;
    }
    
    return result;
}

void gballoc_ll_deinit(void)
{
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_016: [ If the global state is not initialized then gballoc_ll_deinit shall return. ]*/
    if (the_heap == NULL)
    {
        LogError("gballoc_ll_init was not called. the_heap was %p.", the_heap);
    }
    else
    {
        interlocked_exchange(&g_lazy, LAZY_INIT_NOT_DONE);
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_004: [ gballoc_ll_deinit shall call HeapDestroy on the handle stored by gballoc_ll_init in the global variable. ]*/
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
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_022: [ gballoc_ll_malloc shall call lazy_init with parameter do_init set to heap_init. ]*/
    if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_023: [ If lazy_init fails then gballoc_ll_malloc shall return NULL. ]*/
        LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
            &g_lazy, heap_init, &the_heap);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_006: [ gballoc_ll_malloc shall call HeapAlloc. ]*/
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_007: [ gballoc_ll_malloc shall return what HeapAlloc returned. ]*/
        result = HeapAlloc(the_heap, 0, size);

        if (result == NULL)
        {
            LogError("failure in HeapAlloc(the_heap=%p, 0, size=%zu)", the_heap, size);
        }
    }
    return result;
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_009: [ gballoc_ll_free shall call HeapFree. ]*/
    if (!HeapFree(the_heap, 0, ptr))
    {
        LogLastError("failure in HeapFree(the_heap=%p, 0, ptr=%p)", the_heap, ptr);
    }
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_024: [ gballoc_ll_calloc shall call lazy_init with parameter do_init set to heap_init. ]*/
    if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_025: [ If lazy_init fails then gballoc_ll_calloc shall return NULL. ]*/
        LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
            &g_lazy, heap_init, &the_heap);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_011: [ gballoc_ll_calloc shall call HeapAlloc with flags set to HEAP_ZERO_MEMORY. ]*/
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_012: [ gballoc_ll_calloc shall return what HeapAlloc returns. ]*/
        result = HeapAlloc(the_heap, HEAP_ZERO_MEMORY, nmemb * size);
        /*return as is*/

        if (result == NULL)
        {
            LogError("failure in HeapAlloc(the_heap=%p, HEAP_ZERO_MEMORY, nmemb=%zu * size=%zu)", the_heap, nmemb, size);
        }
    }
    return result;
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_026: [ gballoc_ll_realloc shall call lazy_init with parameter do_init set to heap_init. ]*/
    if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_027: [ If lazy_init fails then gballoc_ll_reallocshall return NULL. ]*/
        LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
            &g_lazy, heap_init, &the_heap);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_014: [ If ptr is NULL then gballoc_ll_realloc shall call HeapAlloc and return what HeapAlloc returns. ]*/
        if (ptr == NULL)
        {
            result = HeapAlloc(the_heap, 0, size);
            /*return as is*/

            if (result == NULL)
            {
                LogError("Failure in HeapAlloc(the_heap=%p, 0, size=%zu)", the_heap, size);
            }
        }
        else
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_015: [ If ptr is not NULL then gballoc_ll_realloc shall call HeapReAlloc and return what HeapReAlloc returns. ]*/
            result = HeapReAlloc(the_heap, 0, ptr, size);
            /*return as is*/

            if (result == NULL)
            {
                LogError("Failure in HeapReAlloc(the_heap=%p, 0, ptr=%p, size=%zu)", the_heap, ptr, size);
            }
        }
    }
    return result;
}

size_t gballoc_ll_size(void* ptr)
{
    size_t result;
    
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_017: [ gballoc_ll_size shall call HeapSize and return what HeapSize returns. ]*/
    result = HeapSize(the_heap, 0, ptr);

    if (result == (size_t)(-1))
    {
        LogError("failure in HeapSize(the_heap=%p, 0, ptr=%p);", the_heap, ptr);
    }

    return result;
}

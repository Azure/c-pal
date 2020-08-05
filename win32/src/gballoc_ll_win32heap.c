// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "windows.h"

#include "azure_c_logging/xlogging.h"
//#include "azure_c_pal/interlocked.h"
#include "azure_c_pal/gballoc_ll.h"

static HANDLE the_heap = NULL;

//static volatile_atomic int32_t gballoc_ll_win32heap_state = 0; /*0 - not intialized, 1 - initializing, 2 - initialized*/

/*not thread safe, only supports 1 call to init*/
int gballoc_ll_init(void* params)
{

    (void)params;
    int result;

    //int32_t state;

    //while ((state = interlocked_compare_exchange(&gballoc_ll_win32heap_state, 1, 0)) != 2)
    //{
    //    if (state == 0)
    //    {
    //        /*only 1 thread gets to execute this part*/
    //
    //    }
    //}


    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_001: [ gballoc_ll_init shall call HeapCreate(0,0,0) and store the returned heap handle in a global variable. ]*/
    the_heap = HeapCreate(0, 0, 0);
    if (the_heap == NULL)
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_003: [ If HeapCreate fails then gballoc_ll_init shall fail and return a non-0 value. ]*/
        LogLastError("HeapCreate(0,0,0) failed.");
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
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_004: [ gballoc_ll_deinit shall call HeapDestroy on the handle stored by gballoc_ll_init in the global variable. ]*/
        if (!HeapDestroy(the_heap))
        {
            LogLastError("failure in HeapDestroy(the_heap=%p).", the_heap);
        }
        the_heap = NULL;
    }
}
    MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);

void* gballoc_ll_malloc(size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_005: [ If the global state is not initialized then gballoc_ll_malloc shall return NULL. ]*/
    if (the_heap == NULL)
    {
        LogError("gballoc_ll_init was not called. the_heap was %p.", the_heap);
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
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_008: [ If the global state is not initialized then gballoc_ll_free shall return. ]*/
    if (the_heap == NULL)
    {
        LogError("gballoc_ll_init was not called. the_heap was %p.", the_heap);
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_009: [ gballoc_ll_free shall call HeapFree. ]*/
        if (!HeapFree(the_heap, 0, ptr))
        {
            LogLastError("failure in HeapFree(custom_heap=%p, 0, ptr=%p)", the_heap, ptr);
        }
    }
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_010: [ If the global state is not initialized then gballoc_ll_calloc shall return NULL. ]*/
    if (the_heap == NULL)
    {
        LogError("gballoc_ll_init was not called. the_heap was %p.", the_heap);
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
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_013: [ If the global state is not initialized then gballoc_ll_realloc shall return NULL. ]*/
    if (the_heap == NULL)
    {
        LogError("gballoc_ll_init was not called. the_heap was %p.", the_heap);
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
        LogError("failure in HeapSize(the_heap=%p, 0, ptr=%p);", the_heap,  ptr);
    }

    return result;
}

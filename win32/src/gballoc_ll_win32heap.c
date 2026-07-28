// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdint.h>

#include "windows.h"

#include "c_logging/logger.h"

#include "c_pal/arithmetic.h"
#include "c_pal/lazy_init.h"

#include "c_pal/gballoc_ll.h"

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

static void* gballoc_ll_malloc_internal(size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_006: [ gballoc_ll_malloc shall call HeapAlloc. ]*/

    result = HeapAlloc(the_heap, 0, size);

    if (result == NULL)
    {
        LogError("failure in HeapAlloc(the_heap=%p, 0, size=%zu)", the_heap, size);
    }

    return result;
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
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_007: [ gballoc_ll_malloc shall return what HeapAlloc returned. ]*/
        result = gballoc_ll_malloc_internal(size);
    }
    return result;
}

void* gballoc_ll_malloc_2(size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_028: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
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
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_029: [ gballoc_ll_malloc_2 shall call lazy_init with parameter do_init set to heap_init. ]*/
        if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_030: [ If lazy_init fails then gballoc_ll_malloc_2 shall return NULL. ]*/
            LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
                &g_lazy, heap_init, &the_heap);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_031: [ gballoc_ll_malloc_2 shall call HeapAlloc(nmemb*size) and return what HeapAlloc returned. ]*/
            result = gballoc_ll_malloc_internal(nmemb * size);
        }
    }
    return result;
}

void* gballoc_ll_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_033: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
    if (
        (size != 0) &&
        ((SIZE_MAX - base) / size < nmemb))
    {
        LogError("overflow in computation of base=%zu + nmemb=%zu * size=%zu",
            base, nmemb, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_034: [ gballoc_ll_malloc_flex shall call lazy_init with parameter do_init set to heap_init. ]*/
        if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_035: [ If lazy_init fails then gballoc_ll_malloc_flex shall return NULL. ]*/
            LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
                &g_lazy, heap_init, &the_heap);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_036: [ gballoc_ll_malloc_flex shall return what HeapAlloc(base + nmemb * size) returns. ]*/
            result = gballoc_ll_malloc_internal(base + nmemb * size);
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

void* gballoc_ll_malloc_aligned(size_t alignment, size_t size)
{
    void* result;
    if (!is_power_of_2(alignment) || alignment < sizeof(void*))
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_001: [ If alignment is not a power of 2 or is less than sizeof(void*) then gballoc_ll_malloc_aligned shall fail and return NULL. ]*/
        LogError("invalid alignment=%zu (must be a power of 2 and at least sizeof(void*)=%zu)", alignment, sizeof(void*));
        result = NULL;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_002: [ If size + alignment + sizeof(void*) exceeds SIZE_MAX then gballoc_ll_malloc_aligned shall fail and return NULL. ]*/
        if (size > SIZE_MAX - alignment - sizeof(void*))
        {
            LogError("overflow in computation of size=%zu + alignment=%zu + sizeof(void*)=%zu", size, alignment, sizeof(void*));
            result = NULL;
        }
        else
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_003: [ gballoc_ll_malloc_aligned shall call lazy_init with parameter do_init set to heap_init. ]*/
            if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
            {
                /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_004: [ If lazy_init fails then gballoc_ll_malloc_aligned shall return NULL. ]*/
                LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
                    &g_lazy, heap_init, &the_heap);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_005: [ gballoc_ll_malloc_aligned shall call HeapAlloc to allocate size + alignment + sizeof(void*) bytes. ]*/
                void* base = HeapAlloc(the_heap, 0, size + alignment + sizeof(void*));
                if (base == NULL)
                {
                    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_006: [ If HeapAlloc fails then gballoc_ll_malloc_aligned shall return NULL. ]*/
                    LogError("failure in HeapAlloc(the_heap=%p, 0, size=%zu + alignment=%zu + sizeof(void*)=%zu)", the_heap, size, alignment, sizeof(void*));
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_007: [ gballoc_ll_malloc_aligned shall store the base allocation pointer in the sizeof(void*) bytes preceding the returned aligned pointer and return the aligned pointer. ]*/
                    uintptr_t aligned = ((uintptr_t)base + sizeof(void*) + (alignment - 1)) & ~((uintptr_t)(alignment - 1));
                    ((void**)aligned)[-1] = base;
                    result = (void*)aligned;
                }
            }
        }
    }
    return result;
}

void gballoc_ll_free_aligned(void* ptr)
{
    if (ptr != NULL)
    {
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_22_008: [ gballoc_ll_free_aligned shall recover the base allocation pointer stored in the sizeof(void*) bytes preceding ptr and call HeapFree on it. ]*/
        void* base = ((void**)ptr)[-1];
        if (!HeapFree(the_heap, 0, base))
        {
            LogLastError("failure in HeapFree(the_heap=%p, 0, base=%p)", the_heap, base);
        }
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

static void* gballoc_ll_realloc_internal(void* ptr, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_014: [ If ptr is NULL then gballoc_ll_realloc shall call HeapAlloc and return what HeapAlloc returns. ]*/
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_046: [ If ptr is NULL then gballoc_ll_realloc_2 shall call HeapAlloc(nmemb * size) and return what HeapAlloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_048: [ If ptr is NULL then gballoc_ll_realloc_flex shall return what HeapAlloc(ptr, base + nmemb * size) returns. ]*/
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
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_047: [ If ptr is not NULL then gballoc_ll_realloc_2 shall call HeapReAlloc(ptr, nmemb * size) and return what HeapReAlloc returned. ]*/
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_049: [ If ptr is not NULL then gballoc_ll_realloc_flex shall return what HeapReAlloc(ptr, base + nmemb * size) returns. ]*/
        result = HeapReAlloc(the_heap, 0, ptr, size);
        /*return as is*/

        if (result == NULL)
        {
            LogError("Failure in HeapReAlloc(the_heap=%p, 0, ptr=%p, size=%zu)", the_heap, ptr, size);
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
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_027: [ If lazy_init fails then gballoc_ll_realloc shall return NULL. ]*/
        LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
            &g_lazy, heap_init, &the_heap);
        result = NULL;
    }
    else
    {
        return gballoc_ll_realloc_internal(ptr, size);
    }
    return result;
}

void* gballoc_ll_realloc_2(void* ptr, size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_037: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_2 shall fail and return NULL. ]*/
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
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_038: [ gballoc_ll_realloc_2 shall call lazy_init with parameter do_init set to heap_init. ]*/
        if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_039: [ If lazy_init fails then gballoc_ll_realloc_2 shall return NULL. ]*/
            LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
                &g_lazy, heap_init, &the_heap);
            result = NULL;
        }
        else
        {
            result = gballoc_ll_realloc_internal(ptr, nmemb * size);
        }
    }
    return result;
}

void* gballoc_ll_realloc_flex(void* ptr, size_t base, size_t nmemb, size_t size)
{
    void* result;
   
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_042: [ base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
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
        /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_043: [ gballoc_ll_realloc_flex shall call lazy_init with parameter do_init set to heap_init. ]*/
        if (lazy_init(&g_lazy, heap_init, &the_heap) != LAZY_INIT_OK)
        {
            /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_044: [ If lazy_init fails then gballoc_ll_malloc_flex shall return NULL. ]*/
            LogError("failure in lazy_init(&g_lazy=%p, heap_init=%p, &the_heap=%p)",
                &g_lazy, heap_init, &the_heap);
            result = NULL;
        }
        else
        {
            result = gballoc_ll_realloc_internal(ptr, base + nmemb * size);
        }
    }

    return result;
}

size_t gballoc_ll_size(void* ptr)
{
    size_t result;
    
    /*Codes_SRS_GBALLOC_LL_WIN32HEAP_02_017: [ gballoc_ll_size shall call HeapSize and returns what HeapSize returns. ]*/
    result = HeapSize(the_heap, 0, ptr);

    if (result == (size_t)(-1))
    {
        LogError("failure in HeapSize(the_heap=%p, 0, ptr=%p);", the_heap, ptr);
    }

    return result;
}

void gballoc_ll_print_stats(void)
{
    /* Codes_SRS_GBALLOC_LL_WIN32HEAP_01_001: [ gballoc_ll_print_stats shall return without printing any statistics. ]*/
}

int gballoc_ll_set_option(const char* option_name, void* option_value)
{
    (void)option_name;
    (void)option_value;

    /* Codes_SRS_GBALLOC_LL_WIN32HEAP_28_001: [ gballoc_ll_set_option shall do nothing and return 0. ]*/
    return 0;
}

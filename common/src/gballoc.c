// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include "azure_macro_utils/macro_utils.h"
#include "azure_c_logging/xlogging.h"
#include "lock.h"

#ifndef GB_USE_CUSTOM_HEAP

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)~(size_t)0)
#endif

typedef struct ALLOCATION_TAG
{
    void* ptr;
    void* next;
} ALLOCATION;

typedef enum GBALLOC_STATE_TAG
{
    GBALLOC_STATE_INIT,
    GBALLOC_STATE_NOT_INIT
} GBALLOC_STATE;

static ALLOCATION* head = NULL;
static GBALLOC_STATE gballocState = GBALLOC_STATE_NOT_INIT;

static LOCK_HANDLE gballocThreadSafeLock = NULL;

int gballoc_init(void)
{
    int result;

    if (gballocState != GBALLOC_STATE_NOT_INIT)
    {
        /* Codes_SRS_GBALLOC_01_025: [Init after Init shall fail and return a non-zero value.] */
        result = MU_FAILURE;
    }
    /* Codes_SRS_GBALLOC_01_026: [gballoc_Init shall create a lock handle that will be used to make the other gballoc APIs thread-safe.] */
    else if ((gballocThreadSafeLock = Lock_Init()) == NULL)
    {
        /* Codes_SRS_GBALLOC_01_027: [If the Lock creation fails, gballoc_init shall return a non-zero value.]*/
        result = MU_FAILURE;
    }
    else
    {
        gballocState = GBALLOC_STATE_INIT;

        /* Codes_SRS_GBALLOC_01_024: [gballoc_init shall initialize the gballoc module and return 0 upon success.] */
        result = 0;
    }

    return result;
}

void gballoc_deinit(void)
{
    if (gballocState == GBALLOC_STATE_INIT)
    {
        /* Codes_SRS_GBALLOC_01_028: [gballoc_deinit shall free all resources allocated by gballoc_init.] */
        (void)Lock_Deinit(gballocThreadSafeLock);
        gballocState = GBALLOC_STATE_NOT_INIT;
    }
    else
    {
        /* Codes_SRS_GBALLOC_01_029: [if gballoc is not initialized gballoc_deinit shall do nothing.] */
    }
}

void* gballoc_malloc(size_t size)
{
    void* result;

    if (gballocState != GBALLOC_STATE_INIT)
    {
        /* Codes_SRS_GBALLOC_01_039: [If gballoc was not initialized gballoc_malloc shall simply call malloc without any memory tracking being performed.] */
        result = malloc(size);
    }
    /* Codes_SRS_GBALLOC_01_030: [gballoc_malloc shall ensure thread safety by using the lock created by gballoc_Init.] */
    else if (LOCK_OK != Lock(gballocThreadSafeLock))
    {
        /* Codes_SRS_GBALLOC_01_048: [If acquiring the lock fails, gballoc_malloc shall return NULL.] */
        LogError("Failed to get the Lock.");
        result = NULL;
    }
    else
    {
        ALLOCATION* allocation = (ALLOCATION*)malloc(sizeof(ALLOCATION));
        if (allocation == NULL)
        {
            /* Codes_SRS_GBALLOC_01_013: [When gballoc_malloc fails allocating memory for its internal use, gballoc_malloc shall return NULL.] */
            result = NULL;
        }
        else
        {
            /* Codes_SRS_GBALLOC_01_003: [gb_malloc shall call the C99 malloc function and return its result.] */
            result = malloc(size);
            if (result == NULL)
            {
                /* Codes_SRS_GBALLOC_01_012: [When the underlying malloc call fails, gballoc_malloc shall return NULL.] */
                free(allocation);
            }
            else
            {
                allocation->ptr = result;
                allocation->next = head;
                head = allocation;
            }
        }

        (void)Unlock(gballocThreadSafeLock);
    }

    return result;
}

void* gballoc_calloc(size_t nmemb, size_t size)
{
    void* result;

    if (gballocState != GBALLOC_STATE_INIT)
    {
        /* Codes_SRS_GBALLOC_01_040: [If gballoc was not initialized gballoc_calloc shall simply call calloc without any memory tracking being performed.] */
        result = calloc(nmemb, size);
    }
    /* Codes_SRS_GBALLOC_01_031: [gballoc_calloc shall ensure thread safety by using the lock created by gballoc_Init]  */
    else if (LOCK_OK != Lock(gballocThreadSafeLock))
    {
        /* Codes_SRS_GBALLOC_01_046: [If acquiring the lock fails, gballoc_calloc shall return NULL.] */
        LogError("Failed to get the Lock.");
        result = NULL;
    }
    else
    {
        ALLOCATION* allocation = (ALLOCATION*)malloc(sizeof(ALLOCATION));
        if (allocation == NULL)
        {
            /* Codes_SRS_GBALLOC_01_023: [When gballoc_calloc fails allocating memory for its internal use, gballoc_calloc shall return NULL.] */
            result = NULL;
        }
        else
        {
            /* Codes_SRS_GBALLOC_01_020: [gballoc_calloc shall call the C99 calloc function and return its result.] */
            result = calloc(nmemb, size);
            if (result == NULL)
            {
                /* Codes_SRS_GBALLOC_01_022: [When the underlying calloc call fails, gballoc_calloc shall return NULL.] */
                free(allocation);
            }
            else
            {
                allocation->ptr = result;
                allocation->next = head;
                head = allocation;
            }
        }

        (void)Unlock(gballocThreadSafeLock);
    }

    return result;
}

void* gballoc_realloc(void* ptr, size_t size)
{
    ALLOCATION* curr;
    void* result;
    ALLOCATION* allocation = NULL;

    if (gballocState != GBALLOC_STATE_INIT)
    {
        /* Codes_SRS_GBALLOC_01_041: [If gballoc was not initialized gballoc_realloc shall shall simply call realloc without any memory tracking being performed.] */
        result = realloc(ptr, size);
    }
    /* Codes_SRS_GBALLOC_01_032: [gballoc_realloc shall ensure thread safety by using the lock created by gballoc_Init.] */
    else if (LOCK_OK != Lock(gballocThreadSafeLock))
    {
        /* Codes_SRS_GBALLOC_01_047: [If acquiring the lock fails, gballoc_realloc shall return NULL.] */
        LogError("Failed to get the Lock.");
        result = NULL;
    }
    else
    {
        if (ptr == NULL)
        {
            /* Codes_SRS_GBALLOC_01_017: [When ptr is NULL, gballoc_realloc shall call the underlying realloc with ptr being NULL and the realloc result shall be tracked by gballoc.] */
            allocation = (ALLOCATION*)malloc(sizeof(ALLOCATION));
        }
        else
        {
            curr = head;
            while (curr != NULL)
            {
                if (curr->ptr == ptr)
                {
                    allocation = curr;
                    break;
                }
                else
                {
                    curr = (ALLOCATION*)curr->next;
                }
            }
        }

        if (allocation == NULL)
        {
            /* Codes_SRS_GBALLOC_01_015: [When allocating memory used for tracking by gballoc_realloc fails, gballoc_realloc shall return NULL.] */
            /* Codes_SRS_GBALLOC_01_016: [When the ptr pointer cannot be found in the pointers tracked by gballoc, gballoc_realloc shall return NULL and the underlying realloc shall not be called.] */
            result = NULL;
        }
        else
        {
            /* Codes_SRS_GBALLOC_01_005: [gballoc_realloc shall call the C99 realloc function and return its result.] */
            result = realloc(ptr, size);
            if (result == NULL)
            {
                /* Codes_SRS_GBALLOC_01_014: [When the underlying realloc call fails, gballoc_realloc shall return NULL.] */
                if (ptr == NULL)
                {
                    free(allocation);
                }
            }
            else
            {
                if (ptr != NULL)
                {
                    allocation->ptr = result;
                }
                else
                {
                    /* add block */
                    allocation->ptr = result;
                    allocation->next = head;
                    head = allocation;
                }
            }
        }

        (void)Unlock(gballocThreadSafeLock);
    }

    return result;
}

void gballoc_free(void* ptr)
{
    ALLOCATION* curr = head;
    ALLOCATION* prev = NULL;

    if (gballocState != GBALLOC_STATE_INIT)
    {
        /* Codes_SRS_GBALLOC_01_042: [If gballoc was not initialized gballoc_free shall shall simply call free.] */
        free(ptr);
    }
    /* Codes_SRS_GBALLOC_01_033: [gballoc_free shall ensure thread safety by using the lock created by gballoc_Init.] */
    else if (LOCK_OK != Lock(gballocThreadSafeLock))
    {
        /* Codes_SRS_GBALLOC_01_049: [If acquiring the lock fails, gballoc_free shall do nothing.] */
        LogError("Failed to get the Lock.");
    }
    else
    {
        /* Codes_SRS_GBALLOC_01_009: [gballoc_free shall also look up the size associated with the ptr pointer and release the used memory.] */
        while (curr != NULL)
        {
            if (curr->ptr == ptr)
            {
                /* Codes_SRS_GBALLOC_01_008: [gballoc_free shall call the C99 free function.] */
                free(ptr);
                if (prev != NULL)
                {
                    prev->next = curr->next;
                }
                else
                {
                    head = (ALLOCATION*)curr->next;
                }

                free(curr);
                break;
            }

            prev = curr;
            curr = (ALLOCATION*)curr->next;
        }

        if ((curr == NULL) && (ptr != NULL))
        {
            /* Codes_SRS_GBALLOC_01_019: [When the ptr pointer cannot be found in the pointers tracked by gballoc, gballoc_free shall not free any memory.] */

            /* could not find the allocation */
            LogError("Could not free allocation for address %p (not found)", ptr);
        }
        (void)Unlock(gballocThreadSafeLock);
    }
}

#endif // GB_USE_CUSTOM_HEAP

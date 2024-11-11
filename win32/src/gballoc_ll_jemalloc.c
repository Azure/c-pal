// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>  // for snprintf

#include "c_logging/logger.h"
#include "c_pal/gballoc_ll.h"

#include "macro_utils/macro_utils.h" // for MU_FAILURE

#include "jemalloc/jemalloc.h"

int gballoc_ll_init(void* params)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_001: [ gballoc_ll_init shall return 0. ]*/
    (void)params;
    return 0;
}

void gballoc_ll_deinit(void)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_002: [ gballoc_ll_deinit shall return. ]*/
}

static void* gballoc_ll_malloc_internal(size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_003: [ gballoc_ll_malloc shall call je_malloc and returns what je_malloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_002: [ gballoc_ll_malloc_2 shall call je_malloc(nmemb*size) and returns what je_malloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_005: [ gballoc_ll_malloc_flex shall return what je_malloc(base + nmemb * size) returns. ]*/
    result = je_malloc(size);

    if (result == NULL)
    {
        LogError("failure in je_malloc(size=%zu)", size);
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
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_001: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_2 shall fail and return NULL. ]*/
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
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_002: [ gballoc_ll_malloc_2 shall call je_malloc(nmemb*size) and returns what je_malloc returned. ]*/
        result = gballoc_ll_malloc_internal(nmemb * size);
    }
    return result;
}

void* gballoc_ll_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_004: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_malloc_flex shall fail and return NULL. ]*/
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
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_005: [ gballoc_ll_malloc_flex shall return what je_malloc(base + nmemb * size) returns. ]*/
        result = gballoc_ll_malloc_internal(base + nmemb * size);
    }
    
    return result;
}

void gballoc_ll_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_004: [ gballoc_ll_free shall call je_free(ptr). ]*/
    je_free(ptr);
}

void* gballoc_ll_calloc(size_t nmemb, size_t size)
{
    void* result;

    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_005: [ gballoc_ll_calloc shall call je_calloc(nmemb, size) and return what je_calloc returned. ]*/
    if ((result = je_calloc(nmemb, size)) == NULL)
    {
        LogError("failure in je_calloc(nmemb=%zu, size=%zu)", nmemb, size);
    }

    return result;
}

static void* gballoc_ll_realloc_internal(void* ptr, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_006: [ gballoc_ll_realloc calls je_realloc(ptr, size) and returns what je_realloc returned. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_007: [ gballoc_ll_realloc_2 shall return what je_realloc(ptr, nmemb * size) returns. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_010: [ gballoc_ll_realloc_flex shall return what je_realloc(ptr, base + nmemb * size) returns. ]*/
    result = je_realloc(ptr, size);

    if (result == NULL)
    {
        LogError("failure in je_realloc(ptr=%p, size=%zu)", ptr, size);
    }

    return result;
}

void* gballoc_ll_realloc(void* ptr, size_t size)
{
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_006: [ gballoc_ll_realloc calls je_realloc(ptr, size) and returns what je_realloc returned. ]*/
    return gballoc_ll_realloc_internal(ptr, size);
}

void* gballoc_ll_realloc_2(void* ptr, size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_006: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_2 shall fail and return NULL. ]*/
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
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_007: [ gballoc_ll_realloc_2 shall return what je_realloc(ptr, nmemb * size) returns. ]*/
        result = gballoc_ll_realloc_internal(ptr, nmemb * size);
    }
    return result;
}

void* gballoc_ll_realloc_flex(void* ptr, size_t base, size_t nmemb, size_t size)
{
    void* result;
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_008: [ If nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/
    /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_009: [ If base + nmemb * size exceeds SIZE_MAX then gballoc_ll_realloc_flex shall fail and return NULL. ]*/

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
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_02_010: [ gballoc_ll_realloc_flex shall return what je_realloc(ptr, base + nmemb * size) returns. ]*/
        result = gballoc_ll_realloc_internal(ptr, base + nmemb * size);
    }

    return result;
}

size_t gballoc_ll_size(void* ptr)
{
    size_t result;

    /*Codes_SRS_GBALLOC_LL_JEMALLOC_01_007: [ gballoc_ll_size shall call je_malloc_usable_size and return what je_malloc_usable_size returned. ]*/
    result = je_malloc_usable_size(ptr);

    return result;
}

static void jemalloc_print_stats_callback(void* context, const char* text)
{
    (void)context;

    if (text == NULL)
    {
        /* Codes_SRS_GBALLOC_LL_JEMALLOC_01_009: [ If text is NULL, jemalloc_print_stats_callback shall return. ]*/
    }
    else
    {
        /* Codes_SRS_GBALLOC_LL_JEMALLOC_01_010: [ Otherwise, jemalloc_print_stats_callback shall print (log) text, breaking it does in chunks of LOG_MAX_MESSAGE_LENGTH / 2. ]*/
        size_t text_length = strlen(text);
        size_t pos = 0;
        while (pos < text_length)
        {
            size_t chars_to_print = text_length - pos;
            if (chars_to_print > LOG_MAX_MESSAGE_LENGTH / 2)
            {
                chars_to_print = LOG_MAX_MESSAGE_LENGTH / 2;
            }

            LogInfo("%.*s", (int)chars_to_print, text + pos);
            pos += chars_to_print;
        }
    }
}

void gballoc_ll_print_stats(void)
{
    /* Codes_SRS_GBALLOC_LL_JEMALLOC_01_008: [ gballoc_ll_print_stats shall call je_malloc_stats_print and pass to it jemalloc_print_stats_callback as print callback. ]*/
    je_malloc_stats_print(jemalloc_print_stats_callback, NULL, NULL);
}

int gballoc_ll_set_decay(int64_t decay_milliseconds)
{
    int result = 0;

    /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_001: [ gballoc_ll_set_decay shall advance jemalloc's internal epoch counter. ]*/
    if (je_mallctl("epoch", NULL, NULL, NULL, 0) != 0) {
        /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
        LogError("failed to advance jemalloc epoch");
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_002: [ gballoc_ll_set_decay shall set the dirty decay time for new arenas to decay_milliseconds milliseconds. ]*/
        if (je_mallctl("arenas.dirty_decay_ms", NULL, NULL, &decay_milliseconds, sizeof(decay_milliseconds)) != 0) {
            /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
            LogError("failed to set dirty decay time");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_003: [ gballoc_ll_set_decay shall set the muzzy decay time for new arenas to decay_milliseconds milliseconds. ]*/
            if (je_mallctl("arenas.muzzy_decay_ms", NULL, NULL, &decay_milliseconds, sizeof(decay_milliseconds)) != 0) {
                /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
                LogError("failed to set muzzy decay time"); 
                result = MU_FAILURE; 
            }
            else
            {
                uint64_t narenas;

                /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_004: [ gballoc_ll_set_decay shall fetch the number of existing jemalloc arenas. ]*/
                if (je_mallctl("arenas.narenas", &narenas, &(size_t){sizeof(narenas)}, NULL, 0) != 0) {
                    /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
                    LogError("failed to get number of arenas");
                    result = MU_FAILURE;
                }
                else
                {
                    char command[64];

                    /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_005: [ For each existing arena: ]*/
                    for (uint64_t i = 0; i < narenas; i++) {
                        int snprintf_result = snprintf(command, sizeof(command), "arena.%" PRIu64 ".dirty_decay_ms", i);

                        if (snprintf_result < 0)
                        {
                            /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
                            LogError("snprintf failed for arena %" PRIu64 "", i);
                            result = MU_FAILURE;
                        }
                        else
                        {
                            /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_006: [ gballoc_ll_set_decay shall set the dirty decay time for the arena to decay_milliseconds milliseconds. ]*/
                            if (je_mallctl(command, NULL, NULL, &decay_milliseconds, sizeof(decay_milliseconds)) != 0) {
                                /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
                                LogError("failed to set dirty decay time for arena %" PRIu64 "", i);
                                result = MU_FAILURE;
                                break;
                            }

                            snprintf_result = snprintf(command, sizeof(command), "arena.%" PRIu64 ".muzzy_decay_ms", i);

                            if (snprintf_result < 0)
                            {
                                /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
                                LogError("snprintf failed for arena %" PRIu64 "", i);
                                result = MU_FAILURE;
                            }
                            else
                            {
                                /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_007: [ gballoc_ll_set_decay shall set the muzzy decay time for the arena to decay_milliseconds milliseconds. ]*/
                                if (je_mallctl(command, NULL, NULL, &decay_milliseconds, sizeof(decay_milliseconds)) != 0) {
                                    /* Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ If there are any errors, gballoc_ll_set_decay shall fail and return a non-zero value. ]*/
                                    LogError("failed to set muzzy decay time for arena %" PRIu64 "", i);
                                    result = MU_FAILURE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

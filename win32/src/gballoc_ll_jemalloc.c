// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>  // for snprintf

#include "macro_utils/macro_utils.h" // for MU_FAILURE

#include "c_logging/logger.h"
#include "c_pal/gballoc_ll.h"

#include "jemalloc/jemalloc.h"

// We use int64_t for decay ms, so we need to make sure it's the same size as size_t due to internal jemalloc code using ssize_t
MU_STATIC_ASSERT(sizeof(int64_t) == sizeof(size_t));

// We use uint32_t for number of arenas, so we need to make sure it's the same size as unsigned due to internal jemalloc code using unsigned
MU_STATIC_ASSERT(sizeof(uint32_t) == sizeof(unsigned));

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

static int gballoc_ll_set_decay(int64_t decay_milliseconds, bool is_dirty_decay)
{
    int result;

    const char* decay_type = is_dirty_decay ? "dirty" : "muzzy";
    const char* option_arenas = is_dirty_decay ? "arenas.dirty_decay_ms" : "arenas.muzzy_decay_ms";
    const char* option_arena = is_dirty_decay ? "arena.%" PRIu32 ".dirty_decay_ms" : "arena.%" PRIu32 ".muzzy_decay_ms";

    if (decay_milliseconds < -1)
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_019: [ If decay_milliseconds is less than -1, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
        LogError("decay_milliseconds must be -1 or greater, decay_milliseconds=%" PRId64 "", decay_milliseconds);
        result = MU_FAILURE;
    }
    else
    {
        int64_t old_decay_milliseconds = -1;
        size_t old_decay_milliseconds_size = sizeof(old_decay_milliseconds);

        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_005: [ gballoc_ll_set_option shall retrieve the old decay value and set the new decay value to decay_milliseconds for new arenas by calling je_mallctl with arenas.dirty_decay_ms if option_name is dirty_decay or arenas.muzzy_decay_ms if option_name is muzzy_decay as the command. ]*/
        if (je_mallctl(option_arenas, &old_decay_milliseconds, &old_decay_milliseconds_size, &decay_milliseconds, sizeof(decay_milliseconds)) != 0)
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
            LogError("je_mallctl(option_arenas=%s, &old_decay_milliseconds=%p, &old_decay_milliseconds_size=%p, &decay_milliseconds=%p, sizeof(decay_milliseconds)=%zu) failed, old_decay_milliseconds=%" PRId64 "", option_arenas, &old_decay_milliseconds, &old_decay_milliseconds_size, &decay_milliseconds, sizeof(decay_milliseconds), old_decay_milliseconds);
            result = MU_FAILURE;
        }
        else
        {
            LogInfo("jemalloc %s_decay_ms set to %" PRId64 " for new arenas, old value was %" PRId64 "", decay_type, decay_milliseconds, old_decay_milliseconds);

            uint32_t narenas;
            size_t narenas_size = sizeof(narenas);

            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_007: [ gballoc_ll_set_option shall fetch the number of existing jemalloc arenas by calling je_mallctl with opt.narenas as the command. ]*/
            if (je_mallctl("opt.narenas", &narenas, &narenas_size, NULL, 0) != 0)
            {
                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                LogError("je_mallctl(opt.narenas, &narenas=%p, &narenas_size=%p, NULL, 0) failed", &narenas, &narenas_size);
                result = MU_FAILURE;
            }
            else
            {
                char command[64];
                uint32_t i;
                int snprintf_result;
                int arena_decay_result;

                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ For each existing arena except last (since it is reserved for huge arena) ]*/
                for (i = 0; i < narenas; i++)
                {
                    snprintf_result = snprintf(command, sizeof(command), option_arena, i);

                    if (snprintf_result < 0 || (size_t)snprintf_result >= sizeof(command))
                    {
                        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                        LogError("snprintf(command=%p, sizeof(command)=%zu, option_arena=%s, i=%" PRIu32 ") failed %s", command, sizeof(command), option_arena, i, (snprintf_result < 0) ? " with encoding error" : " due to insufficient buffer size");
                        break;
                    }
                    else
                    {
                        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_009: [ gballoc_ll_set_option shall set the decay time for the arena to decay_milliseconds milliseconds by calling je_mallctl with arena.<i>.dirty_decay_ms if option_name is dirty_decay or arena.<i>.muzzy_decay_ms if option_name is muzzy_decay as the command. ]*/
                        arena_decay_result = je_mallctl(command, NULL, NULL, &decay_milliseconds, sizeof(decay_milliseconds));
                        if (arena_decay_result != 0)
                        {
                            if (arena_decay_result == EFAULT)
                            {
                                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_020: [ If je_mallctl returns EFAULT, gballoc_ll_set_option shall continue without failing as this error is expected when the arena doesn't exist. ]*/
                                LogError("je_mallctl(command=%s, NULL, NULL, &decay_milliseconds=%p, sizeof(decay_milliseconds)=%zu) failed, continuing without failure as the arena doesn't exist, old_decay_milliseconds=%" PRId64 "", command, &decay_milliseconds, sizeof(decay_milliseconds), old_decay_milliseconds);
                            }
                            else
                            {
                                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                                LogError("je_mallctl(command=%s, NULL, NULL, &decay_milliseconds=%p, sizeof(decay_milliseconds)=%zu) failed, old_decay_milliseconds=%" PRId64 "", command, &decay_milliseconds, sizeof(decay_milliseconds), old_decay_milliseconds);
                                break;
                            }
                        }
                    }
                }

                if (i < narenas)
                {
                    for (uint32_t j = 0; j < i; j++)
                    {
                        snprintf_result = snprintf(command, sizeof(command), option_arena, j);
                        if (snprintf_result < 0 || (size_t)snprintf_result >= sizeof(command))
                        {
                            LogError("snprintf(command=%p, sizeof(command)=%zu, option_arena=%s, j=%" PRIu32 ") failed %s during cleanup", command, sizeof(command), option_arena, j, (snprintf_result < 0) ? " with encoding error" : " due to insufficient buffer size");
                        }
                        else
                        {
                            arena_decay_result = je_mallctl(command, NULL, NULL, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
                            if (arena_decay_result != 0)
                            {
                                if (arena_decay_result == EFAULT)
                                {
                                    LogError("je_mallctl(command=%s, NULL, NULL, &old_decay_milliseconds=%p, sizeof(old_decay_milliseconds)=%zu) failed during cleanup as the arenas doesn't exist", command, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
                                }
                                else
                                {
                                    LogError("je_mallctl(command=%s, NULL, NULL, &old_decay_milliseconds=%p, sizeof(old_decay_milliseconds)=%zu) failed during cleanup", command, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
                                }
                            }
                        }
                    }
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                    LogInfo("jemalloc %s_decay_ms set to %" PRId64 ", old value was %" PRId64 "", decay_type, decay_milliseconds, old_decay_milliseconds);
                    goto all_ok;
                }
            }
            
            if (je_mallctl(option_arenas, NULL, NULL, &old_decay_milliseconds, sizeof(old_decay_milliseconds)) != 0)
            {
                LogError("je_mallctl(option_arenas=%s, NULL, NULL, &old_decay_milliseconds=%p, sizeof(old_decay_milliseconds)=%zu) failed during cleanup", option_arenas, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
            }
        }
    }

all_ok:
    return result;
}

int gballoc_ll_set_option(const char* option_name, void* option_value)
{
    int result;

    if (/*Codes_SRS_GBALLOC_LL_JEMALLOC_28_001: [ If option_name is NULL, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
        option_name == NULL ||
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_002: [ If option_value is NULL, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
        option_value == NULL)
    {
        LogError("Invalid args: const char* option_name = %s, void* option_value = %p", MU_P_OR_NULL(option_name), option_value);
        result = MU_FAILURE;
    }
    else
    {
        if (strcmp(option_name, "dirty_decay") == 0)
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_003: [ If option_name has value as dirty_decay or muzzy_decay: ]*/
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_004: [ gballoc_ll_set_option shall fetch the decay_milliseconds value by casting option_value to int64_t. ]*/
            int64_t decay_milliseconds = *(int64_t*)option_value;
            result = gballoc_ll_set_decay(decay_milliseconds, true);
        }
        else if (strcmp(option_name, "muzzy_decay") == 0)
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_003: [ If option_name has value as dirty_decay or muzzy_decay: ]*/
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_004: [ gballoc_ll_set_option shall fetch the decay_milliseconds value by casting option_value to int64_t. ]*/
            int64_t decay_milliseconds = *(int64_t*)option_value;
            result = gballoc_ll_set_decay(decay_milliseconds, false);
        }
        else
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_017: [ Otherwise gballoc_ll_set_option shall fail and return a non-zero value. ]*/
            LogError("Unknown option: %s", option_name);
            result = MU_FAILURE;
        }
    }

    return result;
}

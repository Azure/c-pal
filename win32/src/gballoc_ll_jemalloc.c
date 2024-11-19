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

#ifdef _MSC_VER
    #include <stdint.h> // For intptr_t
    typedef intptr_t ssize_t;
#endif

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

static int gballoc_ll_set_dirty_decay(int64_t decay_milliseconds)
{
    int result;

    if (decay_milliseconds < -1)
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_019: [ If decay_milliseconds is less than -1, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
        LogError("decay_milliseconds must be -1 or greater, decay_milliseconds=%" PRId64 "", decay_milliseconds);
        result = MU_FAILURE;
    }
    else
    {
        int64_t old_decay_milliseconds;
        size_t old_decay_milliseconds_size = sizeof(old_decay_milliseconds);
        MU_STATIC_ASSERT(sizeof(int64_t) == sizeof(ssize_t));

        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_005: [ gballoc_ll_set_option shall retrieve the old value of dirty decay by calling je_mallctl with arenas.dirty_decay_ms as the command. ]*/
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_006: [ gballoc_ll_set_option shall set the dirty decay time for new arenas to decay_milliseconds milliseconds by calling je_mallctl with arenas.dirty_decay_ms as the command. ]*/
        if (je_mallctl("arenas.dirty_decay_ms", &old_decay_milliseconds, &old_decay_milliseconds_size, &decay_milliseconds, sizeof(decay_milliseconds)) != 0)
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
            LogError("je_mallctl(const char* name=arenas.dirty_decay_ms, void* oldp=%p, size_t* oldlenp=%p, void* newp=%p, size_t newlen=%zu) failed", &old_decay_milliseconds, &old_decay_milliseconds_size, &decay_milliseconds, sizeof(decay_milliseconds));
            result = MU_FAILURE;
        }
        else
        {
            uint32_t narenas;
            size_t narenas_size = sizeof(narenas);
            MU_STATIC_ASSERT(sizeof(uint32_t) == sizeof(unsigned));

            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_007: [ gballoc_ll_set_option shall fetch the number of existing jemalloc arenas by calling je_mallctl with arenas.narenas as the command. ]*/
            if (je_mallctl("arenas.narenas", &narenas, &narenas_size, NULL, 0) != 0)
            {
                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                LogError("je_mallctl(const char* name=arenas.narenas, void* oldp=%p, size_t* oldlenp=%p, void* newp=NULL, size_t newlen=0) failed", &narenas, &narenas_size);
                result = MU_FAILURE;
            }
            else
            {
                char command[64];
                uint32_t i;

                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_008: [ For each existing arena ]*/
                for (i = 0; i < narenas; i++)
                {
                    int snprintf_result = snprintf(command, sizeof(command), "arena.%" PRIu32 ".dirty_decay_ms", i);

                    if (snprintf_result < 0 || (size_t)snprintf_result >= sizeof(command))
                    {
                        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                        LogError("snprintf(const char* buffer=%p, size_t size=%zu, const char* format=arena.<i>.dirty_decay_ms, i=%" PRIu32 ") failed %s", command, sizeof(command), i, (snprintf_result < 0) ? " with encoding error" : " due to insufficient buffer size");
                        result = MU_FAILURE;
                        break;
                    }
                    else
                    {
                        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_009: [ gballoc_ll_set_option shall set the dirty decay time for the arena to decay_milliseconds milliseconds by calling je_mallctl with arena.<i>.dirty_decay_ms as the command. ]*/
                        int arena_decay_result = je_mallctl(command, NULL, NULL, &decay_milliseconds, sizeof(decay_milliseconds));
                        if (arena_decay_result != 0)
                        {
                            if (arena_decay_result == EFAULT)
                            {
                                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_020: [ If je_mallctl returns EFAULT, gballoc_ll_set_option shall continue without failing as this error is expected when the arena is deleted or is a huge arena. ]*/
                                LogError("je_mallctl(const char* name=%s, void* oldp=NULL, size_t* oldlenp=NULL, void* newp=%p, size_t newlen=%zu) failed, continuing without failure as the arena is either deleted or it is a huge arena, old_decay_milliseconds=%" PRId64 "", command, &decay_milliseconds, sizeof(decay_milliseconds), old_decay_milliseconds);
                            }
                            else
                            {
                                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                                LogError("je_mallctl(const char* name=%s, void* oldp=NULL, size_t* oldlenp=NULL, void* newp=%p, size_t newlen=%zu) failed, old_decay_milliseconds=%" PRId64 "", command, &decay_milliseconds, sizeof(decay_milliseconds), old_decay_milliseconds);
                                result = MU_FAILURE;
                                break;
                            }
                        }
                    }
                }

                if (i < narenas)
                {
                    for (uint32_t j = 0; j < i; j++)
                    {
                        (void)snprintf(command, sizeof(command), "arena.%" PRIu32 ".dirty_decay_ms", j);
                        (void)je_mallctl(command, NULL, NULL, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
                    }
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                    LogInfo("jemalloc dirty_decay_ms set to %" PRId64 ", old value was %" PRId64 "", decay_milliseconds, old_decay_milliseconds);
                    goto all_ok;
                }
            }
            (void)je_mallctl("arenas.dirty_decay_ms", NULL, NULL, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
        }
    }

all_ok:
    return result;
}

static int gballoc_ll_set_muzzy_decay(int64_t decay_milliseconds)
{
    int result;

    if (decay_milliseconds < -1)
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_019: [ If decay_milliseconds is less than -1, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
        LogError("decay_milliseconds must be -1 or greater, decay_milliseconds=%" PRId64 "", decay_milliseconds);
        result = MU_FAILURE;
    }
    else
    {
        int64_t old_decay_milliseconds;
        size_t old_decay_milliseconds_size = sizeof(old_decay_milliseconds);
        MU_STATIC_ASSERT(sizeof(int64_t) == sizeof(ssize_t));

        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_012: [ gballoc_ll_set_option shall retrieve the old value of muzzy decay by calling je_mallctl with arenas.muzzy_decay_ms as the command. ]*/
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_013: [ gballoc_ll_set_option shall set the muzzy decay time for new arenas to decay_milliseconds milliseconds by calling je_mallctl with arenas.muzzy_decay_ms as the command. ]*/
        if (je_mallctl("arenas.muzzy_decay_ms", &old_decay_milliseconds, &old_decay_milliseconds_size, &decay_milliseconds, sizeof(decay_milliseconds)) != 0)
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
            LogError("je_mallctl(const char* name=arenas.muzzy_decay_ms, void* oldp=%p, size_t* oldlenp=%p, void* newp=%p, size_t newlen=%zu) failed", &old_decay_milliseconds, &old_decay_milliseconds_size, &decay_milliseconds, sizeof(decay_milliseconds));
            result = MU_FAILURE;
        }
        else
        {
            
            uint32_t narenas;
            size_t narenas_size = sizeof(narenas);
            MU_STATIC_ASSERT(sizeof(uint32_t) == sizeof(unsigned));

            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_014: [ gballoc_ll_set_option shall fetch the number of existing jemalloc arenas by calling je_mallctl with arenas.narenas as the command. ]*/
            if (je_mallctl("arenas.narenas", &narenas, &narenas_size, NULL, 0) != 0)
            {
                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                LogError("je_mallctl(const char* name=arenas.narenas, void* oldp=%p, size_t* oldlenp=%p, void* newp=NULL, size_t newlen=0) failed", &narenas, &narenas_size);
                result = MU_FAILURE;
            }
            else
            {
                char command[64];
                uint32_t i;

                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_015: [ For each existing arena ]*/
                for (i = 0; i < narenas; i++)
                {
                    int snprintf_result = snprintf(command, sizeof(command), "arena.%" PRIu32 ".muzzy_decay_ms", i);

                    if (snprintf_result < 0 || (size_t)snprintf_result >= sizeof(command))
                    {
                        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                        LogError("snprintf(const char* buffer=%p, size_t size=%zu, const char* format=arena.<i>.muzzy_decay_ms, i=%" PRIu32 ") failed %s", command, sizeof(command), i, (snprintf_result < 0) ? " with encoding error" : " due to insufficient buffer size");
                        result = MU_FAILURE;
                        break;
                    }
                    else
                    {
                        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_016: [ gballoc_ll_set_option shall set the muzzy decay time for the arena to decay_milliseconds milliseconds by calling je_mallctl with arena.<i>.muzzy_decay_ms as the command. ]*/
                        int arena_decay_result = je_mallctl(command, NULL, NULL, &decay_milliseconds, sizeof(decay_milliseconds));
                        if (arena_decay_result != 0)
                        {
                            if (arena_decay_result == EFAULT)
                            {
                                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_020: [ If je_mallctl returns EFAULT, gballoc_ll_set_option shall continue without failing as this error is expected when the arena is deleted or is a huge arena. ]*/
                                LogError("je_mallctl(const char* name=%s, void* oldp=NULL, size_t* oldlenp=NULL, void* newp=%p, size_t newlen=%zu) failed, continuing without failure as the arena is either deleted or it is a huge arena, old_decay_milliseconds=%" PRId64 "", command, &decay_milliseconds, sizeof(decay_milliseconds), old_decay_milliseconds);
                            }
                            else
                            {
                                /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_018: [ If there are any errors, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
                                LogError("je_mallctl(const char* name=%s, void* oldp=NULL, size_t* oldlenp=NULL, void* newp=%p, size_t newlen=%zu) failed, old_decay_milliseconds=%" PRId64 "", command, &decay_milliseconds, sizeof(decay_milliseconds), old_decay_milliseconds);
                                result = MU_FAILURE;
                                break;
                            }
                        }
                    }
                }

                if (i < narenas)
                {
                    for (uint32_t j = 0; j < i; j++)
                    {
                        (void)snprintf(command, sizeof(command), "arena.%" PRIu32 ".muzzy_decay_ms", j);
                        (void)je_mallctl(command, NULL, NULL, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
                    }
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                    LogInfo("jemalloc muzzy_decay_ms set to %" PRId64 ", old value was %" PRId64 "", decay_milliseconds, old_decay_milliseconds);
                    goto all_ok;
                }
            }
            (void)je_mallctl("arenas.muzzy_decay_ms", NULL, NULL, &old_decay_milliseconds, sizeof(old_decay_milliseconds));
        }
    }
all_ok:
    return result;
}

int gballoc_ll_set_option(char* option_name, void* option_value)
{
    int result;

    if (/*Codes_SRS_GBALLOC_LL_JEMALLOC_28_001: [ If option_name is NULL, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
        option_name == NULL ||
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_002: [ If option_value is NULL, gballoc_ll_set_option shall fail and return a non-zero value. ]*/
        option_value == NULL)
    {
        LogError("Invalid args: char* option_name = %p, void* option_value = %p",
            option_name, option_value);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_003: [ If option_name has value as dirty_decay: ]*/
        if (strcmp(option_name, "dirty_decay") == 0)
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_004: [ gballoc_ll_set_option shall fetch the decay_milliseconds value by casting option_value to int64_t. ]*/
            int64_t decay_milliseconds = *(int64_t*)option_value;
            result = gballoc_ll_set_dirty_decay(decay_milliseconds);
        }
        /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_010: [ Else if option_name has value as muzzy_decay: ]*/
        else if (strcmp(option_name, "muzzy_decay") == 0)
        {
            /*Codes_SRS_GBALLOC_LL_JEMALLOC_28_011: [ gballoc_ll_set_option shall fetch the decay_milliseconds value by casting option_value to int64_t. ]*/
            int64_t decay_milliseconds = *(int64_t*)option_value;
            result = gballoc_ll_set_muzzy_decay(decay_milliseconds);
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

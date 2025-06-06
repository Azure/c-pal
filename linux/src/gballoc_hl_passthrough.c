// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include <string.h>                   // for memset

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_ll.h"

#include "c_pal/gballoc_hl.h"

int gballoc_hl_init(void* gballoc_hl_init_params, void* gballoc_ll_init_params)
{
    (void)gballoc_hl_init_params; /*are ignored, this is "passthrough*/

    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_42_001: [ gballoc_hl_init shall call gballoc_ll_init as function to execute and gballoc_ll_init_params as parameter and return the result. ]*/
    return gballoc_ll_init(gballoc_ll_init_params);
}

void gballoc_hl_deinit(void)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_004: [ gballoc_hl_deinit shall call gballoc_ll_deinit. ]*/
    gballoc_ll_deinit();
}

void* gballoc_hl_malloc(size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_005: [ gballoc_hl_malloc shall call gballoc_ll_malloc(size) and return what gballoc_ll_malloc returned. ]*/
    void* result = gballoc_ll_malloc(size);

    if (result == NULL)
    {
        LogError("failure in gballoc_ll_malloc(size=%zu)", size);
    }
    return result;
}

void* gballoc_hl_malloc_2(size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_030: [ gballoc_hl_malloc_2 shall call gballoc_ll_malloc_2(size) and return what gballoc_ll_malloc_2 returned. ]*/
    void* result = gballoc_ll_malloc_2(nmemb, size);

    if (result == NULL)
    {
        LogError("failure in gballoc_ll_malloc_2(nmemb=%zu, size=%zu)", nmemb, size);
    }
    return result;
}

void* gballoc_hl_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_033: [ gballoc_hl_malloc_flex shall call gballoc_ll_malloc_flex(size) and return what gballoc_hl_malloc_flex returned. ]*/
    void* result = gballoc_ll_malloc_flex(base, nmemb, size);

    if (result == NULL)
    {
        LogError("failure in gballoc_ll_malloc_2(base=%zu, nmemb=%zu, size=%zu)", base, nmemb, size);
    }
    return result;
}

void gballoc_hl_free(void* ptr)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_006: [ gballoc_hl_free shall call gballoc_ll_free(ptr). ]*/
    gballoc_ll_free(ptr);
}

size_t gballoc_hl_size(void* ptr)
{
    /* Codes_SRS_GBALLOC_HL_PASSTHROUGH_01_003: [ Otherwise, gballoc_hl_size shall call gballoc_ll_size with ptr as argument and return the result of gballoc_ll_size. ]*/
    return gballoc_ll_size(ptr);
}

void* gballoc_hl_calloc(size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_007: [ gballoc_hl_calloc shall call gballoc_ll_calloc(nmemb, size) and return what gballoc_ll_calloc returned. ]*/
    void* result = gballoc_ll_calloc(nmemb, size);

    if (result == NULL)
    {
        LogError("failure in gballoc_ll_calloc(nmemb=%zu, size=%zu)", nmemb, size);
    }

    return result;
}


void* gballoc_hl_realloc(void* ptr, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_008: [ gballoc_hl_realloc shall call gballoc_ll_realloc(ptr, size) and return what gballoc_ll_realloc returned. ]*/
    void* result = gballoc_ll_realloc(ptr, size);

    if (result == NULL)
    {
        LogError("Failure in gballoc_ll_realloc(ptr=%p, size=%zu)", ptr, size);
    }

    return result;
}

void* gballoc_hl_realloc_2(void* ptr, size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_036: [ gballoc_hl_realloc_2 shall call gballoc_ll_realloc_2(ptr, nmemb, size) and return what gballoc_ll_realloc_2 returned. ]*/
    void* result = gballoc_ll_realloc_2(ptr, nmemb, size);

    if (result == NULL)
    {
        LogError("Failure in gballoc_ll_realloc_2(ptr=%p, nmemb=%zu, size=%zu)", ptr, nmemb, size);
    }

    return result;
}

void* gballoc_hl_realloc_flex(void* ptr, size_t base, size_t nmemb, size_t size)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_039: [ gballoc_hl_realloc_flex shall call gballoc_ll_realloc_flex(ptr, base, nmemb, size) and return what gballoc_ll_realloc_flex returned. ]*/
    void* result = gballoc_ll_realloc_flex(ptr, base, nmemb, size);

    if (result == NULL)
    {
        LogError("Failure in gballoc_ll_realloc_flex(ptr=%p, base=%zu, nmemb=%zu, size=%zu)", ptr, base, nmemb, size);
    }

    return result;
}

void gballoc_hl_reset_counters(void)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_009: [ gballoc_hl_reset_counters shall return. ]*/
    return;
}

int gballoc_hl_get_malloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_010: [ gballoc_hl_get_malloc_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
    (void)memset(latency_buckets_out, 0, sizeof(GBALLOC_LATENCY_BUCKETS));
    return 0;
}

int gballoc_hl_get_realloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_011: [ gballoc_hl_get_realloc_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
    (void)memset(latency_buckets_out, 0, sizeof(GBALLOC_LATENCY_BUCKETS));
    return 0;
}

int gballoc_hl_get_calloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_012: [ gballoc_hl_get_calloc_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
    (void)memset(latency_buckets_out, 0, sizeof(GBALLOC_LATENCY_BUCKETS));
    return 0;
}

int gballoc_hl_get_free_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_013: [ gballoc_hl_get_free_latency_buckets shall set latency_buckets_out's bytes all to 0 and return 0. ]*/
    (void)memset(latency_buckets_out, 0, sizeof(GBALLOC_LATENCY_BUCKETS));
    return 0;
}


static const GBALLOC_LATENCY_BUCKET_METADATA latency_buckets_metadata[GBALLOC_LATENCY_BUCKET_COUNT] =
{
    { "Bucket [0-511]", 0, 511 },
    { "Bucket [512-1023]", 512, 1023 },
    { "Bucket [1024-2047]", 1024, 2047 },
    { "Bucket [2048-4095]", 2048, 4095 },
    { "Bucket [4096-8191]", 4096, 8191 },
    { "Bucket [8192-16383]", 8192, 16383 },
    { "Bucket [16384-32767]", 16384, 32767 },
    { "Bucket [32768-65535]", 32768, 65535 },
    { "Bucket [65536-131071]", 65536, 131071 },
    { "Bucket [131072-262143]", 131072, 262143 },
    { "Bucket [262144-524287]", 262144, 524287 },
    { "Bucket [524288-1048575]", 524288, 1048575 },
    { "Bucket [1048576-2097151]", 1048576, 2097151 },
    { "Bucket [2097152-4194303]", 2097152, 4194303 },
    { "Bucket [4194304-8388607]", 4194304, 8388607 },
    { "Bucket [8388608-16777215]", 8388608, 16777215 },
    { "Bucket [16777216-33554431]", 16777216, 33554431 },
    { "Bucket [33554432-67108863]", 33554432, 67108863 },
    { "Bucket [67108864-134217727]", 67108864, 134217727 },
    { "Bucket [134217728-268435455]", 134217728, 268435455 },
    { "Bucket [268435456-536870911]", 268435456, 536870911 },
    { "Bucket [536870912-1073741823]", 536870912, 1073741823 },
    { "Bucket [1073741824-2147483647]", 1073741824, 2147483647 },
    { "Bucket [2147483648-4294967295]", 2147483648, 4294967295 }
};


const GBALLOC_LATENCY_BUCKET_METADATA* gballoc_hl_get_latency_bucket_metadata(void)
{
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_014: [ gballoc_hl_get_latency_bucket_metadata shall return an array of size LATENCY_BUCKET_COUNT that contains the metadata for each latency bucket. ]*/
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_015: [ The first latency bucket shall be [0-511]. ]*/
    /*Codes_SRS_GBALLOC_HL_PASSTHROUGH_02_016: [ Each consecutive bucket shall be [1 << n, (1 << (n + 1)) - 1], where n starts at 9. ]*/
    return latency_buckets_metadata;
}

int gballoc_hl_set_option(const char* option_name, void* option_value)
{
    /* Codes_SRS_GBALLOC_HL_PASSTHROUGH_28_001: [ gballoc_hl_set_option shall call gballoc_ll_set_option with option_name and option_value as arguments. ]*/
    return gballoc_ll_set_option(option_name, option_value);
}

// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_GBALLOC_HL_H
#define REAL_GBALLOC_HL_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        gballoc_hl_init                          ,\
        gballoc_hl_deinit                        ,\
        gballoc_hl_malloc                        ,\
        gballoc_hl_malloc_2                      ,\
        gballoc_hl_malloc_flex                   ,\
        gballoc_hl_calloc                        ,\
        gballoc_hl_realloc                       ,\
        gballoc_hl_realloc_2                     ,\
        gballoc_hl_realloc_flex                  ,\
        gballoc_hl_free                          ,\
        gballoc_hl_size                          ,\
        gballoc_hl_reset_counters                ,\
        gballoc_hl_get_malloc_latency_buckets    ,\
        gballoc_hl_get_realloc_latency_buckets   ,\
        gballoc_hl_get_calloc_latency_buckets    ,\
        gballoc_hl_get_free_latency_buckets      ,\
        gballoc_hl_get_latency_bucket_metadata   \
)

#include "c_pal/gballoc_hl.h"

#ifdef __cplusplus
extern "C" {
#endif

    int real_gballoc_hl_init(void* hl_params, void* ll_params);
    void real_gballoc_hl_deinit(void);

    void* real_gballoc_hl_malloc(size_t size);
    void* real_gballoc_hl_malloc_2(size_t nmemb, size_t size);
    void* real_gballoc_hl_malloc_flex(size_t base, size_t nmemb, size_t size);
    void* real_gballoc_hl_calloc(size_t nmemb, size_t size);
    void* real_gballoc_hl_realloc(void* ptr, size_t size);
    void* real_gballoc_hl_realloc_2(void* ptr, size_t nmemb, size_t size);
    void* real_gballoc_hl_realloc_flex(void* ptr, size_t base, size_t nmemb, size_t size);
    void real_gballoc_hl_free(void* ptr);
    size_t real_gballoc_hl_size(void* ptr);

    void real_gballoc_hl_reset_counters(void);

    int real_gballoc_hl_get_malloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out);
    int real_gballoc_hl_get_realloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out);
    int real_gballoc_hl_get_calloc_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out);
    int real_gballoc_hl_get_free_latency_buckets(GBALLOC_LATENCY_BUCKETS* latency_buckets_out);

    const GBALLOC_LATENCY_BUCKET_METADATA* real_gballoc_hl_get_latency_bucket_metadata(void);

#ifdef __cplusplus
}
#endif

#endif // REAL_GBALLOC_HL_H

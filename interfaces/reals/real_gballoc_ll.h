// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_GBALLOC_LL_H
#define REAL_GBALLOC_LL_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_GBALLOC_LL_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2,       \
        gballoc_ll_init    ,\
        gballoc_ll_deinit  ,\
        gballoc_ll_malloc  ,\
        gballoc_ll_free    ,\
        gballoc_ll_calloc  ,\
        gballoc_ll_realloc ,\
        gballoc_ll_size     \
)

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int real_gballoc_ll_init(void* params);
    void real_gballoc_ll_deinit(void);

    void* real_gballoc_ll_malloc(size_t size);
    void real_gballoc_ll_free(void* ptr);
    void* real_gballoc_ll_calloc(size_t nmemb, size_t size);
    void* real_gballoc_ll_realloc(void* ptr, size_t size);

    size_t real_gballoc_ll_size(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* REAL_GBALLOC_LL_H */

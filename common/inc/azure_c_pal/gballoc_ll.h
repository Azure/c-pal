// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GBALLOC_LL_H
#define GBALLOC_LL_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ENABLE_MOCKS
#error
#endif
    MOCKABLE_FUNCTION(, int, gballoc_ll_init, void*, params);
    MOCKABLE_FUNCTION(, void, gballoc_ll_deinit);

    MOCKABLE_FUNCTION(, void*, gballoc_ll_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_ll_free, void*, ptr);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_ll_realloc, void*, ptr, size_t, size);

#ifdef __cplusplus
}
#endif

#endif /* GBALLOC_LL_H */

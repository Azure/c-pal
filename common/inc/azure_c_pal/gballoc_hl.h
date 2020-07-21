// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef GBALLOC_HL_H
#define GBALLOC_HL_H

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

    MOCKABLE_FUNCTION(, int, gballoc_hl_init, void*, gballoc_hl_init_params, void*, gballoc_ll_init_params);
    MOCKABLE_FUNCTION(, void, gballoc_hl_deinit);

    MOCKABLE_FUNCTION(, void*, gballoc_hl_malloc, size_t, size);
    MOCKABLE_FUNCTION(, void, gballoc_hl_free, void*, ptr);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_calloc, size_t, nmemb, size_t, size);
    MOCKABLE_FUNCTION(, void*, gballoc_hl_realloc, void*, ptr, size_t, size);

#ifdef __cplusplus
}
#endif

#endif /* GBALLOC_HL_H */

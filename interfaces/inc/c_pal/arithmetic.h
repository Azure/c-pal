// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

typedef struct PAL_UINT128_TAG
{
    uint64_t high;
    uint64_t low;
}PAL_UINT128;

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, PAL_UINT128, umul64x64, uint64_t, left, uint64_t, right);

#ifdef __cplusplus
}
#endif

#endif /* ARITHMETIC_H */

// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdint.h>

#include "c_pal/arithmetic.h"

#if INTPTR_MAX > 0x7FFFFFFFUL
/*only works if compiling on 64 bits*/
PAL_UINT128 umul64x64(uint64_t left, uint64_t right)
{
    /*Codes_SRS_ARITHMETIC_02_001: [ umul64x64 shall call _umul128 and return the result as PAL_UINT128. ]*/
    PAL_UINT128 result;
    result.low = _umul128(left, right, &result.high);
    return result;
}
#else

static PAL_UINT128 zer0 = { 0 };

PAL_UINT128 umul64x64(uint64_t left, uint64_t right)
{
    return zer0;
}

#endif
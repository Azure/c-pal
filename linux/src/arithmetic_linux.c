// Copyright (C) Microsoft Corporation. All rights reserved.

#include "c_pal/arithmetic.h"

/*only works if compiling on 64 bits - for now*/
PAL_UINT128 umul64x64(uint64_t left, uint64_t right)
{
    /*Codes_SRS_ARITHMETIC_02_001: [ umul64x64 shall call _umul128 and return the result as PAL_UINT128. ]*/
    PAL_UINT128 result;
    unsigned __int128 temp = (unsigned __int128)left * right;
    result.low = temp & 0xFFFFFFFFFFFFFFFF;
    result.high = temp >> 64;
    return result;
}

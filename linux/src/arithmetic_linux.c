// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdint.h>

#include "c_pal/arithmetic.h"

#if INTPTR_MAX > 0x7FFFFFFFUL
/*only works if compiling on 64 bits*/
PAL_UINT128 umul64x64(uint64_t left, uint64_t right)
{
    /*Codes_SRS_ARITHMETIC_02_001: [ umul64x64 shall call _umul128 and return the result as PAL_UINT128. ]*/
    PAL_UINT128 result;
    unsigned __int128 temp = ((unsigned __int128)left) * right
    
    result.high = temp>>64;
    result.low = temp;
    
    return result;
}
#else

PAL_UINT128 umul64x64(uint64_t left, uint64_t right) /*see corresponding windows file for more explanations*/
{
    PAL_UINT128 result;

#define Hi(x) ((x)>>32)
#define Lo(x) ((x)&0xFFFFFFFF)
    uint64_t HiHi = Hi(left) * Hi(right);
    uint64_t HiLo = Hi(left) * Lo(right);
    uint64_t LoHi = Lo(left) * Hi(right);
    uint64_t LoLo = Lo(left) * Lo(right);

    uint64_t X = HiLo + Lo(LoHi) + Hi(LoLo);

    result.high = HiHi + Hi(LoHi) + Hi(X);
    result.low = (Lo(X) << 32) + Lo(LoLo);

    return result;
}
}
#endif
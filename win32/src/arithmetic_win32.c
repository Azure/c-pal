// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdint.h>

#include <intrin.h>

#include "c_pal/arithmetic.h"

#if defined(_WIN64)
/*only works if compiling on 64 bits*/
PAL_UINT128 umul64x64(uint64_t left, uint64_t right)
{
    /*Codes_/*Codes_SRS_ARITHMETIC_02_001: [ umul64x64 shall mutiply left and right and return a PAL_UINT128 value as result. ]*/
    PAL_UINT128 result;
    result.low = _umul128(left, right, &result.high);
    return result;
}
#else

static PAL_UINT128 zer0 = { 0 };

PAL_UINT128 umul64x64(uint64_t left, uint64_t right)
{
    /*Codes_SRS_ARITHMETIC_02_001: [ umul64x64 shall mutiply left and right and return a PAL_UINT128 value as result. ]*/

    /*next comments will have L = left, R = right*/

    /*the following is a nice property of 32 bit multiplication: multiply 2 32 bit numbers and 2 other 32 bit numbers can be added to the result without overflow*/
    /*proof: Let A and B, C, D be a 32 bit unsigned numbers.So A,B,C,D = [0..2^32 - 1]. The maximum result of A*B+C+D =
    (2^32 - 1) * (2^32 - 1) + (2^32 - 1) + (2^32 - 1) = 
    2^64 - 2^32 - 2^32 + 1  + (2^32 - 1) + (2^32 - 1) =
    2^64 - 1.
    */

    /*For the sake of notation, if X is a 64 bit number, then Hi(X) = most significant 32 bits of X, Lo(X) = least significant 32 bits of X. So X can be written X = Hi(X) * 2^32 + Lo(X)*/
    
    /*L * R         =
    (Hi(L) * 2^32 + Lo(L)) * (Hi(R) * 2^32 + Lo(R)) =                                                                                                           (get rid of paranthesis)
    Hi(L) * Hi(R) * 2^64 + Hi(L) * Lo(R) * 2^32 + Lo(L) * Hi(R) * 2^32 + Lo(L) * Lo(R) =                                                                        (regroup)
    Hi(L) * Hi(R) * 2^64 + (Hi(L) * Lo(R) + Lo(L) * Hi(R)) * 2^32 + Lo(L) * Lo(R) =                                                                             (apply Hi/Lo to Lo(L) * Lo(R))
    Hi(L) * Hi(R) * 2^64 + (Hi(L) * Lo(R) + Lo(L) * Hi(R)) * 2^32 + Hi(Lo(L) * Lo(R)) * 2^32 + Lo(Lo(L) * Lo(R)) =                                              (regroup again)
    Hi(L) * Hi(R) * 2^64 + (Hi(L) * Lo(R) + Lo(L) * Hi(R) + Hi(Lo(L) * Lo(R))) * 2^32 + Lo(Lo(L) * Lo(R)) =                                                     (apply Hi/Lo to Lo(L) * Hi(R))
    Hi(L) * Hi(R) * 2^64 + (Hi(L) * Lo(R) + Hi(Lo(L) * Hi(R)) * 2^32 + Lo(Lo(L) * Hi(R)) + Hi(Lo(L) * Lo(R))) * 2^32 + Lo(Lo(L) * Lo(R)) =                      (take out of paranthesis Hi(Lo(L) * Hi(R)) * 2^32)
    Hi(L) * Hi(R) * 2^64 + (Hi(L) * Lo(R) + Lo(Lo(L) * Hi(R)) + Hi(Lo(L) * Lo(R))) * 2^32 + Hi(Lo(L) * Hi(R)) * 2^32 * 2^32 + Lo(Lo(L) * Lo(R)) =               (2^32* 2^32 = 2^4)
    Hi(L) * Hi(R) * 2^64 + (Hi(L) * Lo(R) + Lo(Lo(L) * Hi(R)) + Hi(Lo(L) * Lo(R))) * 2^32 + Hi(Lo(L) * Hi(R)) * 2^64 + Lo(Lo(L) * Lo(R)) =                      (regroup around 2^64)
    (Hi(L) * Hi(R) + Hi(Lo(L) * Hi(R))) * 2^64 + (Hi(L) * Lo(R) + Lo(Lo(L) * Hi(R)) + Hi(Lo(L) * Lo(R))) * 2^32 + Lo(Lo(L) * Lo(R)) =                           (Hi(L) * Lo(R) + Lo(Lo(L) * Hi(R)) + Hi(Lo(L) * Lo(R)) is the multiplication of 2 32 bit numbers(Hi(L), Lo(R)) added with 2 other 32 bit numbers, we know this does not overflow). Call the result of this multiplication X
    (Hi(L) * Hi(R) + Hi(Lo(L) * Hi(R))) * 2^64 + X * 2^32 + Lo(Lo(L) * Lo(R)) =                                                                                 X = Hi(X) * 2^32 + Lo(X)
    (Hi(L) * Hi(R) + Hi(Lo(L) * Hi(R))) * 2^64 + (Hi(X) * 2^32 + Lo(X)) * 2^32 + Lo(Lo(L) * Lo(R)) =                                                            break paranthesis of X...
    (Hi(L) * Hi(R) + Hi(Lo(L) * Hi(R))) * 2^64 + Hi(X) * 2^32 * 2^32 + Lo(X)*2^32 + Lo(Lo(L) * Lo(R)) =                                                         2^32 * 2^32 = 2^64
    (Hi(L) * Hi(R) + Hi(Lo(L) * Hi(R))) * 2^64 + Hi(X) * 2^64 + Lo(X)*2^32 + Lo(Lo(L) * Lo(R)) =                                                                regroup around 2^64
    (Hi(L) * Hi(R) + Hi(Lo(L) * Hi(R)) + Hi(X)) * 2^64 + Lo(X)*2^32 + Lo(Lo(L) * Lo(R)).

    Note: 
    (1) Hi(L) * Hi(R) + Hi(Lo(L) * Hi(R)) + Hi(X) is a multiplication of 2 32bit integers (Hi(L), Hi(R)) to which 2 other 32 bit integers are added (cannot overflow). This is the high 64bits of the result.
    (2) Lo(X) * 2^32 + Lo(Lo(L) * Lo(R)) is the lowest 64 bit of the result.*/

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

#endif


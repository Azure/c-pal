// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;

#include "c_pal/arithmetic.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

typedef struct TEST_TAG
{
    uint64_t left;
    uint64_t right;
    uint64_t high;
    uint64_t low;
}TEST;

/*Tests_SRS_ARITHMETIC_02_001: [ umul64x64 shall call _umul128 and return the result as PAL_UINT128. ]*/
TEST_FUNCTION(umul64x64_combinations) /**/
{
    TEST all[] = {
        {.left = 0, .right = 0, .high = 0, .low = 0},
        {.left = 1, .right = 0, .high = 0, .low = 0},
        {.left = 0, .right = 1, .high = 0, .low = 0},
        {.left = 1, .right = 1, .high = 0, .low = 1},

        {.left = (1ULL<<32) -1, .right = (1ULL << 32) +1, .high = 0, .low = 0xFFFFFFFFFFFFFFFFULL},

        {.left = (1ULL << 32), .right = (1ULL << 32), .high = 1, .low = 0},

        {.left = (1ULL << 63)-1, .right = (1ULL << 63)+1, .high = 0x3FFFFFFFFFFFFFFFULL, .low = 0xFFFFFFFFFFFFFFFFULL},

        {.left = 0xF0E1D2C3B4A59687ULL, .right = (1ULL<<56), .high = 0xF0E1D2C3B4A596ULL, .low = 0x8700000000000000ULL} /*enough of verifying compiler intrinsics*/
    };

    for (uint32_t i = 0; i < sizeof(all) / sizeof(all[0]); i++)
    {
        PAL_UINT128 result = umul64x64(all[i].left, all[i].right);
        ASSERT_ARE_EQUAL(uint64_t, all[i].high, result.high);
        ASSERT_ARE_EQUAL(uint64_t, all[i].low, result.low);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

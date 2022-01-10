// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#include "c_pal/arithmetic.h" // IWYU pragma: keep
#include "c_pal/uuid.h" // IWYU pragma: keep

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "real_threadapi.h"
#include "real_sync.h"
#include "real_arithmetic.h"
#include "real_uuid.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_pal_reals)
{
    // arrange

    // act
    REGISTER_THREADAPI_GLOBAL_MOCK_HOOK();
    REGISTER_SYNC_GLOBAL_MOCK_HOOK();
    REGISTER_ARITHMETIC_GLOBAL_MOCK_HOOK();
    REGISTER_UUID_GLOBAL_MOCK_HOOK();

    // assert
    // no explicit assert, if it builds it works
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

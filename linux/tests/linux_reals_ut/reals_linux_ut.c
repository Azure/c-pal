// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#include "c_pal/arithmetic.h" // IWYU pragma: keep
#include "c_pal/srw_lock.h" // IWYU pragma: keep
#include "c_pal/string_utils.h" // IWYU pragma: keep
#include "c_pal/threadpool.h" // IWYU pragma: keep
#include "c_pal/uuid.h" // IWYU pragma: keep
#include "c_pal/execution_engine.h" // IWYU pragma: keep

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "real_arithmetic.h"
#include "real_srw_lock.h"
#include "real_string_utils.h"
#include "real_sync.h"
#include "real_threadapi.h"
#include "real_threadpool.h"
#include "real_uuid.h"
#include "real_execution_engine.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_pal_reals)
{
    // arrange

    // act
    REGISTER_ARITHMETIC_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();
    REGISTER_SYNC_GLOBAL_MOCK_HOOK();
    REGISTER_THREADAPI_GLOBAL_MOCK_HOOK();
    REGISTER_THREADPOOL_GLOBAL_MOCK_HOOK();
    REGISTER_UUID_GLOBAL_MOCK_HOOK();
    REGISTER_EXECUTION_ENGINE_GLOBAL_MOCK_HOOK();
    // assert
    // no explicit assert, if it builds it works
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

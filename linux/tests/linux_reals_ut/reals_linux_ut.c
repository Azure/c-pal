// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "reals_linux_ut_pch.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_pal_reals) // no-srs
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
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_THANDLE_LOG_CONTEXT_HANDLE_GLOBAL_MOCK_HOOK();
    REGISTER_SOCKET_TRANSPORT_GLOBAL_MOCK_HOOK();
    REGISTER_ASYNC_SOCKET_GLOBAL_MOCK_HOOK();
    // assert
    // no explicit assert, if it builds it works
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

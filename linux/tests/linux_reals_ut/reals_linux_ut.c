// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "azure_c_pal/threadapi.h"
#include "azure_c_pal/call_once.h"
#include "azure_c_pal/lazy_init.h"
#include "azure_c_pal/sync.h"


#include "real_threadapi.h"
#include "real_call_once.h"
#include "real_lazy_init.h"
#include "real_sync.h"

BEGIN_TEST_SUITE(reals_linux_ut)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_pal_reals)
{
    // arrange

    // act
    REGISTER_THREADAPI_GLOBAL_MOCK_HOOK();
    REGISTER_CALL_ONCE_GLOBAL_MOCK_HOOK();
    REGISTER_LAZY_INIT_GLOBAL_MOCK_HOOK();
    REGISTER_SYNC_GLOBAL_MOCK_HOOK();

    // assert
    // no explicit assert, if it builds it works
}

END_TEST_SUITE(reals_linux_ut)

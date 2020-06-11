// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "real_threadapi.h"
#include "threadapi.h"

#if defined _MSC_VER
#include "../../../interfaces/tests/reals/real_srw_lock.h"
#include "../../../interfaces/tests/reals/real_string_utils.h"
#include "../../../interfaces/tests/reals/real_timer.h"
#include "../../../interfaces/tests/reals/real_interlocked_hl.h"

#include "srw_lock.h"
#include "string_utils.h"
#include "timer.h"
#include "interlocked_hl.h"
#endif

BEGIN_TEST_SUITE(azure_c_pal_reals_ut)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_pal_reals)
{
    // arrange

    // act
    REGISTER_THREADAPI_GLOBAL_MOCK_HOOK();

#if defined _MSC_VER
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();
    REGISTER_TIMER_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();
#endif

    // assert
    // no explicit assert, if it builds it works
}

END_TEST_SUITE(azure_c_pal_reals_ut)

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#include "c_pal/threadapi.h"
#include "c_pal/srw_lock.h"
#include "c_pal/string_utils.h"
#include "c_pal/timer.h"
#include "c_pal/interlocked.h"
#include "c_pal/gballoc_ll.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/call_once.h"
#include "c_pal/lazy_init.h"
#include "c_pal/sync.h"
#include "c_pal/arithmetic.h"

#include "macro_utils/macro_utils.h"

#ifdef REGISTER_GLOBAL_MOCK_HOOK
#undef REGISTER_GLOBAL_MOCK_HOOK
#endif 

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "real_threadapi.h"
#include "real_srw_lock.h"
#include "real_string_utils.h"
#include "real_timer.h"
#include "real_interlocked.h"
#include "real_gballoc_ll.h"
#include "real_gballoc_hl.h"
#include "real_call_once.h"
#include "real_lazy_init.h"
#include "real_sync.h"
#include "real_arithmetic.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_pal_reals)
{
    // arrange

    // act
    REGISTER_THREADAPI_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();
    REGISTER_TIMER_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_GBALLOC_LL_GLOBAL_MOCK_HOOK();
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_CALL_ONCE_GLOBAL_MOCK_HOOK();
    REGISTER_LAZY_INIT_GLOBAL_MOCK_HOOK();
    REGISTER_SYNC_GLOBAL_MOCK_HOOK();
    REGISTER_ARITHMETIC_GLOBAL_MOCK_HOOK();

    // assert
    // no explicit assert, if it builds it works
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

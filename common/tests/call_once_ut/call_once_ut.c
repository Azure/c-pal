// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdbool.h>


#include "macro_utils/macro_utils.h" // IWYU pragma: keep

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS_DECL
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#undef ENABLE_MOCKS_DECL

#include "real_interlocked.h"
#include "real_sync.h"
#include "c_pal/call_once.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)


static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

TEST_DEFINE_ENUM_TYPE(CALL_ONCE_RESULT, CALL_ONCE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CALL_ONCE_RESULT, CALL_ONCE_RESULT_VALUES);

static call_once_t g_state = CALL_ONCE_NOT_CALLED;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());

    REGISTER_TYPE(CALL_ONCE_RESULT, CALL_ONCE_RESULT);

    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_SYNC_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*Tests_SRS_CALL_ONCE_02_001: [ call_once_begin shall use interlocked_compare_exchange(state, 1, 0) to determine if user has alredy indicated that the init code was executed with success. ]*/
/*Tests_SRS_CALL_ONCE_02_004: [ If interlocked_compare_exchange returns 0 then call_once_begin shall return CALL_ONCE_PROCEED. ]*/
TEST_FUNCTION(call_once_begin_after_static_init_succeeds)
{
    ///arrange (done globally)
    CALL_ONCE_RESULT canProceed;

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&g_state, 1, 0));

    ///act
    canProceed = call_once_begin(&g_state);

    ///assert
    ASSERT_ARE_EQUAL(CALL_ONCE_RESULT, CALL_ONCE_PROCEED, canProceed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    call_once_end(&g_state, true);
}

/*Tests_SRS_CALL_ONCE_02_001: [ call_once_begin shall use interlocked_compare_exchange(state, 1, 0) to determine if user has alredy indicated that the init code was executed with success. ]*/
/*Tests_SRS_CALL_ONCE_02_004: [ If interlocked_compare_exchange returns 0 then call_once_begin shall return CALL_ONCE_PROCEED. ]*/
TEST_FUNCTION(call_once_begin_after_local_init_succeeds)
{
    ///arrange
    call_once_t state;
    (void)real_interlocked_exchange(&state, CALL_ONCE_NOT_CALLED);
    CALL_ONCE_RESULT canProceed;

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&state, 1, 0));

    ///act
    canProceed = call_once_begin(&state);

    ///assert
    ASSERT_ARE_EQUAL(CALL_ONCE_RESULT, CALL_ONCE_PROCEED, canProceed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    call_once_end(&state, true);
}

/*Tests_SRS_CALL_ONCE_02_002: [ If interlocked_compare_exchange returns 2 then call_once_begin shall return CALL_ONCE_ALREADY_CALLED. ]*/
TEST_FUNCTION(call_once_begin_after_begin_end_fails)
{
    ///arrange
    call_once_t state;
    CALL_ONCE_RESULT canProceed;
    (void)real_interlocked_exchange(&state, CALL_ONCE_NOT_CALLED);

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&state, 1, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(&state, 2));
    STRICT_EXPECTED_CALL(wake_by_address_all(&state));
    canProceed = call_once_begin(&state);
    ASSERT_ARE_EQUAL(CALL_ONCE_RESULT, CALL_ONCE_PROCEED, canProceed);
    call_once_end(&state, true);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&state, 1, 0));

    ///act
    canProceed = call_once_begin(&state);

    ///assert
    ASSERT_ARE_EQUAL(CALL_ONCE_RESULT, CALL_ONCE_ALREADY_CALLED, canProceed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean

}

/*Tests_SRS_CALL_ONCE_02_005: [ If success is true then call_once_end shall call interlocked_exchange setting state to CALL_ONCE_CALLED and shall call wake_by_address_all(state). ]*/
TEST_FUNCTION(call_once_end_with_success_switches_state_to_2)
{
    ///arrange
    call_once_t state;
    CALL_ONCE_RESULT canProceed;
    (void)real_interlocked_exchange(&state, CALL_ONCE_NOT_CALLED);

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&state, 1, 0));
    canProceed = call_once_begin(&state);
    ASSERT_ARE_EQUAL(CALL_ONCE_RESULT, CALL_ONCE_PROCEED, canProceed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(interlocked_exchange(&state, 2));
    STRICT_EXPECTED_CALL(wake_by_address_all(&state));

    ///act
    call_once_end(&state, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_CALL_ONCE_02_006: [ If success is false then call_once_end shall call interlocked_exchange setting state to CALL_ONCE_NOT_CALLED and shall call wake_by_address_all(state). ]*/
TEST_FUNCTION(call_once_end_without_success_switches_state_to_0)
{
    ///arrange
    call_once_t state;
    CALL_ONCE_RESULT canProceed;
    (void)real_interlocked_exchange(&state, CALL_ONCE_NOT_CALLED);

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(&state, 1, 0));
    canProceed = call_once_begin(&state);
    ASSERT_ARE_EQUAL(CALL_ONCE_RESULT, CALL_ONCE_PROCEED, canProceed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(interlocked_exchange(&state, 0));
    STRICT_EXPECTED_CALL(wake_by_address_all(&state));

    ///act
    call_once_end(&state, false);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

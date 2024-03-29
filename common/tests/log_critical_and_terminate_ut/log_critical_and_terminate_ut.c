// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#define ENABLE_MOCKS

#include "c_pal/ps_util.h"

#undef ENABLE_MOCKS

#include "c_pal/log_critical_and_terminate.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        umock_c_reset_all_calls();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
    }

    /* Tests_SRS_LOG_CRITICAL_AND_TERMINATE_01_001: [ LogCriticalAndTerminate shall call ps_util_terminate_process. ]*/
    TEST_FUNCTION(LogCriticalAndTerminate_succeeds)
    {
        ///arrange
        STRICT_EXPECTED_CALL(ps_util_terminate_process());

        ///act
        LogCriticalAndTerminate("Test");

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_LOG_CRITICAL_AND_TERMINATE_01_001: [ LogCriticalAndTerminate shall call ps_util_terminate_process. ]*/
    TEST_FUNCTION(LogCriticalAndTerminate_with_some_args_succeeds)
    {
        ///arrange
        uint32_t x = 42;
        STRICT_EXPECTED_CALL(ps_util_terminate_process());

        ///act
        LogCriticalAndTerminate("Test with x=%" PRIu32 "", x);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

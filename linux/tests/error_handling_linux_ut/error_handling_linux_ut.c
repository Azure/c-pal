// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
 #include <unistd.h>
#include <fcntl.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"

#include "umock_c/umock_c_prod.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "c_pal/windows_defines_errors.h"

#include "c_pal/error_handling.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
    real_gballoc_ll_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_ERROR_HANDLING_LINUX09_002: [ error_handling_linux_set_last_error shall assign a non-NULL value to last_error_code. ]*/
TEST_FUNCTION(set_last_error_code_SUCCESS)
{
    ///arrange
    /*Tests_SRS_ERROR_HANDLING_LINUX09_003: [ error_handling_linux_set_last_error shall call interlocked_exchange_32 with err_code and last_error_code. ]*/
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, ERROR_INVALID_ACCESS));

    ///act
    uint32_t err_code = ERROR_INVALID_ACCESS;
    error_handling_linux_set_last_error(err_code);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
} 

/*Tests_SRS_ERROR_HANDLING_LINUX09_005: [ On success, error_handling_linux_get_last_error shall return the value last set through set_last_error or zero ]*/
TEST_FUNCTION(get_last_error_SUCCEEDS)
{
    ///arrange
    error_handling_linux_set_last_error(ERROR_LOG_DEDICATED);
    umock_c_reset_all_calls();

    /*Tests_SRS_ERROR_HANDLING_LINUX09_006: [ error_handling_linux_get_last_error shall call interlocked_add with last_error_code and zero. ]*/
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    
    ///act
    uint32_t result = 0;
    result = error_handling_linux_get_last_error();

    ///Asserts
    ASSERT_ARE_EQUAL(uint32_t, ERROR_LOG_DEDICATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
} 

TEST_FUNCTION(get_last_error_equals_set_last_error)
{
    ///arrange
    error_handling_linux_set_last_error(ERROR_LOG_MULTIPLEXED);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    uint32_t result = 0;
    result = error_handling_linux_get_last_error();
    ASSERT_ARE_EQUAL(uint32_t, ERROR_LOG_MULTIPLEXED, result);

    error_handling_linux_set_last_error(ERROR_LOG_ARCHIVE_NOT_IN_PROGRESS);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    ///act
    result = error_handling_linux_get_last_error();

    ///Asserts
    ASSERT_ARE_EQUAL(uint32_t, ERROR_LOG_ARCHIVE_NOT_IN_PROGRESS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

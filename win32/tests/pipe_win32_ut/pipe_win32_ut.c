// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>


#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}



#define ENABLE_MOCKS
#include "mock_pipe.h"
#undef ENABLE_MOCKS



#include "real_gballoc_hl.h"


#include "c_pal/pipe.h"

static FILE* test_file_handle = (FILE*)0x1001;


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

//
// pipe_popen
//

/*Tests_SRS_WIN32_PIPE_42_001: [ pipe_popen shall call _popen with command and "rt" as type. ]*/
/*Tests_SRS_WIN32_PIPE_42_002: [ pipe_popen shall return the result of _popen. ]*/
TEST_FUNCTION(pipe_popen_calls_popen_and_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(mock_popen("test command", "rt"))
        .SetReturn(test_file_handle);

    // act
    FILE* result = pipe_popen("test command");

    // assert
    ASSERT_ARE_EQUAL(void_ptr, test_file_handle, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_PIPE_42_002: [ If any error occurs then pipe_popen shall fail and return NULL. ]*/
/*Tests_SRS_WIN32_PIPE_42_001: [ pipe_popen shall call _popen with command and "rt" as type. ]*/
/*Tests_SRS_WIN32_PIPE_42_002: [ pipe_popen shall return the result of _popen. ]*/
TEST_FUNCTION(pipe_popen_calls_popen_and_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(mock_popen("test command", "rt"))
        .SetReturn(NULL);

    // act
    FILE* result = pipe_popen("test command");

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// pipe_pclose
//

/*Tests_SRS_WIN32_PIPE_42_007: [ If exit_code is NULL then pipe_pclose shall fail and return a non-zero value. ]*/
TEST_FUNCTION(pipe_pclose_with_NULL_exit_code_fails)
{
    // arrange

    // act
    int result = pipe_pclose(test_file_handle, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WIN32_PIPE_42_003: [ pipe_pclose shall call _pclose with stream. ]*/
/*Tests_SRS_WIN32_PIPE_42_005: [ Otherwise, pipe_pclose shall return 0. ]*/
/*Tests_SRS_WIN32_PIPE_42_006: [ pipe_pclose shall store the return value of _pclose in exit_code. ]*/
TEST_FUNCTION(pipe_pclose_succeeds_returns_0)
{
    // arrange
    int exit_code;

    STRICT_EXPECTED_CALL(mock_pclose(test_file_handle))
        .SetReturn(0);

    // act
    int result = pipe_pclose(test_file_handle, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, exit_code);
}

/*Tests_SRS_WIN32_PIPE_42_003: [ pipe_pclose shall call _pclose with stream. ]*/
/*Tests_SRS_WIN32_PIPE_42_005: [ Otherwise, pipe_pclose shall return 0. ]*/
/*Tests_SRS_WIN32_PIPE_42_006: [ pipe_pclose shall store the return value of _pclose in exit_code. ]*/
TEST_FUNCTION(pipe_pclose_succeeds_returns_0_with_non_zero_exit_code)
{
    // arrange
    int exit_code;

    STRICT_EXPECTED_CALL(mock_pclose(test_file_handle))
        .SetReturn(42);

    // act
    int result = pipe_pclose(test_file_handle, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 42, exit_code);
}

/*Tests_SRS_WIN32_PIPE_42_003: [ pipe_pclose shall call _pclose with stream. ]*/
/*Tests_SRS_WIN32_PIPE_42_005: [ Otherwise, pipe_pclose shall return 0. ]*/
/*Tests_SRS_WIN32_PIPE_42_006: [ pipe_pclose shall store the return value of _pclose in exit_code. ]*/
TEST_FUNCTION(pipe_pclose_succeeds_returns_0_with_negative_exit_code)
{
    // arrange
    int exit_code;

    STRICT_EXPECTED_CALL(mock_pclose(test_file_handle))
        .SetReturn(-42);

    // act
    int result = pipe_pclose(test_file_handle, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, -42, exit_code);
}

/*Tests_SRS_PIPE_42_004: [ If any error occurs then pipe_pclose shall fail and return a non-zero value. ]*/
/*Tests_SRS_WIN32_PIPE_42_004: [ pipe_pclose shall return a non-zero value if the return value of _pclose is -1. ]*/
TEST_FUNCTION(pipe_pclose_fails_when_close_fails)
{
    // arrange
    int exit_code;

    STRICT_EXPECTED_CALL(mock_pclose(test_file_handle))
        .SetReturn(-1);

    // act
    int result = pipe_pclose(test_file_handle, &exit_code);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

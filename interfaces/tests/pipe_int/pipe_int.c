//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdio.h>               // for feof, fgets, FILE
#include <stddef.h>


#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/pipe.h"

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)


TEST_SUITE_INITIALIZE(a)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(b)
{
    TEST_MUTEX_DESTROY(g_testByTest);
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(c)
{
    ASSERT_ARE_EQUAL(int, 0, TEST_MUTEX_ACQUIRE(g_testByTest), "our mutex is ABANDONED. Failure in test framework");
}

TEST_FUNCTION_CLEANUP(d)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_PIPE_42_001: [ pipe_popen shall execute the command command and pipe its output to the returned FILE*. ]*/
/*Tests_SRS_PIPE_42_003: [ pipe_pclose shall close the pipe stream. ]*/
/*Tests_SRS_PIPE_42_005: [ pipe_pclose shall store the result of the executed command in exit_code. ]*/
/*Tests_SRS_PIPE_42_006: [ pipe_pclose shall succeed and return 0. ]*/
TEST_FUNCTION(pipe_open_close_succeeds)
{
    // arrange
    const char* command = "echo 42";
    int exit_code;

    // act
    // assert
    FILE* file_handle = pipe_popen(command);
    ASSERT_IS_NOT_NULL(file_handle);

    char buffer[128];

    while (fgets(buffer, 128, file_handle))
    {
        ASSERT_ARE_EQUAL(char_ptr, "42\n", buffer);
    }

    ASSERT_IS_TRUE(feof(file_handle));

    ASSERT_ARE_EQUAL(int, 0, pipe_pclose(file_handle, &exit_code));
    ASSERT_ARE_EQUAL(int, 0, exit_code);
}

/*Tests_SRS_PIPE_42_005: [ pipe_pclose shall store the result of the executed command in exit_code. ]*/
TEST_FUNCTION(pipe_open_close_succeeds_returns_non_zero_exit_code)
{
    // arrange
    const char* command = "exit 42";
    int exit_code;

    // act
    // assert
    FILE* file_handle = pipe_popen(command);
    ASSERT_IS_NOT_NULL(file_handle);

    ASSERT_ARE_EQUAL(int, 0, pipe_pclose(file_handle, &exit_code));
    ASSERT_ARE_EQUAL(int, 42, exit_code);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

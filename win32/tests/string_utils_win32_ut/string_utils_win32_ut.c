// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "windows.h"
#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"


#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "mocks.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/string_utils.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static SYSTEMTIME with_ms = { /* 2022-06-09T20:59:25.467Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 9, .wHour = 20, .wMinute = 59, .wSecond = 25, .wMilliseconds = 467 };
static SYSTEMTIME without_ms = { /* 2022-06-09T21:00:06Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 9, .wHour = 21, .wMinute = 00, .wSecond = 06, .wMilliseconds = 0 };

static BOOL hook_FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime)
{
    ASSERT_IS_NOT_NULL(lpFileTime);
    ASSERT_IS_NOT_NULL(lpSystemTime);
    BOOL result;
    if (lpFileTime->dwHighDateTime == 1) /*by convention, if dwHighDateTime is "1" then hook_FileTimeToSystemTime returns with_ms */
    {
        (void)memcpy(lpSystemTime, &with_ms, sizeof(SYSTEMTIME));
        result = TRUE;
    }
    else if (lpFileTime->dwHighDateTime == 2) /*by convention, if dwHighDateTime is "2" then hook_FileTimeToSystemTime returns without_ms */
    {
        (void)memcpy(lpSystemTime, &without_ms, sizeof(SYSTEMTIME));
        result = TRUE;
    }
    else
    {
        ASSERT_FAIL("unknown transformation of FILETIME to SYSTEMTIME");
    }
    return result;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_FileTimeToSystemTime, hook_FileTimeToSystemTime);
    REGISTER_UMOCK_ALIAS_TYPE(LPSYSTEMTIME, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPFILETIME, void*);
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
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_STRING_UTILS_02_001: [ If fileTime is NULL then FILETIME_to_string_UTC shall fail and return NULL. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_fileTime_NULL_fails)
{
    ///arrange
    char* result;

    ///act
    result = FILETIME_to_string_UTC(NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean

}

/*Tests_SRS_STRING_UTILS_02_002: [ FILETIME_to_string_UTC shall call FileTimeToSystemTime to convert fileTime to a SYSTEMTIME structure. ]*/
/*Tests_SRS_STRING_UTILS_02_003: [ If SYSTEMTIME structure's wMilliseconds field is not zero then FILETIME_to_string_UTC shall return a string produced by the format string "(SYSTEMTIME){ \/* %.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 ".%.3" PRIu16 "Z *\/ .wYear = % .4" PRIu16 ", .wMonth = % .2" PRIu16 ", .wDayOfWeek = % .1" PRIu16 ", .wDay = % .2" PRIu16 ", .wHour = % .2" PRIu16 ", .wMinute = % .2" PRIu16 ", .wSecond = % .2" PRIu16 ", .wMilliseconds = % .3" PRIu16 " }". ]*/
/*Tests_SRS_STRING_UTILS_02_005: [ FILETIME_to_string_UTC shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_123_milliseconds_succeeds)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 1, .dwHighDateTime = 1 }; 

    STRICT_EXPECTED_CALL(mocked_FileTimeToSystemTime(&mock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is sprintf_char*/

    ///act
    result = FILETIME_to_string_UTC(&mock);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "(SYSTEMTIME){ /* 2022-06-09T20:59:25.467Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 09, .wHour = 20, .wMinute = 59, .wSecond = 25, .wMilliseconds = 467 }", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_STRING_UTILS_02_002: [ FILETIME_to_string_UTC shall call FileTimeToSystemTime to convert fileTime to a SYSTEMTIME structure. ]*/
/*Tests_SRS_STRING_UTILS_02_004: [ If SYSTEMTIME structure's wMilliseconds field is zero then FILETIME_to_string_UTC shall return a string produced by the format string "(SYSTEMTIME){ \/* %.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 "Z *\/ .wYear = % .4" PRIu16 ", .wMonth = % .2" PRIu16 ", .wDayOfWeek = % .1" PRIu16 ", .wDay = % .2" PRIu16 ", .wHour = % .2" PRIu16 ", .wMinute = % .2" PRIu16 ", .wSecond = % .2" PRIu16 ", .wMilliseconds = % .3" PRIu16 " }". ]*/
/*Tests_SRS_STRING_UTILS_02_005: [ FILETIME_to_string_UTC shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_0_milliseconds_succeeds)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 2, .dwHighDateTime = 2 };

    STRICT_EXPECTED_CALL(mocked_FileTimeToSystemTime(&mock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is sprintf_char*/

    ///act
    result = FILETIME_to_string_UTC(&mock);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "(SYSTEMTIME){ /* 2022-06-09T21:00:06Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 09, .wHour = 21, .wMinute = 00, .wSecond = 06, .wMilliseconds = 000 }", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_STRING_UTILS_02_006: [ If there are any failures then FILETIME_to_string_UTC shall fail and return NULL. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_when_malloc_fails_it_fails_1)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 2, .dwHighDateTime = 2 };

    STRICT_EXPECTED_CALL(mocked_FileTimeToSystemTime(&mock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = FILETIME_to_string_UTC(&mock);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_STRING_UTILS_02_006: [ If there are any failures then FILETIME_to_string_UTC shall fail and return NULL. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_when_malloc_fails_it_fails_2)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 1, .dwHighDateTime = 1 };

    STRICT_EXPECTED_CALL(mocked_FileTimeToSystemTime(&mock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = FILETIME_to_string_UTC(&mock);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_STRING_UTILS_02_006: [ If there are any failures then FILETIME_to_string_UTC shall fail and return NULL. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_when_FileTimeToSystemTime_fails_it_fails)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 2, .dwHighDateTime = 2 };

    STRICT_EXPECTED_CALL(mocked_FileTimeToSystemTime(&mock, IGNORED_ARG))
        .SetReturn(FALSE);

    ///act
    result = FILETIME_to_string_UTC(&mock);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

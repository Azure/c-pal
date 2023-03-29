// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

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
        result = FALSE;
        ASSERT_FAIL("unknown transformation of FILETIME to SYSTEMTIME");
    }
    return result;
}

static int hook_vsnprintf(char* buffer, size_t buffer_size, const char* format, va_list va)
{
    return vsnprintf(buffer, buffer_size, format, va);
}


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_FileTimeToSystemTime, hook_FileTimeToSystemTime);
    
    REGISTER_GLOBAL_MOCK_HOOK(mocked_vsnprintf, hook_vsnprintf);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_vsnprintf, -1);

    REGISTER_UMOCK_ALIAS_TYPE(LPSYSTEMTIME, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPFILETIME, void*);
    REGISTER_UMOCK_ALIAS_TYPE(va_list, void*);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    umock_c_negative_tests_deinit();
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
/*Tests_SRS_STRING_UTILS_02_003: [ If SYSTEMTIME structure's wMilliseconds field is not zero then FILETIME_to_string_UTC shall return a string produced by the format string "%.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 ".%.3" PRIu16 "Z". ]*/
/*Tests_SRS_STRING_UTILS_02_005: [ FILETIME_to_string_UTC shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_123_milliseconds_succeeds)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 1, .dwHighDateTime = 1 }; 

    STRICT_EXPECTED_CALL(mocked_FileTimeToSystemTime(&mock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(NULL, 0, IGNORED_ARG, IGNORED_ARG));                  /*this is sprintf_char*/
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));                                                  /*this is sprintf_char*/
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*this is sprintf_char*/

    ///act
    result = FILETIME_to_string_UTC(&mock);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "2022-06-09T20:59:25.467Z", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

static void FILETIME_to_string_UTC_inert_path(FILETIME* mock)
{
    STRICT_EXPECTED_CALL(mocked_FileTimeToSystemTime(mock, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(NULL, 0, IGNORED_ARG, IGNORED_ARG));                  /*this is sprintf_char*/
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));                                                  /*this is sprintf_char*/
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*this is sprintf_char*/
}

/*Tests_SRS_STRING_UTILS_02_002: [ FILETIME_to_string_UTC shall call FileTimeToSystemTime to convert fileTime to a SYSTEMTIME structure. ]*/
/*Tests_SRS_STRING_UTILS_02_004: [ If SYSTEMTIME structure's wMilliseconds field is zero then FILETIME_to_string_UTC shall return a string produced by the format string "%.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 "Z". ]*/
/*Tests_SRS_STRING_UTILS_02_005: [ FILETIME_to_string_UTC shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_0_milliseconds_succeeds)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 2, .dwHighDateTime = 2 };

    FILETIME_to_string_UTC_inert_path(&mock);

    ///act
    result = FILETIME_to_string_UTC(&mock);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "2022-06-09T21:00:06Z", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_STRING_UTILS_02_006: [ If there are any failures then FILETIME_to_string_UTC shall fail and return NULL. ]*/
TEST_FUNCTION(FILETIME_to_string_UTC_with_when_vsnprintf_fails_it_fails_0_5)
{
    ///arrange
    char* result;
    FILETIME mock = { .dwLowDateTime = 2, .dwHighDateTime = 2 };

    FILETIME_to_string_UTC_inert_path(&mock);

    umock_c_negative_tests_snapshot();

    for (int i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            result = FILETIME_to_string_UTC(&mock);

            ///assert
            ASSERT_IS_NULL(result);
        }
    }

    ///clean
}

static char* vsprintf_char_wrapper(const char* format, ...)
{
    char* result;

    va_list va;
    va_start(va, format);

    result = vsprintf_char(format, va);
    va_end(va);

    return result;
}

/*Tests_SRS_STRING_UTILS_02_007: [ If format is NULL then vsprintf_char shall fail and return NULL. ]*/
TEST_FUNCTION(vsprintf_char_with_format_NULL_fails)
{
    ///arrange
    char* result;

    ///act
    result = vsprintf_char_wrapper(NULL, 1, 2, 3);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}


static void vsprintf_char_wrapper_inert_path(void)
{
    STRICT_EXPECTED_CALL(mocked_vsnprintf(NULL, 0, "%d%s%d", IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, "%d%s%d", IGNORED_ARG));
}

/*Tests_SRS_STRING_UTILS_02_008: [ vsprintf_char shall obtain the length of the string by calling vsnprintf(NULL, 0, format, va);. ]*/
/*Tests_SRS_STRING_UTILS_02_009: [ vsprintf_char shall allocate enough memory for the string and the null terminator. ]*/
/*Tests_SRS_STRING_UTILS_02_010: [ vsprintf_char shall output the string in the previously allocated memory by calling vsnprintf. ]*/
/*Tests_SRS_STRING_UTILS_02_012: [ vsprintf_char shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(vsprintf_char_succeeds)
{
    ///arrange
    char* result;

    vsprintf_char_wrapper_inert_path();

    ///act
    result = vsprintf_char_wrapper("%d%s%d", 1, "2", 3);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_STRING_UTILS_02_011: [ If there are any failures vsprintf_char shall fail and return NULL. ]*/
TEST_FUNCTION(vsprintf_char_unhappy_paths)
{
    ///arrange
    char* result;

    vsprintf_char_wrapper_inert_path();

    umock_c_negative_tests_snapshot();

    for (int i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            result = vsprintf_char_wrapper("%d%s%d", 1, "2", 3);

            ///assert
            ASSERT_IS_NULL(result);
        }
    }

}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

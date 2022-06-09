// Copyright(C) Microsoft Corporation.All rights reserved.


#include <inttypes.h>
#include <stdbool.h>
#include <locale.h>


#include "windows.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/string_utils.h"


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{

}

TEST_FUNCTION_CLEANUP(cleanup)
{

}

TEST_FUNCTION(mbs_to_wcs_converts_a_simple_LOCALE_C_string)
{
    ///arrange
    const char* s = "a";

    ///act
    wchar_t* result = mbs_to_wcs(s);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_IS_TRUE(result[0] == L'a');
    ASSERT_IS_TRUE(result[1] == L'\0');

    ///clean
    free(result);
}

/*the values in this test are taken from https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/mbstowcs-mbstowcs-l?view=vs-2017*/
TEST_FUNCTION(mbs_to_wcs_converts_a_Japanese_string)
{
    ///arrange
    char* localeInfo = setlocale(LC_ALL, "Japanese_Japan.932");
    ASSERT_IS_NOT_NULL(localeInfo);

    const char* multibyteJapanese = "\x82\xa0\x82\xa1";

    ///act
    wchar_t* result = mbs_to_wcs(multibyteJapanese);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_IS_TRUE(result[0] == L'\x3042');
    ASSERT_IS_TRUE(result[1] == L'\x3043');

    ///clean
    free(result);
    localeInfo=setlocale(LC_ALL, "");
    ASSERT_IS_NOT_NULL(localeInfo);
}

TEST_FUNCTION(wcs_to_mbs_converts_a_simple_LOCALE_C_string)
{
    ///arrange
    const wchar_t* s = L"a";

    ///act
    char* result = wcs_to_mbs(s);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_IS_TRUE(result[0] == 'a');
    ASSERT_IS_TRUE(result[1] == '\0');

    ///clean
    free(result);
}

/*the values in this test are taken from https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/mbstowcs-mbstowcs-l?view=vs-2017*/
TEST_FUNCTION(wcs_to_mbs_converts_a_Japanese_string)
{
    ///arrange
    char* localeInfo = setlocale(LC_ALL, "Japanese_Japan.932");
    ASSERT_IS_NOT_NULL(localeInfo);

    const wchar_t* wideJapanese = L"\x3042\x3043";

    ///act
    char* result = wcs_to_mbs(wideJapanese);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_IS_TRUE(result[0] == '\x82');
    ASSERT_IS_TRUE(result[1] == '\xa0');
    ASSERT_IS_TRUE(result[2] == '\x82');
    ASSERT_IS_TRUE(result[3] == '\xa1');

    ///clean
    free(result);
    localeInfo = setlocale(LC_ALL, "");
    ASSERT_IS_NOT_NULL(localeInfo);
}

TEST_FUNCTION(sprintf_char_with_empty_string_succeeds)
{
    ///arrange
    char* result;

    ///act
    result = sprintf_char("%s", "");

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, "", result);

    /// cleanup
    free(result);
}

TEST_FUNCTION(sprintf_char_with_a_non_empty_string_succeeds)
{
    ///arrange
    char* result;

    ///act
    result = sprintf_char("%s", "Kardel Sharpeye");

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, "Kardel Sharpeye", result);

    /// cleanup
    free(result);
}

static char* vsprintf_char_wrapper_function(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_char(format, va);
    va_end(va);
    return result;
}

TEST_FUNCTION(vsprintf_char_with_empty_string_succeeds)
{
    ///arrange
    char* result;

    ///act
    result = vsprintf_char_wrapper_function("%s", "");

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, "", result);

    /// cleanup
    free(result);
}

TEST_FUNCTION(vsprintf_char_with_a_non_empty_string_succeeds)
{
    ///arrange
    char* result;

    ///act
    result = vsprintf_char_wrapper_function("%s", "Kardel Sharpeye");

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, "Kardel Sharpeye", result);

    /// cleanup
    free(result);
}

TEST_FUNCTION(sprintf_wchar_with_empty_string_succeeds)
{
    ///arrange
    wchar_t* result;

    ///act
    result = sprintf_wchar(L"%ls", L"");

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, "", result);

    /// cleanup
    free(result);
}

TEST_FUNCTION(sprintf_wchar_with_a_non_empty_string_succeeds)
{
    ///arrange
    wchar_t* result;

    ///act
    result = sprintf_wchar(L"%s", L"Kardel Sharpeye");

    ///assert
    // ctest could use a wchar_t
    ASSERT_ARE_EQUAL(int, 0, wcscmp(result, L"Kardel Sharpeye"));

    /// cleanup
    free(result);
}

static wchar_t* vsprintf_wchar_wrapper_function(const wchar_t* format, ...)
{
    wchar_t* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_wchar(format, va);
    va_end(va);
    return result;
}

TEST_FUNCTION(vsprintf_wchar_with_empty_string_succeeds)
{
    ///arrange
    wchar_t* result;

    ///act
    result = vsprintf_wchar_wrapper_function(L"%s", L"");

    ///assert
    ASSERT_ARE_EQUAL(int, 0, wcscmp(result, L""));

    /// cleanup
    free(result);
}

TEST_FUNCTION(vsprintf_wchar_with_a_non_empty_string_succeeds)
{
    ///arrange
    wchar_t* result;

    ///act
    result = vsprintf_wchar_wrapper_function(L"%s", L"Kardel Sharpeye");

    ///assert
    ASSERT_ARE_EQUAL(int, 0, wcscmp(result, L"Kardel Sharpeye"));

    /// cleanup
    free(result);
}

TEST_FUNCTION(FILETIME_to_string_UTC_with_now_time_succeeds)
{
    ///arrange
    FILETIME source;
    GetSystemTimeAsFileTime(&source); /*now time*/
    char* result;

    ///act
    result = FILETIME_to_string_UTC(&source);

    ///assert
    ASSERT_IS_NOT_NULL(result);

    ///clean
    free(result);
}

TEST_FUNCTION(FILETIME_to_string_UTC_with_476ms_succeeds)
{
    ///arrange
    SYSTEMTIME s = { /* 2022-06-09T18:59:48.476Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 9, .wHour = 18, .wMinute = 59, .wSecond = 48, .wMilliseconds = 476 };

    FILETIME f;
    ASSERT_IS_TRUE(SystemTimeToFileTime(&s, &f));

    char* result;

    ///act
    result = FILETIME_to_string_UTC(&f);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "(SYSTEMTIME){ /* 2022-06-09T18:59:48.476Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 09, .wHour = 18, .wMinute = 59, .wSecond = 48, .wMilliseconds = 476 }", result);

    ///clean
    free(result);
}

TEST_FUNCTION(FILETIME_to_string_UTC_with_0ms_succeeds)
{
    ///arrange
    SYSTEMTIME s = { /* 2022-06-09T18:59:48Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 9, .wHour = 18, .wMinute = 59, .wSecond = 48, .wMilliseconds = 0 };

    FILETIME f;
    ASSERT_IS_TRUE(SystemTimeToFileTime(&s, &f));

    char* result;

    ///act
    result = FILETIME_to_string_UTC(&f);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "(SYSTEMTIME){ /* 2022-06-09T18:59:48Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 09, .wHour = 18, .wMinute = 59, .wSecond = 48, .wMilliseconds = 000 }", result);

    ///clean
    free(result);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

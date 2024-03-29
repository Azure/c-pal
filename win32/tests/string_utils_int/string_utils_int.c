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
    ASSERT_ARE_EQUAL(char_ptr, "2022-06-09T18:59:48.476Z", result);

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
    ASSERT_ARE_EQUAL(char_ptr, "2022-06-09T18:59:48Z", result);

    ///clean
    free(result);
}

TEST_FUNCTION(vsprintf_char_succeeds)
{
    ///arrange
    char* result;

    ///act
    result = vsprintf_char_wrapper_function("%d %s %ls", 1, "2", L"3");

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "1 2 3", result);

    ///clean
    free(result);
}

TEST_FUNCTION(vsprintf_char_fails_with_invalid_sequence_characters)
{
    ///arrange
    char* result;

    /*original string is */
    wchar_t s[] = { 
        0x54+ (0x00<<8), /*T*/
        0x65+ (0x00<<8), /*e*/
        0x73+ (0x00<<8), /*s*/
        0x74+ (0x00<<8), /*t*/
        0x20+ (0x00<<8), /* */
        0x13+ (0x20<<8), /*0x13, 0x20 is an invalid sequence (EILSEQ) for wchar_t according to C locale, some renderers [Visual Studio debugger] will output a '-' for the character*/
        0x00+ (0x00<<8)  /*null terminator*/
        /*original string is szOID 1.3.6.1.4.1.311.21.8.7587021.751874.11030412.6202749.3702260.207.10315819.14858157 (which is much longer than the excerpt here)*/
    };

    char* t = setlocale(LC_CTYPE, NULL);
    (void)setlocale(LC_CTYPE, "C");

    ///act
    result = vsprintf_char_wrapper_function("%d %s %ls", 1, "2", s);

    ///assert
    ASSERT_IS_NULL(result);

    ///clean
    setlocale(LC_CTYPE, t);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

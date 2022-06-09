// Copyright (C) Microsoft Corporation. All rights reserved.

/*poor man's string routines*/
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/string_utils.h"

IMPLEMENT_MOCKABLE_FUNCTION(, char*, vsprintf_char, const char*, format, va_list, va)
{
    char* result;
    va_list va_clone;
    va_copy(va_clone, va);
    int neededSize = vsnprintf(NULL, 0, format, va);
    if (neededSize < 0)
    {
        LogError("failure in vsnprintf");
        result = NULL;
    }
    else
    {
        result = (char*)malloc((neededSize + 1U) * sizeof(char));
        if (result == NULL)
        {
            LogError("failure in malloc((neededSize=%d + 1U) * sizeof(char)=%zu)", neededSize, sizeof(char));
            /*return as is*/
        }
        else
        {
            if (vsnprintf(result, neededSize + 1, format, va_clone) != neededSize)
            {
                LogError("inconsistent vsnprintf behavior");
                free(result);
                result = NULL;
            }
        }
    }
    va_end(va_clone);
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, wchar_t*, vsprintf_wchar, const wchar_t*, format, va_list, va)
{
    wchar_t* result;
    va_list va_clone;
    va_copy(va_clone, va);
    int neededSize = vswprintf(NULL, 0, format, va);
    if (neededSize < 0)
    {
        LogError("failure in swprintf");
        result = NULL;
    }
    else
    {
        result = (wchar_t*)malloc_flex(sizeof(wchar_t), neededSize, sizeof(wchar_t));
        if (result == NULL)
        {
            LogError("failure in malloc_flex(sizeof(wchar_t)=%zu, neededSize=%d, sizeof(wchar_t)=%zu", sizeof(wchar_t), neededSize, sizeof(wchar_t));
            /*return as is*/
        }
        else
        {
            if (vswprintf(result, neededSize + 1, format, va_clone) != neededSize)
            {
                LogError("inconsistent vswprintf behavior");
                free(result);
                result = NULL;
            }
        }
    }
    va_end(va_clone);
    return result;
}

/*returns a char* that is as if printed by printf*/
/*needs to be free'd after usage*/
char* sprintf_char_function(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_char(format, va);
    va_end(va);
    return result;
}

wchar_t* sprintf_wchar_function(const wchar_t* format, ...)
{
    wchar_t* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_wchar(format, va);
    va_end(va);
    return result;
}

/*takes a FILETIME, returns a nice string representation of it*/
char* FILETIME_toAsciiArray(const FILETIME* fileTime)
{
    char* result;
    if (fileTime == NULL)
    {
        LogError("invalid argument const FILETIME* fileTime=%p", fileTime);
        result = NULL;
    }
    else
    {
        FILETIME localFileTime;
        if (FileTimeToLocalFileTime(fileTime, &localFileTime) == 0)
        {
            LogLastError("failure in FileTimeToLocalFileTime");
            result = NULL;
        }
        else
        {
            SYSTEMTIME systemTime;
            if (FileTimeToSystemTime(&localFileTime, &systemTime) == 0)
            {
                LogLastError("failure in FileTimeToLocalFileTime");
                result = NULL;
            }
            else
            {
                char localDate[255];
                if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systemTime, NULL, localDate, sizeof(localDate)/sizeof(localDate[0])) == 0)
                {
                    LogLastError("failure in GetDateFormat");
                    result = NULL;
                }
                else
                {
                    char localTime[255];
                    if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, NULL, localTime, sizeof(localTime)/sizeof(localTime[0])) == 0)
                    {
                        LogLastError("failure in GetTimeFormat");
                        result = NULL;
                    }
                    else
                    {
                        result = sprintf_char("%s %s", localDate, localTime);
                        /*return as is*/
                    }
                }
            }
        }
    }
    return result;
}

char* FILETIME_to_string_UTC(const FILETIME* fileTime)
{
    char* result;
    /*Codes_SRS_STRING_UTILS_02_001: [ If fileTime is NULL then FILETIME_to_string_UTC shall fail and return NULL. ]*/
    if (fileTime == NULL)
    {
        LogError("invalid arguments const FILETIME* fileTime=%p", fileTime);
        result = NULL;
    }
    else
    {
        /*https://docs.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-filetimetosystemtime says "Converts a file time to system time format. System time is based on Coordinated Universal Time (UTC)." */

        SYSTEMTIME temp;

        /*Codes_SRS_STRING_UTILS_02_002: [ FILETIME_to_string_UTC shall call FileTimeToSystemTime to convert fileTime to a SYSTEMTIME structure. ]*/
        if (!FileTimeToSystemTime(fileTime, &temp))
        {
            /*Codes_SRS_STRING_UTILS_02_006: [ If there are any failures then FILETIME_to_string_UTC shall fail and return NULL. ]*/
            LogLastError("failure in FileTimeToSystemTime(fileTime=%p, &temp=%p)", fileTime, &temp);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_STRING_UTILS_02_003: [ If SYSTEMTIME structure's wMilliseconds field is not zero then FILETIME_to_string_UTC shall return a string produced by the format string "%.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 ".%.3" PRIu16 "Z". ]*/
            if (temp.wMilliseconds != 0)
            {
                result = sprintf_char("%.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 ".%.3" PRIu16 "Z", temp.wYear, temp.wMonth, temp.wDay, temp.wHour, temp.wMinute, temp.wSecond, temp.wMilliseconds);
                if (result == NULL)
                {
                    LogError("failure in sprintf_char(\"%%.4\" PRIu16 \"-%%.2\" PRIu16 \"-%%.2\" PRIu16 \"T%%.2\" PRIu16 \":%%.2\" PRIu16 \":%%.2\" PRIu16 \".%%.3\" PRIu16 \"Z\", temp.wYear=%" PRIu16 ", temp.wMonth=%" PRIu16 ", temp.wDay=%" PRIu16 ", temp.wHour=%" PRIu16 ", temp.wMinute=%" PRIu16 ", temp.wSecond=%" PRIu16 ", temp.wMilliseconds=%" PRIu16 ")",
                        temp.wYear, temp.wMonth, temp.wDay, temp.wHour, temp.wMinute, temp.wSecond, temp.wMilliseconds);
                    /*Codes_SRS_STRING_UTILS_02_006: [ If there are any failures then FILETIME_to_string_UTC shall fail and return NULL. ]*/
                    /*return as is*/
                }
                else
                {
                    /*Codes_SRS_STRING_UTILS_02_005: [ FILETIME_to_string_UTC shall succeed and return a non-NULL value. ]*/
                    /*return as is*/
                }
            }
            else
            /*Codes_SRS_STRING_UTILS_02_004: [ If SYSTEMTIME structure's wMilliseconds field is zero then FILETIME_to_string_UTC shall return a string produced by the format string "%.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 "Z". ]*/
            {
                result = sprintf_char("%.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 "Z", temp.wYear, temp.wMonth, temp.wDay, temp.wHour, temp.wMinute, temp.wSecond);
                if (result == NULL)
                {
                    LogError("failure in sprintf_char(\"%%.4\" PRIu16 \"-%%.2\" PRIu16 \"-%%.2\" PRIu16 \"T%%.2\" PRIu16 \":%%.2\" PRIu16 \":%%.2\" PRIu16 \"Z\", temp.wYear=%" PRIu16 ", temp.wMonth=%" PRIu16 ", temp.wDay=%" PRIu16 ", temp.wHour=%" PRIu16 ", temp.wMinute=%" PRIu16 ", temp.wSecond=%" PRIu16 ");",
                        temp.wYear, temp.wMonth, temp.wDay, temp.wHour, temp.wMinute, temp.wSecond);
                    /*Codes_SRS_STRING_UTILS_02_006: [ If there are any failures then FILETIME_to_string_UTC shall fail and return NULL. ]*/
                    /*return as is*/
                }
                else
                {
                    /*Codes_SRS_STRING_UTILS_02_005: [ FILETIME_to_string_UTC shall succeed and return a non-NULL value. ]*/
                    /*return as is*/
                }
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, wchar_t*, mbs_to_wcs, const char*, source)
{
    wchar_t *result;
    if (source == NULL)
    {
        LogError("invalid argument const char* source=%s", MU_P_OR_NULL(source));
        result = NULL;
    }
    else
    {
        const char* sameAsSource = source;
        /*assuming source is a multibyte character string*/
        mbstate_t state = { 0 };/*initial state!*/
        size_t nwc = mbsrtowcs(NULL, &sameAsSource, 0, &state); /*note 350 from C standard seems to indicate that NULL is a valid pointer to pass here*/
        if (nwc == (size_t)(-1))
        {
            LogError("failure to get the length of the string %s in multibyte characters", strerror(errno));
            result = NULL;
        }
        else
        {
            result = (wchar_t*)malloc_flex(sizeof(wchar_t), nwc, sizeof(wchar_t));
            if (result == NULL)
            {
                LogError("failure in malloc_flex(sizeof(wchar_t)=%zu, nwc=%zu, sizeof(wchar_t)=%zu", sizeof(wchar_t), nwc, sizeof(wchar_t));
                /*return as is*/
            }
            else
            {
                size_t nwc2 = mbsrtowcs(result, &sameAsSource, nwc+1, &state);
                if (nwc2 != nwc)
                {
                    LogError("unexpected inconsistency in mbsrtowcs");
                }
                else
                {
                    /*all is fine*/
                    goto allOk;
                }
                free(result);
                result = NULL;
            }
allOk:;
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, char*, wcs_to_mbs, const wchar_t*, source)
{
    char *result;
    if (source == NULL)
    {
        LogError("invalid argument const wchar_t* source=%ls", MU_WP_OR_NULL(source));
        result = NULL;
    }
    else
    {
        const wchar_t* sameAsSource = source;
        mbstate_t state = { 0 };/*initial state!*/
        size_t nc = wcsrtombs(NULL, &sameAsSource, 0, &state);
        if (nc == (size_t)(-1))
        {
            LogError("failure to get the length of the string %s in characters", strerror(errno));
            result = NULL;
        }
        else
        {
            result = (char*)malloc(sizeof(char)*(nc + 1)); /*this addition is always possible without overflow (line "if (nc == (size_t)(-1))" says nc is not SIZE_MAX)*/
            if (result == NULL)
            {
                LogError("failure in malloc(sizeof(char)*(nc=%zu + 1))", nc);
                /*return as is*/
            }
            else
            {
                size_t nc2 = wcsrtombs(result, &sameAsSource, nc + 1, &state);
                if (nc2 != nc)
                {
                    LogError("unexpected inconsistency in wcsrtombs");
                }
                else
                {
                    /*all is fine*/
                    goto allOk;
                }
                free(result);
                result = NULL;
            }
        allOk:;
        }
    }
    return result;
}

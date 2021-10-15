// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdarg.h>                // for va_list, va_end, va_copy, va_start
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "umock_c/umock_c_prod.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep

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
            LogError("failure in malloc");
            /*return as is*/
        }
        else
        {
            if (vsnprintf(result, neededSize + 1U, format, va_clone) != neededSize)
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
        result = (wchar_t*)malloc_2(neededSize + 1U,sizeof(wchar_t));
        if (result == NULL)
        {
            LogError("failure in malloc_2((neededSize=%d + 1U),sizeof(wchar_t)=%zu)", neededSize , sizeof(wchar_t));
            /*return as is*/
        }
        else
        {
            if (vswprintf(result, neededSize + 1U, format, va_clone) != neededSize)
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
            result = (wchar_t*)malloc_flex(sizeof(wchar_t), sizeof(wchar_t), nwc));
            if (result == NULL)
            {
                LogError("failure in malloc_flex(sizeof(wchar_t)=%zu, sizeof(wchar_t)=%zu, nwc=%zu)", sizeof(wchar_t), sizeof(wchar_t), nwc);
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
            result = (char*)malloc_flex(sizeof(char), nc, sizeof(char)));
            if (result == NULL)
            {
                LogError("failure in malloc_flex(sizeof(char)=%zu, nc=%zu, sizeof(char)=%zu)", sizeof(char), nc, sizeof(char));
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

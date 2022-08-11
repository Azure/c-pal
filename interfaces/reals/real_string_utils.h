// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_STRING_UTILS_H
#define REAL_STRING_UTILS_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#ifdef WIN32
#include "windows.h"
#endif

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#ifdef WIN32
#define REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        vsprintf_char, \
        vsprintf_wchar, \
        FILETIME_toAsciiArray, \
        FILETIME_to_string_UTC, \
        mbs_to_wcs, \
        wcs_to_mbs \
)
#else
    #define REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        vsprintf_char, \
        vsprintf_wchar, \
        mbs_to_wcs, \
        wcs_to_mbs \
)
#endif

#ifdef __cplusplus
extern "C" {
#endif

char* real_sprintf_char_function(const char* format, ...);

wchar_t* real_sprintf_wchar_function(const wchar_t* format, ...);

char* real_vsprintf_char(const char* format, va_list va);

wchar_t* real_vsprintf_wchar(const wchar_t* format, va_list va);

#ifdef WIN32
char* real_FILETIME_toAsciiArray(const FILETIME* fileTime);

char* real_FILETIME_to_string_UTC(const FILETIME* fileTime);
#endif

wchar_t* real_mbs_to_wcs(const char* source);

char* real_wcs_to_mbs(const wchar_t* source);

#ifdef __cplusplus
}
#endif

#endif // REAL_STRING_UTILS_H

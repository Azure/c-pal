// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#ifdef __cplusplus
#include <cstdarg>
#include <cinttypes>
#include <cstdio>
#include <cwchar>
#else
#include <stddef.h>
#include <stdarg.h>
#include <wchar.h>
#include <inttypes.h>
#include <stdio.h>
#endif

/*note: linux implementation is missing all the wide char functions that can be found on windows implementation because the wide char functions have a completely different functionality on linux
regarding their return value. Basically, they (swprintf and vswprintf) will return -1 when called with NULL, 0 first two arguments*/

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

/*produces a string as if printed by printf*/
char* sprintf_char_function(const char* format, ...);

/*produces a string as if printed by printf (will also verify arguments)*/
#define sprintf_char(format, ...) (0?printf((format), __VA_ARGS__):0, sprintf_char_function((format), __VA_ARGS__))

MOCKABLE_INTERFACE(string_utils_printf,
    /*produces a string as if printed by vprintf*/
    FUNCTION(, char*, vsprintf_char, const char*, format, va_list, va),
    /*produces a string as if printed by vwprintf*/
    FUNCTION(, wchar_t*, vsprintf_wchar, const wchar_t*, format, va_list, va)
)

MOCKABLE_INTERFACE(string_utils_convert,
    /*produces the wchar_t* string representation of source (which is assumed to be multibyte). Returns NULL on any failure.*/
    FUNCTION(, wchar_t*, mbs_to_wcs, const char*, source),

    /*produces the multibyte char* string representation of source. Returns NULL on any failure.*/
    FUNCTION(, char*, wcs_to_mbs, const wchar_t*, source)
)

#ifdef __cplusplus
}
#endif

#endif

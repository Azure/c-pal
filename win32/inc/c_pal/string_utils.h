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


#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

/*produces a string as if printed by printf*/
char* sprintf_char_function(const char* format, ...);

/*produces a string as if printed by printf (will also verify arguments)*/
#define sprintf_char(format, ...) (0?printf((format), ## __VA_ARGS__):0, sprintf_char_function((format), ##__VA_ARGS__))

/*produces a string as if printed by wprintf*/
wchar_t* sprintf_wchar_function(const wchar_t* format, ...);
#define sprintf_wchar(format, ...) (0?wprintf((format), ##__VA_ARGS__):0, sprintf_wchar_function((format), ##__VA_ARGS__))

MOCKABLE_INTERFACE(string_utils_printf,
    /*produces a string as if printed by vprintf*/
    FUNCTION(, char*, vsprintf_char, const char*, format, va_list, va),
    /*produces a string as if printed by vwprintf*/
    FUNCTION(, wchar_t*, vsprintf_wchar, const wchar_t*, format, va_list, va)
)

/*takes a FILETIME, returns a nice string representation of it*/
MOCKABLE_FUNCTION(, char*, FILETIME_toAsciiArray, const FILETIME*, fileTime);

/*takes a FILETIME, returns a string in UTC time zone*/
MOCKABLE_FUNCTION(, char*, FILETIME_to_string_UTC, const FILETIME*, fileTime);

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

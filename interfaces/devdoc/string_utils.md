`string_utils` requirements
============

## Overview

`string_utils` is a collection of string utils. The set of requirements in this document is far from complete.

## Exposed API

```c
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
```

### FILETIME_to_string_UTC
```c
MOCKABLE_FUNCTION(, char*, FILETIME_to_string_UTC, const FILETIME*, fileTime);
```

`FILETIME_to_string_UTC` returns a human readable representation of `fileTime`. Here's an example: `(SYSTEMTIME){ /* 2022-06-09T18:59:48.476Z */ .wYear = 2022, .wMonth = 06, .wDayOfWeek = 4, .wDay = 09, .wHour = 18, .wMinute = 59, .wSecond = 48, .wMilliseconds = 476 }`. The returned string needs to be `free`d by the caller.

**SRS_STRING_UTILS_02_001: [** If `fileTime` is `NULL` then `FILETIME_to_string_UTC` shall fail and return `NULL`. **]**

**SRS_STRING_UTILS_02_002: [** `FILETIME_to_string_UTC` shall call `FileTimeToSystemTime` to convert `fileTime` to a `SYSTEMTIME` structure. **]**

**SRS_STRING_UTILS_02_003: [** If `SYSTEMTIME` structure's `wMilliseconds` field is not zero then `FILETIME_to_string_UTC` shall return a string produced by the format string `"(SYSTEMTIME){ /* %.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 ".%.3" PRIu16 "Z */ .wYear = %.4" PRIu16 ", .wMonth = %.2" PRIu16 ", .wDayOfWeek = %.1" PRIu16 ", .wDay = %.2" PRIu16 ", .wHour = %.2" PRIu16 ", .wMinute = %.2" PRIu16 ", .wSecond = %.2" PRIu16 ", .wMilliseconds = %.3" PRIu16 " }"`. **]**

**SRS_STRING_UTILS_02_004: [** If `SYSTEMTIME` structure's `wMilliseconds` field is zero then `FILETIME_to_string_UTC` shall return a string produced by the format string `"(SYSTEMTIME){ /* %.4" PRIu16 "-%.2" PRIu16 "-%.2" PRIu16 "T%.2" PRIu16 ":%.2" PRIu16 ":%.2" PRIu16 "Z */ .wYear = %.4" PRIu16 ", .wMonth = %.2" PRIu16 ", .wDayOfWeek = %.1" PRIu16 ", .wDay = %.2" PRIu16 ", .wHour = %.2" PRIu16 ", .wMinute = %.2" PRIu16 ", .wSecond = %.2" PRIu16 ", .wMilliseconds = %.3" PRIu16 " }"`. **]**

**SRS_STRING_UTILS_02_005: [** `FILETIME_to_string_UTC` shall succeed and return a non-`NULL` value. **]**

**SRS_STRING_UTILS_02_006: [** If there are any failures then `FILETIME_to_string_UTC` shall fail and return `NULL`. **]**


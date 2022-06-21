// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdarg.h>
#include <stddef.h>

#include "windows.h"

#include "umock_c/umock_c_prod.h"

    MOCKABLE_FUNCTION(, BOOL, mocked_FileTimeToSystemTime, const FILETIME*, lpFileTime, LPSYSTEMTIME, lpSystemTime);
    MOCKABLE_FUNCTION(, int, mocked_vsnprintf, char*, buffer, size_t, buffer_size, const char*, format, va_list, va);



// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "mocks.h"

#define FileTimeToSystemTime mocked_FileTimeToSystemTime

#include <stdio.h>

#define vsnprintf mocked_vsnprintf

BOOL mocked_FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime);
int mocked_vsnprintf(char* buffer, size_t buffer_size, const char* format, va_list va);

#include "../../src/string_utils.c"

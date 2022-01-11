// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

#define GetSystemInfo mocked_GetSystemInfo

void mocked_GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);

#include "../../src/sysinfo_win32.c"

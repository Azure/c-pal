// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

#define GetActiveProcessorCount mocked_GetActiveProcessorCount

DWORD mocked_GetActiveProcessorCount(WORD GroupNumber);

#include "../../src/sysinfo_win32.c"

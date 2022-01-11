// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"


#define QueryPerformanceFrequency mocked_QueryPerformanceFrequency
#define QueryPerformanceCounter mocked_QueryPerformanceCounter


BOOLEAN mocked_QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
BOOLEAN mocked_QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

#include "../../src/timer_win32.c"

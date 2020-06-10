// Copyright (c) Microsoft. All rights reserved.

#define CreateThreadpool mocked_CreateThreadpool
#define CloseThreadpool mocked_CloseThreadpool
#define SetThreadpoolThreadMinimum mocked_SetThreadpoolThreadMinimum
#define SetThreadpoolThreadMaximum mocked_SetThreadpoolThreadMaximum

#include "windows.h"

#pragma warning(disable: 4273)

extern PTP_POOL WINAPI mocked_CreateThreadpool(LPVOID reserved);

#include "../../src/execution_engine_win32.c"

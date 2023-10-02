// Copyright (c) Microsoft. All rights reserved.

#define GetModuleFileNameA mocked_GetModuleFileNameA
#define GetCurrentProcessId mocked_GetCurrentProcessId
#define PdhOpenQueryA mocked_PdhOpenQueryA
#define PdhAddCounterA mocked_PdhAddCounterA
#define PdhCollectQueryData mocked_PdhCollectQueryData
#define PdhCloseQuery mocked_PdhCloseQuery
#define PdhGetFormattedCounterValue mocked_PdhGetFormattedCounterValue

#include "windows.h"
#include "pdh.h"

#include "../../src/single_performance_counter_win32.c"

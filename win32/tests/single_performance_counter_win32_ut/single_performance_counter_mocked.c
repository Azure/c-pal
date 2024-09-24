// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"
#include "pdh.h"

#define GetModuleFileNameA mocked_GetModuleFileNameA
#define GetCurrentProcessId mocked_GetCurrentProcessId
#define PdhOpenQueryA mocked_PdhOpenQueryA
#define PdhAddCounterA mocked_PdhAddCounterA
#define PdhCollectQueryData mocked_PdhCollectQueryData
#define PdhCloseQuery mocked_PdhCloseQuery
#define PdhGetFormattedCounterValue mocked_PdhGetFormattedCounterValue

extern DWORD GetModuleFileNameA(
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize
);

extern DWORD GetCurrentProcessId();

extern PDH_FUNCTION PdhOpenQueryA(
    LPCSTR     szDataSource,
    DWORD_PTR  dwUserData,
    PDH_HQUERY* phQuery
);

extern PDH_FUNCTION PdhAddCounterA(
    PDH_HQUERY   hQuery,
    LPCSTR       szFullCounterPath,
    DWORD_PTR    dwUserData,
    PDH_HCOUNTER* phCounter
);

extern PDH_FUNCTION PdhCollectQueryData(
    PDH_HQUERY hQuery
);

extern PDH_FUNCTION PdhCloseQuery(
    PDH_HQUERY hQuery
);

extern PDH_FUNCTION PdhGetFormattedCounterValue(
    PDH_HCOUNTER          hCounter,
    DWORD                 dwFormat,
    LPDWORD               lpdwType,
    PPDH_FMT_COUNTERVALUE pValue
);

#include "../../src/single_performance_counter_win32.c"

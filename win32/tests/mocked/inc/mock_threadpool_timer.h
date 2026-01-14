// Copyright (c) Microsoft. All rights reserved.

#ifndef MOCK_THREADPOOL_TIMER_H
#define MOCK_THREADPOOL_TIMER_H

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

// Only define types if windows.h is not included
#ifndef _WINDOWS_
typedef void VOID;
#define CALLBACK
typedef void* PTP_TIMER;
typedef void* PVOID;
typedef void* PTP_CALLBACK_ENVIRON;
typedef void* PTP_CALLBACK_INSTANCE;
typedef void* PFILETIME;
typedef VOID (CALLBACK *PTP_TIMER_CALLBACK)(PTP_CALLBACK_INSTANCE, PVOID, PTP_TIMER);
typedef unsigned long DWORD;
typedef long BOOL;
#endif

#define CreateThreadpoolTimer mocked_CreateThreadpoolTimer
#define SetThreadpoolTimer mocked_SetThreadpoolTimer
#define WaitForThreadpoolTimerCallbacks mocked_WaitForThreadpoolTimerCallbacks
#define CloseThreadpoolTimer mocked_CloseThreadpoolTimer

MOCKABLE_FUNCTION(, PTP_TIMER, mocked_CreateThreadpoolTimer, PTP_TIMER_CALLBACK, pfnti, PVOID, pv, PTP_CALLBACK_ENVIRON, pcbe);
MOCKABLE_FUNCTION(, void, mocked_SetThreadpoolTimer, PTP_TIMER, pti, PFILETIME, pftDueTime, DWORD, msPeriod, DWORD, msWindowLength);
MOCKABLE_FUNCTION(, void, mocked_WaitForThreadpoolTimerCallbacks, PTP_TIMER, pti, BOOL, fCancelPendingCallbacks);
MOCKABLE_FUNCTION(, void, mocked_CloseThreadpoolTimer, PTP_TIMER, pti);

#ifdef __cplusplus
}
#endif

#endif // MOCK_THREADPOOL_TIMER_H

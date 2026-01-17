// Copyright (c) Microsoft. All rights reserved.

#ifndef MOCK_THREADPOOL_TIMER_H
#define MOCK_THREADPOOL_TIMER_H

// Requires windows.h to be included first for type definitions

#include "umock_c/umock_c_prod.h"

#define CreateThreadpoolTimer mocked_CreateThreadpoolTimer
#define SetThreadpoolTimer mocked_SetThreadpoolTimer
#define WaitForThreadpoolTimerCallbacks mocked_WaitForThreadpoolTimerCallbacks
#define CloseThreadpoolTimer mocked_CloseThreadpoolTimer

MOCKABLE_FUNCTION(, PTP_TIMER, mocked_CreateThreadpoolTimer, PTP_TIMER_CALLBACK, pfnti, PVOID, pv, PTP_CALLBACK_ENVIRON, pcbe);
MOCKABLE_FUNCTION(, void, mocked_SetThreadpoolTimer, PTP_TIMER, pti, PFILETIME, pftDueTime, DWORD, msPeriod, DWORD, msWindowLength);
MOCKABLE_FUNCTION(, void, mocked_WaitForThreadpoolTimerCallbacks, PTP_TIMER, pti, BOOL, fCancelPendingCallbacks);
MOCKABLE_FUNCTION(, void, mocked_CloseThreadpoolTimer, PTP_TIMER, pti);

#endif // MOCK_THREADPOOL_TIMER_H

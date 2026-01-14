// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

#pragma warning(disable: 4273)

#define CreateThreadpoolWork mocked_CreateThreadpoolWork
#define InitializeThreadpoolEnvironment mocked_InitializeThreadpoolEnvironment
#define SetThreadpoolCallbackPool mocked_SetThreadpoolCallbackPool
#define CreateThreadpoolCleanupGroup mocked_CreateThreadpoolCleanupGroup
#define SetThreadpoolCallbackCleanupGroup mocked_SetThreadpoolCallbackCleanupGroup
#define CloseThreadpoolCleanupGroupMembers mocked_CloseThreadpoolCleanupGroupMembers
#define CloseThreadpoolWork mocked_CloseThreadpoolWork
#define CloseThreadpoolCleanupGroup mocked_CloseThreadpoolCleanupGroup
#define DestroyThreadpoolEnvironment mocked_DestroyThreadpoolEnvironment
#define SubmitThreadpoolWork mocked_SubmitThreadpoolWork
#define WaitForThreadpoolWorkCallbacks mocked_WaitForThreadpoolWorkCallbacks

PTP_WORK mocked_CreateThreadpoolWork(PTP_WORK_CALLBACK pfnwk, PVOID pv, PTP_CALLBACK_ENVIRON pcbe);
void mocked_InitializeThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);
void mocked_SetThreadpoolCallbackPool(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp);
PTP_CLEANUP_GROUP mocked_CreateThreadpoolCleanupGroup(void);
VOID mocked_SetThreadpoolCallbackCleanupGroup(PTP_CALLBACK_ENVIRON pcbe, PTP_CLEANUP_GROUP ptpcg, PTP_CLEANUP_GROUP_CANCEL_CALLBACK pfng);
void mocked_CloseThreadpoolCleanupGroupMembers(PTP_CLEANUP_GROUP ptpcg, BOOL fCancelPendingCallbacks, PVOID pvCleanupContext);
void mocked_CloseThreadpoolWork(PTP_WORK pwk);
void mocked_CloseThreadpoolCleanupGroup(PTP_CLEANUP_GROUP ptpcg);
void mocked_DestroyThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);
void mocked_SubmitThreadpoolWork(PTP_WORK pwk);
void mocked_WaitForThreadpoolWorkCallbacks(PTP_WORK pwk, BOOL fCancelPendingCallbacks);

// Timer mock prototypes from shared header
#include "../mocked/inc/mock_threadpool_timer.h"

#include "../../src/threadpool_win32.c"

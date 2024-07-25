// Copyright (c) Microsoft. All rights reserved.

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "c_pal/socket_transport.h"

#pragma warning(disable: 4273)

#define CreateThreadpoolIo mocked_CreateThreadpoolIo
#define InitializeThreadpoolEnvironment mocked_InitializeThreadpoolEnvironment
#define SetThreadpoolCallbackPool mocked_SetThreadpoolCallbackPool
#define CreateThreadpoolCleanupGroup mocked_CreateThreadpoolCleanupGroup
#define CloseThreadpoolCleanupGroupMembers mocked_CloseThreadpoolCleanupGroupMembers
#define CloseThreadpoolIo mocked_CloseThreadpoolIo
#define CloseThreadpoolCleanupGroup mocked_CloseThreadpoolCleanupGroup
#define DestroyThreadpoolEnvironment mocked_DestroyThreadpoolEnvironment
#define CreateEventA mocked_CreateEventA
#define StartThreadpoolIo mocked_StartThreadpoolIo
#define WSAGetLastError mocked_WSAGetLastError
#define CloseHandle mocked_CloseHandle
#define WaitForThreadpoolIoCallbacks mocked_WaitForThreadpoolIoCallbacks
#define CancelThreadpoolIo mocked_CancelThreadpoolIo

PTP_IO WINAPI mocked_CreateThreadpoolIo(HANDLE fl, PTP_WIN32_IO_CALLBACK pfnio, PVOID pv, PTP_CALLBACK_ENVIRON pcbe);
void mocked_InitializeThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);
void mocked_SetThreadpoolCallbackPool(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp);
PTP_CLEANUP_GROUP mocked_CreateThreadpoolCleanupGroup(void);
void mocked_CloseThreadpoolCleanupGroupMembers(PTP_CLEANUP_GROUP ptpcg, BOOL fCancelPendingCallbacks, PVOID pvCleanupContext);
void mocked_CloseThreadpoolIo(PTP_IO pio);
void mocked_CloseThreadpoolCleanupGroup(PTP_CLEANUP_GROUP ptpcg);
void mocked_DestroyThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);
HANDLE mocked_CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
void mocked_StartThreadpoolIo(PTP_IO pio);
int mocked_WSAGetLastError(void);
BOOL mocked_CloseHandle(HANDLE hObject);
void mocked_WaitForThreadpoolIoCallbacks(PTP_IO pio, BOOL fCancelPendingCallbacks);
void mocked_CancelThreadpoolIo(PTP_IO pio);

#include "../../src/async_socket_win32.c"

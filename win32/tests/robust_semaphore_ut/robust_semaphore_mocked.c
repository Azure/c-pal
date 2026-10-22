// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "sddl.h"

#define ConvertStringSecurityDescriptorToSecurityDescriptorA mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA
#define LocalFree mocked_LocalFree
#define CreateMutexExA mocked_CreateMutexExA
#define CreateEventA mocked_CreateEventA
#define CloseHandle mocked_CloseHandle
#define SetEvent mocked_SetEvent
#define ResetEvent mocked_ResetEvent
#define WaitForMultipleObjects mocked_WaitForMultipleObjects
#define WaitForSingleObject mocked_WaitForSingleObject
#define ReleaseMutex mocked_ReleaseMutex
#define GetCurrentThreadId mocked_GetCurrentThreadId

BOOL WINAPI mocked_ConvertStringSecurityDescriptorToSecurityDescriptorA(LPCSTR StringSecurityDescriptor, DWORD StringSDRevision, PSECURITY_DESCRIPTOR* SecurityDescriptor, PULONG SecurityDescriptorSize);
HLOCAL WINAPI mocked_LocalFree(HLOCAL hMem);
HANDLE WINAPI mocked_CreateMutexExA(LPSECURITY_ATTRIBUTES lpMutexAttributes, LPCSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess);
HANDLE WINAPI mocked_CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
BOOL WINAPI mocked_CloseHandle(HANDLE hObject);
BOOL WINAPI mocked_SetEvent(HANDLE hEvent);
BOOL WINAPI mocked_ResetEvent(HANDLE hEvent);
DWORD WINAPI mocked_WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
DWORD WINAPI mocked_WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
BOOL WINAPI mocked_ReleaseMutex(HANDLE hMutex);
DWORD WINAPI mocked_GetCurrentThreadId(void);

#include "../../src/robust_semaphore.c"

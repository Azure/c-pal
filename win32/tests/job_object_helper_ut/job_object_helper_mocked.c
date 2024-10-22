// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

#undef CreateJobObject
#define CreateJobObject mocked_CreateJobObject
HANDLE mocked_CreateJobObject(LPSECURITY_ATTRIBUTES lpJobAttributes, LPCSTR lpName);

#undef GetCurrentProcess
#define GetCurrentProcess mocked_GetCurrentProcess
HANDLE mocked_GetCurrentProcess(VOID);

#undef AssignProcessToJobObject
#define AssignProcessToJobObject mocked_AssignProcessToJobObject
BOOL mocked_AssignProcessToJobObject(HANDLE hJob, HANDLE hProcess);

#undef GlobalMemoryStatusEx
#define GlobalMemoryStatusEx mocked_GlobalMemoryStatusEx
BOOL mocked_GlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer);

#undef CloseHandle
#define CloseHandle mocked_CloseHandle
BOOL mocked_CloseHandle(HANDLE hObject);

#undef SetInformationJobObject
#define SetInformationJobObject mocked_SetInformationJobObject
BOOL mocked_SetInformationJobObject(HANDLE hJob, JOBOBJECTINFOCLASS JobObjectInformationClass, LPVOID lpJobObjectInformation, DWORD cbJobObjectInformationLength);

#include "../../src/job_object_helper.c"

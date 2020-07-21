// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "mock_file.h"
#define CreateFileA mock_CreateFileA
#define SetFileCompletionNotificationModes mock_SetFileCompletionNotificationModes
#define InitializeThreadpoolEnvironment mock_InitializeThreadpoolEnvironment
#define SetThreadpoolCallbackPool mock_SetThreadpoolCallbackPool
#define CreateThreadpoolCleanupGroup mock_CreateThreadpoolCleanupGroup
#define SetThreadpoolCallbackCleanupGroup mock_SetThreadpoolCallbackCleanupGroup
#define CreateThreadpoolIo mock_CreateThreadpoolIo
#define CloseThreadpoolIo mock_CloseThreadpoolIo
#define WaitForThreadpoolIoCallbacks mock_WaitForThreadpoolIoCallbacks
#define CloseThreadpoolCleanupGroup mock_CloseThreadpoolCleanupGroup
#define DestroyThreadpoolEnvironment mock_DestroyThreadpoolEnvironment
#define CloseHandle mock_CloseHandle
#define StartThreadpoolIo mock_StartThreadpoolIo
#undef CreateEvent
#define CreateEvent mock_CreateEvent
#define WriteFile mock_WriteFile
#define ReadFile mock_ReadFile
#define GetLastError mock_GetLastError
#define CancelThreadpoolIo mock_CancelThreadpoolIo

#include "../../src/file_win32.c"

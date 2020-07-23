// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"

#define HeapCreate mock_HeapCreate
#define HeapDestroy mock_HeapDestroy
#define HeapAlloc mock_HeapAlloc
#define HeapFree mock_HeapFree
#define HeapReAlloc mock_HeapReAlloc
#define HeapSize mock_HeapSize

#ifdef __cplusplus
extern "C" {
#endif
    HANDLE mock_HeapCreate(
        DWORD  flOptions,
        SIZE_T dwInitialSize,
        SIZE_T dwMaximumSize
    );

    BOOL mock_HeapDestroy(
        HANDLE hHeap
    );

    LPVOID mock_HeapAlloc(
        HANDLE hHeap,
        DWORD  dwFlags,
        SIZE_T dwBytes
    );

    BOOL HeapFree(
        HANDLE                 hHeap,
        DWORD                  dwFlags,
        _Frees_ptr_opt_ LPVOID lpMem
    );

    LPVOID HeapReAlloc(
        HANDLE                 hHeap,
        DWORD                  dwFlags,
        _Frees_ptr_opt_ LPVOID lpMem,
        SIZE_T                 dwBytes
    );

    SIZE_T HeapSize(
        HANDLE  hHeap,
        DWORD   dwFlags,
        LPCVOID lpMem
    );

#ifdef __cplusplus
}
#endif

#include "../../src/gballoc_ll_win32heap.c"

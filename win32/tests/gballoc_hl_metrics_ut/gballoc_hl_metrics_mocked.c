// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

#define HeapCreate mock_HeapCreate
#define HeapDestroy mock_HeapDestroy
#define HeapAlloc mock_HeapAlloc
#define HeapReAlloc mock_HeapReAlloc
#define HeapFree mock_HeapFree
#define HeapSize mock_HeapSize

extern HANDLE mock_HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
extern BOOL mock_HeapDestroy(HANDLE hHeap);
extern LPVOID mock_HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
extern LPVOID mock_HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
extern BOOL mock_HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
extern SIZE_T mock_HeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem);

#include "../../src/gballoc_hl_metrics.c"

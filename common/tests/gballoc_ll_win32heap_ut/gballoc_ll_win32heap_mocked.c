// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define HeapCreate mock_HeapCreate
#define HeapDestroy mock_HeapDestroy
#define HeapAlloc mock_HeapAlloc
#define HeapFree mock_HeapFree
#define HeapReAlloc mock_HeapReAlloc

#include "../../src/gballoc_ll_win32heap.c"

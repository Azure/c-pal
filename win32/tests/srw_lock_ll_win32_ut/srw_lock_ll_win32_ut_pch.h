// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for srw_lock_ll_win32_ut

#ifndef SRW_LOCK_LL_WIN32_UT_PCH_H
#define SRW_LOCK_LL_WIN32_UT_PCH_H

#include <stdlib.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_windows.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS


#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/srw_lock_ll.h"

#endif // SRW_LOCK_LL_WIN32_UT_PCH_H

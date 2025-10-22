// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for call_once_ut

#ifndef CALL_ONCE_UT_PCH_H
#define CALL_ONCE_UT_PCH_H

#include <stdbool.h>


#include "macro_utils/macro_utils.h" // IWYU pragma: keep

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_interlocked.h"
#include "real_sync.h"
#include "c_pal/call_once.h"

#endif // CALL_ONCE_UT_PCH_H

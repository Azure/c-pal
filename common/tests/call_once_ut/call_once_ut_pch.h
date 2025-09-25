// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for call_once_ut

#include <stdbool.h>


#include "macro_utils/macro_utils.h" // IWYU pragma: keep

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#undef ENABLE_MOCKS

#include "real_interlocked.h"
#include "real_sync.h"
#include "c_pal/call_once.h"

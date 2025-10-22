// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for lazy_init_ut

#ifndef LAZY_INIT_UT_PCH_H
#define LAZY_INIT_UT_PCH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/call_once.h"
#include "c_pal/interlocked.h"          // for volatile_atomic
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/lazy_init.h"

#endif // LAZY_INIT_UT_PCH_H

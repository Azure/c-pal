// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for sm_ut

#ifndef SM_UT_PCH_H
#define SM_UT_PCH_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
// IWYU pragma: no_include "c_pal/ps_util.h"
#include "c_pal/interlocked.h"// IWYU pragma: keep

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/log_critical_and_terminate.h" // IWYU pragma: keep
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_interlocked_hl.h"
#include "real_gballoc_hl.h"

#include "c_pal/sm.h"

#endif // SM_UT_PCH_H

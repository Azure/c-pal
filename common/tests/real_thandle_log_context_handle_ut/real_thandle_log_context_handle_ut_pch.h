// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for real_thandle_log_context_handle_ut

#ifndef REAL_THANDLE_LOG_CONTEXT_HANDLE_UT_PCH_H
#define REAL_THANDLE_LOG_CONTEXT_HANDLE_UT_PCH_H

#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h" // IWYU pragma: keep

#include "c_logging/log_context.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/thandle_ll.h"
#include "c_pal/thandle_ptr.h"
#include "c_pal/thandle_log_context_handle.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS
#include "real_gballoc_hl.h"
#include "real_thandle_log_context_handle.h"

#include "module.h"

#endif // REAL_THANDLE_LOG_CONTEXT_HANDLE_UT_PCH_H

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for error_handling_linux_ut

#ifndef ERROR_HANDLING_LINUX_UT_PCH_H
#define ERROR_HANDLING_LINUX_UT_PCH_H

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"

#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "c_pal/windows_defines_errors.h"

#include "c_pal/error_handling.h"

#endif // ERROR_HANDLING_LINUX_UT_PCH_H

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for robust_semaphore_ut

#ifndef ROBUST_SEMAPHORE_UT_PCH_H
#define ROBUST_SEMAPHORE_UT_PCH_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

#include "windows.h"
#include "sddl.h"

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_srw_lock_ll.h"

#include "c_pal/string_utils.h"

#include "c_pal/robust_semaphore.h"

#endif // ROBUST_SEMAPHORE_UT_PCH_H

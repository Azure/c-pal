// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for thandle_ptr_ut

#ifndef THANDLE_PTR_UT_PCH_H
#define THANDLE_PTR_UT_PCH_H

#include <stdlib.h>

#include "macro_utils/macro_utils.h"    // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "c_pal/thandle_ll.h"           // for THANDLE, THANDLE_ASSIGN

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "umock_c/umock_c.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thandle_ptr.h"

#endif // THANDLE_PTR_UT_PCH_H

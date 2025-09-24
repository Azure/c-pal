// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for thandle_ptr_ut

#include <stdlib.h>

#include "macro_utils/macro_utils.h"    // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "c_pal/thandle_ll.h"           // for THANDLE, THANDLE_ASSIGN

#define ENABLE_MOCKS
#include "umock_c/umock_c.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thandle_ptr.h"
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for thandle_ut

#include <stdlib.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "umock_c/umock_c.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thandle.h"
#include "thandle_user.h"
#include "thandle_user_33_characters.h" // IWYU pragma: keep
#include "thandle_flex_user.h"

#include "c_pal/thandle_ll.h"

#define TEST_A_DEFINE 1

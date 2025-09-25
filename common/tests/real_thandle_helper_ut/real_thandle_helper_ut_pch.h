// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for real_thandle_helper_ut

#include <stdint.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "testrunnerswitcher.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/interlocked.h" // IWYU pragma: keep
#include "c_pal/interlocked_hl.h" // IWYU pragma: keep
#include "c_pal/thandle.h"
#include "c_pal/thandle_ll.h"

// The incomplete type we want to mock
struct MOCKED_STRUCT_TAG; // IWYU pragma: private
typedef struct MOCKED_STRUCT_TAG MOCKED_STRUCT; // IWYU pragma: private
THANDLE_TYPE_DECLARE(MOCKED_STRUCT);

#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"


#include "real_gballoc_hl.h"
#include "real_interlocked.h" // IWYU pragma: keep
#include "real_interlocked_hl.h" // IWYU pragma: keep

#include "real_thandle_helper.h"

#include "real_interlocked_renames.h" // IWYU pragma: keep

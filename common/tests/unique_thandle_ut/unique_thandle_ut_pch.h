// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for unique_thandle_ut

#ifndef UNIQUE_THANDLE_UT_PCH_H
#define UNIQUE_THANDLE_UT_PCH_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#include "c_logging/logger.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thandle_ll.h" // IWYU pragma: keep
#include "c_pal/unique_thandle.h"
#include "c_pal/unique_thandle_ll.h" // IWYU pragma: keep

#include "unique_thandle_user.h"

/*a type whose UNIQUE_THANDLE functions are mocked*/
typedef struct MOCKED_STRUCT_TAG MOCKED_STRUCT;

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

UNIQUE_THANDLE_TYPE_DECLARE(MOCKED_STRUCT);

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#endif // UNIQUE_THANDLE_UT_PCH_H

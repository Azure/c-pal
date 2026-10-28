// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for unique_thandle_ut

#ifndef UNIQUE_THANDLE_UT_PCH_H
#define UNIQUE_THANDLE_UT_PCH_H

#include <stddef.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#include "c_pal/unique_thandle.h"
#include "c_pal/unique_thandle_ll.h" // IWYU pragma: keep

/*a type whose UNIQUE_THANDLE functions are mocked*/
typedef struct MOCKED_STRUCT_TAG MOCKED_STRUCT;

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

UNIQUE_THANDLE_TYPE_DECLARE(MOCKED_STRUCT);

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#endif // UNIQUE_THANDLE_UT_PCH_H

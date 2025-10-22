// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for file_util_linux_ut

#ifndef FILE_UTIL_LINUX_UT_PCH_H
#define FILE_UTIL_LINUX_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>
#include <string.h> // IWYU pragma: keep
#include <unistd.h> // IWYU pragma: keep
#include <fcntl.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h" // IWYU pragma: keep
#include "umock_c/umocktypes.h" // IWYU pragma: keep
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/windows_defines.h"

#include "c_pal/file_util.h"

#endif // FILE_UTIL_LINUX_UT_PCH_H

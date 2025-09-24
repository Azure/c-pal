// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for interlocked_linux_ut

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#include "c_pal/interlocked.h"
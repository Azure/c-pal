// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for interlocked_hl_ut

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "umock_c/umocktypes.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"

#define ENABLE_MOCKS
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#undef ENABLE_MOCKS

#include "real_interlocked.h"
#include "real_sync.h"
#include "real_gballoc_hl.h"
#include "c_pal/interlocked_hl.h"

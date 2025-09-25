// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for gballoc_hl_passthrough_ut

#include <stdlib.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_ll.h"
#undef ENABLE_MOCKS

#include "real_gballoc_ll.h"

#include "c_pal/gballoc_hl.h"

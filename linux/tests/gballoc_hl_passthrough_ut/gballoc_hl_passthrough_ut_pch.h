// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for gballoc_hl_passthrough_ut

#ifndef GBALLOC_HL_PASSTHROUGH_UT_PCH_H
#define GBALLOC_HL_PASSTHROUGH_UT_PCH_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>              // for memset, memcmp
#include <malloc.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_ll.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/gballoc_hl.h"

#endif // GBALLOC_HL_PASSTHROUGH_UT_PCH_H

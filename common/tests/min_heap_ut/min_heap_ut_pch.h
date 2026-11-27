// Copyright (C) Microsoft Corporation. All rights reserved.

// Precompiled header for min_heap_ut

#ifndef MIN_HEAP_UT_PCH_H
#define MIN_HEAP_UT_PCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/min_heap.h"

#endif // MIN_HEAP_UT_PCH_H

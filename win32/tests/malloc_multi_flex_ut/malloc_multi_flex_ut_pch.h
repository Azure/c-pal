// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for malloc_multi_flex_ut

#ifndef MALLOC_MULTI_FLEX_UT_PCH_H
#define MALLOC_MULTI_FLEX_UT_PCH_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>

#include "windows.h"
#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "test_module.h"

#endif // MALLOC_MULTI_FLEX_UT_PCH_H

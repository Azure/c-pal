// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for refcount_ut

#ifndef REFCOUNT_UT_PCH_H
#define REFCOUNT_UT_PCH_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"

#include "some_refcount_impl.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/interlocked.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/refcount.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"

#endif // REFCOUNT_UT_PCH_H

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for refcount_ut

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"

#include "some_refcount_impl.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
#include "c_pal/interlocked.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/refcount.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
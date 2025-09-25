// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for thandle_2_ut

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" /*THANDLE needs malloc/malloc_flex/free to exist*/
#include "malloc_mocks.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "t_off.h"
#include "t_on.h"

#include "c_pal/thandle_ll.h"

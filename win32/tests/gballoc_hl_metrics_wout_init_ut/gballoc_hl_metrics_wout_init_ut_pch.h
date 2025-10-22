// Copyright (c) Microsoft. All rights reserved.


// Precompiled header for gballoc_hl_metrics_wout_init_ut

#ifndef GBALLOC_HL_METRICS_WOUT_INIT_UT_PCH_H
#define GBALLOC_HL_METRICS_WOUT_INIT_UT_PCH_H

#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_ll.h"
#include "c_pal/lazy_init.h"
#include "c_pal/interlocked.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_lazy_init.h"
#include "real_interlocked.h"

#include "c_pal/gballoc_hl.h"

#endif // GBALLOC_HL_METRICS_WOUT_INIT_UT_PCH_H

// Copyright (c) Microsoft. All rights reserved.


// Precompiled header for gballoc_hl_metrics_ut

#include <stddef.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "windows.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"

#define ENABLE_MOCKS
#include "c_pal/timer.h"
#include "c_pal/gballoc_ll.h"
#include "c_pal/lazy_init.h"
#include "c_pal/interlocked.h"
#undef ENABLE_MOCKS

#include "real_lazy_init.h"
#include "real_interlocked.h"

#include "c_pal/gballoc_hl.h"

// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for threadpool_linux_ut

#ifndef THREADPOOL_LINUX_UT_PCH_H
#define THREADPOOL_LINUX_UT_PCH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/sync.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/thandle.h" // IWYU pragma: keep

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_srw_lock_ll.h"

#include "c_pal/threadpool.h"

#endif // THREADPOOL_LINUX_UT_PCH_H

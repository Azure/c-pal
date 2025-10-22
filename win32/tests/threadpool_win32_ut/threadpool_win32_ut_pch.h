// Copyright (c) Microsoft. All rights reserved.


// Precompiled header for threadpool_win32_ut

#ifndef THREADPOOL_WIN32_UT_PCH_H
#define THREADPOOL_WIN32_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>

#include "windows.h"
#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_win32.h"
#include "c_pal/interlocked.h"
#include "c_pal/ps_util.h"
#include "c_pal/srw_lock_ll.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/thandle.h"

#include "real_gballoc_hl.h"
#include "real_srw_lock_ll.h"

#include "c_pal/string_utils.h"
#include "c_pal/threadpool.h"

#endif // THREADPOOL_WIN32_UT_PCH_H

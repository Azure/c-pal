// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for process_watchdog_win32_ut

#ifndef PROCESS_WATCHDOG_WIN32_UT_PCH_H
#define PROCESS_WATCHDOG_WIN32_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>

#include "windows.h"
#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/interlocked.h"
#include "c_pal/ps_util.h"

#include "../mocked/inc/mock_threadpool_timer.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_interlocked.h"

#include "c_pal/process_watchdog.h"

#endif // PROCESS_WATCHDOG_WIN32_UT_PCH_H

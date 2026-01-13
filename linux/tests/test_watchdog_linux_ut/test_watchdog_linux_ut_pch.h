// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for test_watchdog_linux_ut

#ifndef TEST_WATCHDOG_LINUX_UT_PCH_H
#define TEST_WATCHDOG_LINUX_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "mock_timer.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/test_watchdog.h"

#endif // TEST_WATCHDOG_LINUX_UT_PCH_H

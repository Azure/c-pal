// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for execution_engine_linux_ut

#ifndef EXECUTION_ENGINE_LINUX_UT_PCH_H
#define EXECUTION_ENGINE_LINUX_UT_PCH_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/ps_util.h"
#include "c_pal/threadapi.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_interlocked.h"
#include "real_gballoc_hl.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#include "mock_execution_engine.h"

#endif // EXECUTION_ENGINE_LINUX_UT_PCH_H

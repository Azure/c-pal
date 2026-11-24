// Copyright (C) Microsoft Corporation. All rights reserved.

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
#include <ucontext.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "real_gballoc_ll.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/doublylinkedlist.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/interlocked.h"
#include "c_pal/min_heap.h"
#include "c_pal/thread_count_policy_linux.h"
#include "c_pal/timer.h"
#include "c_pal/worker_pool_linux.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_doublylinkedlist.h"
#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_min_heap.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#endif // EXECUTION_ENGINE_LINUX_UT_PCH_H

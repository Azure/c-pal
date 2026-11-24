// Copyright (C) Microsoft Corporation. All rights reserved.

// Precompiled header for timer_queue_linux_ut

#ifndef TIMER_QUEUE_LINUX_UT_PCH_H
#define TIMER_QUEUE_LINUX_UT_PCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/min_heap.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/timer_queue_linux.h"

#endif // TIMER_QUEUE_LINUX_UT_PCH_H

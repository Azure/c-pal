// Copyright (C) Microsoft Corporation. All rights reserved.

// Precompiled header for thread_count_policy_linux_ut

#ifndef THREAD_COUNT_POLICY_LINUX_UT_PCH_H
#define THREAD_COUNT_POLICY_LINUX_UT_PCH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thread_count_policy_linux.h"

#endif // THREAD_COUNT_POLICY_LINUX_UT_PCH_H

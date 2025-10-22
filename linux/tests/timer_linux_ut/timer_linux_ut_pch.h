// Copyright(C) Microsoft Corporation.All rights reserved.


// Precompiled header for timer_linux_ut

#ifndef TIMER_LINUX_UT_PCH_H
#define TIMER_LINUX_UT_PCH_H

#include <stdlib.h>
#include <time.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep




#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/timer.h"

// No idea why iwyu warns about this since we include time.h but...
// IWYU pragma: no_forward_declare timespec

#endif // TIMER_LINUX_UT_PCH_H

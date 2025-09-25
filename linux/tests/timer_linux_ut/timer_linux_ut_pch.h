// Copyright(C) Microsoft Corporation.All rights reserved.


// Precompiled header for timer_linux_ut

#include <stdlib.h>
#include <time.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep




#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/timer.h"

// No idea why iwyu warns about this since we include time.h but...
// IWYU pragma: no_forward_declare timespec

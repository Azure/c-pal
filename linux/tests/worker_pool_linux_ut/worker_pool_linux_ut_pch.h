// Copyright (C) Microsoft Corporation. All rights reserved.

// Precompiled header for worker_pool_linux_ut

#ifndef WORKER_POOL_LINUX_UT_PCH_H
#define WORKER_POOL_LINUX_UT_PCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <ucontext.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "real_gballoc_ll.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/doublylinkedlist.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_doublylinkedlist.h"
#include "real_gballoc_hl.h"

#include "c_pal/worker_pool_linux.h"

#endif // WORKER_POOL_LINUX_UT_PCH_H

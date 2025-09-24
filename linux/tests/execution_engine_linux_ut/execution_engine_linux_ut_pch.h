// Copyright (c) Microsoft. All rights reserved.


// Precompiled header for execution_engine_linux_ut

#include <inttypes.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"

#undef ENABLE_MOCKS

#include "real_interlocked.h"
#include "real_gballoc_hl.h"
#include "../reals/real_interlocked_hl.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"


#define DEFAULT_MIN_THREAD_COUNT 4
#define DEFAULT_MAX_THREAD_COUNT 0
#define MIN_THREAD_COUNT 5
#define MAX_THREAD_COUNT 10
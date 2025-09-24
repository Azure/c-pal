// Copyright (c) Microsoft. All rights reserved.



// Precompiled header for execution_engine_win32_ut

#include <stdlib.h>
#include <inttypes.h>


#include "windows.h"
#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "c_pal/execution_engine.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/execution_engine_win32.h"
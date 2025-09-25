// Copyright (c) Microsoft. All rights reserved.


// Precompiled header for srw_lock_linux_ut

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <pthread.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "real_gballoc_ll.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/string_utils.h"
#undef ENABLE_MOCKS

/*following function cannot be mocked because of variable number of arguments:( so it is copy&pasted here*/

#include "real_gballoc_hl.h"
#include "real_string_utils.h"

#include "c_pal/srw_lock.h"

// Copyright(C) Microsoft Corporation.All rights reserved.



// Precompiled header for job_object_helper_ut

#include <stdlib.h>

#include "windows.h"
#include "jobapi2.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_windows.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"

#define ENABLE_MOCKS
    #include "c_pal/gballoc_hl.h"
    #include "c_pal/gballoc_hl_redirect.h"


#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "c_pal/job_object_helper.h"

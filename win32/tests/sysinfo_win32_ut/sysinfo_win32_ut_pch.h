// Copyright(C) Microsoft Corporation.All rights reserved.



// Precompiled header for sysinfo_win32_ut

#include <stdlib.h>


#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"


#undef ENABLE_MOCKS

#include "c_pal/sysinfo.h"
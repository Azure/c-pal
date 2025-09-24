// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for pipe_win32_ut

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"

#define ENABLE_MOCKS
#include "mock_pipe.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/pipe.h"

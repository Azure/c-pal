// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for log_critical_and_terminate_ut

#include <inttypes.h>

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#define ENABLE_MOCKS

#include "c_pal/ps_util.h"

#undef ENABLE_MOCKS

#include "c_pal/log_critical_and_terminate.h"

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for srw_lock_ll_linux_ut

#ifndef SRW_LOCK_LL_LINUX_UT_PCH_H
#define SRW_LOCK_LL_LINUX_UT_PCH_H

#include <stdlib.h>

#include <pthread.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS


#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/srw_lock_ll.h"

#endif // SRW_LOCK_LL_LINUX_UT_PCH_H

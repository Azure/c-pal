// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for sync_linux_ut

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <limits.h>
#include <time.h>
#include <errno.h>


#include <syscall.h>
#include <linux/futex.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"

#include "c_pal/interlocked.h"
#include "c_pal/sync.h"

// No idea why iwyu warns about this since we include time.h but...
// IWYU pragma: no_forward_declare timespec
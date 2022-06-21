// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MOCK_SYNC_H
#define MOCK_SYNC_H
#include <time.h>
#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, int, mock_syscall, long, call_code, int*, uaddr, int, futex_op, int, val, const struct timespec*, timeout, int*, uaddr2, int, val3);

#endif
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MOCK_SYNC_H
#define MOCK_SYNC_H
#include <stdint.h>
#include <time.h>
#include "umock_c/umock_c_prod.h"

// Mock for the legacy SYS_futex syscall (used by 32-bit wait_on_address / wake_by_address).
MOCKABLE_FUNCTION(, int, mock_syscall, long, call_code, int*, uaddr, int, futex_op, int, val, const struct timespec*, timeout, int*, uaddr2, int, val3);

// Mocks for the Linux 6.7+ futex2 syscalls (used by 64-bit wait_on_address_64 / wake_by_address_*_64).
MOCKABLE_FUNCTION(, long, mock_futex_wait, long, call_code, void*, uaddr, uint64_t, val, uint64_t, mask, unsigned int, flags, struct timespec*, deadline, int, clockid);
MOCKABLE_FUNCTION(, long, mock_futex_wake, long, call_code, void*, uaddr, uint64_t, mask, int, nr, unsigned int, flags);

extern int mock_errno;

#endif

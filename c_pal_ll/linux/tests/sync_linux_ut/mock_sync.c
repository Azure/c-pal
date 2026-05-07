// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <errno.h> // IWYU pragma: keep
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#include "mock_sync.h"

#undef errno
#define errno mock_errno

int mock_errno;

// Variadic dispatcher that routes glibc-style variadic syscall calls to the
// appropriate mock based on the syscall number. Source code calls in
// sync_linux.c look like syscall(SYS_futex, ...) / syscall(SYS_futex_wait, ...) /
// syscall(SYS_futex_wake, ...) and have different argument layouts.
static long mock_syscall_dispatcher(long sysno, ...)
{
    va_list ap;
    va_start(ap, sysno);
    long ret;
    if (sysno == SYS_futex)
    {
        int* uaddr = va_arg(ap, int*);
        int op = va_arg(ap, int);
        int val = va_arg(ap, int);
        struct timespec* ts = va_arg(ap, struct timespec*);
        int* uaddr2 = va_arg(ap, int*);
        int val3 = va_arg(ap, int);
        ret = (long)mock_syscall(sysno, uaddr, op, val, ts, uaddr2, val3);
    }
    else if (sysno == SYS_futex_wait)
    {
        void* uaddr = va_arg(ap, void*);
        uint64_t val = va_arg(ap, uint64_t);
        uint64_t mask = va_arg(ap, uint64_t);
        unsigned int flags = va_arg(ap, unsigned int);
        struct timespec* deadline = va_arg(ap, struct timespec*);
        int clockid = va_arg(ap, int);
        ret = mock_futex_wait(sysno, uaddr, val, mask, flags, deadline, clockid);
    }
    else if (sysno == SYS_futex_wake)
    {
        void* uaddr = va_arg(ap, void*);
        uint64_t mask = va_arg(ap, uint64_t);
        int nr = va_arg(ap, int);
        unsigned int flags = va_arg(ap, unsigned int);
        ret = mock_futex_wake(sysno, uaddr, mask, nr, flags);
    }
    else
    {
        ret = -1;
    }
    va_end(ap);
    return ret;
}

#undef syscall
#define syscall mock_syscall_dispatcher

#include "../../src/sync_linux.c"

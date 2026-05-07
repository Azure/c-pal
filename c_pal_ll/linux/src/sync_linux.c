// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "c_logging/logger.h"

#include "sys/syscall.h"
#include "linux/futex.h"


#include "c_pal/interlocked.h"     // for volatile_atomic
#include "c_pal/sync.h"

// The 64-bit variants of wait_on_address / wake_by_address use the Linux 6.7+
// futex2 syscalls (SYS_futex_wait, SYS_futex_wake) with FUTEX2_SIZE_U64 |
// FUTEX2_PRIVATE so the kernel performs a true 64-bit atomic check-before-sleep.
// SYS_futex_wait / SYS_futex_wake come from <sys/syscall.h> (requires glibc 2.39+),
// FUTEX2_SIZE_U64 / FUTEX2_PRIVATE come from <linux/futex.h> (requires linux-libc-dev
// from kernel 6.7+). Build environments with older headers will fail to compile.

MU_DEFINE_ENUM_STRINGS(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES)

WAIT_ON_ADDRESS_RESULT wait_on_address(volatile_atomic int32_t* address, int32_t compare_value, uint32_t timeout_ms)
{
    WAIT_ON_ADDRESS_RESULT result;

    /* Codes_SRS_SYNC_43_001: [ wait_on_address shall atomically compare *address and *compare_address.] */
    /* Codes_SRS_SYNC_43_002: [ wait_on_address shall immediately return true if *address is not equal to *compare_address.] */
    /* Codes_SRS_SYNC_43_007: [ If *address is equal to *compare_address, wait_on_address shall cause the thread to sleep. ] */
    /* Codes_SRS_SYNC_43_009: [ If timeout_ms milliseconds elapse, wait_on_address shall return false. ] */
    /* Codes_SRS_SYNC_43_008: [wait_on_address shall wait indefinitely until it is woken up by a call to wake_by_address_[single/all] if timeout_ms is equal to UINT32_MAX] */
    /* Codes_SRS_SYNC_43_003: [ wait_on_address shall wait until another thread in the same process signals at address using wake_by_address_[single/all] and return true. ] */
    /* Codes_SRS_SYNC_LINUX_43_001: [ wait_on_address shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ] */
    struct timespec timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1e6 };

    /* Codes_SRS_SYNC_LINUX_43_002: [ wait_on_address shall call syscall to wait on value at address to change to a value different than the one provided in compare_value. ] */
    int syscall_result = syscall(SYS_futex, address, FUTEX_WAIT_PRIVATE, compare_value, &timeout, NULL, 0);
    if (syscall_result == 0)
    {
        /* Codes_SRS_SYNC_LINUX_43_003: [ If the value at address changes to a value different from compare_value then wait_on_address shall return WAIT_ON_ADDRESS_OK. ] */
        result = WAIT_ON_ADDRESS_OK;
    }
    else
    {
        if (errno == EAGAIN)
        {
            /* Codes_SRS_SYNC_LINUX_01_001: [ if syscall returns a non-zero value and errno is EAGAIN, wait_on_address shall return WAIT_ON_ADDRESS_OK. ] */
            result = WAIT_ON_ADDRESS_OK;
        }
        else if (errno == ETIMEDOUT)
        {
            /* Codes_SRS_SYNC_LINUX_24_001: [ if syscall returns a non-zero value and errno is ETIMEDOUT, wait_on_address shall return WAIT_ON_ADDRESS_TIMEOUT. ] */
            result = WAIT_ON_ADDRESS_TIMEOUT;
        }
        else
        {
            char err_msg[128];
            (void)strerror_r(errno, err_msg, 128);
            LogError("Failure in syscall, Error: %d: (%s)", errno, err_msg);
            /* Codes_SRS_SYNC_LINUX_43_004: [ Otherwise, wait_on_address shall return WAIT_ON_ADDRESS_ERROR. ] */
            result = WAIT_ON_ADDRESS_ERROR;
        }
    }

    return result;
}

WAIT_ON_ADDRESS_RESULT wait_on_address_64(volatile_atomic int64_t* address, int64_t compare_value, uint32_t timeout_ms)
{
    WAIT_ON_ADDRESS_RESULT result;

    /* Codes_SRS_SYNC_LINUX_43_007: [ wait_on_address_64 shall compute an absolute CLOCK_MONOTONIC deadline equal to now + timeout_ms milliseconds, or pass NULL when timeout_ms is UINT32_MAX. ] */
    struct timespec deadline;
    struct timespec* deadline_p;
    if (timeout_ms == UINT32_MAX)
    {
        deadline_p = NULL;
    }
    else
    {
        clock_gettime(CLOCK_MONOTONIC, &deadline);
        deadline.tv_sec += timeout_ms / 1000;
        deadline.tv_nsec += (long)((timeout_ms % 1000) * 1000000L);
        if (deadline.tv_nsec >= 1000000000L)
        {
            deadline.tv_sec += 1;
            deadline.tv_nsec -= 1000000000L;
        }
        deadline_p = &deadline;
    }

    /* Codes_SRS_SYNC_LINUX_43_008: [ wait_on_address_64 shall call syscall(SYS_futex_wait) with FUTEX2_SIZE_U64 | FUTEX2_PRIVATE and a CLOCK_MONOTONIC absolute deadline, performing a true 64-bit atomic check-before-sleep. ] */
    long syscall_result = syscall(SYS_futex_wait,
                                   address,
                                   (uint64_t)compare_value,
                                   ~(uint64_t)0,
                                   (unsigned int)(FUTEX2_SIZE_U64 | FUTEX2_PRIVATE),
                                   deadline_p,
                                   CLOCK_MONOTONIC);
    if (syscall_result == 0)
    {
        /* Codes_SRS_SYNC_LINUX_05_003: [ If the value at address changes to a value different from compare_value then wait_on_address_64 shall return WAIT_ON_ADDRESS_OK. ] */
        result = WAIT_ON_ADDRESS_OK;
    }
    else
    {
        if (errno == EAGAIN)
        {
            /* Codes_SRS_SYNC_LINUX_05_004: [ If syscall returns a non-zero value and errno is EAGAIN, wait_on_address_64 shall return WAIT_ON_ADDRESS_OK. ] */
            result = WAIT_ON_ADDRESS_OK;
        }
        else if (errno == ETIMEDOUT)
        {
            /* Codes_SRS_SYNC_LINUX_05_005: [ If syscall returns a non-zero value and errno is ETIMEDOUT, wait_on_address_64 shall return WAIT_ON_ADDRESS_TIMEOUT. ] */
            result = WAIT_ON_ADDRESS_TIMEOUT;
        }
        else
        {
            char err_msg[128];
            (void)strerror_r(errno, err_msg, 128);
            LogError("failure in syscall, Error: %d: (%s)", errno, err_msg);
            /* Codes_SRS_SYNC_LINUX_05_006: [ Otherwise, wait_on_address_64 shall return WAIT_ON_ADDRESS_ERROR. ] */
            result = WAIT_ON_ADDRESS_ERROR;
        }
    }

    return result;
}

void wake_by_address_all(volatile_atomic int32_t* address)
{
    /* Codes_SRS_SYNC_43_004: [ wake_by_address_all shall cause all the thread(s) waiting on a call to wait_on_address with argument address to continue execution. ] */
    /* Codes_SRS_SYNC_LINUX_43_005: [ wake_by_address_all shall call syscall to wake all listeners listening on address. ] */
    syscall(SYS_futex, address, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0);
}

void wake_by_address_all_64(volatile_atomic int64_t* address)
{
    /* Codes_SRS_SYNC_LINUX_43_009: [ wake_by_address_all_64 shall call syscall(SYS_futex_wake) with FUTEX2_SIZE_U64 | FUTEX2_PRIVATE to wake all listeners on the 64-bit address. ] */
    syscall(SYS_futex_wake,
            address,
            ~(uint64_t)0,
            INT_MAX,
            (unsigned int)(FUTEX2_SIZE_U64 | FUTEX2_PRIVATE));
}

void wake_by_address_single(volatile_atomic int32_t* address)
{
    /* Codes_SRS_SYNC_43_005: [ wake_by_address_single shall cause one thread waiting on a call to wait_on_address with argument address to continue execution. ] */
    /* Codes_SRS_SYNC_LINUX_43_006: [ wake_by_address_single shall call syscall to wake any single listener listening on address. ] */
    syscall(SYS_futex, address, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
}

void wake_by_address_single_64(volatile_atomic int64_t* address)
{
    /* Codes_SRS_SYNC_LINUX_43_010: [ wake_by_address_single_64 shall call syscall(SYS_futex_wake) with FUTEX2_SIZE_U64 | FUTEX2_PRIVATE to wake one listener on the 64-bit address. ] */
    syscall(SYS_futex_wake,
            address,
            ~(uint64_t)0,
            1,
            (unsigned int)(FUTEX2_SIZE_U64 | FUTEX2_PRIVATE));
}

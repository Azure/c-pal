# sync linux

## Overview

`sync linux` is the Linux implementation of the `sync` header using [futex](https://www.man7.org/linux/man-pages/man2/futex.2.html).

## Exposed API

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms);
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address);
```

## wait_on_address

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms)
```

**SRS_SYNC_LINUX_43_001: [** `wait_on_address` shall initialize a `timespec` struct with `.tv_nsec` equal to `timeout_ms* 10^6`. **]**

**SRS_SYNC_LINUX_43_002: [** `wait_on_address` shall call `syscall` from `sys/syscall.h` with arguments `SYS_futex`, `address`, `FUTEX_WAIT_PRIVATE`, `compare_value`, `*timeout_struct`, `NULL`, `0`. **]**

**SRS_SYNC_LINUX_43_003: [** `wait_on_address` shall return `WAIT_ON_ADDRESS_OK` if `syscall` returns `0`.**]**

**SRS_SYNC_LINUX_01_001: [** if `syscall` returns a non-zero value and `errno` is `EAGAIN`, `wait_on_address` shall return `WAIT_ON_ADDRESS_OK`. **]**

**SRS_SYNC_LINUX_24_001: [** if `syscall` returns a non-zero value and `errno` is `ETIMEDOUT`, `wait_on_address` shall return `WAIT_ON_ADDRESS_TIMEOUT`. **]**

**SRS_SYNC_LINUX_43_004: [** Otherwise, `wait_on_address` shall return `WAIT_ON_ADDRESS_ERROR`.**]**

## wake_by_address_all

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address)
```

**SRS_SYNC_LINUX_43_005: [** `wake_by_address_all` shall call `syscall` from `sys/syscall.h` with arguments `SYS_futex`, `address`, `FUTEX_WAKE_PRIVATE`, `INT_MAX`, `NULL`, `NULL`, `0`. **]**

## wake_by_address_single

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address)
```

**SRS_SYNC_LINUX_43_006: [** `wake_by_address_single` shall call `syscall` from `sys/syscall.h` with arguments `SYS_futex`, `address`, `FUTEX_WAKE_PRIVATE`, `1`, `NULL`, `NULL`, `0`. **]**

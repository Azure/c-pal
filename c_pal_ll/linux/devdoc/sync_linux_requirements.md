# sync linux

## Overview

`sync linux` is the Linux implementation of the `sync` header using [futex](https://www.man7.org/linux/man-pages/man2/futex.2.html).

## Exposed API

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms);
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address_64, volatile_atomic int64_t*, address, int64_t, compare_value, uint32_t, timeout_ms);
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_all_64, volatile_atomic int64_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single_64, volatile_atomic int64_t*, address);
```

## wait_on_address

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms)
```

**SRS_SYNC_LINUX_43_001: [** `wait_on_address` shall initialize a `timespec` struct with `.tv_nsec` equal to `timeout_ms* 10^6`. **]**

**SRS_SYNC_LINUX_43_002: [** `wait_on_address` shall call `syscall` to wait on value at `address` to change to a value different than the one provided in `compare_value`. **]**

**SRS_SYNC_LINUX_43_003: [** If the value at `address` changes to a value different from `compare_value` then `wait_on_address` shall return `WAIT_ON_ADDRESS_OK`. **]**

**SRS_SYNC_LINUX_01_001: [** if `syscall` returns a non-zero value and `errno` is `EAGAIN`, `wait_on_address` shall return `WAIT_ON_ADDRESS_OK`. **]**

**SRS_SYNC_LINUX_24_001: [** if `syscall` returns a non-zero value and `errno` is `ETIMEDOUT`, `wait_on_address` shall return `WAIT_ON_ADDRESS_TIMEOUT`. **]**

**SRS_SYNC_LINUX_43_004: [** Otherwise, `wait_on_address` shall return `WAIT_ON_ADDRESS_ERROR`.**]**

## wait_on_address_64

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address_64, volatile_atomic int64_t*, address, int64_t, compare_value, uint32_t, timeout_ms)
```

**SRS_SYNC_LINUX_43_007: [** `wait_on_address_64` shall compute an absolute `CLOCK_MONOTONIC` deadline equal to now + `timeout_ms` milliseconds, or pass `NULL` when `timeout_ms` is `UINT32_MAX`. **]**

**SRS_SYNC_LINUX_43_008: [** `wait_on_address_64` shall call `syscall(SYS_futex_wait)` with `FUTEX2_SIZE_U64 | FUTEX2_PRIVATE` and a `CLOCK_MONOTONIC` absolute deadline, performing a true 64-bit atomic check-before-sleep. **]**

**SRS_SYNC_LINUX_05_003: [** If the value at `address` changes to a value different from `compare_value` then `wait_on_address_64` shall return `WAIT_ON_ADDRESS_OK`. **]**

**SRS_SYNC_LINUX_05_004: [** If `syscall` returns a non-zero value and `errno` is `EAGAIN`, `wait_on_address_64` shall return `WAIT_ON_ADDRESS_OK`. **]**

**SRS_SYNC_LINUX_05_005: [** If `syscall` returns a non-zero value and `errno` is `ETIMEDOUT`, `wait_on_address_64` shall return `WAIT_ON_ADDRESS_TIMEOUT`. **]**

**SRS_SYNC_LINUX_05_006: [** Otherwise, `wait_on_address_64` shall return `WAIT_ON_ADDRESS_ERROR`. **]**


## wake_by_address_all

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address)
```

**SRS_SYNC_LINUX_43_005: [** `wake_by_address_all` shall call `syscall` to wake all listeners listening on `address`. **]**

## wake_by_address_all_64

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all_64, volatile_atomic int64_t*, address)
```

**SRS_SYNC_LINUX_43_009: [** `wake_by_address_all_64` shall call `syscall(SYS_futex_wake)` with `FUTEX2_SIZE_U64 | FUTEX2_PRIVATE` to wake all listeners on the 64-bit `address`. **]**

## wake_by_address_single

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address)
```

**SRS_SYNC_LINUX_43_006: [** `wake_by_address_single` shall call `syscall` to wake any single listener listening on `address`. **]**

## wake_by_address_single_64

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single_64, volatile_atomic int64_t*, address)
```

**SRS_SYNC_LINUX_43_010: [** `wake_by_address_single_64` shall call `syscall(SYS_futex_wake)` with `FUTEX2_SIZE_U64 | FUTEX2_PRIVATE` to wake one listener on the 64-bit `address`. **]**
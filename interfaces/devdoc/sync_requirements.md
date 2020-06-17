# sync
================

## Overview

`sync` provides platform-independent synchronization primitives:

- `wait_on_address`: causes the thread to wait until another thread calls `wake_by_address_[single/all]` on the same address.
- `wake_on_address_[single/all]`: causes the thread(s) that are waiting inside a `wait_on_address` call to continue execution.

## Exposed API

```c
MOCKABLE_FUNCTION(, bool, wait_on_address, volatile_atomic int32_t*, address, int32_t*, compare_address, uint32_t, timeout_ms);
MOCKABLE_FUNCTION(, void, wake_by_address_all, void*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single, void*, address);
```

## wait_on_address

```c
MOCKABLE_FUNCTION(, bool, wait_on_address, volatile_atomic int32_t*, address, int32_t*, compare_address, uint32_t, timeout_ms)
```
`wait_on_address` causes the executing thread to sleep if the value `*address` and `*compare_address` have the same value. The thread sleeps until the timeout elapses or another thread calls `wake_by_address_[single/all]` on the same address that the current thread is waiting on.

**SRS_SYNC_43_001: [** `wait_on_address` shall atomically compare `*address` and `compare_address`.**]**

**SRS_SYNC_43_002: [** `wait_on_address` shall immediately return `false` if `*address` is not equal to `*compare_address`.**]**

**SRS_SYNC_43_007: [** If `*address` is equal to `*compare_address`, `wait_on_address` shall cause the thread to sleep for atmost `timeout_ms` milliseconds, if `timeout_ms` is not equal to `UINT32_MAX`. **]**

**SRS_SYNC_43_008: [**`wait_on_address` shall wait indefinitely until it is woken up by a call to `wake_by_address_[single/all]` if `timeout_ms` is equal to `UINT32_MAX`**]**

**SRS_SYNC_43_003: [** `wait_on_address` shall wait until another thread in the same process signals at `address` using `wake_by_address_[single/all]` or the timeout elapses. **]**

**SRS_SYNC_43_006: [** `wait_on_address` shall return `true` if the thread is woken up or the timeout expires.**]**

## wake_by_address_all

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all, void*, address)
```
`wake_by_address_all` wakes up all the threads waiting in a `wait_on_address` call on the given `address`.

**SRS_SYNC_43_004: [** `wake_by_address_all` shall cause all the thread(s) waiting on a call to `wait_on_address` with argument `address` to continue execution. **]**

## wake_by_address_single

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single, void*, address)
```
`wake_by_address_single` wakes up a single thread waiting in a `wait_on_address` call on the given `address`.

**SRS_SYNC_43_005: [** `wake_by_address_single` shall cause one thread waiting on a call to `wait_on_address` with argument `address` to continue execution. **]**
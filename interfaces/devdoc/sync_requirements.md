# sync
================

## Overview

`sync` provides platform-independent synchronization primitives.

## Exposed API

```c
MOCKABLE_FUNCTION(, bool, wait_on_address, volatile_atomic int32_t*, address, int32_t*, compare_address, uint32_t, timeout);
MOCKABLE_FUNCTION(, void, wake_by_address_all, void*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single, void*, address);
```

## wait_on_address

```c
MOCKABLE_FUNCTION(, bool, wait_on_address, volatile_atomic int32_t*, address, int32_t*, compare_address, uint32_t, timeout)
```

**SRS_SYNC_43_001: [** `wait_on_address` shall atomically compare `*address` and `compare_address`.**]**

**SRS_SYNC_43_002: [** `wait_on_address` shall immediately return false if `*address` is not equal to `*compare_address`.**]**

**SRS_SYNC_43_003: [** `wait_on_address` shall wait until another thread in the same process signals at `address` using `wake_by_address_single` or `wake_by_address_all`, or `timeout` milliseconds pass, whichever comes first. **]**

## wake_by_address_all

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all, void*, address)
```

**SRS_SYNC_43_004: [** `wake_by_address_all` shall cause all the threads that called `wait_on_address` on `address` to continue execution. **]**

## wake_by_address_single

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single, void*, address)
```

**SRS_SYNC_43_005: [** `wake_by_address_single` shall cause one of the threads that called `wait_on_address` on `address` to continue execution. **]**
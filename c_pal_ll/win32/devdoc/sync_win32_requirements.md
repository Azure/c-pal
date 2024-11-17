# sync win32

## Overview

`sync win32` is the Windows implementation of the `sync` header using [syncapi](https://docs.microsoft.com/en-us/windows/win32/api/synchapi).

## Exposed API

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms);
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address_64, volatile_atomic int64_t*, address, int64_t, compare_value, uint32_t, timeout_ms);
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_all_64_, volatile_atomic int64_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address);
MOCKABLE_FUNCTION(, void, wake_by_address_single_64_, volatile_atomic int64_t*, address);
```

## wait_on_address

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address, volatile_atomic int32_t*, address, int32_t, compare_value, uint32_t, timeout_ms)
```

**SRS_SYNC_WIN32_43_001: [** `wait_on_address` shall call `WaitOnAddress` to wait on the value at 32-bit `address` to be different than `compare_value` for `timeout_ms` milliseconds. **]**

**SRS_SYNC_WIN32_24_001: [** If `WaitOnAddress` fails due to timeout, `wait_on_address` shall fail and return `WAIT_ON_ADDRESS_TIMEOUT`. **]**

**SRS_SYNC_WIN32_24_002: [** If `WaitOnAddress` fails due to any other reason, `wait_on_address` shall fail and return `WAIT_ON_ADDRESS_ERROR`. **]**

**SRS_SYNC_WIN32_24_003: [** If `WaitOnAddress` succeeds, `wait_on_address` shall return `WAIT_ON_ADDRESS_OK`. **]**

## wait_on_address_64

```c
MOCKABLE_FUNCTION(, WAIT_ON_ADDRESS_RESULT, wait_on_address_64, volatile_atomic int64_t*, address, int64_t, compare_value, uint32_t, timeout_ms)
```
**SRS_SYNC_WIN32_05_001: [** `wait_on_address_64` shall call `WaitOnAddress` to wait on the value at 64-bit `address` to be different than `compare_value` for `timeout_ms` milliseconds. **]**

**SRS_SYNC_WIN32_05_002: [** If `WaitOnAddress` fails due to timeout, `wait_on_address_64` shall fail and return `WAIT_ON_ADDRESS_TIMEOUT`. **]**

**SRS_SYNC_WIN32_05_003: [** If `WaitOnAddress` fails due to any other reason, `wait_on_address_64` shall fail and return `WAIT_ON_ADDRESS_ERROR`. **]**

**SRS_SYNC_WIN32_05_004: [** If `WaitOnAddress` succeeds, `wait_on_address_64` shall return `WAIT_ON_ADDRESS_OK`. **]**

## wake_by_address_all

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address)
```
**SRS_SYNC_WIN32_43_003: [** `wake_by_address_all` shall call `WakeByAddressAll` to notify all listeners waiting on the 32-bit `address`. **]**

## wake_by_address_all_64

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all_64, volatile_atomic int64_t*, address)
```
**SRS_SYNC_WIN32_05_005: [** `wake_by_address_all_64` shall call `WakeByAddressAll` to notify all listeners waiting on the 64-bit `address`. **]**

## wake_by_address_single

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address)
```

**SRS_SYNC_WIN32_43_004: [** `wake_by_address_single` shall call `WakeByAddressSingle` to notify a single listeners waiting on the 32-bit `address`. **]**

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single_64, volatile_atomic int64_t*, address)
```

**SRS_SYNC_WIN32_05_006: [** `wake_by_address_single_64` shall call `WakeByAddressSingle` to notify a single listeners waiting on the 64-bit `address`. **]**

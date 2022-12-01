# sync win32

## Overview

`sync win32` is the Windows implementation of the `sync` header using [syncapi](https://docs.microsoft.com/en-us/windows/win32/api/synchapi).

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

**SRS_SYNC_WIN32_43_001: [** `wait_on_address` shall call `WaitOnAddress` from `windows.h` with `address` as `Address`, a pointer to the value `compare_value` as `CompareAddress`, `4` as `AddressSize` and `timeout_ms` as `dwMilliseconds`. **]**

**SRS_SYNC_WIN32_24_001: [** If `WaitOnAddress` fails due to timeout, `wait_on_address` shall fail and return `WAIT_ON_ADDRESS_TIMEOUT`. **]**

**SRS_SYNC_WIN32_24_002: [** If `WaitOnAddress` fails due to any other reason, `wait_on_address` shall fail and return `WAIT_ON_ADDRESS_ERROR`. **]**

**SRS_SYNC_WIN32_24_003: [** If `WaitOnAddress` succeeds, `wait_on_address` shall return `WAIT_ON_ADDRESS_OK`. **]**

## wake_by_address_all

```c
MOCKABLE_FUNCTION(, void, wake_by_address_all, volatile_atomic int32_t*, address)
```
**SRS_SYNC_WIN32_43_003: [** `wake_by_address_all` shall call `WakeByAddressAll` from `windows.h` with `address` as `Address`. **]**

## wake_by_address_single

```c
MOCKABLE_FUNCTION(, void, wake_by_address_single, volatile_atomic int32_t*, address)
```

**SRS_SYNC_WIN32_43_004: [** `wake_by_address_single` shall call `WakeByAddressSingle` from `windows.h` with `address` as `Address`. **]**

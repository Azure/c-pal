# error handling

## Overview

Linux implentaton of GetLastError and SetLastError 

## Exposed API

```c
uint64_t last_error_code;

MOCKABLE_FUNCTION(, void, error_handling_set_last_error, uint32_t, err_code);
MOCKABLE_FUNCTION(, uint32_t, error_handling_get_last_error);
```

### set_last_error

```c
MOCKABLE_FUNCTION(, void, error_handling_set_last_error, uint32_t, err_code);
```
`error_handling_set_last_error` implements the PAL wrapper for the SetLastError function from Windows.

**SRS_FILE_UTIL_LINUX_09_006: [** `error_handling_set_last_error` shall succeed and assign a non-`NULL` value to `last_error_code`. **]**

**SRS_FILE_UTIL_LINUX_09_002: [** `error_handling_set_last_error` shall call `interlocked_exchange_64` with `err_code` and `last_error_code`. **]**


**SRS_FILE_UTIL_LINUX_09_003: [** If the `err_code` is `NULL`, `error_handling_set_last_error` shall not change `last_error_code`. **]**

### get_last_error

```c
MOCKABLE_FUNCTION(, uint32_t, error_handling_get_last_error);
```
`error_handling_get_last_error` implements the PAL wrapper for the GetLastError function from Windows.

**SRS_FILE_UTIL_LINUX_09_007: [** On success, `error_handling_get_last_error` shall return a non-`NULL` value. **]**

**SRS_FILE_UTIL_LINUX_09_004: [** `error_handling_get_last_error` shall call `interlocked_add` with `last_error_code` and zero. **]**

**SRS_FILE_UTIL_LINUX_09_005: [** If there are any errors, `error_handling_get_last_error` shall fail and return `NULL`. **]**

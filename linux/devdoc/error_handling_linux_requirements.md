# error handling

## Overview

Linux implentaton of GetLastError and SetLastError 

## Exposed API

```c
MOCKABLE_FUNCTION(, void, error_handling_linux_set_last_error, uint32_t, err_code);
MOCKABLE_FUNCTION(, uint32_t, error_handling_linux_get_last_error);
```

### set_last_error

```c
MOCKABLE_FUNCTION(, void, error_handling_linux_set_last_error, volatile_atomic int32_t, err_code);
```
`error_handling_linux_set_last_error` implements the PAL wrapper for the SetLastError function from Windows.

**SRS_ERROR_HANDLING_LINUX09_002: [** `error_handling_linux_set_last_error` shall assign a non-`NULL` value to `last_error_code`. **]**

**SRS_ERROR_HANDLING_LINUX09_003: [** `error_handling_linux_set_last_error` shall call `interlocked_exchange_32` with `err_code` and `last_error_code`. **]**

### get_last_error

```c
MOCKABLE_FUNCTION(, uint32_t, error_handling_linux_get_last_error);
```
`error_handling_linux_get_last_error` implements the PAL wrapper for the GetLastError function from Windows.

**SRS_ERROR_HANDLING_LINUX09_005: [** On success, `error_handling_linux_get_last_error` shall return the value last set through `set_last_error` or zero **]**

**SRS_ERROR_HANDLING_LINUX09_006: [** `error_handling_linux_get_last_error` shall call `interlocked_add` with `last_error_code` and zero. **]**


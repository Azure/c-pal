# error handling

## Overview

Linux implentaton of GetLastError and SetLastError 

## Exposed API

```c
MOCKABLE_FUNCTION(, void, error_handling_set_last_error,  volatile_atomic int64_t, err_code);
MOCKABLE_FUNCTION(, uint64_t, error_handling_get_last_error);
```

### set_last_error

```c
MOCKABLE_FUNCTION(, void, error_handling_set_last_error,  volatile_atomic int64_t, err_code);
```
`error_handling_set_last_error` implements the PAL wrapper for the SetLastError function from Windows.

`error_handling_set_last_error` shall assign a non-`NULL` value to `last_error_code`.

`error_handling_set_last_error` shall call `interlocked_exchange_64` with `err_code` and `last_error_code`.

### get_last_error

```c
MOCKABLE_FUNCTION(, uint64_t, error_handling_get_last_error);
```
`error_handling_get_last_error` implements the PAL wrapper for the GetLastError function from Windows.

On success, `error_handling_get_last_error` shall return the value last set through `set_last_error` or zero 

`error_handling_get_last_error` shall call `interlocked_add` with `last_error_code` and zero.

If there are any errors, `error_handling_get_last_error` shall fail and return `ERROR_INVALID_DATA`.

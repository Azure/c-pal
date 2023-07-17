# error handling

## Overview

Linux implementation of SetLastError and GetLastError from Windows

## Exposed API

```c
MOCKABLE_FUNCTION(, void, error_handling_linux_set_last_error, volatile_atomic int64_t, err_code);
MOCKABLE_FUNCTION(, uint64_t, error_handling_linux_get_last_error);
```

### set_last_error

```c
MOCKABLE_FUNCTION(, void, error_handling_linux_set_last_error, atomic_uint64_t, err_code);
```

error_handling_linux_set_last_error is a PAL wrapper for the SetLastError function in Windows. 
    [SetLastError]("https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setlasterror")

### get_last_error

```c
MOCKABLE_FUNCTION(, uint64_t, error_handling_linux_get_last_error);
```

error_handling_linux_get_last_error is a PAL wrapper for the GetLastError function in Windows
    [GetLastError]("https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror")
    

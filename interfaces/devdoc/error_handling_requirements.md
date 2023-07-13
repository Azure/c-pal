# error handling

## Overview

Linux implementation of SetLastError and GetLastError from Windows

## Exposed API

```c
uint64_t last_error_code;

MOCKABLEFUNCTION(void, error_handling_set_last_error, uint32_t, err_code);
MOCKABLEFUNCTION(uint32_t, error_handling_get_last_error);
```

### set_last_error

```c
MOCKABLEFUNCTION(void, error_handling_set_last_error, uint32_t, err_code);
```

error_handling_set_last_error is a PAL wrapper for the SetLastError function in Windows. 
    [SetLastError]("https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setlasterror")

### get_last_error

```c
MOCKABLEFUNCTION(uint32_t, error_handling_get_last_error);
```

error_handling_get_last_error is a PAL wrapper for the GetLastError function in Windows
    [GetLastError]("https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror")
    

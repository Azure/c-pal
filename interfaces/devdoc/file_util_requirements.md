# file_util_linux

## Overview

Linux implementation of the file operation functions.

## Exposed API 

```c
#define INVALID_HANDLE_VALUE            NULL

typedef void *HANDLE;
typedef void *LPSECURITY_ATTRIBUTES;

MOCKABLE_FUNCTION(, HANDLE, file_util_open_file, const char*, full_file_name, unsigned long, desired_access, unsigned long, share_mode, LPSECURITY_ATTRIBUTES, security_attributes, unsigned long, creation_disposition, unsigned long, flags_and_attributes, HANDLE, template_file);
MOCKABLE_FUNCTION(, bool, file_util_close_file, HANDLE, handle_input);
```

### file_util_open_file

```c
MOCKABLE_FUNCTION(, HANDLE, file_util_open_file, const char*, full_file_name, unsigned long, desired_access, unsigned long, share_mode, LPSECURITY_ATTRIBUTES, security_attributes, unsigned long, creation_disposition, unsigned long, flags_and_attributes, HANDLE, template_file);
```

`file_util_open_file` is a PAL wrapper for Linux for the CreateFileA function in Windows. Uses the `open` function in C for Linux. 
    [CreateFileA]([https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#return-value])

share_mode, security_attributes, flags_and_attributes, and template_file parameters are ignored in Linux

### file_util_close_file

```c
MOCKABLE_FUNCTION(, bool, file_util_close_file, HANDLE, handle_input);
```

`file_util_close_file` is a PAL wrapper for Linux for the CloseHandle function in Windows. Uses the `close` function in C for Linux. 
    [CloseHandle]([https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle])
    

# file_util_linux

## Overview

Linux implementation of the file operation functions.

## Exposed API 

```c
// Most defines removed for space constraints
#define INVALID_HANDLE_VALUE            NULL

typedef void *HANDLE;
typedef void *LPSECURITY_ATTRIBUTES;

MOCKABLE_FUNCTION(, HANDLE, file_util_create_file, const char*, lpFileName, unsigned long, desired_access, unsigned long, share_mode, LPSECURITY_ATTRIBUTES, lp_security_attributes, unsigned long, creation_disposition, unsigned long, flags_and_attributes, HANDLE, hTemplateFile);
MOCKABLE_FUNCTION(, bool, file_util_close_handle, HANDLE, handle_input);
```

### file_util_create_file

```c
MOCKABLE_FUNCTION(, HANDLE, file_util_create_file, const char*, full_file_name, unsigned long, desired_access, unsigned long, share_mode, LPSECURITY_ATTRIBUTES, lp_security_attributes, unsigned long, creation_disposition, unsigned long, flags_and_attributes, HANDLE, h_template_file);
```
//share_mode, lp_security_attributes, flags_and_attributes, and h_template_file parameters are ignored in Linux

`file_util_create_file` is a PAL wrapper for Linux for the CreateFileA function in Windows. Uses the `open` function in C for Linux. 
    https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#return-value

### file_util_close_handle

```c
MOCKABLE_FUNCTION(, bool, file_util_close_handle, HANDLE, handle_input);
```

`file_util_close_handle` is a PAL wrapper for Linux for the CloseHandle function in Windows. Uses the `close` function in C for Linux. 
    https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
    

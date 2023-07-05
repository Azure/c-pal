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

`file_util_create_file` implements the creating file function in Linux. Uses the `open` function in C for Linux.

**SRS_FILE_UTIL_LINUX_09_001: [** If the `full_file_name` input is either empty or `NULL`, `file_util_create_file` shall return an `INVALID_HANDLE_VALUE`. **]**

**SRS_FILE_UTIL_LINUX_09_002: [** `file_util_create_file` shall allocate memory for the file handle. `file_util_create_file` will succeed and return a non-`NULL` value.  **]**

**SRS_FILE_UTIL_LINUX_09_003: [** If memory allocation for `result` fails, `file_util_create_file` shall return an `INVALID_HANDLE_VALUE`. **]**

**SRS_FILE_UTIL_LINUX_09_004: [** If `desired_access` is `GENERIC_READ`, `file_util_create_file` will call `open` with `O_RDONLY` and shall return a file handle for read only.  **]**

**SRS_FILE_UTIL_LINUX_09_005: [** If `desired_access` is `GENERIC_WRITE`, `file_util_create_file` will call `open` with `O_WRONLY` and shall return a file handle for write only.  **]**

**SRS_FILE_UTIL_LINUX_09_006: [** If `desired_access` is `GENERIC_ALL` or `GENERIC_READ&GENERIC_WRITE`, `file_util_create_file` will call `open` with `O_RDWR` and shall return a file handle for read and write.  **]**

**SRS_FILE_UTIL_LINUX_09_014: [** If `creation_disposition` is `CREATE_ALWAYS` or `OPEN_ALWAYS`, `file_util_create_file` will call `open` with `O_CREAT` and shall either create a new file handle if the specificied pathname exists and return it or return an existing file handle.  **]**

**SRS_FILE_UTIL_LINUX_09_015: [** If `creation_disposition` is `CREATE_NEW`, `file_util_create_file` will call `open` with `O_CREAT|O_EXCL` and shall return a new file handle if the file doesn't already exist.  **]**

**SRS_FILE_UTIL_LINUX_09_016: [** If `creation_disposition` is `CREATE_NEW` and the file already exists, `file_util_create_file` will fail and return `INVALID_HANDLE_VALUE`.  **]**

**SRS_FILE_UTIL_LINUX_09_017: [** If `creation_disposition` is `TRUNCATE_EXISTING`, `file_util_create_file` will call `open` with `O_TRUNC` and shall return a file handle who's size has been truncated to zero bytes.  **]**

**SRS_FILE_UTIL_LINUX_09_008: [** If there are any failures, `file_util_create_file` shall fail and return `INVALID_HANDLE_VALUE`.  **]**


### file_util_close_handle

```c
MOCKABLE_FUNCTION(, bool, file_util_close_handle, HANDLE, handle_input);
```

`file_util_close_handle` implements the close handle function in Linux. Uses the `close` function in C for Linux.

**SRS_FILE_UTIL_LINUX_09_009: [** If `handle_input` is `NULL`, `file_util_close_handle` returns false.  **]**

**SRS_FILE_UTIL_LINUX_09_011: [** `file_util_close_handle` closes a file handle in linux and returns true.  **]**

**SRS_FILE_UTIL_LINUX_09_012: [** If `close` returns a non-zero integer, `file_util_close_handle` returns false.  **]**

**SRS_FILE_UTIL_LINUX_09_013: [** If `close` returns a zero, then `file_util_close_handle` shall return true.  **]**

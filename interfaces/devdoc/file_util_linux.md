# file_util_linux

## Overview

Linux implementation of the file operation functions.

## Exposed API 

```c
// Defines removed for space constraints

typedef void *HANDLE;
typedef void *LPSECURITY_ATTRIBUTES;

MOCKABLE_FUNCTION(, HANDLE, CreateFileA, const char*, lpFileName, unsigned long, dwDesiredAccess, unsigned long, dwShareMode, LPSECURITY_ATTRIBUTES, lpSecurityAttributes, unsigned long, dwCreationDisposition, unsigned long, dwFlagsAndAttributes, HANDLE, hTemplateFile);
MOCKABLE_FUNCTION(, bool, CloseHandle, HANDLE, handle_input);
```

### CreateFileA

```c
MOCKABLE_FUNCTION(, HANDLE, CreateFileA, const char*, lpFileName, unsigned long, dwDesiredAccess, unsigned long, dwShareMode, LPSECURITY_ATTRIBUTES, lpSecurityAttributes, unsigned long, dwCreationDisposition, unsigned long, dwFlagsAndAttributes, HANDLE, hTemplateFile);
```

`CreateFileA` implements the creating file function in Linux. Uses the `open` function in C for Linux.

**SRS_FILE_UTIL_LINUX_09_001: [** If the `full_file_name` input is either empty or `NULL`, `CreateFileA` shall return an `INVALID_HANDLE_VALUE`. **]**

**SRS_FILE_UTIL_LINUX_09_002: [** `CreateFileA` shall allocate memory for the file handle and on success return a non-`NULL` value.  **]**

**SRS_FILE_UTIL_LINUX_09_003: [** If memory allocation for `result` fails, `CreateFileA` shall return a `NULL` value. **]**

**SRS_FILE_UTIL_LINUX_09_004: [** If `dwDesiredAccess` is `GENERIC_READ`, `CreateFileA` will use 0 on `open` and shall return a file handle for read only.  **]**

**SRS_FILE_UTIL_LINUX_09_005: [** If `dwDesiredAcces` is `GENERIC_WRITE`, `CreateFileA` will use 1 on `open` and shall return a file handle for write only.  **]**

**SRS_FILE_UTIL_LINUX_09_006: [** If `dwDesiredAccess` is `GENERIC_ALL` or `GENERIC_READ&GENERIC_WRITE`, `CreateFileA` will use 2 on `open` and shall return a file handle for read and write.  **]**

**SRS_FILE_UTIL_LINUX_09_007: [** If `open` returns a -1, `CreateFileA` shall free memory and return an `INVALID_HANDLE_VALUE`.  **]**

**SRS_FILE_UTIL_LINUX_09_008: [** If any error occurs, `CreateFileA` shall fail and return `INVALID_HANDLE_VALUE`.  **]**


### CloseHandle

```c
MOCKABLE_FUNCTION(, bool, CloseHandle, HANDLE, handle_input);
```

`CloseHandle` implements the close handle function in Linux. Uses the `close` function in C for Linux.

**SRS_FILE_UTIL_LINUX_09_009: [** If `handle_input` is `NULL`, `CloseHandle` returns false.  **]**

**SRS_FILE_UTIL_LINUX_09_011: [** ``CloseHandle` passes value to `close`, sets success to 0, and returns true.  **]**

**SRS_FILE_UTIL_LINUX_09_012: [** If `close` returns a non-zero integer, `CloseHandle` returns false.  **]**

**SRS_FILE_UTIL_LINUX_09_013: [** Otherwise `CloseHandle` will return true.  **]**




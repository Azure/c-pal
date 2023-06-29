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

**SRS_FILE_UTIL_LINUX_09_004: [** If `dwDesiredAccess` is `GENERIC_READ`, `CreateFileA` will use `O_RDONLY` on `open` and shall return a file handle for read only.  **]**

**SRS_FILE_UTIL_LINUX_09_005: [** If `dwDesiredAcces` is `GENERIC_WRITE`, `CreateFileA` will use `O_WRONLY` on `open` and shall return a file handle for write only.  **]**

**SRS_FILE_UTIL_LINUX_09_006: [** If `dwDesiredAccess` is `GENERIC_ALL` or `GENERIC_READ&GENERIC_WRITE`, `CreateFileA` will use `O_RDWR` on `open` and shall return a file handle for read and write.  **]**

**SRS_FILE_UTIL_LINUX_09_014: [** If `creation_disposition` is `CREATE_ALWAYS` or `OPEN_ALWAYS`, `CreateFileA` will use `O_CREAT` on `open` and shall either create a new file handle if the specificied pathname exists and return it or return an existing file handle.  **]**

**SRS_FILE_UTIL_LINUX_09_015: [** If `creation_disposition` is `CREATE_NEW`, `CreateFileA` will use `O_CREAT|O_EXCEL` on `open` and shall return a new file handle if the file doesn't already exist.  **]**

**SRS_FILE_UTIL_LINUX_09_016: [** If `creation_disposition` is `CREATE_NEW` and the file already exists, `CreateFileA` will fail and return `NULL`.  **]**

**SRS_FILE_UTIL_LINUX_09_017: [** If creation_disposition is `TRUNCATE_EXISTING`, `CreateFileA` will use `O_TRUNC` on `open` and shall return a file handle who's size has been truncated to zero bytes.  **]**

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




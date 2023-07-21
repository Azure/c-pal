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
MOCKABLE_FUNCTION(, bool, file_util_get_file_size, PLARGE_INTEGER*, file_size, HANDLE, handle_input);
MOCKABLE_FUNCTION(, bool, file_util_set_file_pointer_ex, HANDLE, handle_input, LARGE_INTEGER, distance_to_move, PLARGE_INTEGER, new_file_pointer, uint32_t, move_method);
MOCKABLE_FUNCTION(, bool, file_util_set_end_of_file, HANDLE, handle_input);
MOCKABLE_FUNCTION(, bool, file_util_write_file_ex, HANDLE, handle_input, LPCVOID, buffer, uint32_t, number_of_bytes_to_write);
```

### file_util_open_file

```c
MOCKABLE_FUNCTION(, HANDLE, file_util_open_file, const char*, full_file_name, unsigned long, desired_access, unsigned long, share_mode, LPSECURITY_ATTRIBUTES, security_attributes, unsigned long, creation_disposition, unsigned long, flags_and_attributes, HANDLE, template_file);
```

`file_util_open_file` implements the creating file function in Linux. Uses the `open` function in C for Linux.

share_mode, security_attributes, flags_and_attributes, and template_file parameters are ignored in Linux

**SRS_FILE_UTIL_LINUX_09_001: [** If the `full_file_name` input is either empty or `NULL`, `file_util_open_file` shall return an `INVALID_HANDLE_VALUE`. **]**

**SRS_FILE_UTIL_LINUX_09_002: [** `file_util_open_file` shall allocate memory for the file handle. **]**

**SRS_FILE_UTIL_LINUX_09_004: [** If `desired_access` is `GENERIC_READ`, `file_util_open_file` shall call `open` with `O_RDONLY` and shall return a file handle for read only.  **]**

**SRS_FILE_UTIL_LINUX_09_005: [** If `desired_access` is `GENERIC_WRITE`, `file_util_open_file` shall call `open` with `O_WRONLY` and shall return a file handle for write only.  **]**

**SRS_FILE_UTIL_LINUX_09_006: [** If `desired_access` is `GENERIC_ALL` or `GENERIC_READ&GENERIC_WRITE`, `file_util_open_file` shall call `open` with `O_RDWR` and shall return a file handle for read and write.  **]**

**SRS_FILE_UTIL_LINUX_09_014: [** If `creation_disposition` is `CREATE_ALWAYS` or `OPEN_ALWAYS`, `file_util_open_file` shall call `open` with `O_CREAT` and shall either create a new file handle if the specificied pathname exists and return it or return an existing file handle.  **]**

**SRS_FILE_UTIL_LINUX_09_015: [** If `creation_disposition` is `CREATE_NEW`, `file_util_open_file` shall call `open` with `O_CREAT|O_EXCL` and shall return a new file handle if the file doesn't already exist.  **]**

**SRS_FILE_UTIL_LINUX_09_016: [** If `creation_disposition` is `CREATE_NEW` and the file already exists, `file_util_open_file` shall fail and return `INVALID_HANDLE_VALUE`.  **]**

**SRS_FILE_UTIL_LINUX_09_017: [** If `creation_disposition` is `TRUNCATE_EXISTING`, `file_util_open_file` shall call `open` with `O_TRUNC` and shall return a file handle whose size has been truncated to zero bytes.  **]**

**SRS_FILE_UTIL_LINUX_09_008: [** If there are any failures, `file_util_open_file` shall fail and return `INVALID_HANDLE_VALUE`.  **]**

**SRS_FILE_UTIL_LINUX_09_020: [** `file_util_open_file` shall succeed and return a non-`NULL` value.  **]**

### file_util_close_file

```c
MOCKABLE_FUNCTION(, bool, file_util_close_file, HANDLE, handle_input);
```

`file_util_close_file` implements the close handle function in Linux. Uses the `close` function in C for Linux.

**SRS_FILE_UTIL_LINUX_09_009: [** If there are any failures, `file_util_close_file` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_021: [** If `handle_input` is `NULL`, `file_util_close_file` shall fail and return false.  **]**

**SRS_FILE_UTIL_LINUX_09_018: [** `file_util_close_file` shall succeed and return true. **]**

**SRS_FILE_UTIL_LINUX_09_019: [** `file_util_close_file` shall call `close` on the given handle_input. **]**

### file_util_get_file_size_ex

```c
MOCKABLE_FUNCTION(, bool, file_util_get_file_size, PLARGE_INTEGER*, file_size, HANDLE, handle_input);
```

`file_util_get_file_size_ex` implements the GetFileSizeEx function in Linux.

If the file handle does not have the `GENERIC_WRITE` or `GENERIC_READ` access, `file_util_get_file_size_ex` shall fail and return false.

If there are any failures, `file_util_get_file_size_ex` shall fail and return false.

`file_util_get_file_size_ex` shall succeed and return true.

`file_util_get_size_ex` shall call `stat` on `handle_input->full_file_name` and shall return the non-zero file size.

### file_util_set_file_pointer_ex

```c
MOCKABLE_FUNCTION(, bool, file_util_set_file_pointer_ex, HANDLE, handle_input, LARGE_INTEGER, distance_to_move, PLARGE_INTEGER, new_file_pointer, uint32_t, move_method);
```

If there are any failures, `file_util_set_file_pointer_ex` shall fail and return false.

If the file handle does not have the `GENERIC_WRITE` or `GENERIC_READ` access, `file_util_set_file_pointer_ex` shall fail and return false.

`file_util_set_file_pointer_ex` shall call `fopen` on `handle_input->full_file_name` and `'r'` and return a file stream.

`file_util_set_file_pointer_ex` shall call `fseek` on `out_file_stream`, `distance_to_move`, and `move_method` and return a zero when successful.

`file_util_set_file_pointer_ex` shall succeed and return true.

### file_util_set_end_of_file

```c
MOCKABLE_FUNCTION(, bool, file_util_set_end_of_file, HANDLE, handle_input);
```

If there are any failures, `file_util_set_end_of_file` shall fail and return false.

If `handle_input` does not have the `GENERIC_WRITE` access, `file_util_set_end_of_file` shall fail and return false.

`file_util_set_end_of_file` shall call `fopen on `handle_input->full_file_name` to get the file stream.

`file_util_set_end_of_file` shall call `ftell` on the input file stream and get the number of bytes to extend or truncate by.

`file_util_set_end_of_file` shall call `truncate` on `handle_input->full_file_name` and `num_bytes`.

`file_util_set_end_of_file` shall succeed and return true.

### file_util_write_file_ex

```c
MOCKABLE_FUNCTION(, bool, file_util_write_file_ex, HANDLE, handle_input, LPCVOID, buffer, uint32_t, number_of_bytes_to_write);
```

If there are any failures `file_util_write_file_ex` shall fail and return false;

`file_util_write_file_ex` shall call `write` on `handle_input->h_file`, `buffer`, and `number_of_bytes_to_write`.

`file_util_write_file_ex` shall succeed and return true;

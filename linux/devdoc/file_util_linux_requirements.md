# file_util_linux

## Overview

Linux implementation of the file operation functions.

## Exposed API 

```c
typedef struct CREATE_FILE_LINUX_TAG CREATE_FILE_LINUX;

MOCKABLE_FUNCTION(, HANDLE, file_util_open_file, const char*, full_file_name, unsigned long, desired_access, unsigned long, share_mode, LPSECURITY_ATTRIBUTES, security_attributes, unsigned long, creation_disposition, unsigned long, flags_and_attributes, HANDLE, template_file);
MOCKABLE_FUNCTION(, bool, file_util_close_file, HANDLE, handle_input);
MOCKABLE_FUNCTION(, bool, file_util_write_file, HANDLE, handle_input, LPCVOID, buffer, uint32_t, number_of_bytes_to_write, LPOVERLAPPED, overlapped);
MOCKABLE_FUNCTION(, bool, file_util_delete_file, LPCSTR, full_file_name);
MOCKABLE_FUNCTION(, PTP_IO, file_util_create_threadpool_io, HANDLE, handle_input, PTP_WIN32_IO_CALLBACK, pfnio, PVOID, pv);
MOCKABLE_FUNCTION(, PTP_CLEANUP_GROUP, file_util_create_threadpool_cleanup_group);
MOCKABLE_FUNCTION(, bool, file_util_set_file_completion_notification_modes, HANDLE, handle_input, UCHAR, flags);
MOCKABLE_FUNCTION(, HANDLE, file_util_create_event, LPSECURITY_ATTRIBUTES, lpEventAttributes, bool, bManualReset, bool, bInitialState, LPCSTR, lpName);
MOCKABLE_FUNCTION(, bool, file_util_query_performance_counter, LARGE_INTEGER*, performance_count);
MOCKABLE_FUNCTION(, void, file_util_cancel_threadpool_io, PTP_IO, pio);
MOCKABLE_FUNCTION(, bool, file_util_read_file, HANDLE, handle_in, LPVOID, buffer, DWORD, number_of_bytes_to_read, LPDWORD, number_of_bytes_read, LPOVERLAPPED, overlapped);
MOCKABLE_FUNCTION(, bool, file_util_set_file_information_by_handle, HANDLE, handle_in, FILE_INFO_BY_HANDLE_CLASS, file_info_class, LPVOID, file_info, DWORD, buffer_size);
MOCKABLE_FUNCTION(, void, file_util_close_threadpool_cleanup_group, PTP_CLEANUP_GROUP, ptpcg);
MOCKABLE_FUNCTION(, void, file_util_destroy_threadpool_environment, PTP_CALLBACK_ENVIRON, pcbe);
MOCKABLE_FUNCTION(, void, file_util_close_threadpool_io, PTP_IO, pio);
MOCKABLE_FUNCTION(, bool, file_util_get_file_size_ex, HANDLE, hfile, PLARGE_INTEGER, file_size);
MOCKABLE_FUNCTION(, bool, file_util_set_file_pointer_ex, HANDLE, hFile, LARGE_INTEGER, DistanceToMove, PLARGE_INTEGER, new_file_pointer, DWORD, move_method);
MOCKABLE_FUNCTION(, bool, file_util_set_end_of_file, HANDLE, hFile);
MOCKABLE_FUNCTION(, bool, file_util_set_file_valid_data, HANDLE, hFile, LONGLONG, valid_data_length);
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

`file_util_get_file_size_ex` implements the `GetFileSizeEx` function in Linux.

**SRS_FILE_UTIL_LINUX_09_024: [** If there are any failures, `file_util_get_file_size_ex` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_025: [** `file_util_get_size_ex` shall call `stat` on `handle_input->full_file_name` and shall return the non-zero file size. **]**

**SRS_FILE_UTIL_LINUX_09_026: [** `file_util_get_file_size_ex` shall succeed and return true. **]**

### file_util_set_file_pointer_ex

```c
MOCKABLE_FUNCTION(, bool, file_util_set_file_pointer_ex, HANDLE, handle_input, LARGE_INTEGER, distance_to_move, PLARGE_INTEGER, new_file_pointer, uint32_t, move_method);
```

`file_util_set_file_pointer_ex` implements the `SetFilePointerEx` function in Linux. Uses the `fdopen` and `fseek` in C for Linux.

**SRS_FILE_UTIL_LINUX_09_028: [** If there are any failures, `file_util_set_file_pointer_ex` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_029: [** `file_util_set_file_pointer_ex` shall call `fdopen` on `handle_input->h_file` and `mode`. **]**

**SRS_FILE_UTIL_LINUX_09_030: [** If `move_method` is `FILE_BEGIN`, `file_util_set_file_pointer_ex` shall call `fseek` with `temp_file`, `move`, and `SEEK_SET`. **]**

**SRS_FILE_UTIL_LINUX_09_031: [** If `move_method` is `FILE_CURRENT`, `file_util_set_file_pointer_ex` shall call `fseek` with `temp_file`, `move`, and `SEEK_CUR`. **]**

**SRS_FILE_UTIL_LINUX_09_032: [** If `move_method` is `FILE_END`, `file_util_set_file_pointer_ex` shall call `fseek` with `temp_file`, `move`, and `SEEK_END`. **]**

**SRS_FILE_UTIL_LINUX_09_033: [** `file_util_set_file_pointer_ex` shall succeed and return true. **]**

### file_util_set_end_of_file

```c
MOCKABLE_FUNCTION(, bool, file_util_set_end_of_file, HANDLE, handle_input);
```

`file_util_set_end_of_file` implements the `SetEndOfFile` function in Linux. Uses the `fopen`, `ftell`, and `truncate` functions in C for Linux.

**SRS_FILE_UTIL_LINUX_09_035: [** If there are any failures, `file_util_set_end_of_file` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_036: [** If `handle_input` does not have the `GENERIC_WRITE` access, `file_util_set_end_of_file` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_037: [** `file_util_set_end_of_file` shall call `fdopen on `handle_input->full_file_name` to get the file stream. **]**

**SRS_FILE_UTIL_LINUX_09_038: [** `file_util_set_end_of_file` shall call `ftell` on the input file stream and get the number of bytes to extend or truncate by. **]**

**SRS_FILE_UTIL_LINUX_09_039: [** `file_util_set_end_of_file` shall call `truncate` on `handle_input->full_file_name` and `num_bytes`. **]**

**SRS_FILE_UTIL_LINUX_09_040: [** `file_util_set_end_of_file` shall succeed and return true. **]**

### file_util_write_file

```c
MOCKABLE_FUNCTION(, bool, file_util_write_file, HANDLE, handle_input, LPCVOID, buffer, uint32_t, number_of_bytes_to_write);
```

`file_util_write_file` implements the `WriteFile` function in Linux. Uses the `write` function in C for Linux.

**SRS_FILE_UTIL_LINUX_09_042: [** If there are any failures `file_util_write_file` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_044: [** If overlapped is `NULL`, `file_util_write_file` shall call `file_util_work_function_return` on `tp_input`. **]**

**SRS_FILE_UTIL_LINUX_09_045: [** If overlapped is not `NULL`, `file_util_write_file` shall call `threadpool_schedule_work` on `handle_input->threadpool`, `file_util_work_function`, and `tp_input`. **]**

**SRS_FILE_UTIL_LINUX_09_046: [** `file_util_write_file` shall call `write` on `handle_input->h_file`, `buffer`, and `number_of_bytes_to_write`. **]**

**SRS_FILE_UTIL_LINUX_09_047: [** `file_util_write_file` shall succeed and return true. **]**

```c
MOCKABLE_FUNCTION(, HANDLE, file_util_create_event, LPSECURITY_ATTRIBUTES, lpEventAttributes, bool, bManualReset, bool, bInitialState, LPCSTR lpName);
```

`file_util_create_event` implements the `CreateEvent` funtion in Linux.

**SRS_FILE_UTIL_LINUX_09_049: [** `file_util_create_event` returns a random handle value. **]**

```c
MOCKABLE_FUNCTION(, bool, file_util_delete_file, LPCSTR, full_file_name);
```

`file_util_delete_file` implements the `DeleteFile` function for Linux. Uses `unlink` from Linux.

**SRS_FILE_UTIL_LINUX_09_051: [** If there are any failures, `file_util_delete_file` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_052: [** `file_util_delete_file` shall call `unlink` on `full_file_name`. **]**

**SRS_FILE_UTIL_LINUX_09_053: [** `file_util_delete_file` shall succeed and return true. **]**

```c
MOCKABLE_FUNCTION(PTP_IO, file_util_create_threadpool_io, HANDLE, handle_in, PTP_WIN32_IO_CALLBACK, pfnio_in, PVOID, pv);
```

`file_util_create_threadpool_io` implements the `CreateThreadpoolIo` function for Linux.

**SRS_FILE_UTIL_LINUX_09_055: [** If there are any failures, `file_util_create_threadpool_io` shall fail and return `Null`. **]**

**SRS_FILE_UTIL_LINUX_09_056: [** `file_util_create_threadpool_io` shall succeed and return a non-`Null` value. **]**

```c
MOCKABLE_FUNCTION(bool, file_util_set_file_completion_notification_modes, HANDLE, handle_input, UCHAR, flags);
```

**SRS_FILE_UTIL_LINUX_09_057: [** `file_util_set_file_completion_notification_modes` shall create a CREATE_FILE_LINUX object and return true. **]**

```c
MOCKABLE_FUNCTION(bool, file_util_read_file, HANDLE, handle_in, LPVOID, buffer, DWORD, number_of_bytes_to_read, LPDWORD, number_of_bytes_read, LPOVERLAPPED, overlapped);
```

`file_util_read_file` shall implement `ReadFile` for Linux. Uses the `read` function in Linux.

**SRS_FILE_UTIL_LINUX_09_059: [** If there are any failures, `file_util_read_file` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_060: [** `file_util_read_file` shall call `read` on `handle_input->h_file`, `buffer`, and `number_of_bytes_to_read`. **]**

```c
MOCKABLE_FUNCTION(bool, file_util_set_information_by_handle, HANDLE, handle_in, FILE_INFO_BY_HANDLE_CLASS, file_info_class, LPVOID, file_info, DWORD, buffer_size)
```

`file_util_set_information_by_handle` implements the `SetInformationByHandle` function for Linux. Uses the `rename` function in Linux.

**SRS_FILE_UTIL_LINUX_09_062: [** `file_util_set_information_by_handle` shall call `rename` on `handle_input->full_file_name` and `rename_info->FileName`. **]**

**SRS_FILE_UTIL_LINUX_09_063: [** If there are any failures, `file_util_set_information_by_handle` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_064: [** `file_util_set_information_by_handle` shall succeed and return true. **]**

```c
MOCKABLE_FUNCTION(bool, file_util_set_end_of_file, HANDLE, hfile);
```

`file_util_set_end_of_file` shall implement the `SetEndOfFile` for Linux. Uses the `fdopen`, `ftell`, and `truncate` functions from Linux.

**SRS_FILE_UTIL_LINUX_09_066: [** If there are any failures, `file_util_set_end_of_file` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_067: [** `file_util_set_end_of_file` shall call `fdopen` on `handle_input->h_file`, and `mode`. **]**

**SRS_FILE_UTIL_LINUX_09_068: [** `file_util_set_end_of_file` shall set the pointer position by calling `ftell` on `temp_file`. **]**

**SRS_FILE_UTIL_LINUX_09_069: [** `file_util_set_end_of_file` shall call `truncate` on `handle_input->full_file_name` and `file_pointer_pos`. **]**

**SRS_FILE_UTIL_LINUX_09_070: [** `file_util_set_end_of_file` shall succeed and return true. **]**

```c
MOCKABLE_FUNCTION(bool, file_util_set_file_valid_data, HANDLE, hfile, LONGLONG, valid_data_length);
```

`file_util_set_file_valid_data` implements the `SetFileValidData` for Linux. Uses the `truncate` function from Linux.

**SRS_FILE_UTIL_LINUX_09_072: [** If there are any failures, `file_util_set_file_valid_data` shall fail and return false. **]**

**SRS_FILE_UTIL_LINUX_09_073: [** `file_util_set_file_valid_data` shall call `truncate` on `handle_input->full_file_name` and `valid_data_length`. **]**

**SRS_FILE_UTIL_LINUX_09_074: [** `file_util_set_file_valid_data` shall succeed and return true. **]**

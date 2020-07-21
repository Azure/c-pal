# file
================

## Overview

The `file` module provides a platform-independent API for asynchronous file operations.

-`file_create`: returns a file handle for the given file name.
-`file_destroy`: closes the given file handle.
-`file_write_async`: enqueues an asynchronous write request for a file at a given position.
-`file_read_async`: enqueues an asynchronous read request for a file at a given position and size.
-`file_extend`: expands the given file to be of desired size.

## Exposed API

```c
#define FILE_WRITE_ASYNC_VALUES \
    FILE_WRITE_ASYNC_INVALID_ARGS, \
    FILE_WRITE_ASYNC_WRITE_ERROR, \
    FILE_WRITE_ASYNC_ERROR,\
    FILE_WRITE_ASYNC_OK
MU_DEFINE_ENUM(FILE_WRITE_ASYNC_RESULT, FILE_WRITE_ASYNC_VALUES);

#define FILE_READ_ASYNC_VALUES \
    FILE_READ_ASYNC_INVALID_ARGS, \
    FILE_READ_ASYNC_READ_ERROR, \
    FILE_READ_ASYNC_ERROR,\
    FILE_READ_ASYNC_OK
MU_DEFINE_ENUM(FILE_READ_ASYNC_RESULT, FILE_READ_ASYNC_VALUES);

typedef struct FILE_HANDLE_DATA_TAG* FILE_HANDLE;
typedef void(*FILE_REPORT_FAULT)(void* user_report_fault_context, const char* information);

typedef void(*FILE_CB)(void* user_context, bool is_successful);

MOCKABLE_FUNCTION(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context);
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);

MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, const unsigned char*, source, uint32_t, size, uint64_t, position, FILE_CB, user_callback, void*, user_context)(FILE_WRITE_ASYNC_OK, FILE_WRITE_ASYNC_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, unsigned char*, destination, uint32_t, size, uint64_t, position, FILE_CB, user_callback, void*, user_context)(FILE_READ_ASYNC_OK, FILE_READ_ASYNC_ERROR);

MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend, FILE_HANDLE, handle, uint64_t, desired_size)(0, MU_FAILURE);
```

## file_create

```c
MOCKABLE_FUNCTION(, FILE_HANDLE, file_create,FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name);
```

If a file by the name `full_file_name` does not exist, `file_create` creates a file by that name, opens it and returns its handle. If the file does exist, `file_create` opens the file and returns its handle. `file_create` shall register `user_report_fault_callback` with argument `user_report_fault_context` as the callback function to be called when the callback specified by the user for a specific asynchronous operation cannot be called.

**SRS_FILE_43_033: [** If `execution_engine` is `NULL`, `file_create` shall fail and return `NULL`. **]**

**SRS_FILE_43_002: [** If `full_file_name` is `NULL` then `file_create` shall fail and return `NULL`. **]**

**SRS_FILE_43_037: [** If `full_file_name` is an empty string, `file_create` shall fail and return `NULL`. **]**

**SRS_FILE_43_003: [** If a file with name `full_file_name` does not exist, `file_create` shall create a file with that name.**]**

**SRS_FILE_43_001: [** `file_create` shall open the file named `full_file_name` for asynchronous operations and return its handle. **]**

**SRS_FILE_43_034: [** If there are any failures, `file_create` shall fail and return `NULL`. **]**

## file_destroy 

```c
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);
```

`file_destroy` closes the given file handle.

**SRS_FILE_43_005: [** If `handle` is `NULL`, `file_destroy` shall return. **]**

**SRS_FILE_43_006: [** `file_destroy` shall wait for all pending I/O operations to complete. **]**

**SRS_FILE_43_007: [** `file_destroy` shall close the file handle `handle`. **]**

## file_write_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, const unsigned char*, source, uint32_t, size, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(FILE_WRITE_ASYNC_OK, FILE_WRITE_ASYNC_ERROR);
```

`file_write_async` issues an asynchronous write request. If the write goes beyond the size of the file, the file grows to accomodate the write. On linux, the grow-and-write is atomic. On Windows, ...(waiting for response from Windows 32-bit Sys Prgrmmg Questions).

**SRS_FILE_43_009: [** If `handle` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_010: [** If `source` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_012: [** If `user_callback` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_040: [** If `position` + `size` is greater than `INT64_MAX`, then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_042: [** If `size` is 0 then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_014: [** `file_write_async` shall enqueue a write request to write `source`'s content to the `position` offset in the file. **]**

**SRS_FILE_43_041: [** If `position` + `size` is greater than the size of the file and the call to write is successfull, `file_write_async` shall grow the file to accomodate the write. **]**

**SRS_FILE_43_035: [** If the call to write the file fails, `file_write_async` shall fail and return `FILE_WRITE_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_43_008: [** `file_write_async` shall call `user_call_back` passing `user_context` and `success` depending on the success of the asynchronous write operation.**]**

**SRS_FILE_43_015: [** If there are any failures, `file_write_async` shall fail and return `FILE_WRITE_ASYNC_ERROR`. **]**

**SRS_FILE_43_030: [** `file_write_async` shall succeed and return `FILE_WRITE_ASYNC_OK`. **]**

## file_read_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint64_t, position, unsigned char*, destination, uint32_t, size,FILE_READ_CB, user_callback, void*, user_context)(FILE_READ_ASYNC_OK, FILE_READ_ASYNC_ERROR);
```

`file_read_async` issues an asynchronous read request.

**SRS_FILE_43_017: [** If `handle` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_032: [** If `destination` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_020: [** If `user_callback` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_043: [** If `size` is 0 then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_021: [** `file_read_async` shall enqueue a read request to read `handle`'s content at `position` offset and write it to `destination`. **]**

**SRS_FILE_43_036: [** If the call to read the file fails, `file_read_async` shall fail and return `FILE_READ_ASYNC_READ_ERROR`. **]**

**SRS_FILE_43_039: [** If `position` + `size` exceeds the size of the file, `user_callback` shall be called with `success` as `false`. **]**

**SRS_FILE_43_016: [** `file_read_async` shall call `user_callback` passing `user_context` and `success` depending on the success of the asynchronous read operation.**]**

**SRS_FILE_43_022: [** If there are any failures then `file_read_async` shall fail and return `FILE_READ_ASYNC_ERROR`. **]**

**SRS_FILE_43_031: [** `file_read_async` shall succeed and return `FILE_READ_ASYNC_OK`. **]**

## file_extend

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend, FILE_HANDLE, handle, uint64_t, desired_size)(0, MU_FAILURE);
```

`file_extend` grows the file to `desired_size`. The contents of the file between the old end and new end are not defined.

**SRS_FILE_43_024: [** If `handle` is `NULL`, `file_extend` shall fail and return a non-zero value. **]**

**SRS_FILE_43_025: [** If `desired_size` is greater than `INT64_MAX`, `file_extend` shall fail and return a non-zero value. **]**

**SRS_FILE_43_026: [** If `desired_size` is less than the current size of the file, `file_extend` shall fail and return a non-zero value. **]**

**SRS_FILE_43_027: [** `file_extend` shall set the valid file data size of the given file to `desired_size`. **]**

**SRS_FILE_43_028: [** If there are any failures, `file_extend` shall return a non-zero value. **]**

**SRS_FILE_43_029: [** If there are no failures, `file_extend` will return 0. **]**
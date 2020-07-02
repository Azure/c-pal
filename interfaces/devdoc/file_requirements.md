# file
================

## Overview

The `file` module provides a platform-independent API for asynchronous file operations.

-`file_create`: returns a file handle for the given file name.
-`file_destroy`: closes the given file handle.
-`file_write_async`: enqueues an asynchronous write request for a file at a given position.
-`file_read_async`: enqueues an asynchronous read request for a file at a given position and size.
-`file_extend_filesize`: expands the given file to be of desired size.

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

typedef void(*FILE_WRITE_CB)(void* user_context, bool is_successful);
typedef void(*FILE_READ_CB)(void* user_context, bool is_successful, CONSTBUFFER_HANDLE content);
MOCKABLE_FUNCTION(, FILE_HANDLE, file_create,FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name);
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint32_t, size, uint64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend_filesize, FILE_HANDLE, handle, uint64_t, desired_size, bool, has_manage_volume)(0, MU_FAILURE);
```

## file_create

```c
MOCKABLE_FUNCTION(, FILE_HANDLE, file_create,FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name);
```

`file_create` creates a file by the name of `full_file_name` it doesn't exist. `file_create` opens a file by the name `full_file_name` and returns its file handle.

**SRS_FILE_43_033: [** If `execution_engine` is `NULL`, `file_create` shall fail and return `NULL`. **]**

**SRS_FILE_43_002: [** If `full_file_name` is `NULL` then `file_create` shall fail and return `NULL`. **]**

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
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

`file_write_async` issues an asynchronous write request.

**SRS_FILE_43_009: [** If `handle` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_010: [** If `source` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_012: [** If `user_callback` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_014: [** `file_write_async` shall enqueue a write request to write `source`'s content to the `position` offset in the file. **]**

**SRS_FILE_43_035: [** If the call to write the file fails, `file_write_async` shall fail and return `FILE_WRITE_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_43_008: [** `file_write_async` shall call `user_call_back` passing `user_context` and `success` depending on the success of the asynchronous write operation.**]**

**SRS_FILE_43_015: [** If there are any failures, `file_write_async` shall fail and return `FILE_WRITE_ASYNC_ERROR`. **]**

**SRS_FILE_43_030: [** `file_write_async` shall succeed and return `FILE_WRITE_ASYNC_OK`. **]**

## file_read_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint32_t, size, uint64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

`file_read_async` issues an asynchronous read request.

**SRS_FILE_43_017: [** If `handle` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_020: [** If `user_callback` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_43_032: [** `file_read_async` shall allocate `size` bytes named `destination` for reading the data. **]**

**SRS_FILE_43_021: [** `file_read_async` shall enqueue a read request to read `handle`'s content at `position` offset and write it to `destination`. **]**

**SRS_FILE_43_036: [** If the call to read the file fails, `file_read_async` shall fail and return `FILE_READ_ASYNC_READ_ERROR`. **]**

**SRS_FILE_43_016: [** `file_read_async` shall call `user_call_back` passing `user_context` and `success` depending on the success of the asynchronous write operation.**]**

**SRS_FILE_43_022: [** If there are any failures then `file_read_async` shall fail and return `FILE_READ_ASYNC_ERROR`. **]**

**SRS_FILE_43_031: [** `file_read_async` shall succeed and return `FILE_READ_ASYNC_OK`. **]**

## file_extend_filesize

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend_filesize, FILE_HANDLE, handle, uint64_t, desired_size, bool, has_manage_volume)(0, MU_FAILURE);
```

`file_extend_filesize` resizes the file to `desired_size`.

**SRS_FILE_43_024: [** If `handle` is `NULL`, `file_extend_filesize` shall return a non-zero value. **]**

**SRS_FILE_43_025: [** If `desired_size` is greater than `INT64_MAX`, `file_extend_filesize` shall return a non-zero value. **]**

**SRS_FILE_43_026: [** If `desired_size` is less than the current size of the file, `file_extend_filesize` shall return a non-zero value. **]**

**SRS_FILE_43_027: [** If `has_manage_volume` is `true`, `file_extend_filesize` shall set the valid file data size of the given file to `desired_size`. **]**

**SRS_FILE_43_028: [** If there are any failures, `file_extend_filesize` shall return a non-zero value. **]**

**SRS_FILE_43_029: [** If there are no failures, `file_extend_filesize` will return `0`. **]**
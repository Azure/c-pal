# file_win32
================

## Overview

Windows implementation of the `file` module.

## Exposed API

```c
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

**SRS_FILE_WIN32_43_001: [** `file_create` shall call `CreateFileA` with `full_file_name` as `lpFileName`, `GENERIC_READ|GENERIC_WRITE` as `dwDesiredAccess`, `FILE_SHARED_READ` as `dwShareMode`, `NULL` as `lpSecurityAttributes`, `OPEN_ALWAYS` as `dwCreationDisposition`, `FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH` as `dwFlagsAndAttributes` and `NULL` as `hTemplateFiles`. **]**

**SRS_FILE_WIN32_43_041: [** `file_create` shall allocate a `FILE_HANDLE`. **]**

**SRS_FILE_WIN32_43_002: [** `file_create` shall call `SetFileCompletionNotificationModes` to disable calling the completion port when an async operations finishes synchrounously. **]**

**SRS_FILE_WIN32_43_003: [** `file_create` shall initialize a threadpool environment. **]**

**SRS_FILE_WIN32_43_004: [** `file_create` shall obtain a `PTP_POOL` struct by calling `execution_engine_win32_get_threadpool` on `execution_engine`. **]**

**SRS_FILE_WIN32_43_005: [** `file_create` shall register the threadpool environment with `ptpPool` **]**

**SRS_FILE_WIN32_43_006: [** `file_create` shall create a cleanup group. **]**

**SRS_FILE_WIN32_43_007: [** `file_create` shall register the cleanup group with the threadpool environment. **]**

**SRS_FILE_WIN32_43_033: [** `file_create` shall create a threadpool io and use `onFileIoComplete` as a callback. **]**

**SRS_FILE_WIN32_43_008: [** If there are any failures, `file_create` shall return `NULL`. **]**

**SRS_FILE_WIN32_43_009: [** `file_create` shall succeed and return the file handle of the opened file. **]**

## file_destroy

```c
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);
```

**SRS_FILE_WIN32_43_011: [** `file_destroy` shall wait for all I/O to complete. **]**

**SRS_FILE_WIN32_43_012: [** `file_destroy` shall close the cleanup group. **]**

**SRS_FILE_WIN32_43_013: [** `file_destroy` shall destroy the environment. **]**

**SRS_FILE_WIN32_43_015: [** `file_destroy` shall close the threadpool IO. **]**

**SRS_FILE_WIN32_43_016: [** `file_destroy` shall call `CloseHandle` on `handle` **]**

**SRS_FILE_WIN32_43_042: [** `file_destroy` shall free the `FILE_HANDLE`. **]**


## file_write_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

**SRS_FILE_WIN32_43_017: [** `file_write_async` shall call `StartThreadpoolIo`. **]**

**SRS_FILE_WIN32_43_018: [** `file_write_async` shall allocate a context to store `handle`, write as the type of asyncronous operation, `source`, `user_callback` and `user_context. **]**

**SRS_FILE_WIN32_43_020: [** `file_write_async` shall populate the `ov` field of the `FILE_WIN32_IO` using `position`. **]**

**SRS_FILE_WIN32_43_021: [** `file_write_async` shall call `WriteFile`. **]**

**SRS_FILE_WIN32_43_022: [** If `WriteFile` fails synchronously and `GetLastError` indicates `ERROR_IO_PENDING` then `file_write_async` shall succeed and return `FILE_WRITE_ASYNC_OK`. **]**

**SRS_FILE_WIN32_43_023: [** If `WriteFile` fails synchronously and `GetLastError` does not indicate `ERROR_IO_PENDING` then `file_write_async` shall fail, call `CancelThreadpoolIo` and return `FILE_WRITE_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_WIN32_43_024: [** If `WriteFile` succeeds synchronously then `file_write_async` shall succeed, call `CancelThreadpoolIo` and `user_callback` and return `FILE_WRITE_ASYNC_OK`. **]**

## file_read_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint32_t, size, uint64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

**SRS_FILE_WIN32_43_025: [** `file_read_async` shall call `StartThreadpoolIo`. **]**

**SRS_FILE_WIN32_43_026: [** `file_read_async` shall allocate a context to store `handle`, read as the type of asynchronous operation, `user_callback` and `user_context` **]**

**SRS_FILE_WIN32_43_028: [** `file_read_async` shall populate the `ov` field of the `FILE_WIN32_IO` using `position`. **]**

**SRS_FILE_WIN32_43_029: [** `file_read_async` shall call `ReadFile`. **]**

**SRS_FILE_WIN32_43_030: [** If `ReadFile` fails synchronously and `GetLastError` indicates `ERROR_IO_PENDING` then `file_read_async` shall succeed and return `FILE_READ_ASYNC_OK`. **]**

**SRS_FILE_WIN32_43_031: [** If `ReadFile` fails synchronously and `GetLastError` does not indicate `ERROR_IO_PENDING` then `file_read_async` shall fail, call `CancelThreadpoolIo` and return `FILE_READ_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_WIN32_43_032: [** If `ReadFile` succeeds synchronously then `file_read_async` shall succeed, call `CancelThreadpoolIo` and `user_callback` and return `FILE_READ_ASYNC_OK`. **]**


## on_file_io_complete_win32

```c
static VOID CALLBACK on_file_io_complete_win32( /*called when some read/write operation is finished*/
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_opt_ PVOID                 Overlapped,
    _In_        ULONG                 IoResult,
    _In_        ULONG_PTR             NumberOfBytesTransferred,
    _Inout_     PTP_IO                Io
);
```
`on_file_io_complete_win32` is called when a file operation completes asynchronously.


**SRS_FILE_WIN32_43_034: [** `on_file_io_complete_win32` shall recover the context containing `Overlapped`. **]**

**SRS_FILE_WIN32_43_035: [** If `IoResult` is not `NO_ERROR`, `on_file_io_complete_win32` shall call `user_report_fault_callback` with `user_report_fault_context` which were specified when `file_create` was called. **]**

**SRS_FILE_WIN32_43_036: [** If the type of the asynchronous operation is read, `on_file_io_complete_win32` shall  construct a `CONSTBUFFER_HANDLE` by calling `CONSTBUFFER_CreateWithMoveMemory` from the bytes read by `ReadFile`. **]**

**SRS_FILE_WIN32_43_037: [** If the construction of the `CONSTBUFFER_HANDLE` fails, `on_file_io_complete_win32` shall call `user_callback` with `user_context`, `false` as `is_successful` and `NULL` as `content`. **]**

**SRS_FILE_WIN32_43_038: [** If the construction of the `CONSTBUFFER_HANDLE` succeeds, `on_file_io_complete_win32` shall call `user_callback` with `user_context`, `true` as `is_successful` and the created `CONSTBUFFER_HANDLE` as `content`. **]**

**SRS_FILE_WIN32_43_039: [** If the type of the asynchronous operation is write, `on_file_io_complete_win32` shall call `user_callback` with `user_context` and `true` as `is_successful`. **]**

**SRS_FILE_WIN32_43_040: [** If the type of asynchronous operation is neither read nor write, `on_file_io_complete_win32` shall call `user_report_fault_callback` with `user_report_fault_context` which were specified when `file_create` was called. **]**
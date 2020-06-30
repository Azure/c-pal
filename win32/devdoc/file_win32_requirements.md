# file_win32
================

## Overview

Windows implementation of the `file` module.

## Exposed API

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, int64_t, desired_file_size, bool, has_manage_volume)(0, MU_FAILURE);
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, int64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint32_t, size, int64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend_filesize, FILE_HANDLE, handle, uint64_t, desired_size, bool, has_manage_volume)(0, MU_FAILURE);
```

## file_create

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, int64_t, desired_file_size, bool, has_manage_volume)(0, MU_FAILURE);
```

**SRS_FILE_WIN32_43_001: [** `file_create` shall call `CreatFileA` with `full_file_name` as `lpFileName`, `GENERIC_READ|GENERIC_WRITE` as `dwDesiredAccess`, `FILE_SHARED_READ` as `dwShareMode`, `NULL` as `lpSecurityAttributes`, `OPEN_ALWAYS` as `dwCreationDisposition`, `FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH` as `dwFlagsAndAttributes` and `NULL` as `hTemplateFiles`. **]**

**SRS_FILE_WIN32_43_002: [** `file_create` shall call `SetFileCompletionNotificationModes` to disable calling the completion port when an async operations finishes synchrounously. **]**

**SRS_FILE_WIN32_43_003: [** `file_create` shall initialize a threadpool environment. **]**

**SRS_FILE_WIN32_43_004: [** `file_create` shall obtain a `PTP_POOL` struct by calling `execution_engine_win32_get_threadpool` on `execution_engine`. **]**

**SRS_FILE_WIN32_43_005: [** `file_create` shall register the threadpool environment with `ptpPool` **]**

**SRS_FILE_WIN32_43_006: [** `file_create` shall create a cleanup group. **]**

**SRS_FILE_WIN32_43_007: [** `file_create` shall register the cleanup group with the threadpool environment. **]**

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

## file_write_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, int64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

**SRS_FILE_WIN32_43_017: [** `file_write_async` shall call `StartThreadpoolIo`. **]**

**SRS_FILE_WIN32_43_018: [** `file_write_async` shall create a `FILE_WIN32_IO` struct with `handle` as `handle`, `FILE_ASYNC_WRITE` as `type`. **]**

**SRS_FILE_WIN32_43_019: [** `file_write_async` shall populate the `data` field of the `FILE_WIN32_IO` struct using `source`, `user_callback` and `user_context`. **]**

**SRS_FILE_WIN32_43_020: [** `file_write_async` shall populate the `ov` field of the `FILE_WIN32_IO` using `position`. **]**

**SRS_FILE_WIN32_43_021: [** `file_write_async` shall call `WriteFile`. **]**

**SRS_FILE_WIN32_43_022: [** If `WriteFile` fails synchronously and `GetLastError` indicates `ERROR_IO_PENDING` then `file_write_async` shall succeed and return `FILE_WRITE_ASYNC_OK`. **]**

**SRS_FILE_WIN32_43_023: [** If `WriteFile` fails synchronously and `GetLastError` does not indicate `ERROR_IO_PENDING` then `file_write_async` shall fail, call `CancelThreadpoolIo` and return `FILE_WRITE_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_WIN32_43_024: [** If `WriteFile` succeeds synchronously then `file_write_async` shall succeed, call `CancelThreadpoolIo` and `user_callback` and return `FILE_WRITE_ASYNC_OK`. **]**

## file_read_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint32_t, size, int64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

**SRS_FILE_WIN32_43_025: [** `file_read_async` shall call `StartThreadpoolIo`. **]**

**SRS_FILE_WIN32_43_026: [** `file_read_async` shall create a `FILE_WIN32_IO` struct with `handle` as `handle`, `FILE_ASYNC_READ` as `type`. **]**

**SRS_FILE_WIN32_43_027: [** `file_read_async` shall populate the `data` field of the `FILE_WIN32_IO` struct using `user_callback` and `user_context`. **]**

**SRS_FILE_WIN32_43_028: [** `file_read_async` shall populate the `ov` field of the `FILE_WIN32_IO` using `position`. **]**

**SRS_FILE_WIN32_43_029: [** `file_read_async` shall call `ReadFile`. **]**

**SRS_FILE_WIN32_43_030: [** If `ReadFile` fails synchronously and `GetLastError` indicates `ERROR_IO_PENDING` then `file_read_async` shall succeed and return `FILE_READ_ASYNC_OK`. **]**

**SRS_FILE_WIN32_43_031: [** If `ReadFile` fails synchronously and `GetLastError` does not indicate `ERROR_IO_PENDING` then `file_read_async` shall fail, call `CancelThreadpoolIo` and return `FILE_READ_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_WIN32_43_032: [** If `ReadFile` succeeds synchronously then `file_read_async` shall succeed, call `CancelThreadpoolIo` and `user_callback` and return `FILE_READ_ASYNC_OK`. **]**

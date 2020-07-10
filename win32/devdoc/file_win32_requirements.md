# file_win32
================

## Overview

Windows implementation of the `file` module.

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
typedef void(*FILE_READ_CB)(void* user_context, bool is_successful);

MOCKABLE_FUNCTION(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context);
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);

MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, const unsigned char*, source, uint32_t, size, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(FILE_WRITE_ASYNC_OK, FILE_WRITE_ASYNC_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint64_t, position, unsigned char*, destination, uint32_t, size,FILE_READ_CB, user_callback, void*, user_context)(FILE_READ_ASYNC_OK, FILE_READ_ASYNC_ERROR);

MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend, FILE_HANDLE, handle, uint64_t, desired_size)(0, MU_FAILURE);
```

## file_create

```c
MOCKABLE_FUNCTION(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context);
```

**SRS_FILE_WIN32_43_040: [** If `execution_engine` is `NULL`, `file_create` shall fail and return `NULL`. **]**

**SRS_FILE_WIN32_43_048: [** If `full_file_name` is `NULL` then `file_create` shall fail and return `NULL`. **]**

**SRS_FILE_WIN32_43_041: [** `file_create` shall allocate a `FILE_HANDLE`. **]**

**SRS_FILE_WIN32_43_001: [** `file_create` shall call `CreateFileA` with `full_file_name` as `lpFileName`, `GENERIC_READ|GENERIC_WRITE` as `dwDesiredAccess`, `FILE_SHARED_READ` as `dwShareMode`, `NULL` as `lpSecurityAttributes`, `OPEN_ALWAYS` as `dwCreationDisposition`, `FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH` as `dwFlagsAndAttributes` and `NULL` as `hTemplateFile`. **]**

**SRS_FILE_WIN32_43_002: [** `file_create` shall call `SetFileCompletionNotificationModes` to disable calling the completion port when an async operations finishes synchrounously. **]**

**SRS_FILE_WIN32_43_003: [** `file_create` shall allocate a threadpool environment. **]**

**SRS_FILE_WIN32_43_004: [** `file_create` shall obtain a `PTP_POOL` struct by calling `execution_engine_win32_get_threadpool` on `execution_engine`. **]**

**SRS_FILE_WIN32_43_005: [** `file_create` shall register the threadpool environment with `ptpPool` **]**

**SRS_FILE_WIN32_43_006: [** `file_create` shall create a cleanup group. **]**

**SRS_FILE_WIN32_43_007: [** `file_create` shall register the cleanup group with the threadpool environment. **]**

**SRS_FILE_WIN32_43_033: [** `file_create` shall create a threadpool io with the allocated `FILE_HANDLE` and `on_file_io_complete_win32` as a callback. **]**

**SRS_FILE_WIN32_43_008: [** If there are any failures, `file_create` shall return `NULL`. **]**

**SRS_FILE_WIN32_43_009: [** `file_create` shall succeed and return a non-`NULL` value. **]**

## file_destroy

```c
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);
```

**SRS_FILE_WIN32_43_049: [** If `handle` is `NULL`, `file_destroy` shall return. **]**

**SRS_FILE_WIN32_43_011: [** `file_destroy` shall wait for all I/O to complete. **]**

**SRS_FILE_WIN32_43_012: [** `file_destroy` shall close the cleanup group. **]**

**SRS_FILE_WIN32_43_013: [** `file_destroy` shall destroy the environment. **]**

**SRS_FILE_WIN32_43_016: [** `file_destroy` shall call `CloseHandle` on `handle` **]**

**SRS_FILE_WIN32_43_015: [** `file_destroy` shall close the threadpool IO. **]**

**SRS_FILE_WIN32_43_042: [** `file_destroy` shall free the `FILE_HANDLE`. **]**


## file_write_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, const unsigned char*, source, uint32_t, size, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(FILE_WRITE_ASYNC_OK, FILE_WRITE_ASYNC_ERROR);
```

**SRS_FILE_WIN32_43_043: [** If `handle` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_WIN32_43_044: [** If `source` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_WIN32_43_045: [** If `user_callback` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_WIN32_43_017: [** `file_write_async` shall call `StartThreadpoolIo`. **]**

**SRS_FILE_WIN32_43_054: [** `file_write_async` shall create an event by calling `CreateEvent`. **]**

**SRS_FILE_WIN32_43_020: [** `file_write_async` shall allocate an `OVERLAPPED` struct and populate it with the created event and `position`. **]**

**SRS_FILE_WIN32_43_018: [** `file_write_async` shall allocate a context to store the allocated `OVERLAPPED` struct, `handle`, write as the type of asynchronous operation, `source`, `user_callback` and `user_context`. **]**

**SRS_FILE_WIN32_43_021: [** `file_write_async` shall call `WriteFile` with `handle`, `source`, `size` and the allocated `OVERLAPPED` struct. **]**

**SRS_FILE_WIN32_43_022: [** If `WriteFile` fails synchronously and `GetLastError` indicates `ERROR_IO_PENDING` then `file_write_async` shall succeed and return `FILE_WRITE_ASYNC_OK`. **]**

**SRS_FILE_WIN32_43_023: [** If `WriteFile` fails synchronously and `GetLastError` does not indicate `ERROR_IO_PENDING` then `file_write_async` shall fail, call `CancelThreadpoolIo` and return `FILE_WRITE_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_WIN32_43_024: [** If `WriteFile` succeeds synchronously then `file_write_async` shall succeed, call `CancelThreadpoolIo`, call `user_callback` and return `FILE_WRITE_ASYNC_OK`. **]**

## file_read_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint64_t, position, unsigned char*, destination, uint32_t, size,FILE_READ_CB, user_callback, void*, user_context)(FILE_READ_ASYNC_OK, FILE_READ_ASYNC_ERROR);
```

**SRS_FILE_WIN32_43_046: [** If `handle` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_WIN32_43_051: [** If `destination` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_WIN32_43_047: [** If `user_callback` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**SRS_FILE_WIN32_43_025: [** `file_read_async` shall call `StartThreadpoolIo`. **]**

**SRS_FILE_WIN32_43_055: [** `file_write_async` shall create an event by calling `CreateEvent`. **]**

**SRS_FILE_WIN32_43_028: [** `file_read_async` shall allocate an `OVERLAPPED` struct and populate it with the created event and `position`. **]**

**SRS_FILE_WIN32_43_026: [** `file_read_async` shall allocate a context to store the allocated `OVERLAPPED` struct, `destination`, `handle`, read as the type of asynchronous operation, `user_callback` and `user_context` **]**

**SRS_FILE_WIN32_43_029: [** `file_read_async` shall call `ReadFile` with `handle`, `destination`, `size` and the allocated `OVERLAPPED` struct. **]**

**SRS_FILE_WIN32_43_030: [** If `ReadFile` fails synchronously and `GetLastError` indicates `ERROR_IO_PENDING` then `file_read_async` shall succeed and return `FILE_READ_ASYNC_OK`. **]**

**SRS_FILE_WIN32_43_031: [** If `ReadFile` fails synchronously and `GetLastError` does not indicate `ERROR_IO_PENDING` then `file_read_async` shall fail, call `CancelThreadpoolIo` and return `FILE_READ_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_WIN32_43_032: [** If `ReadFile` succeeds synchronously then `file_read_async` shall succeed, call `CancelThreadpoolIo`, call `user_callback` and return `FILE_READ_ASYNC_OK`. **]**

## file_extend

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend, FILE_HANDLE, handle, uint64_t, desired_size)(0, MU_FAILURE);
```

**SRS_FILE_WIN32_43_050: [** `file_extend` shall return `0`. **]**

## on_file_io_complete_win32

```c
static void on_file_io_complete_win32(
    PTP_CALLBACK_INSTANCE instance,
    PVOID context,
    PVOID overlapped,
    ULONG io_result,
    ULONG_PTR number_of_bytes_transferred,
    PTP_IO io
);
```
`on_file_io_complete_win32` is called when a file operation completes asynchronously.

**SRS_FILE_WIN32_43_057: [** If `overlapped` is `NULL`, `on_file_io_complete_win32` shall call `user_report_fault_callback` with `user_report_fault_context`. **]**

**SRS_FILE_WIN32_43_034: [** `on_file_io_complete_win32` shall recover the context containing `overlapped`. **]**

**SRS_FILE_WIN32_43_053: [** `on_file_io_complete_win32` shall determine the type of asynchronous operation from the context. **]**

**SRS_FILE_WIN32_43_056: [** If the type of asynchronous operation is neither read nor write, `on_file_io_complete_win32` shall call `user_report_fault_callback` with `user_report_fault_context`. **]**

**SRS_FILE_WIN32_43_035: [** If `io_result` is not `NO_ERROR`, `on_file_io_complete_win32` shall call `user_callback` with `false` as `is_successful`. **]**

**SRS_FILE_WIN32_43_036: [** If the type of the asynchronous operation is read, `on_file_io_complete_win32` shall call `user_callback` with `user_context` and `true` as `is_successful`. **]**

**SRS_FILE_WIN32_43_039: [** If the type of the asynchronous operation is write, `on_file_io_complete_win32` shall call `user_callback` with `user_context` and `true` as `is_successful`. **]**
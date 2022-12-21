# file_linux

## Overview

Linux implementation of the `file` module.

-`file_create` uses [`open`](https://www.man7.org/linux/man-pages/man2/open.2.html).
-`file_destroy` uses [`close`](https://www.man7.org/linux/man-pages/man2/close.2.html).
-`file_write_async` and `file_read_async` create [`sigevent`](https://man7.org/linux/man-pages/man7/sigevent.7.html)s to specify callbacks.
-`file_write_async` uses [`aio_write`](https://man7.org/linux/man-pages/man3/aio_write.3.html).
-`file_read_async` uses [`aio_read`](https://man7.org/linux/man-pages/man3/aio_read.3.html).
-`file_extend` uses [`ftruncate`](https://www.man7.org/linux/man-pages/man3/ftruncate.3p.html)

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
MOCKABLE_FUNCTION(, FILE_HANDLE, file_create, EXECUTION_ENGINE_HANDLE, execution_engine, const char*, full_file_name, FILE_REPORT_FAULT, user_report_fault_callback, void*, user_report_fault_context);
```

**NON_FILE_LINUX_43_037: [** If `execution_engine` is `NULL`, `file_create` shall fail and return `NULL`. **]**

**NON_FILE_LINUX_43_038: [** If `full_file_name` is `NULL` then `file_create` shall fail and return `NULL`. **]**

**NON_FILE_LINUX_43_050: [** If full_file_name is an empty string, `file_create` shall fail and return `NULL`.  **]**

**NON_FILE_LINUX_43_029: [** `file_create` shall allocate a `FILE_HANDLE`. **]**

**NON_FILE_LINUX_43_001: [** `file_create` shall call `open` with `full_file_name` as `pathname` and flags `O_CREAT`, `O_RDWR`, `O_DIRECT` and `O_LARGEFILE`. **]**

**NON_FILE_LINUX_43_002: [** `file_create` shall return the file handle returned by the call to `open`.**]**

## file_destroy

```c
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);
```

**NON_FILE_LINUX_43_036: [** If `handle` is `NULL`, `file_destroy` shall return. **]**

**NON_FILE_LINUX_43_003: [** `file_destroy` shall call `close` with `fd` as `handle`.**]**

**NON_FILE_LINUX_43_030: [** `file_destroy` shall free the `FILE_HANDLE`. **]**

## file_write_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, const unsigned char*, source, uint32_t, size, uint64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(FILE_WRITE_ASYNC_OK, FILE_WRITE_ASYNC_ERROR);
```

**NON_FILE_LINUX_43_031: [** If `handle` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_032: [** If `source` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_051: [** If `position` + `size` is greater than `INT64_MAX`, then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_049: [** If `user_callback` is `NULL` then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_048: [** If `size` is 0 then `file_write_async` shall fail and return `FILE_WRITE_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_019: [** `file_write_async` shall allocate a struct to hold `handle`, `source`, `size`, `user_callback` and `user_context`. **]**

**NON_FILE_LINUX_43_004: [** `file_write_async` shall allocate a `sigevent` struct with `SIGEV_THREAD` as `sigev_notify`, the allocated struct as `sigev_value`, `on_file_write_complete_linux` as `sigev_notify_function`, `NULL` as `sigev_notify_attributes`. **]**

**NON_FILE_LINUX_43_005: [** `file_write_async` shall allocate an `aiocb` struct with `handle` as `aio_fildes`, `position` as `aio_offset`, source as `aio_buf`, `size` as `aio_nbytes`, and the allocated `sigevent` struct as `aio_sigevent`. **]**

**NON_FILE_LINUX_43_006: [** `file_write_async` shall call `aio_write` with the allocated `aiocb` struct as `aiocbp`.**]**

**NON_FILE_LINUX_43_012: [** If `aio_write` fails, `file_write_async` shall return `FILE_WRITE_ASYNC_WRITE_ERROR`. **]**

**NON_FILE_LINUX_43_007: [** If `aio_write` succeeds, `file_write_async` shall return `FILE_WRITE_ASYNC_OK`. **]**

**NON_FILE_LINUX_43_013: [** If there are any other failures, `file_write_async` shall return `FILE_WRITE_ASYNC_ERROR`. **]**

## file_read_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, unsigned char*, destination, uint32_t, size, uint64_t, position, FILE_CB, user_callback, void*, user_context)(FILE_READ_ASYNC_OK, FILE_READ_ASYNC_ERROR);
```

**NON_FILE_LINUX_43_034: [** If `handle` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_043: [** `If `destination` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_035: [** If `user_callback` is `NULL` then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_052: [** If `size` is 0 then `file_read_async` shall fail and return `FILE_READ_ASYNC_INVALID_ARGS`. **]**

**NON_FILE_LINUX_43_045: [** `file_read_async` shall allocate a struct to hold `handle`, `destination`, `user_callback` and `user_context`. **]**

**NON_FILE_LINUX_43_008: [** `file_read_async` shall allocate a `sigevent` struct with `SIGEV_THREAD` as `sigev_notify`, `user_context` as `sigev_value`, `on_file_read_complete_linux` as `sigev_notify_function`, `NULL` as `sigev_notify_attributes`. **]**

**NON_FILE_LINUX_43_009: [** `file_read_async` shall allocate an `aiocb` struct with `handle` as `aio_fildes`, `position` as `aio_offset`, the allocated buffer as `aio_buf`, `size` as `aio_nbytes`, and the allocated `sigevent` struct as `aio_sigevent`. **]**

**NON_FILE_LINUX_43_010: [** `file_read_async` shall call `aio_read` with the allocated `aiocb` struct as `aiocbp`.  **]**

**NON_FILE_LINUX_43_011: [** If `aio_read` fails, `file_read_async` shall return `FILE_READ_ASYNC_READ_ERROR`. **]**

**NON_FILE_LINUX_43_014: [** If `aio_read` succeeds, `file_read_async` shall return `FILE_READ_ASYNC_OK`. **]**

**NON_FILE_LINUX_43_015: [** If there are any failures, `file_read_async` shall return `FILE_READ_ASYNC_ERROR`. **]**

## file_extend

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend, FILE_HANDLE, handle, uint64_t, desired_size)(0, MU_FAILURE);
```

**NON_FILE_LINUX_43_018: [** `file_extend` shall return 0. **]**

Will be implemented later.

## on_file_write_complete_linux

```c
static void on_file_write_complete_linux( FILE_LINUX_WRITE* write_info);
```

`on_file_write_complete_linux` is called when an asynchronous write operation completes. `on_file_write_complete_linux` calls the user-specified callback with the user-specified context and a bool indicating the success or failure of the asynchronous operation.

**NON_FILE_LINUX_43_047: [** If `write_info` is `NULL`, `on_file_write_complete_linux` shall call `user_report_fault_callback` with `user_report_fault_context`. **]**

**NON_FILE_LINUX_43_021: [** `on_file_write_complete_linux` shall recover the `aiocb` struct that was used to create the current asynchronous write operation. **]**

**NON_FILE_LINUX_43_022: [** `on_file_write_complete_linux` shall call `aio_return` to determine if the asynchronous write operation succeeded. **]**

**NON_FILE_LINUX_43_023: [** If the asynchronous write operation did not succeed, `on_file_io_complete_linux` shall call `user_callback` with `user_context` and `false` as `is_successful`. **]**

**NON_FILE_LINUX_43_027: [** If the asynchronous write operation succeeded, `on_file_write_complete_linux` shall call `user_callback` with `user_context` and `true` as `is_successful`. **]**

## on_file_read_complete_linux

```c
static void on_file_read_complete_linux( FILE_LINUX_READ* read_info);
```

`on_file_read_complete_linux` is called when an asynchronous read operation completes. `on_file_write_complete_linux` calls the user-specified callback with the user-specified context and a bool indicating the success or failure of the asynchronous operation.

**NON_FILE_LINUX_43_046: [** If `read_info` is `NULL`, `on_file_write_complete_linux` shall call `user_report_fault_callback` with `user_report_fault_context`. **]**

**NON_FILE_LINUX_43_039: [** `on_file_read_complete_linux` shall recover the `aiocb` struct that was used to create the current asynchronous read operation. **]**

**NON_FILE_LINUX_43_040: [** `on_file_read_complete_linux` shall call `aio_return` to determine if the asynchronous read operation succeeded. **]**

**NON_FILE_LINUX_43_041: [** If the asynchronous read operation did not succeed, `on_file_io_complete_linux` shall call `user_callback` with `user_context` and `false` as `is_successful`. **]**

**NON_FILE_LINUX_43_042: [** If the asynchronous read operation succeeded, `on_file_read_complete_linux` shall call `user_callback` with `user_context` and `false` as `is_successful`. **]**

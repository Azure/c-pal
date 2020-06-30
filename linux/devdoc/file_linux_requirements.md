# file_linux
================

## Overview

Linux implementation of the `file` module.

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

**SRS_FILE_LINUX_43_001: [** `file_create` shall call `open` with `full_file_name` as `pathname` and flags `O_CREAT` and `O_RDWR`. **]**

**SRS_FILE_LINUX_43_002: [** `file_create` shall return the file handle returned by the call to open. [`open` documentation](https://www.man7.org/linux/man-pages/man2/open.2.html) **]**

## file_destroy

```c
MOCKABLE_FUNCTION(, void, file_destroy, FILE_HANDLE, handle);
```

**SRS_FILE_LINUX_43_003: [** `file_destroy` shall call `close` with `fd` as `handle`. [`close` documentation](https://www.man7.org/linux/man-pages/man2/close.2.html) **]**

## file_write_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_WRITE_ASYNC_RESULT, file_write_async, FILE_HANDLE, handle, CONSTBUFFER_HANDLE, source, int64_t, position, FILE_WRITE_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

**SRS_FILE_LINUX_43_016: [** `file_write_async` shall create a `FILE_LINUX_IO` struct with `handle` as `handle`, `FILE_ASYNC_WRITE` as `type`. **]**

**SRS_FILE_LINUX_43_017: [** `file_write_async` shall populate the `data` field of the `FILE_LINUX_IO` struct using `source`, `user_callback` and `user_context`. **]**

**SRS_FILE_LINUX_43_004: [** `file_write_async` shall initialize a `sigevent` struct with `SIGEV_THREAD` as `sigev_notify`, the created `FILE_LINUX_IO` struct as `sigev_value`, `on_file_io_complete_linux` as `sigev_notify_function`, `NULL` as `sigev_notify_attributes`(unsure about what `sigev_notify_attributes` should be). [`sigevent` documentation](https://man7.org/linux/man-pages/man7/sigevent.7.html). **]**

**SRS_FILE_LINUX_43_005: [** `file_write_async` shall initialize an `aiocb` struct with `handle` as `aio_fildes`, `position` as `aio_offset`, source as `aio_buf`, `source->alias->size` as `aio_nbytes`, and the initialized `sigevent` struct as `aio_sigevent`. **]**

**SRS_FILE_LINUX_43_006: [** `file_write_async` shall call `aio_write` with the initialized `aiocb` struct as `aiocbp`. [`aio_write` documentation](https://man7.org/linux/man-pages/man3/aio_write.3.html) **]**

**SRS_FILE_LINUX_43_012: [** If `aio_write` fails, `file_write_async` shall return `FILE_WRITE_ASYNC_WRITE_ERROR`. **]**

**SRS_FILE_LINUX_43_007: [** If `aio_write` succeeds, `file_write_async` shall return `FILE_WRITE_ASYNC_OK`. **]**

**SRS_FILE_LINUX_43_013: [** If there are any failures, `file_write_async` shall return `FILE_WRITE_ASYNC_ERROR`. **]**

## file_read_async

```c
MOCKABLE_FUNCTION_WITH_RETURNS(, FILE_READ_ASYNC_RESULT, file_read_async, FILE_HANDLE, handle, uint32_t, size, int64_t, position, FILE_READ_CB, user_callback, void*, user_context)(0, MU_FAILURE);
```

**SRS_FILE_LINUX_43_016: [** `file_read_async` shall create a `FILE_LINUX_IO` struct with `handle` as `handle`, `FILE_ASYNC_READ` as `type`. **]**

**SRS_FILE_LINUX_43_017: [** `file_read_async` shall populate the `data` field of the `FILE_LINUX_IO` struct using `user_callback` and `user_context`. **]**

**SRS_FILE_LINUX_43_008: [** `file_read_async` shall initialize a `sigevent` struct with `SIGEV_THREAD` as `sigev_notify`, `user_context` as `sigev_value`, `user_callback` as `sigev_notify_function`, `NULL` as `sigev_notify_attributes`(unsure about what `sigev_notify_attributes` should be). [`sigevent` documentation](https://man7.org/linux/man-pages/man7/sigevent.7.html). **]**

**SRS_FILE_LINUX_43_009: [** `file_read_async` shall initialize an `aiocb` struct with `handle` as `aio_fildes`, `position` as `aio_offset`, source as `aio_buf`, `source->alias->size` as `aio_nbytes`, and the initialized `sigevent` struct as `aio_sigevent`. **]**

**SRS_FILE_LINUX_43_010: [** `file_read_async` shall call `aio_read` with the initialized `aiocb` struct as `aiocbp`.  **]**

**SRS_FILE_LINUX_43_011: [** If `aio_read` fails, `file_read_async` shall return `FILE_READ_ASYNC_READ_ERROR`. **]**

**SRS_FILE_LINUX_43_014: [** If `aio_read` succeeds, `file_read_async` shall return `FILE_READ_ASYNC_OK`. **]**

**SRS_FILE_LINUX_43_015: [** If there are any failures, `file_read_async` shall return `FILE_READ_ASYNC_ERROR`. **]**


## file_extend_filesize
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, int, file_extend_filesize, FILE_HANDLE, handle, uint64_t, desired_size, bool, has_manage_volume)(0, MU_FAILURE);
```

Will do this later.
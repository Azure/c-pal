`async_socket_win32` requirements
================

## Overview

`async_socket_win32` is an implementation of `async_socket` that wraps the asynchronous socket APIs for Windows.

## Design

`async_socket_win32` is using the WSA Windows functions with a PTP_POOL in order to perform asynchronous socket send and receives.
`async_socket_win32` creates its own threadpool environment and cleanup group.

## Exposed API

`async_socket_win32` implements the `async_socket` API:

```c
typedef struct ASYNC_SOCKET* ASYNC_SOCKET_HANDLE;

#define ASYNC_SOCKET_OPEN_RESULT_VALUES \
    ASYNC_SOCKET_OPEN_OK, \
    ASYNC_SOCKET_OPEN_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_RESULT_VALUES \
    ASYNC_SOCKET_SEND_OK, \
    ASYNC_SOCKET_SEND_ERROR, \
    ASYNC_SOCKET_SEND_BECAUSE_CLOSE

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)

#define ASYNC_SOCKET_RECEIVE_RESULT_VALUES \
    ASYNC_SOCKET_RECEIVE_OK, \
    ASYNC_SOCKET_RECEIVE_ERROR, \
    ASYNC_SOCKET_RECEIVE_BECAUSE_CLOSE

MU_DEFINE_ENUM(ASYNC_SOCKET_RECEIVE_RESULT, ASYNC_SOCKET_RECEIVE_RESULT_VALUES)

typedef void (*ON_ASYNC_SOCKET_OPEN_COMPLETE)(void* context, ASYNC_SOCKET_OPEN_RESULT open_result);
typedef void (*ON_ASYNC_SOCKET_SEND_COMPLETE)(void* context, ASYNC_SOCKET_SEND_RESULT send_result);
typedef void (*ON_ASYNC_SOCKET_RECEIVE_COMPLETE)(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received);

typedef struct ASYNC_SOCKET_BUFFER_TAG
{
    void* buffer;
    uint32_t length;
} ASYNC_SOCKET_BUFFER;

MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);

MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
MOCKABLE_FUNCTION(, int, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, buffers, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, buffers, uint32_t, buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
```

### async_socket_create

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
```

`async_socket_create` creates an async socket.

X**SRS_ASYNC_SOCKET_WIN32_01_001: [** `async_socket_create` shall allocate a new async socket and on success shall return a non-NULL handle. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_002: [** If `execution_engine` is NULL, `async_socket_create` shall fail and return NULL. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_034: [** If `socket_handle` is `INVALID_SOCKET`, `async_socket_create` shall fail and return NULL. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_035: [** Otherwise, `async_socket_open_async` shall obtain the PTP_POOL from the execution engine passed to `async_socket_create` by calling `execution_engine_win32_get_threadpool`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_003: [** If any error occurs, `async_socket_create` shall fail and return NULL. **]**

### async_socket_destroy

```c
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_destroy` frees all resources associated with `async_socket`.

X**SRS_ASYNC_SOCKET_WIN32_01_004: [** If `async_socket` is NULL, `async_socket_destroy` shall return. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_005: [** Otherwise, `async_socket_destroy` shall free all resources associated with `async_socket`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_093: [** While `async_socket` is OPENING or CLOSING, `async_socket_destroy` shall wait for the open to complete either succesfully or with error. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_006: [** `async_socket_destroy` shall perform an implicit close if `async_socket` is OPEN. **]**

### async_socket_open_async

```c
MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
```

`async_socket_open_async` opens the async socket.

X**SRS_ASYNC_SOCKET_WIN32_01_007: [** If `async_socket` is NULL, `async_socket_open_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_008: [** If `on_open_complete` is NULL, `async_socket_open_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_009: [** `on_open_complete_context` shall be allowed to be NULL. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_023: [** Otherwise, `async_socket_open_async` shall switch the state to OPENING. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_014: [** On success, `async_socket_open_async` shall return 0. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_015: [** If `async_socket` is already OPEN or OPENING, `async_socket_open_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_016: [** Otherwise `async_socket_open_async` shall initialize a thread pool environment by calling `InitializeThreadpoolEnvironment`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_036: [** `async_socket_open_async` shall set the thread pool for the environment to the pool obtained from the execution engine by calling `SetThreadpoolCallbackPool`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_037: [** `async_socket_open_async` shall create a threadpool cleanup group by calling `CreateThreadpoolCleanupGroup`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_058: [** `async_socket_open_async` shall create a threadpool IO by calling `CreateThreadpoolIo` and passing `socket_handle`, the callback environment to it and `on_io_complete` as callback. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_094: [** `async_socket_open_async` shall set the state to OPEN. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_017: [** On success `async_socket_open_async` shall call `on_open_complete_context` with `ASYNC_SOCKET_OPEN_OK`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_039: [** If any error occurs, `async_socket_open_async` shall fail and return a non-zero value. **]**

### async_socket_close

```c
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_close` closes an open `async_socket`.

X**SRS_ASYNC_SOCKET_WIN32_01_018: [** If `async_socket` is NULL, `async_socket_close` shall return. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_019: [** Otherwise, `async_socket_close` shall switch the state to CLOSING. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_020: [** `async_socket_close` shall wait for all executing `async_socket_send_async` and `async_socket_receive_async` APIs. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_021: [** Then `async_socket_close` shall close the async socket, leaving it in a state where an `async_socket_open_async` can be performed. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_040: [** `async_socket_close` shall wait for any executing callbacks by calling `WaitForThreadpoolIoCallbacks`, passing FALSE as `fCancelPendingCallbacks`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_059: [** `async_socket_close` shall close the threadpool IO created in `async_socket_open_async` by calling `CloseThreadpoolIo`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_041: [** `async_socket_close` shall close the threadpool cleanup group by calling `CloseThreadpoolCleanupGroup`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_042: [** `async_socket_close` shall destroy the thread pool environment created in `async_socket_open_async`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_022: [** If `async_socket` is not OPEN, `async_socket_close` shall return. **]**

### async_socket_send_async

```c
MOCKABLE_FUNCTION(, int, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
```

`async_socket_send_async` sends a number of buffers asynchronously.

X**SRS_ASYNC_SOCKET_WIN32_01_024: [** If `async_socket` is NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_025: [** If `buffers` is NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_085: [** If `buffer_count` is 0, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_089: [** If any of the buffers in `payload` has `buffer` set to NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_090: [** If any of the buffers in `payload` has `length` set to 0, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_101: [** If the sum of buffer lengths for all the buffers in `payload` is greater than `UINT32_MAX`, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_026: [** If `on_send_complete` is NULL, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_027: [** `on_send_complete_context` shall be allowed to be NULL. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_097: [** If `async_socket` is not OPEN, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_028: [** Otherwise `async_socket_send_async` shall create a context for the send where the `payload`, `on_send_complete` and `on_send_complete_context` shall be stored. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_050: [** The context shall also allocate enough memory to keep an array of `buffer_count` WSABUF items. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_103: [** If the amount of memory needed to allocate the context and the WSABUF items is exceeding UINT32_MAX, `async_socket_send_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_056: [** `async_socket_send_async` shall set the WSABUF items to point to the memory/length of the buffers in `payload`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_057: [** An event to be used for the `OVERLAPPED` structure passed to `WSASend` shall be created and stored in the context. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_060: [** An asynchronous IO shall be started by calling `StartThreadpoolIo`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_061: [** The `WSABUF` array associated with the context shall be sent by calling `WSASend` and passing to it the `OVERLAPPED` structure with the event that was just created, `dwFlags` set to 0, `lpNumberOfBytesSent` set to NULL and `lpCompletionRoutine` set to NULL. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_062: [** If `WSASend` fails, `async_socket_send_async` shall call `WSAGetLastError`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_053: [** If `WSAGetLastError` returns `WSA_IO_PENDING`, it shall be not treated as an error. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_100: [** If `WSAGetLastError` returns any other error, `async_socket_send_async` shall call `CancelThreadpoolIo`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_106: [** If `WSASend` fails with any other error, `async_socket_send_async` shall call `CancelThreadpoolIo` and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_045: [** On success, `async_socket_send_async` shall return 0. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_029: [** If any error occurs, `async_socket_send_async` shall fail and return a non-zero value. **]**

### async_socket_receive_async

```c
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_SOCKET_ASYNC_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
```

`async_socket_receive_async` receives in a number of buffers asynchronously.

X**SRS_ASYNC_SOCKET_WIN32_01_073: [** If `async_socket` is NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_074: [** If `buffers` is NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_086: [** If `buffer_count` is 0, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_091: [** If any of the buffers in `payload` has `buffer` set to NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_092: [** If any of the buffers in `payload` has `length` set to 0, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_096: [** If the sum of buffer lengths for all the buffers in `payload` is greater than `UINT32_MAX`, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_075: [** If `on_receive_complete` is NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_076: [** `on_receive_complete_context` shall be allowed to be NULL. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_098: [** If `async_socket` is not OPEN, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_077: [** Otherwise `async_socket_receive_async` shall create a context for the send where the `payload`, `on_receive_complete` and `on_receive_complete_context` shall be stored. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_078: [** The context shall also allocate enough memory to keep an array of `buffer_count` WSABUF items. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_104: [** If the amount of memory needed to allocate the context and the WSABUF items is exceeding UINT32_MAX, `async_socket_receive_async` shall fail and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_079: [** `async_socket_receive_async` shall set the WSABUF items to point to the memory/length of the buffers in `payload`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_080: [** An event to be used for the `OVERLAPPED` structure passed to `WSARecv` shall be created and stored in the context. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_081: [** An asynchronous IO shall be started by calling `StartThreadpoolIo`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_082: [** A receive shall be started for the `WSABUF` array associated with the context calling `WSARecv` and passing to it the `OVERLAPPED` structure with the event that was just created, `dwFlags` set to 0, `lpNumberOfBytesSent` set to NULL and `lpCompletionRoutine` set to NULL.. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_054: [** If `WSARecv` fails with `SOCKET_ERROR`, `async_socket_receive_async` shall call `WSAGetLastError`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_105: [** If `WSARecv` fails with any other error, `async_socket_receive_async` shall call `CancelThreadpoolIo` and return a non-zero value. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_055: [** If `WSAGetLastError` returns `IO_PENDING`, it shall be not treated as an error. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_099: [** If `WSAGetLastError` returns any other error, `async_socket_receive_async` shall call `CancelThreadpoolIo`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_083: [** On success, `async_socket_receive_async` shall return 0. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_084: [** If any error occurs, `async_socket_receive_async` shall fail and return a non-zero value. **]**

### on_io_complete

```c
static VOID CALLBACK on_io_complete(PTP_CALLBACK_INSTANCE instance, PVOID context, PVOID overlapped, ULONG io_result, ULONG_PTR number_of_bytes_transferred, PTP_IO io)
```

`on_io_complete` handles the threadpool IO callbacks.

X**SRS_ASYNC_SOCKET_WIN32_01_063: [** If `overlapped` is NULL, `on_io_complete` shall return. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_064: [** `overlapped` shall be used to determine the context of the IO. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_065: [** If the context of the IO indicates that a send has completed: **]**

   - X**SRS_ASYNC_SOCKET_WIN32_01_066: [** If `io_result` is `NO_ERROR`, the `on_send_complete` callback passed to `async_socket_send_async` shall be called with `on_send_complete_context` as argument and `ASYNC_SOCKET_SEND_OK`. **]**

   - X**SRS_ASYNC_SOCKET_WIN32_01_067: [** If `io_result` is not `NO_ERROR`, the `on_send_complete` callback passed to `async_socket_send_async` shall be called with `on_send_complete_context` as argument and `ASYNC_SOCKET_SEND_ERROR`. **]**

   - X**SRS_ASYNC_SOCKET_WIN32_01_102: [** If `io_result` is `NO_ERROR`, but the number of bytes send is different than the sum of all buffer sizes passed to `async_socket_send_async`, the `on_send_complete` callback passed to `async_socket_send_async` shall be called with `on_send_complete_context` as context and `ASYNC_SOCKET_SEND_ERROR`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_071: [** If the context of the IO indicates that a receive has completed: **]**

   - X**SRS_ASYNC_SOCKET_WIN32_01_069: [** If `io_result` is `NO_ERROR`, the `on_receive_complete` callback passed to `async_socket_receive_async` shall be called with `on_receive_complete_context` as context, `ASYNC_SOCKET_RECEIVE_OK` as result and `number_of_bytes_transferred` as `bytes_received`. **]**

   - X**SRS_ASYNC_SOCKET_WIN32_01_070: [** If `io_result` is not `NO_ERROR`, the `on_receive_complete` callback passed to `async_socket_receive_async` shall be called with `on_receive_complete_context` as context, `ASYNC_SOCKET_RECEIVE_ERROR` as result and 0 for `bytes_received`. **]**

   - X**SRS_ASYNC_SOCKET_WIN32_01_095: [**If `io_result` is `NO_ERROR`, but the number of bytes received is greater than the sum of all buffer sizes passed to `async_socket_receive_async`, the `on_receive_complete` callback passed to `async_socket_receive_async` shall be called with `on_receive_complete_context` as context, `ASYNC_SOCKET_RECEIVE_ERROR` as result and `number_of_bytes_transferred` for `bytes_received`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_068: [** `on_io_complete` shall close the event handle created in `async_socket_send_async`/`async_socket_receive_async`. **]**

X**SRS_ASYNC_SOCKET_WIN32_01_072: [** `on_io_complete` shall free the IO context. **]**


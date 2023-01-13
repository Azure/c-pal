# `async_socket_linux` requirements

## Overview

`async_socket_linux` is an implementation of `async_socket` that wraps the asynchronous socket APIs for linux.

## Design

`async_socket` is an interface for a socket that allows asynchronous sends/receives.

An `async_socket` owns the underlying platform specific socket passed on create. After successful creation, it will handle closing the platform-specific socket.

`async_socket` does not take ownership of the buffers passed to `async_socket_send_async` and `async_socket_receive_async`.

## Threading

`async_socket_linux` shall not create any threads, but use the completion port linux object for its threading.  The API shall be thread-safe with the exception of `async_socket_destroy` which should be only called from one thread.

## Exposed API

`async_socket_linux` implements the `async_socket` API:

```c
typedef struct ASYNC_SOCKET* ASYNC_SOCKET_HANDLE;

#define ASYNC_SOCKET_OPEN_RESULT_VALUES \
    ASYNC_SOCKET_OPEN_OK, \
    ASYNC_SOCKET_OPEN_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES \
    ASYNC_SOCKET_SEND_SYNC_OK, \
    ASYNC_SOCKET_SEND_SYNC_ERROR, \
    ASYNC_SOCKET_SEND_SYNC_ABANDONED

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_SYNC_RESULT, ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_RESULT_VALUES \
    ASYNC_SOCKET_SEND_OK, \
    ASYNC_SOCKET_SEND_ERROR, \
    ASYNC_SOCKET_SEND_ABANDONED

MU_DEFINE_ENUM(ASYNC_SOCKET_SEND_RESULT, ASYNC_SOCKET_SEND_RESULT_VALUES)

#define ASYNC_SOCKET_RECEIVE_RESULT_VALUES \
    ASYNC_SOCKET_RECEIVE_OK, \
    ASYNC_SOCKET_RECEIVE_ERROR, \
    ASYNC_SOCKET_RECEIVE_ABANDONED

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
MOCKABLE_FUNCTION(, ASYNC_SOCKET_SEND_SYNC_RESULT, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, buffers, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, buffers, uint32_t, buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
```

### async_socket_create

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
```

`async_socket_create` creates an async socket.

**SRS_ASYNC_SOCKET_LINUX_11_001: [** `async_socket_create` shall allocate a new async socket and on success shall return a non-`NULL` handle. **]**

**SRS_ASYNC_SOCKET_LINUX_11_002: [** `execution_engine` shall be allowed to be `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_003: [** If `socket_handle` is `INVALID_SOCKET`, `async_socket_create` shall fail and return `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_005: [** `async_socket_create` shall retrieve an `COMPLETION_PORT_HANDLE` object by calling `platform_get_completion_port`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_006: [** If any error occurs, `async_socket_create` shall fail and return `NULL`. **]**

### async_socket_destroy

```c
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_destroy` frees all resources associated with `async_socket`.

**SRS_ASYNC_SOCKET_LINUX_11_019: [** If `async_socket` is `NULL`, `async_socket_destroy` shall return. **]**

**SRS_ASYNC_SOCKET_LINUX_11_020: [** While `async_socket` is `OPENING` or `CLOSING`, `async_socket_destroy` shall wait for the open/close to complete either successfully or with error. **]**

**SRS_ASYNC_SOCKET_LINUX_11_021: [** `async_socket_destroy` shall perform an implicit close if `async_socket` is `OPEN`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_022: [** `async_socket_destroy` shall decrement the reference count on the completion port. **]**

**SRS_ASYNC_SOCKET_LINUX_11_023: [** `async_socket_destroy` shall free all resources associated with `async_socket`. **]**

### async_socket_open_async

```c
MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
```

`async_socket_open_async` opens the async socket.

**SRS_ASYNC_SOCKET_LINUX_11_024: [** If `async_socket` is `NULL`, `async_socket_open_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_025: [** If `on_open_complete` is `NULL`, `async_socket_open_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_026: [** `on_open_complete_context` shall be allowed to be `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_027: [** Otherwise, `async_socket_open_async` shall switch the state to `OPENING`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_028: [** On success, `async_socket_open_async` shall return 0. **]**

**SRS_ASYNC_SOCKET_LINUX_11_029: [** If `async_socket` is already OPEN or OPENING, `async_socket_open_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_030: [** If `async_socket` has already closed the underlying socket handle then `async_socket_open_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_031: [** `async_socket_open_async` shall add the socket to the epoll system by calling `epoll_ctl` with `EPOLL_CTL_ADD`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_032: [** `async_socket_open_async` shall set the state to OPEN. **]**

**SRS_ASYNC_SOCKET_LINUX_11_033: [** On success `async_socket_open_async` shall call `on_open_complete_context` with `ASYNC_SOCKET_OPEN_OK`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_034: [** If any error occurs, `async_socket_open_async` shall fail and return a non-zero value. **]**

### async_socket_close

```c
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_close` closes an open `async_socket`.

**SRS_ASYNC_SOCKET_LINUX_11_035: [** If `async_socket` is `NULL`, `async_socket_close` shall return. **]**

**SRS_ASYNC_SOCKET_LINUX_11_036: [** Otherwise, `async_socket_close` shall switch the state to CLOSING. **]**

**SRS_ASYNC_SOCKET_LINUX_11_037: [** `async_socket_close` shall wait for all executing `async_socket_send_async` and `async_socket_receive_async` APIs. **]**

**SRS_ASYNC_SOCKET_LINUX_11_039: [** `async_socket_close` shall call `close` on the underlying socket. **]**

**SRS_ASYNC_SOCKET_LINUX_11_041: [** `async_socket_close` shall set the state to CLOSED. **]**

**SRS_ASYNC_SOCKET_LINUX_11_042: [** If `async_socket` is not OPEN, `async_socket_close` shall return. **]**

### async_socket_send_async

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_SEND_SYNC_RESULT, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
```

`async_socket_send_async` sends a number of buffers asynchronously.

**SRS_ASYNC_SOCKET_LINUX_11_043: [** If `async_socket` is `NULL`, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_044: [** If `buffers` is `NULL`, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_045: [** If `buffer_count` is 0, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_046: [** If any of the buffers in `payload` has `buffer` set to `NULL`, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_047: [** If any of the buffers in `payload` has `length` set to 0, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_048: [** If the sum of buffer lengths for all the buffers in `payload` is greater than `UINT32_MAX`, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_049: [** If `on_send_complete` is `NULL`, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_050: [** `on_send_complete_context` shall be allowed to be `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_051: [** If `async_socket` is not OPEN, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ABANDONED`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_052: [** `async_socket_send_async` shall attempt to send the data by calling `send` with the `MSG_NOSIGNAL` flag to ensure SIGPIPE is not generated on errors. **]**

**SRS_ASYNC_SOCKET_LINUX_11_053: [** `async_socket_send_async` shall continue to send the data until the payload length has been sent. **]**

**SRS_ASYNC_SOCKET_LINUX_11_054: [** If the `send` fails to send the data, `async_socket_send_async` shall do the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_11_055: [** If the `errno` value is `EAGAIN` or `EWOULDBLOCK`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_056: [** `async_socket_send_async` shall create a context for the send where the `payload`, `on_send_complete` and `on_send_complete_context` shall be stored. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_057: [** The context shall then be added to the completion port system by calling `completion_port_add` with `EPOLL_CTL_MOD` and `event_complete_callback` as the callback. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_059: [** If the `errno` value is `ECONNRESET`, `ENOTCONN`, or `EPIPE` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ABANDONED`. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_060: [** If any other error is encountered, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_061: [** If the `send` is successful, `async_socket_send_async` shall call the `on_send_complete` with `on_send_complete_context` and `ASYNC_SOCKET_SEND_SYNC_OK`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_062: [** On success, `async_socket_send_async` shall return `ASYNC_SOCKET_SEND_SYNC_OK`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_063: [** If any error occurs, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

### async_socket_receive_async

```c
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_SOCKET_ASYNC_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
```

`async_socket_receive_async` receives in a number of buffers asynchronously.

**SRS_ASYNC_SOCKET_LINUX_11_064: [** If `async_socket` is `NULL`, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_065: [** If `buffers` is `NULL`, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_066: [** If `buffer_count` is 0, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_067: [** If any of the buffers in `payload` has `buffer` set to NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_068: [** If any of the buffers in `payload` has `length` set to 0, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_069: [** If the sum of buffer lengths for all the buffers in `payload` is greater than `UINT32_MAX`, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_070: [** If `on_receive_complete` is NULL, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_071: [** `on_receive_complete_context` shall be allowed to be `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_072: [** If `async_socket` is not OPEN, `async_socket_receive_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_073: [** Otherwise `async_socket_receive_async` shall create a context for the recv where the `payload`, `on_receive_complete` and `on_receive_complete_context` shall be stored. **]**

**SRS_ASYNC_SOCKET_LINUX_11_074: [** The context shall also allocate enough memory to keep an array of `buffer_count` items. **]**

**SRS_ASYNC_SOCKET_LINUX_11_102: [** Then the context shall then be added to the completion port system by calling `completion_port_add` with `EPOLLIN` and `event_complete_callback` as the callback. **]**

**SRS_ASYNC_SOCKET_LINUX_11_077: [** On success, `async_socket_receive_async` shall return 0. **]**

**SRS_ASYNC_SOCKET_LINUX_11_078: [** If any error occurs, `async_socket_receive_async` shall fail and return a non-zero value. **]**

### event_complete_callback

```c
static void event_complete_callback(void* context, COMPLETION_PORT_EPOLL_ACTION action)
```

`event_complete_callback` handles all the completion port callback info.  In case of a disconnection all events will be services in the order they were queued.

**SRS_ASYNC_SOCKET_LINUX_11_079: [** If context is `NULL`, `event_complete_callback` shall do nothing. **]**

**SRS_ASYNC_SOCKET_LINUX_11_080: [** If `COMPLETION_PORT_EPOLL_ACTION` is `COMPLETION_PORT_EPOLL_EPOLLRDHUP` or `COMPLETION_PORT_EPOLL_ABANDONED`, `event_complete_callback` shall do the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_11_081: [** `event_complete_callback` shall call either the send or recv complete callback with an `ABANDONED` flag. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_084: [** Then `event_complete_callback` shall free the `context` memory. **]**

**SRS_ASYNC_SOCKET_LINUX_11_082: [** If `COMPLETION_PORT_EPOLL_ACTION` is `COMPLETION_PORT_EPOLL_EPOLLIN`, `event_complete_callback` shall do the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_11_083: [** `event_complete_callback` shall call `recv` with the `recv_buffer` buffer and length and do the following: **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_088: [** If the recv size < 0, then: **]**

    - **SRS_ASYNC_SOCKET_LINUX_11_089: [** If `errno` is `EAGAIN` or `EWOULDBLOCK`, then no data is available and `event_complete_callback` will break out of the function. **]**

    - **SRS_ASYNC_SOCKET_LINUX_11_090: [** If `errno` is `ECONNRESET`, then `event_complete_callback` shall call the `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_ABANDONED`. **]**

    - **SRS_ASYNC_SOCKET_LINUX_11_095: [** If `errno` is any other error, then `event_complete_callback` shall call the `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_ERROR`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_091: [** If the recv size equals 0, then `event_complete_callback` shall call `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_ABANDONED`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_092: [** If the recv size > 0, if we have another buffer to fill then we will attempt another read, otherwise we shall call `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_OK`. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_093: [** `event_complete_callback` shall then free the `io_context` memory. **]**

**SRS_ASYNC_SOCKET_LINUX_11_094: [** If the events value contains `COMPLETION_PORT_EPOLL_EPOLLOUT`, `event_complete_callback` shall the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_11_096: [** `event_complete_callback` shall call `send` on the data in the `ASYNC_SOCKET_SEND_CONTEXT` buffer. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_097: [** If send returns value is < 0 `event_complete_callback` shall do the following: **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_098: [** if `errno` is `ECONNRESET`, then `on_send_complete` shall be called with `ASYNC_SOCKET_SEND_ABANDONED`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_099: [** if `errno` is anything else, then `on_send_complete` shall be called with `ASYNC_SOCKET_SEND_ERROR`. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_101: [** If send returns a value > 0 but less than the amount to be sent, `event_complete_callback` shall continue to send the data until the payload length has been sent. **]**

**SRS_ASYNC_SOCKET_LINUX_11_100: [** Then `event_complete_callback` shall free the `io_context` memory **]**

**SRS_ASYNC_SOCKET_LINUX_11_085: [** If the events value contains `COMPLETION_PORT_EPOLL_ERROR`, `event_complete_callback` shall the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_11_086: [** `event_complete_callback` shall call either the send or recv complete callback with an `ERROR` flag. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_087: [** Then `event_complete_callback` shall and free the `io_context` memory. **]**

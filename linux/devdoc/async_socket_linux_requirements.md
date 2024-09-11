# `async_socket_linux` requirements

## Overview

`async_socket_linux` is an implementation of `async_socket` that wraps the asynchronous socket APIs for linux.

## Design

`async_socket` is an interface for a socket that allows asynchronous sends/receives.

An `async_socket` owns the underlying platform specific socket passed on create. After successful creation, it will handle closing the platform-specific socket.

`async_socket` does not take ownership of the buffers passed to `async_socket_send_async` and `async_socket_receive_async`.

## Threading

`async_socket_linux` shall not create any threads, but use the completion port linux object for its threading.  The API shall be thread-safe with the exception of `async_socket_destroy` which should be only called from one thread.

## Custom Transport

The `async_socket` may be created with a custom transport handler for send and receive functions.

This requires callbacks of type `ON_ASYNC_SOCKET_SEND` and `ON_ASYNC_SOCKET_RECV`.

`ON_ASYNC_SOCKET_SEND` must behave like the `send` function from the libc socket API. It must return the number of bytes sent or `-1` to indicate an error. In case of an error, it must set `errno` with the error code.

`ON_ASYNC_SOCKET_RECV` must behave like the `recv` function from the libc socket API. It must return the number of bytes received or `-1` to indicate an error. In case of an error, it must set `errno` with the error code.

## Exposed API

`async_socket_linux` implements the `async_socket` API:

```c
typedef struct ASYNC_SOCKET_TAG* ASYNC_SOCKET_HANDLE;

#define ASYNC_SOCKET_OPEN_RESULT_VALUES \
    ASYNC_SOCKET_OPEN_OK, \
    ASYNC_SOCKET_OPEN_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_OPEN_RESULT, ASYNC_SOCKET_OPEN_RESULT_VALUES)

#define ASYNC_SOCKET_SEND_SYNC_RESULT_VALUES \
    ASYNC_SOCKET_SEND_SYNC_OK, \
    ASYNC_SOCKET_SEND_SYNC_ERROR, \
    ASYNC_SOCKET_SEND_SYNC_NOT_OPEN

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

#define ASYNC_SOCKET_NOTIFY_IO_TYPE_VALUES \
    ASYNC_SOCKET_NOTIFY_IO_TYPE_IN, \
    ASYNC_SOCKET_NOTIFY_IO_TYPE_OUT

MU_DEFINE_ENUM(ASYNC_SOCKET_NOTIFY_IO_TYPE, ASYNC_SOCKET_NOTIFY_IO_TYPE_VALUES)

#define ASYNC_SOCKET_NOTIFY_IO_RESULT_VALUES \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_IN, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_OUT, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_ABANDONED, \
    ASYNC_SOCKET_NOTIFY_IO_RESULT_ERROR

MU_DEFINE_ENUM(ASYNC_SOCKET_NOTIFY_IO_RESULT, ASYNC_SOCKET_NOTIFY_IO_RESULT_VALUES)

typedef void (*ON_ASYNC_SOCKET_OPEN_COMPLETE)(void* context, ASYNC_SOCKET_OPEN_RESULT open_result);
typedef void (*ON_ASYNC_SOCKET_SEND_COMPLETE)(void* context, ASYNC_SOCKET_SEND_RESULT send_result);
typedef void (*ON_ASYNC_SOCKET_RECEIVE_COMPLETE)(void* context, ASYNC_SOCKET_RECEIVE_RESULT receive_result, uint32_t bytes_received);
typedef void (*ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE)(void* context, ASYNC_SOCKET_NOTIFY_IO_RESULT notify_io_result);
typedef int (*ON_ASYNC_SOCKET_SEND)(void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len);
typedef int (*ON_ASYNC_SOCKET_RECV)(void* context, ASYNC_SOCKET_HANDLE async_socket, void* buf, size_t len);

typedef struct ASYNC_SOCKET_BUFFER_TAG
{
    void* buffer;
    uint32_t length;
} ASYNC_SOCKET_BUFFER;

MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create_with_transport, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle, ON_ASYNC_SOCKET_SEND, on_send, void*, on_send_context, ON_ASYNC_SOCKET_RECV, on_recv, void*, on_recv_context);
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);

MOCKABLE_FUNCTION(, int, async_socket_open_async, ASYNC_SOCKET_HANDLE, async_socket, ON_ASYNC_SOCKET_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
MOCKABLE_FUNCTION(, void, async_socket_close, ASYNC_SOCKET_HANDLE, async_socket);
MOCKABLE_FUNCTION(, ASYNC_SOCKET_SEND_SYNC_RESULT, async_socket_send_async, ASYNC_SOCKET_HANDLE, async_socket, const ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_SEND_COMPLETE, on_send_complete, void*, on_send_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_receive_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_BUFFER*, payload, uint32_t, buffer_count, ON_ASYNC_SOCKET_RECEIVE_COMPLETE, on_receive_complete, void*, on_receive_complete_context);
MOCKABLE_FUNCTION(, int, async_socket_notify_io_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE, io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE, on_notify_io_complete, void*, on_notify_io_complete_context);
```

### async_socket_create

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle);
```

`async_socket_create_with_transport` creates an async socket.

**SRS_ASYNC_SOCKET_LINUX_04_001: [** `async_socket_create` shall delegate to `async_socket_create_with_transport` passing in callbacks for `on_send` and `on_recv` that implement socket read and write by calling `send` and `recv` respectively from system socket API. **]**

### async_socket_create_with_transport

```c
MOCKABLE_FUNCTION(, ASYNC_SOCKET_HANDLE, async_socket_create_with_transport, EXECUTION_ENGINE_HANDLE, execution_engine, SOCKET_HANDLE, socket_handle, ON_ASYNC_SOCKET_SEND, on_send, void*, on_send_context, ON_ASYNC_SOCKET_RECV, on_recv, void*, on_recv_context);
```

`async_socket_create_with_transport` creates an async socket. It allows the passing in of callback functions that implement the actual write and read on the socket via the `on_send` and `on_recv` callbacks.

**SRS_ASYNC_SOCKET_LINUX_11_001: [** `async_socket_create_with_transport` shall allocate a new async socket and on success shall return a non-`NULL` handle. **]**

**SRS_ASYNC_SOCKET_LINUX_11_002: [** `execution_engine` shall be allowed to be `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_04_002: [** If `on_send` is `NULL` , `async_socket_create_with_transport` shall fail and return `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_04_003: [** If `on_recv` is `NULL` , `async_socket_create_with_transport` shall fail and return `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_005: [** `async_socket_create_with_transport` shall retrieve an `COMPLETION_PORT_HANDLE` object by calling `platform_get_completion_port`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_006: [** If any error occurs, `async_socket_create_with_transport` shall fail and return `NULL`. **]**

### async_socket_destroy

```c
MOCKABLE_FUNCTION(, void, async_socket_destroy, ASYNC_SOCKET_HANDLE, async_socket);
```

`async_socket_destroy` frees all resources associated with `async_socket`.

**SRS_ASYNC_SOCKET_LINUX_11_019: [** If `async_socket` is `NULL`, `async_socket_destroy` shall return. **]**

**SRS_ASYNC_SOCKET_LINUX_11_020: [** While `async_socket` is `OPENING` or `CLOSING`, `async_socket_destroy` shall wait for the open/close to complete either successfully or with error. **]**

**SRS_ASYNC_SOCKET_LINUX_11_021: [** `async_socket_destroy` shall perform an implicit close if `async_socket` is `OPEN`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_103: [** If the `socket_transport` is not NULL, `async_socket_destroy` shall call `socket_transport_destroy`. **]**

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

**SRS_ASYNC_SOCKET_LINUX_11_003: [** If `socket_transport_handle` is `NULL`, `async_socket_open_async` shall fail and return non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_11_027: [** Otherwise, `async_socket_open_async` shall switch the state to `OPENING`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_028: [** On success, `async_socket_open_async` shall return 0. **]**

**SRS_ASYNC_SOCKET_LINUX_11_029: [** If `async_socket` is already OPEN or OPENING, `async_socket_open_async` shall fail and return a non-zero value. **]**

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

**SRS_ASYNC_SOCKET_LINUX_11_041: [** `async_socket_close` shall set the state to CLOSED. **]**

**SRS_ASYNC_SOCKET_LINUX_11_104: [** `async_socket_close` shall call `socket_transport_disconnect`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_042: [** If `async_socket` is not OPEN, `async_socket_close` shall return. **]**

### on_socket_send

```c
static int on_socket_send(void* context, ASYNC_SOCKET_HANDLE async_socket, const void* buf, size_t len)
```

`on_socket_send` is the default send callback used when `async_socket_create` is called. This implementation calls the system socket `send` API.

**SRS_ASYNC_SOCKET_LINUX_11_052: [** `on_socket_send` shall attempt to send the data by calling `send` with the `MSG_NOSIGNAL` flag to ensure SIGPIPE is not generated on errors. **]**

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

**SRS_ASYNC_SOCKET_LINUX_04_004: [** `async_socket_send_async` shall call the `on_send` callback to send the buffer. **]**

**SRS_ASYNC_SOCKET_LINUX_11_053: [** `async_socket_send_async` shall continue to send the data until the payload length has been sent. **]**

**SRS_ASYNC_SOCKET_LINUX_11_054: [** If `socket_transport_send` fails to send the data, `async_socket_send_async` shall do the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_11_055: [** If the `errno` value is `EAGAIN` or `EWOULDBLOCK`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_056: [** `async_socket_send_async` shall create a context for the send where the `payload`, `on_send_complete` and `on_send_complete_context` shall be stored. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_057: [** The context shall then be added to the completion port system by calling `completion_port_add` with `EPOLL_CTL_MOD` and `event_complete_callback` as the callback. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_059: [** If the `errno` value is `ECONNRESET`, `ENOTCONN`, or `EPIPE` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ABANDONED`. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_060: [** If any other error is encountered, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_061: [** If the `send` is successful, `async_socket_send_async` shall call the `on_send_complete` with `on_send_complete_context` and `ASYNC_SOCKET_SEND_SYNC_OK`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_062: [** On success, `async_socket_send_async` shall return `ASYNC_SOCKET_SEND_SYNC_OK`. **]**

**SRS_ASYNC_SOCKET_LINUX_11_063: [** If any error occurs, `async_socket_send_async` shall fail and return `ASYNC_SOCKET_SEND_SYNC_ERROR`. **]**

### on_socket_recv

```c
static int on_socket_recv(void* context, ASYNC_SOCKET_HANDLE async_socket, void* buf, size_t len)
```

`on_socket_recv` is the default recv callback used when `async_socket_create` is called. This implementation calls the system socket `socket_transport_receive` API.

**SRS_ASYNC_SOCKET_LINUX_04_007: [** `on_socket_recv` shall attempt to receive data by calling the system `socket_transport_receive` socket API. **]**

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

- **SRS_ASYNC_SOCKET_LINUX_11_081: [** `event_complete_callback` shall call either the `socket_transport_send` or `socket_transport_receive` complete callback with an `ABANDONED` flag when the IO type is either `ASYNC_SOCKET_IO_TYPE_SEND` or `ASYNC_SOCKET_IO_TYPE_RECEIVE` respectively. **]**

- **SRS_ASYNC_SOCKET_LINUX_04_008: [** `event_complete_callback` shall call the notify complete callback with an `ABANDONED` flag when the IO type is `ASYNC_SOCKET_IO_TYPE_NOTIFY`. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_084: [** Then `event_complete_callback` shall free the `context` memory. **]**

**SRS_ASYNC_SOCKET_LINUX_11_082: [** If `COMPLETION_PORT_EPOLL_ACTION` is `COMPLETION_PORT_EPOLL_EPOLLIN`, `event_complete_callback` shall do the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_04_009: [** If the IO type is `ASYNC_SOCKET_IO_TYPE_NOTIFY` then `event_complete_callback` shall call the notify complete callback with an `IN` flag. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_083: [** Otherwise `event_complete_callback` shall call the `on_recv` callback with the `recv_buffer` buffer and length and do the following: **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_088: [** If the `socket_transport_receive` size < 0, then: **]**

    - **SRS_ASYNC_SOCKET_LINUX_11_089: [** If `errno` is `EAGAIN` or `EWOULDBLOCK`, then no data is available and `event_complete_callback` will break out of the function. **]**

    - **SRS_ASYNC_SOCKET_LINUX_11_090: [** If `errno` is `ECONNRESET`, then `event_complete_callback` shall call the `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_ABANDONED`. **]**

    - **SRS_ASYNC_SOCKET_LINUX_11_095: [** If `errno` is any other error, then `event_complete_callback` shall call the `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_ERROR`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_091: [** If the `socket_transport_receive` size equals 0, then `event_complete_callback` shall call `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_ABANDONED`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_092: [** If the `socket_transport_receive` size > 0, if we have another buffer to fill then we will attempt another read, otherwise we shall call `on_receive_complete` callback with the `on_receive_complete_context` and `ASYNC_SOCKET_RECEIVE_OK`. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_093: [** `event_complete_callback` shall then free the `io_context` memory. **]**

**SRS_ASYNC_SOCKET_LINUX_11_094: [** If the events value contains `COMPLETION_PORT_EPOLL_EPOLLOUT`, `event_complete_callback` shall the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_04_010: [** If the IO type is `ASYNC_SOCKET_IO_TYPE_NOTIFY` then `event_complete_callback` shall call the notify complete callback with an `OUT` flag. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_096: [** `event_complete_callback` shall call `socket_transport_send` on the data in the `ASYNC_SOCKET_SEND_CONTEXT` buffer. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_097: [** If `socket_transport_send` returns value is < 0 `event_complete_callback` shall do the following: **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_098: [** if `errno` is `ECONNRESET`, then `on_send_complete` shall be called with `ASYNC_SOCKET_SEND_ABANDONED`. **]**

  - **SRS_ASYNC_SOCKET_LINUX_11_099: [** if `errno` is anything else, then `on_send_complete` shall be called with `ASYNC_SOCKET_SEND_ERROR`. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_101: [** If `socket_transport_send` returns a value > 0 but less than the amount to be sent, `event_complete_callback` shall continue to `socket_transport_send` the data until the payload length has been sent. **]**

**SRS_ASYNC_SOCKET_LINUX_11_100: [** Then `event_complete_callback` shall free the `io_context` memory **]**

**SRS_ASYNC_SOCKET_LINUX_11_085: [** If the events value contains `COMPLETION_PORT_EPOLL_ERROR`, `event_complete_callback` shall the following: **]**

- **SRS_ASYNC_SOCKET_LINUX_04_011: [** If the IO type is `ASYNC_SOCKET_IO_TYPE_NOTIFY` then `event_complete_callback` shall call the notify complete callback with an `ERROR` flag. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_086: [** Otherwise `event_complete_callback` shall call either the `socket_transport_send` or `socket_transport_receive` complete callback with an `ERROR` flag. **]**

- **SRS_ASYNC_SOCKET_LINUX_11_087: [** Then `event_complete_callback` shall and free the `io_context` memory. **]**

### async_socket_notify_io_async

```c
MOCKABLE_FUNCTION(, int, async_socket_notify_io_async, ASYNC_SOCKET_HANDLE, async_socket, ASYNC_SOCKET_NOTIFY_IO_TYPE, io_type, ON_ASYNC_SOCKET_NOTIFY_IO_COMPLETE, on_notify_io_complete, void*, on_notify_io_complete_context);
```

`async_socket_notify_io_async` uses the underlying completion port mechanism to register the supplied callback (`on_notify_io_complete`) to be invoked when the IO of type signified by `io_type` occurs on the socket.

**SRS_ASYNC_SOCKET_LINUX_04_012: [** If `async_socket` is `NULL`, `async_socket_notify_io_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_04_013: [** If `on_notify_io_complete` is `NULL`, `async_socket_notify_io_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_04_014: [** If `io_type` has an invalid value, then `async_socket_notify_io_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_04_015: [** If the async socket's current state is not `ASYNC_SOCKET_LINUX_STATE_OPEN` then `async_socket_notify_io_async` shall fail and return a non-zero value. **]**

**SRS_ASYNC_SOCKET_LINUX_04_016: [** `on_notify_io_complete_context` is allowed to be `NULL`. **]**

**SRS_ASYNC_SOCKET_LINUX_04_017: [** Otherwise `async_socket_notify_io_async` shall create a context for the notify where the `on_notify_io_complete` and `on_notify_io_complete_context` shall be stored. **]**

**SRS_ASYNC_SOCKET_LINUX_04_018: [** Then the context shall then be added to the completion port system by calling `completion_port_add` with `EPOLLIN` if `io_type` is `ASYNC_SOCKET_NOTIFY_IO_TYPE_IN` and `EPOLLOUT` otherwise and `event_complete_callback` as the callback. **]**

**SRS_ASYNC_SOCKET_LINUX_04_019: [** On success, `async_socket_notify_io_async` shall return `0`. **]**

**SRS_ASYNC_SOCKET_LINUX_04_020: [** If any error occurs, `async_socket_notify_io_async` shall fail and return a non-zero value. **]**